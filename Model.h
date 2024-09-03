#pragma once
#include <string>

class SceneObject;
class Mesh;

class Model
{
public:
	static SceneObject* LoadModel(std::string filePath);

private:
	static void processData(tinygltf::Model model, SceneObject* rootSceneObject);
	static void processScene(tinygltf::Scene scene, tinygltf::Model model, SceneObject* rootSceneObject);
	static SceneObject* processNode(tinygltf::Node& node, tinygltf::Model model);
	static Mesh* processPrimitive(tinygltf::Primitive primitive, tinygltf::Model model);
};

