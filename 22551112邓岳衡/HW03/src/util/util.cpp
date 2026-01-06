#include "util/util.hpp"

#include "spdlog/spdlog.h"
#include <fstream>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION

namespace util
{
    int updateFps()
    {
        static int frameCount = 0;
        static double lastTime = 0.0;
        double currentTime = glfwGetTime();
        frameCount++;

        int fps = static_cast<int>(frameCount / (currentTime - lastTime));
        if (currentTime - lastTime >= 1.0)
        {
            frameCount = 0;
            lastTime = currentTime;
        }
        return fps;
    }

    bool readFileAsBytes(const std::string &filename, std::vector<uint32_t> &outBytes)
    {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);
        if (!file.is_open())
        {
            spdlog::error("Failed to open file: {}", filename);
            return false;
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        outBytes.resize((fileSize + 3) / 4); // round up to nearest uint32_t
        
        file.seekg(0);
        file.read(reinterpret_cast<char*>(outBytes.data()), fileSize);
        file.close();

        return true;
    }
} // namespace util
