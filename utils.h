#include <filesystem>
#include <vector>
#include "stb_image.h"

namespace fs = std::filesystem;
using namespace wgpu;

bool loadGeometry(const fs::path& path, std::vector<float>& pointData, std::vector<uint16_t>& indexData, int dimensions);
ShaderModule loadShaderModule(const fs::path& path, Device device);
uint32_t ceilToNextMultiple(uint32_t value, uint32_t step);
std::vector<uint8_t> createGradientTexture(TextureDescriptor textureDesc);
std::vector<uint8_t> createAmazingTexture(TextureDescriptor textureDesc);
Texture loadTexture(const fs::path& path, Device device, TextureView* pTextureView);
static void writeMipMaps(Device device, Texture texture, Extent3D textureSize, uint32_t mipLevelCount, const unsigned char* pixelData);
uint32_t bit_width(uint32_t m);