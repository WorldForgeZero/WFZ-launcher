#include "window_icon.h"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <limits>
#include <vector>

#include <GLFW/glfw3.h>

#ifdef WFZ_EMBED_ASSETS
#include "resources/embedded_resources.h"
#endif

namespace wfz::app
{
    namespace
    {
        constexpr const char *WINDOW_ICON_PATH = "assets/icon/window_icon.tga";

        constexpr std::size_t TGA_HEADER_SIZE = 18;

        struct Image
        {
            int width = 0;
            int height = 0;
            std::vector<unsigned char> pixels;
        };

#ifndef WFZ_EMBED_ASSETS
        bool ReadFile(const char *path, std::vector<std::uint8_t> &data)
        {
            std::ifstream file(path, std::ios::binary | std::ios::ate);

            if (!file.is_open())
                return false;

            const std::streamsize size = file.tellg();

            if (size <= 0)
                return false;

            file.seekg(0, std::ios::beg);

            data.resize(static_cast<std::size_t>(size));

            return static_cast<bool>(file.read(reinterpret_cast<char *>(data.data()), size));
        }
#endif

        std::uint16_t ReadU16(const std::uint8_t *data)
        {
            return static_cast<std::uint16_t>(data[0] | (static_cast<std::uint16_t>(data[1]) << 8));
        }

        bool DecodeTga(const std::uint8_t *data, const std::size_t size, Image &image)
        {
            if (!data || size < TGA_HEADER_SIZE)
            {
                return false;
            }

            const std::uint8_t id_length = data[0];
            const std::uint8_t color_map_type = data[1];
            const std::uint8_t image_type = data[2];

            const std::uint16_t width = ReadU16(data + 12);
            const std::uint16_t height = ReadU16(data + 14);

            const std::uint8_t pixel_depth = data[16];
            const std::uint8_t descriptor = data[17];

            /*
             * Only uncompressed true-color TGA.
             */
            if (color_map_type != 0 || image_type != 2)
            {
                return false;
            }

            if (width == 0 || height == 0)
            {
                return false;
            }

            if (pixel_depth != 24 && pixel_depth != 32)
            {
                return false;
            }

            const std::size_t bytes_per_pixel = pixel_depth / 8;

            const std::size_t pixel_count = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);

            if (pixel_count > std::numeric_limits<std::size_t>::max() / bytes_per_pixel)
            {
                return false;
            }

            const std::size_t pixel_data_size = pixel_count * bytes_per_pixel;

            const std::size_t pixel_data_offset = TGA_HEADER_SIZE + static_cast<std::size_t>(id_length);

            if (pixel_data_offset > size || pixel_data_size > size - pixel_data_offset)
            {
                return false;
            }

            if (pixel_count > std::numeric_limits<std::size_t>::max() / 4)
            {
                return false;
            }

            image.width = static_cast<int>(width);

            image.height = static_cast<int>(height);

            image.pixels.resize(pixel_count * 4);

            const bool top_origin = (descriptor & 0x20) != 0;

            const bool right_origin = (descriptor & 0x10) != 0;

            const std::uint8_t *source = data + pixel_data_offset;

            for (std::size_t y = 0; y < height; ++y)
            {
                const std::size_t source_y =
                    top_origin
                        ? y
                        : static_cast<std::size_t>(height - 1) - y;

                for (std::size_t x = 0; x < width; ++x)
                {
                    const std::size_t source_x =
                        right_origin
                            ? static_cast<std::size_t>(width - 1) - x
                            : x;

                    const std::size_t source_index = (source_y * static_cast<std::size_t>(width) + source_x) * bytes_per_pixel;

                    const std::size_t destination_index = (y * static_cast<std::size_t>(width) + x) * 4;

                    /*
                     * TGA stores true-color pixels as BGR(A).
                     */
                    image.pixels[destination_index + 0] = source[source_index + 2];
                    image.pixels[destination_index + 1] = source[source_index + 1];
                    image.pixels[destination_index + 2] = source[source_index + 0];
                    image.pixels[destination_index + 3] = bytes_per_pixel == 4
                                                              ? source[source_index + 3]
                                                              : 255;
                }
            }

            return true;
        }
    }

    bool SetWindowIcon(GLFWwindow *window)
    {
        if (!window)
            return false;

        const std::uint8_t *data = nullptr;
        std::size_t size = 0;

#ifndef WFZ_EMBED_ASSETS
        std::vector<std::uint8_t> file_data;

        if (!ReadFile(WINDOW_ICON_PATH, file_data))
        {
            return false;
        }

        data = file_data.data();
        size = file_data.size();
#else
        const wfz::resources::ResourceView
            resource = wfz::resources::Find(WINDOW_ICON_PATH);

        if (!resource)
            return false;

        data = resource.data;
        size = resource.size;
#endif

        Image image;

        if (!DecodeTga(data, size, image))
            return false;

        GLFWimage glfw_image{image.width, image.height, image.pixels.data()};

        glfwSetWindowIcon(window, 1, &glfw_image);

        return true;
    }
}
