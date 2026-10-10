#include "crypto/sha256.h"

#include <openssl/evp.h>

#include <array>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace wfz::crypto
{
    std::string sha256(const std::string &data)
    {
        using MdContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;

        MdContextPtr context(EVP_MD_CTX_new(), EVP_MD_CTX_free);

        if (!context)
            throw std::runtime_error("Failed to create OpenSSL digest context");

        if (EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) != 1)
            throw std::runtime_error("Failed to initialize SHA-256");

        if (EVP_DigestUpdate(context.get(), data.data(), data.size()) != 1)
            throw std::runtime_error("Failed to update SHA-256");

        std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
        unsigned int hash_size = 0;

        if (EVP_DigestFinal_ex(context.get(), hash.data(), &hash_size) != 1)
            throw std::runtime_error("Failed to finalize SHA-256");

        std::ostringstream stream;
        stream << std::hex << std::setfill('0');

        for (unsigned int i = 0; i < hash_size; ++i)
            stream << std::setw(2) << static_cast<int>(hash[i]);

        return stream.str();
    }
}

namespace wfz::crypto
{
    std::string sha256_file(const std::filesystem::path &path)
    {
        std::ifstream file(path, std::ios::binary);
        if (!file)
            throw std::runtime_error("Cannot open file for SHA-256");

        using ContextPtr = std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)>;
        ContextPtr ctx(EVP_MD_CTX_new(), EVP_MD_CTX_free);
        if (!ctx || EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr) != 1)
            throw std::runtime_error("Cannot initialize SHA-256");

        std::array<char, 65536> block{};
        while (file)
        {
            file.read(block.data(), block.size());
            const auto count = file.gcount();
            if (count > 0 && EVP_DigestUpdate(ctx.get(), block.data(), static_cast<std::size_t>(count)) != 1)
                throw std::runtime_error("Cannot update SHA-256");
        }

        if (!file.eof())
            throw std::runtime_error("Cannot read file for SHA-256");

        std::array<unsigned char, EVP_MAX_MD_SIZE> hash{};
        unsigned int size = 0;
        if (EVP_DigestFinal_ex(ctx.get(), hash.data(), &size) != 1)
            throw std::runtime_error("Cannot finalize SHA-256");

        std::ostringstream result;
        result << std::hex << std::setfill('0');
        for (unsigned i = 0; i < size; ++i)
            result << std::setw(2) << static_cast<int>(hash[i]);

        return result.str();
    }
}
