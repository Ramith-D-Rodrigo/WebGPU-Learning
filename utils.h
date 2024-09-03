#include <filesystem>
#include <vector>

namespace fs = std::filesystem;
using namespace wgpu;

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions);
ShaderModule loadShaderModule(const fs::path& path, Device device);
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);
std::vector<uint8_t> createGradientTexture(TextureDescriptor textureDesc);
