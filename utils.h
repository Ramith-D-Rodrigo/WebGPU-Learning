#define WEBGPU_CPP_IMPLEMENTATION
#include<webgpu/webgpu.hpp>

#include <filesystem>
#include <vector>

namespace fs = std::filesystem;

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData);
ShaderModule loadShaderModule(const fs::path& path, Device device);

