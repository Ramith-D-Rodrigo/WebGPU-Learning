#include "Mesh.h"
#include <iostream>
#include <webgpu/webgpu.hpp>

using namespace std;
using namespace wgpu;

Mesh::Mesh(const float* vertices, size_t numVertices, 
	const uint32_t* indices, size_t numIndices, 
	const float* normals, size_t numNormals,
	const float* uvs, size_t numUvs,
	TextureView textureView, BindGroupLayout textureBindGroupLayout, Device device, Sampler sampler)
{
	this->vertices = vertices;
	this->numVertices = numVertices;
	this->indices = indices;
	this->numIndices = numIndices;
	this->normals = normals;
	this->numNormals = numNormals;
	this->uvs = uvs;
	this->numUvs = numUvs;

    this->indexBuffer = nullptr;
    this->vertexBuffer = nullptr;
    this->normalBuffer = nullptr;
	this->uvBuffer = nullptr;

	vector<BindGroupEntry> bindings(2);
	bindings[0].binding = 0;
	bindings[0].textureView = textureView;

	bindings[1].binding = 1;
	bindings[1].sampler = sampler;

	BindGroupDescriptor bindGroupDesc;
	bindGroupDesc.layout = textureBindGroupLayout;
	bindGroupDesc.entryCount = (uint32_t)bindings.size();
	bindGroupDesc.entries = bindings.data();
	this->textureBindGroup = device.createBindGroup(bindGroupDesc);

	cout<<"setting buffers"<<"\n";

	setBuffers(device, device.getQueue());
}

Mesh::~Mesh()
{
    delete[] vertices;
	delete[] indices;
	delete[] normals;
    this->indexBuffer.destroy();
	this->indexBuffer.release();
    this->vertexBuffer.destroy();
	this->vertexBuffer.release();
    this->normalBuffer.destroy();
	this->normalBuffer.release();
	this->textureBindGroup.release();
}

const float* Mesh::getVertices()
{
	return vertices;
}

size_t Mesh::getNumVertices()
{
	return numVertices;
}

const uint32_t* Mesh::getIndices()
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

const float* Mesh::getUVs()
{
	return uvs;
}

size_t Mesh::getNumUVs()
{
	return numUvs;
}

void Mesh::setBuffers(Device device, Queue queue)
{
    BufferDescriptor bufferDescriptor = {};
    bufferDescriptor.label = "Vertex Buffer";
    bufferDescriptor.size = numVertices * sizeof(float);
    bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
    bufferDescriptor.mappedAtCreation = false;
    this->vertexBuffer = device.createBuffer(bufferDescriptor);

	cout<<"writing vertex buffer"<<"\n";

    queue.writeBuffer(this->vertexBuffer, 0, this->vertices, bufferDescriptor.size);

    //create the normal buffer
    bufferDescriptor.label = "Normal Buffer";
    bufferDescriptor.size = numNormals * sizeof(float);
    bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
    this->normalBuffer = device.createBuffer(bufferDescriptor);

	cout<<"writing normal buffer"<<"\n";

    queue.writeBuffer(this->normalBuffer, 0, this->normals, bufferDescriptor.size);

    //create the index buffer
    bufferDescriptor.label = "Index Buffer";
    bufferDescriptor.size = numIndices * sizeof(uint32_t);
    bufferDescriptor.usage = BufferUsage::Index | BufferUsage::CopyDst; //must add index usage
    this->indexBuffer = device.createBuffer(bufferDescriptor);

	cout<<"writing index buffer and index count : "<<numIndices<<"\n";

    queue.writeBuffer(this->indexBuffer, 0, this->indices, bufferDescriptor.size);

	//create the uv buffer
	bufferDescriptor.label = "UV Buffer";
	bufferDescriptor.size = numUvs * sizeof(float);
	bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
	this->uvBuffer = device.createBuffer(bufferDescriptor);

	cout<<"writing uv buffer"<<"\n";

	queue.writeBuffer(this->uvBuffer, 0, this->uvs, bufferDescriptor.size);
}
