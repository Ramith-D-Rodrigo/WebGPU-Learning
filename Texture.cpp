#include "Texture.h"
#include "utils.h"

custom::Texture::Texture(const fs::path& path, wgpu::Device device)
{
	this->texture = loadTexture(path, device, &this->textureView);
    if(this->texture == nullptr)
	{
		throw std::runtime_error("Failed to load texture from file: " + path.string());
	}

    if(this->textureView == nullptr)
    {
		this->texture.release();
        throw std::runtime_error("Failed to create texture view");
    }
}

custom::Texture::~Texture()
{
	this->textureView.release();
    this->texture.destroy();
	this->texture.release();
}
