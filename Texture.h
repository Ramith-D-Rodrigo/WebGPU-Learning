#pragma once
#include <webgpu/webgpu.hpp>
#include <filesystem>

namespace fs = std::filesystem;

namespace custom {
	class Texture
	{
		private:
			wgpu::Texture texture = nullptr;
			wgpu::TextureView textureView = nullptr;

		public:
			Texture(const fs::path& path, wgpu::Device device);
			~Texture();

			wgpu::Texture GetTexture() const { return texture; }
			wgpu::TextureView GetTextureView() const { return textureView; }
	};
}


