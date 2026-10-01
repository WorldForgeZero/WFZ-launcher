#include "embedded_file_interface.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <string>

#include "resources/embedded_resources.h"

namespace fs = std::filesystem;

namespace wfz::ui
{
    namespace
    {
        struct OpenFile
        {
            wfz::resources::ResourceView resource;
            std::size_t position;
        };

        std::string NormalizePath(
            const Rml::String &path)
        {
            std::string normalized = path;

            std::replace(
                normalized.begin(),
                normalized.end(),
                '\\',
                '/');

            normalized =
                fs::path(normalized)
                    .lexically_normal()
                    .generic_string();

            while (
                normalized.size() >= 2 &&
                normalized[0] == '.' &&
                normalized[1] == '/')
            {
                normalized.erase(0, 2);
            }

            while (
                !normalized.empty() &&
                normalized.front() == '/')
            {
                normalized.erase(
                    normalized.begin());
            }

            return normalized;
        }

        OpenFile *GetOpenFile(
            const Rml::FileHandle file)
        {
            return reinterpret_cast<OpenFile *>(
                file);
        }
    }

    Rml::FileHandle EmbeddedFileInterface::Open(
        const Rml::String &path)
    {
        const wfz::resources::ResourceView resource =
            wfz::resources::Find(
                NormalizePath(path));

        if (!resource)
            return 0;

        OpenFile *file =
            new OpenFile{
                resource,
                0,
            };

        return reinterpret_cast<Rml::FileHandle>(
            file);
    }

    void EmbeddedFileInterface::Close(
        const Rml::FileHandle file)
    {
        delete GetOpenFile(file);
    }

    std::size_t EmbeddedFileInterface::Read(
        void *buffer,
        const std::size_t size,
        const Rml::FileHandle file)
    {
        OpenFile *open_file =
            GetOpenFile(file);

        if (!open_file || !buffer)
            return 0;

        if (open_file->position >=
            open_file->resource.size)
        {
            return 0;
        }

        const std::size_t remaining =
            open_file->resource.size -
            open_file->position;

        const std::size_t read_size =
            std::min(
                size,
                remaining);

        std::memcpy(
            buffer,
            open_file->resource.data +
                open_file->position,
            read_size);

        open_file->position +=
            read_size;

        return read_size;
    }

    bool EmbeddedFileInterface::Seek(
        const Rml::FileHandle file,
        const long offset,
        const int origin)
    {
        OpenFile *open_file =
            GetOpenFile(file);

        if (!open_file)
            return false;

        std::int64_t base = 0;

        switch (origin)
        {
        case SEEK_SET:
            base = 0;
            break;

        case SEEK_CUR:
            base =
                static_cast<std::int64_t>(
                    open_file->position);
            break;

        case SEEK_END:
            base =
                static_cast<std::int64_t>(
                    open_file->resource.size);
            break;

        default:
            return false;
        }

        const std::int64_t position =
            base +
            static_cast<std::int64_t>(
                offset);

        if (position < 0)
            return false;

        if (position >
            static_cast<std::int64_t>(
                open_file->resource.size))
        {
            return false;
        }

        open_file->position =
            static_cast<std::size_t>(
                position);

        return true;
    }

    std::size_t EmbeddedFileInterface::Tell(
        const Rml::FileHandle file)
    {
        OpenFile *open_file =
            GetOpenFile(file);

        if (!open_file)
            return 0;

        return open_file->position;
    }

    std::size_t EmbeddedFileInterface::Length(
        const Rml::FileHandle file)
    {
        OpenFile *open_file =
            GetOpenFile(file);

        if (!open_file)
            return 0;

        return open_file->resource.size;
    }
}
