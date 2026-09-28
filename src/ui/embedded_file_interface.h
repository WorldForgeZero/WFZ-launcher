#pragma once

#include <RmlUi/Core/FileInterface.h>

namespace wfz::ui
{
    class EmbeddedFileInterface final : public Rml::FileInterface
    {
    public:
        Rml::FileHandle Open(const Rml::String &path) override;
        void Close(Rml::FileHandle file) override;
        std::size_t Read(void *buffer, std::size_t size, Rml::FileHandle file) override;
        bool Seek(Rml::FileHandle file, long offset, int origin) override;
        std::size_t Tell(Rml::FileHandle file) override;
        std::size_t Length(Rml::FileHandle file) override;
    };
}
