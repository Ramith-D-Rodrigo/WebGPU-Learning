#pragma once
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <webgpu/webgpu.hpp>

using namespace std;

class Mesh;

class SceneObject
{
public:
	SceneObject(wgpu::Device* device, wgpu::BindGroupLayout* modelBindGroupLayout);
	~SceneObject();

	void setTranslation(glm::vec3 translation);
	void setRotation(glm::quat rotation);
	void setScale(glm::vec3 scale);
	void addChild(SceneObject* child);
	void addVisualObject(Mesh* visualObject);
	glm::mat4 calculateModelMatrix();
	wgpu::BindGroup getModelBindGroup() { return modelBindGroup; }
	vector<SceneObject*> getChildren() { return children; }
	vector<Mesh*> getVisualObjects() { return visualObjects; }
	void writeModelUniformBuffer(wgpu::Queue queue, glm::mat4* modelMatrix);

private:
	glm::vec3 localTranslation;
	glm::quat localRotation;
	glm::vec3 localScale;

	vector<SceneObject*> children;
	vector<Mesh*> visualObjects;

	wgpu::Buffer modelUniformBuffer = nullptr;
	wgpu::BindGroup modelBindGroup = nullptr;
};

