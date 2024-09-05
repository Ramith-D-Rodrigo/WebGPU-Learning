#pragma once
#include <iostream>
#include <webgpu/webgpu.hpp>

using namespace std;
using namespace wgpu;

class Mesh
{
private:
	const float* vertices;
	size_t numVertices;
	const unsigned char* indices;
	size_t numIndices;
	IndexFormat indexFormat = IndexFormat::Uint16;
	const float* normals;
	size_t numNormals;
	const float* uvs;
	size_t numUvs;

	Buffer vertexBuffer = nullptr;
	Buffer indexBuffer = nullptr;
	Buffer normalBuffer = nullptr;
	Buffer uvBuffer = nullptr;
	BindGroup textureBindGroup = nullptr;

public:
	Mesh(const float* vertices, size_t numVertices, 
		const unsigned char* indices, size_t numIndices, IndexFormat indexFormat,
		const float* normals, size_t numNormals, 
		const float* uvs, size_t numUvs,
		TextureView textureView, BindGroupLayout textureBindGroupLayout, Device device, Sampler sampler);
	~Mesh();

	const float* getVertices();
	size_t getNumVertices();
	const unsigned char* getIndices();
	size_t getNumIndices();
	IndexFormat getIndexFormat();
	const float* getNormals();
	size_t getNumNormals();
	const float* getUVs();
	size_t getNumUVs();

	Buffer getVertexBuffer() { return vertexBuffer; }
	Buffer getIndexBuffer() { return indexBuffer; }
	Buffer getNormalBuffer() { return normalBuffer; }
	Buffer getUVBuffer() { return uvBuffer; }

	BindGroup getTextureBindGroup() { return textureBindGroup; }

private:
	void setBuffers(Device device, Queue queue);
};

