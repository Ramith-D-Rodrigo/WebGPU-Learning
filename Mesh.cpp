#include "Mesh.h"
#include <iostream>
#include <webgpu/webgpu.hpp>

using namespace std;
using namespace wgpu;

Mesh::Mesh(const float* vertices, size_t numVertices, const uint16_t* indices, size_t numIndices, const float* normals, size_t numNormals)
{
	this->vertices = vertices;
	this->numVertices = numVertices;
	this->indices = indices;
	this->numIndices = numIndices;
	this->normals = normals;
	this->numNormals = numNormals;
    this->indexBuffer = nullptr;
    this->vertexBuffer = nullptr;
    this->normalBuffer = nullptr;
}

Mesh::~Mesh()
{
}

const float* Mesh::getVertices()
{
	return vertices;
}

size_t Mesh::getNumVertices()
{
	return numVertices;
}

const uint16_t* Mesh::getIndices()
{
	return indices;
}

size_t Mesh::getNumIndices()
{
	return numIndices;
}

const float* Mesh::getNormals()
{
	return normals;
}

size_t Mesh::getNumNormals()
{
	return numNormals;
}

void Mesh::setBuffers(Device device, Queue queue)
{
    BufferDescriptor bufferDescriptor = {};
    bufferDescriptor.label = "Vertex Buffer";
    bufferDescriptor.size = numVertices * sizeof(float);
    bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
    bufferDescriptor.mappedAtCreation = false;
    this->vertexBuffer = device.createBuffer(bufferDescriptor);


    queue.writeBuffer(this->vertexBuffer, 0, this->vertices, bufferDescriptor.size);

    //create the normal buffer
    bufferDescriptor.label = "Normal Buffer";
    bufferDescriptor.size = numNormals * sizeof(float);
    bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
    this->normalBuffer = device.createBuffer(bufferDescriptor);

    queue.writeBuffer(this->normalBuffer, 0, this->normals, bufferDescriptor.size);


    //create the index buffer
    bufferDescriptor.label = "Index Buffer";
    bufferDescriptor.size = numIndices * sizeof(uint32_t);
    bufferDescriptor.usage = BufferUsage::Index | BufferUsage::CopyDst; //must add index usage
    this->indexBuffer = device.createBuffer(bufferDescriptor);

    bufferDescriptor.size = (bufferDescriptor.size + 3) & ~3; // round up to the next multiple of 4

    queue.writeBuffer(this->indexBuffer, 0, this->indices, bufferDescriptor.size);
}
