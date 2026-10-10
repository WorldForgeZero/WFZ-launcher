#include "source_storage.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "etc/paths.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace fs = std::filesystem;

namespace
{
    constexpr std::array<const char *, 2> PERSISTENT = {"game", "launcher"};

    fs::path Canonical(const fs::path &path)
    {
        return fs::weakly_canonical(fs::absolute(path));
    }

    bool IsAncestor(const fs::path &a, const fs::path &b)
    {
        auto x = a.begin();
        auto y = b.begin();
        for (; x != a.end(); ++x, ++y)
        {
            if (y == b.end() || *x != *y)
                return false;
        }
        return true;
    }

    bool IsProtected(const fs::path &path)
    {
        const fs::path root = Canonical(path);
        const fs::path exe = Canonical(wfz::paths::ExecutableDir());
        const fs::path config = Canonical(wfz::paths::ConfigDir());
        return root == root.root_path() || IsAncestor(root, exe) || IsAncestor(root, config) || IsAncestor(config, root);
    }

    bool IsReparsePoint(const fs::path &path)
    {
#ifdef _WIN32
        const DWORD attrs = GetFileAttributesW(path.c_str());
        return attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT);
#else
        (void)path;
        return false;
#endif
    }

    bool ContainsSymlinksOrSpecial(const fs::path &path)
    {
        if (IsReparsePoint(path) || fs::is_symlink(fs::symlink_status(path)))
            return true;

        if (!fs::exists(path))
            return false;

        if (!fs::is_directory(path))
            return true;

        for (fs::recursive_directory_iterator it(path), end; it != end; ++it)
        {
            const auto status = it->symlink_status();
            if (IsReparsePoint(it->path()) ||
                (!fs::is_directory(status) && !fs::is_regular_file(status)))
                return true;
        }
        return false;
    }

    bool EqualFiles(const fs::path &a, const fs::path &b)
    {
        if (fs::file_size(a) != fs::file_size(b))
            return false;

        std::ifstream x(a, std::ios::binary), y(b, std::ios::binary);
        if (!x || !y)
            return false;

        std::array<char, 65536> xb{}, yb{};
        while (x)
        {
            x.read(xb.data(), xb.size());
            const auto count = x.gcount();
            y.read(yb.data(), count);
            if (y.gcount() != count || !std::equal(xb.begin(), xb.begin() + count, yb.begin()))
                return false;
        }
        return x.eof() && !x.bad() && !y.bad();
    }

    bool VerifyTree(const fs::path &source, const fs::path &copied)
    {
        if (!fs::is_directory(copied))
            return false;

        for (fs::recursive_directory_iterator it(source), end; it != end; ++it)
        {
            const auto relative = it->path().lexically_relative(source);
            const auto target = copied / relative;
            if (it->is_directory())
            {
                if (!fs::is_directory(target))
                    return false;
            }

            else if (!fs::is_regular_file(target) || !EqualFiles(it->path(), target))
                return false;
        }
        return true;
    }
}

namespace wfz::source_storage
{
    bool ValidateRoots(const fs::path &old_dir, const fs::path &new_dir, std::string &error)
    {
        try
        {
            if (old_dir.empty() || new_dir.empty() || !old_dir.is_absolute() || !new_dir.is_absolute())
            {
                error = "Source paths must be absolute";
                return false;
            }

            const fs::path old_root = Canonical(old_dir);
            const fs::path new_root = Canonical(new_dir);
            if (IsProtected(old_root) || IsProtected(new_root))
            {
                error = "Selected directory overlaps a protected application directory";
                return false;
            }

            if (IsAncestor(old_root, new_root) || IsAncestor(new_root, old_root))
            {
                error = "Source directories cannot overlap";
                return false;
            }

            if ((fs::exists(old_root) && fs::is_symlink(fs::symlink_status(old_dir))) ||
                (fs::exists(new_root) && fs::is_symlink(fs::symlink_status(new_dir))))
            {
                error = "Symbolic links are not supported for source directories";
                return false;
            }

            return true;
        }
        catch (const fs::filesystem_error &e)
        {
            error = e.what();
            return false;
        }
    }

    bool Prepare(const fs::path &old_dir, const fs::path &new_dir, Operation operation, std::string &error)
    {
        if (operation == Operation::KeepOld || operation == Operation::DiscardOld)
            return true;

        if (!ValidateRoots(old_dir, new_dir, error))
            return false;

        fs::path stage;
        std::vector<fs::path> installed;
        try
        {
            for (const char *name : PERSISTENT)
            {
                const auto source = old_dir / name;
                if (!fs::exists(source))
                    continue;

                if (ContainsSymlinksOrSpecial(source))
                {
                    error = "Source contains a symlink or special file";
                    return false;
                }

                if (fs::exists(new_dir / name))
                {
                    error = "Destination already contains launcher/game data. Refusing to overwrite";
                    return false;
                }
            }

            fs::create_directories(new_dir);
            const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
            stage = new_dir / (".wfz-migration-" + std::to_string(nonce));
            if (!fs::create_directory(stage))
            {
                error = "Failed to create a migration staging directory";
                return false;
            }

            for (const char *name : PERSISTENT)
            {
                const auto source = old_dir / name;
                if (!fs::exists(source))
                    continue;

                fs::copy(source, stage / name, fs::copy_options::recursive | fs::copy_options::skip_symlinks);
                if (!VerifyTree(source, stage / name))
                {
                    error = "Copied files failed verification";
                    fs::remove_all(stage);
                    return false;
                }
            }
            for (const char *name : PERSISTENT)
            {
                if (!fs::exists(stage / name))
                    continue;

                fs::rename(stage / name, new_dir / name);
                installed.push_back(new_dir / name);
            }
            fs::remove(stage);
            return true;
        }
        catch (const fs::filesystem_error &e)
        {
            error = e.what();
            for (const auto &entry : installed)
            {
                std::error_code ec;
                fs::remove_all(entry, ec);
            }
            if (!stage.empty())
            {
                std::error_code ec;
                fs::remove_all(stage, ec);
            }
            return false;
        }
    }

    bool Cleanup(const fs::path &dir, std::string &error)
    {
        try
        {
            if (dir.empty() || !dir.is_absolute() || IsProtected(dir))
            {
                error = "Refusing to clean a protected or invalid directory";
                return false;
            }

            if (!fs::exists(dir))
                return true;

            if (!fs::is_directory(dir) || fs::is_symlink(fs::symlink_status(dir)))
            {
                error = "Source directory is not a regular directory";
                return false;
            }

            if (ContainsSymlinksOrSpecial(dir))
            {
                error = "Source contains a symbolic link or special file";
                return false;
            }

            fs::remove_all(dir);
            return true;
        }
        catch (const fs::filesystem_error &e)
        {
            error = e.what();
            return false;
        }
    }
}
