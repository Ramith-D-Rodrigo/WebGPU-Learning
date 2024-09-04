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
	const uint32_t* indices;
	size_t numIndices;
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
		const uint32_t* indices, size_t numIndices, 
		const float* normals, size_t numNormals, 
		const float* uvs, size_t numUvs,
		TextureView textureView, BindGroupLayout textureBindGroupLayout, Device device, Sampler sampler);
	~Mesh();

	const float* getVertices();
	size_t getNumVertices();
	const uint32_t* getIndices();
	size_t getNumIndices();
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

