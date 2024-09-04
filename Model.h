#pragma once
#include <string>
#include "tiny_gltf.h"
#include <webgpu/webgpu.hpp>

class SceneObject;
class Mesh;

using namespace std;

class Model
{
public:
	static SceneObject* LoadModel(const string& filePath, wgpu::Device device, wgpu::BindGroupLayout textureBindGroupLayout, 
		wgpu::BindGroupLayout modelBindGroupLayout, wgpu::TextureView textureView, wgpu::Sampler sampler);

private:
	static void processData(const tinygltf::Model& model, SceneObject* rootSceneObject);
	static void processScene(const tinygltf::Scene& scene, const tinygltf::Model& model, SceneObject* rootSceneObject);
	static SceneObject* processNode(const tinygltf::Node& node, const tinygltf::Model& model);
	static Mesh* processPrimitive(const tinygltf::Primitive& primitive, const tinygltf::Model& model);
	static wgpu::Device device;
	static wgpu::BindGroupLayout textureBindGroupLayout;
	static wgpu::BindGroupLayout modelBindGroupLayout;
	static wgpu::Buffer modelUniformBuffer;
	static wgpu::TextureView textureView;
	static wgpu::Sampler sampler;
};

