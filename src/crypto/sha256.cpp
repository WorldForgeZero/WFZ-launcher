#include "crypto/sha256.h"

#include <openssl/evp.h>

#include <array>
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
