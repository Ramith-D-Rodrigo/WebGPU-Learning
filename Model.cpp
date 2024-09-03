#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

#include <string>
#include "Model.h"
#include "Mesh.h"
#include "SceneObject.h"

using namespace std;

SceneObject* Model::LoadModel(string filePath) {
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    string err;
    string warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, "D:\\Uni\\3D Models\\models\\base_sponza\\NewSponza_Main_glTF_003.gltf");
    //bool ret = loader.LoadBinaryFromFile(&model, &err, &warn, argv[1]); // for binary glTF(.glb)

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("Err: %s\n", err.c_str());
    }

    if (!ret) {
        printf("Failed to parse glTF\n");
        return nullptr;
    }

    SceneObject* rootSceneObject = new SceneObject();
    processData(model, rootSceneObject);

    return rootSceneObject;
}
void Model::processData(tinygltf::Model model, SceneObject* rootSceneObject) {
    // Access the GLTF JSON data and buffers

    // traverse all the scenes
    for (int i = 0; i < model.scenes.size(); i++) {
        tinygltf::Scene scene = model.scenes[i];
        processScene(scene, model, rootSceneObject);
    }
}

void Model::processScene(tinygltf::Scene scene, tinygltf::Model model, SceneObject* rootSceneObject) {
    if (scene.nodes.empty()) {
        return;
    }

    //traverse all the nodes
    for (int i = 0; i < scene.nodes.size(); i++) {
        tinygltf::Node& node = model.nodes[scene.nodes[i]];

        SceneObject* child = processNode(node, model);
        rootSceneObject->addChild(child);
    }
}

SceneObject* Model::processNode(tinygltf::Node& node, tinygltf::Model model) {
    vector<double> localTranslation = node.translation.size() == 0 ? vector<double>{ 0, 0, 0 } : node.translation;
    vector<double> localRotation = node.rotation.size() == 0 ? vector<double>{0, 0, 0, 0} : node.rotation;
    vector<double> localScale = node.scale.size() == 0 ? vector<double>{1, 1, 1} : node.scale;

    SceneObject* sceneObject = new SceneObject();
    sceneObject->setTranslation(localTranslation);
    sceneObject->setRotation(localRotation);
    sceneObject->setScale(localScale);

    if (node.mesh != -1) {
        //traverse all the primitives of the mesh
        int meshIndex = node.mesh;
        tinygltf::Mesh mesh = model.meshes[meshIndex];
        for (int i = 0; i < mesh.primitives.size(); i++) {
            tinygltf::Primitive primitive = mesh.primitives[i];
            Mesh* meshPrimitive = processPrimitive(primitive, model);
            sceneObject->addVisualObject(meshPrimitive);
        }

    }

    if (node.children.size() != 0) {
        for (int i = 0; i < node.children.size(); i++) {
            tinygltf::Node childNode = model.nodes[node.children[i]];
            SceneObject* childResult = processNode(childNode, model);
            sceneObject->addChild(childResult);
        }
    }

    return sceneObject;
}

Mesh* Model::processPrimitive(tinygltf::Primitive primitive, tinygltf::Model model) {
    const float* vertices = nullptr;
    size_t vertexCount = 0;

    // Get the POSITION attribute
    if (primitive.attributes.find("POSITION") != primitive.attributes.end()) {
        int verticesIndex = primitive.attributes["POSITION"];
        const tinygltf::Accessor& accessor = model.accessors[verticesIndex];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        vertices = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        vertexCount = accessor.count * 3; // Assuming each vertex has x, y, z components
    }

    const float* normals = nullptr;
    size_t normalCount = 0;

    // Get the NORMAL attribute
    if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
        int normalsIndex = primitive.attributes["NORMAL"];
        const tinygltf::Accessor& accessor = model.accessors[normalsIndex];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        normals = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        normalCount = accessor.count * 3; // Assuming each normal has x, y, z components
    }

    const uint16_t* indices = nullptr;
    size_t indexCount = 0;

    // Get the indices
    if (primitive.indices >= 0) {
        const tinygltf::Accessor& accessor = model.accessors[primitive.indices];
        const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
        const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

        indices = reinterpret_cast<const uint16_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        indexCount = accessor.count;
    }

    // Create a mesh object
    return new Mesh(vertices, vertexCount, indices, indexCount, normals, normalCount);
}
