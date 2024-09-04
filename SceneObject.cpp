#include "SceneObject.h"
#include "Mesh.h"


SceneObject::SceneObject(wgpu::Device* device, wgpu::BindGroupLayout* modelBindGroupLayout)
{
	this->localTranslation = glm::vec3();
	this->localRotation = glm::quat();
	this->localScale = glm::vec3(1.0f, 1.0f, 1.0f);
	this->children = vector<SceneObject*>();
	this->visualObjects = vector<Mesh*>();

	//model uniform buffer
	BufferDescriptor bufferDescriptor = Default;
	bufferDescriptor.label = "Model Uniform Buffer";
	bufferDescriptor.size = sizeof(glm::mat4);
	bufferDescriptor.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
	bufferDescriptor.mappedAtCreation = false;

	this->modelUniformBuffer = device->createBuffer(bufferDescriptor);

	glm::mat4 identityMatrix = glm::mat4(1.0f);
	device->getQueue().writeBuffer(this->modelUniformBuffer, 0, &identityMatrix, sizeof(glm::mat4));

	wgpu::BindGroupDescriptor desc = wgpu::Default;
	desc.entryCount = 1;
	desc.label = "Model Bind Group";
	desc.layout = *modelBindGroupLayout;
	
	wgpu::BindGroupEntry entry = wgpu::Default;
	entry.binding = 0;
	entry.buffer = this->modelUniformBuffer;
	entry.offset = 0;
	entry.size = sizeof(glm::mat4);

	desc.entries = &entry;

	this->modelBindGroup = device->createBindGroup(desc);
}

SceneObject::~SceneObject()
{
	for (SceneObject* child : this->children) {
		delete child;
	}
	this->children.clear(); // Optional: clear the vector to avoid potential dangling pointers

	// Delete all visual objects
	for (Mesh* visualObject : this->visualObjects) {
		delete visualObject;
	}
	this->visualObjects.clear(); // Optional: clear the vector to avoid potential dangling pointers

	this->modelUniformBuffer.destroy();
	this->modelUniformBuffer.release();
	this->modelBindGroup.release();
}

void SceneObject::setTranslation(glm::vec3 translation)
{
	this->localTranslation = translation;
}

void SceneObject::setRotation(glm::quat rotation)
{
	this->localRotation = rotation;
}

void SceneObject::setScale(glm::vec3 scale)
{
	this->localScale = scale;
}

void SceneObject::addChild(SceneObject* child)
{
	this->children.push_back(child);
}

void SceneObject::addVisualObject(Mesh* visualObject)
{
	this->visualObjects.push_back(visualObject);
}

glm::mat4 SceneObject::calculateModelMatrix()
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, this->localTranslation);
	model *= glm::mat4_cast(this->localRotation);
	model = glm::scale(model, this->localScale);

	return model;
}

void SceneObject::writeModelUniformBuffer(wgpu::Queue queue, glm::mat4* modelMatrix)
{
	queue.writeBuffer(this->modelUniformBuffer, 0, modelMatrix, sizeof(glm::mat4));
}
