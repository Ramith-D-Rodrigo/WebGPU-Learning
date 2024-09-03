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
	const uint16_t* indices;
	size_t numIndices;
	const float* normals;
	size_t numNormals;
	Buffer vertexBuffer = nullptr;
	Buffer indexBuffer = nullptr;
	Buffer normalBuffer = nullptr;

public:
	Mesh(const float* vertices, size_t numVertices, const uint16_t* indices, size_t numIndices, const float* normals, size_t numNormals);
	~Mesh();

	const float* getVertices();
	size_t getNumVertices();
	const uint16_t* getIndices();
	size_t getNumIndices();
	const float* getNormals();
	size_t getNumNormals();

private:
	void setBuffers(Device device, Queue queue);
};

