#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>

#include <webgpu/webgpu.hpp>

namespace fs = std::filesystem;

using namespace wgpu;

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }

    pointData.clear();
    indexData.clear();

    enum class Section {
        None,
        Points,
        Indices,
    };
    Section currentSection = Section::None;

    float value;
    uint16_t index;
    std::string line;
    while (!file.eof()) {
        getline(file, line);

        // overcome the `CRLF` problem
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line == "[points]") {
            currentSection = Section::Points;
        }
        else if (line == "[indices]") {
            currentSection = Section::Indices;
        }
        else if (line[0] == '#' || line.empty()) {
            // Do nothing, this is a comment
        }
        else if (currentSection == Section::Points) {
            std::istringstream iss(line);
            // Get x, y, z, r, g, b
            for (int i = 0; i < dimensions + 3; ++i) {
                iss >> value;
                pointData.push_back(value);
            }
        }
        else if (currentSection == Section::Indices) {
            std::istringstream iss(line);
            // Get corners #0 #1 and #2
            for (int i = 0; i < 3; ++i) {
                iss >> index;
                indexData.push_back(index);
            }
        }
    }
    return true;
}

ShaderModule loadShaderModule(const fs::path& path, Device device) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return nullptr;
    }
    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    std::string shaderSource(size, ' ');
    file.seekg(0);
    file.read(shaderSource.data(), size);

    ShaderModuleWGSLDescriptor shaderCodeDesc{};
    shaderCodeDesc.chain.next = nullptr;
    shaderCodeDesc.chain.sType = SType::ShaderModuleWGSLDescriptor;
    shaderCodeDesc.code = shaderSource.c_str();
    ShaderModuleDescriptor shaderDesc{};
    #ifdef WEBGPU_BACKEND_WGPU
        shaderDesc.hintCount = 0;
        shaderDesc.hints = nullptr;
    #endif // WEBGPU_BACKEND_WGPU
    shaderDesc.nextInChain = &shaderCodeDesc.chain;
    return device.createShaderModule(shaderDesc);
}

// Returns the smallest multiple of `step` that is greater than or equal to `value`.
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step) {
    uint32_t divide_and_ceil = value / step + (value % step == 0 ? 0 : 1);
    return step * divide_and_ceil;
}

std::vector<uint8_t> createGradientTexture(TextureDescriptor textureDesc) {
    // Create image data
    std::vector<uint8_t> pixels(4 * textureDesc.size.width * textureDesc.size.height);
    for (uint32_t i = 0; i < textureDesc.size.width; ++i) {
        for (uint32_t j = 0; j < textureDesc.size.height; ++j) {
            uint8_t* p = &pixels[4 * (j * textureDesc.size.width + i)];
            p[0] = (uint8_t)i; // r
            p[1] = (uint8_t)j; // g
            p[2] = 128; // b
            p[3] = 255; // a
        }
    }
    
    return pixels;
}

std::vector<uint8_t> createAmazingTexture(TextureDescriptor textureDesc) {
    std::vector<uint8_t> pixels(4 * textureDesc.size.width * textureDesc.size.height);
    for (uint32_t i = 0; i < textureDesc.size.width; ++i) {
        for (uint32_t j = 0; j < textureDesc.size.height; ++j) {
            uint8_t* p = &pixels[4 * (j * textureDesc.size.width + i)];
            p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
            p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
            p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
            p[3] = 255; // a
        }
    }
    return pixels;
}
