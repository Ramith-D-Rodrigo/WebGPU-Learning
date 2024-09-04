#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <string>
#include "Model.h"
#include "Mesh.h"
#include "SceneObject.h"

// Define static members
wgpu::Device Model::device = nullptr;
wgpu::BindGroupLayout Model::textureBindGroupLayout = nullptr;
wgpu::BindGroupLayout Model::modelBindGroupLayout = nullptr;
wgpu::Buffer Model::modelUniformBuffer = nullptr;
wgpu::TextureView Model::textureView = nullptr;
wgpu::Sampler Model::sampler = nullptr;

SceneObject* Model::LoadModel(const std::string& filePath,
    wgpu::Device pDevice,
    wgpu::BindGroupLayout pTextureBindGroupLayout,
    wgpu::BindGroupLayout pModelBindGroupLayout,
    wgpu::TextureView pTextureView,
    wgpu::Sampler pSampler) {

    // Use move semantics to avoid unnecessary copying
    Model::device = std::move(pDevice);
    Model::textureBindGroupLayout = std::move(pTextureBindGroupLayout);
    Model::modelBindGroupLayout = std::move(pModelBindGroupLayout);
    Model::textureView = std::move(pTextureView);
    Model::sampler = std::move(pSampler);

    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    if (!loader.LoadASCIIFromFile(&model, &err, &warn, filePath)) {
        if (!warn.empty()) {
            printf("Warn: %s\n", warn.c_str());
        }
        if (!err.empty()) {
            printf("Err: %s\n", err.c_str());
        }
        printf("Failed to parse glTF\n");
        return nullptr;
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    auto rootSceneObject = std::make_unique<SceneObject>(&Model::device, &Model::modelBindGroupLayout);
    processData(model, rootSceneObject.get());

    return rootSceneObject.release();
}

void Model::processData(const tinygltf::Model& model, SceneObject* rootSceneObject) {
    std::cout << "processing data\n";

    for (const auto& scene : model.scenes) {
        processScene(scene, model, rootSceneObject);
    }
}

void Model::processScene(const tinygltf::Scene& scene, const tinygltf::Model& model, SceneObject* rootSceneObject) {
    if (scene.nodes.empty()) return;

    std::cout << "processing scene\n";

    for (const auto nodeIdx : scene.nodes) {
        auto& node = model.nodes[nodeIdx];
        auto child = processNode(node, model);
        rootSceneObject->addChild(child);
    }
}

SceneObject* Model::processNode(const tinygltf::Node& node, const tinygltf::Model& model) {
    std::cout << "processing node\n";

    glm::vec3 localTranslation = node.translation.size() == 0 ? glm::vec3() :
        glm::vec3((float)node.translation[0], (float)node.translation[1], (float)node.translation[2]);
    glm::quat localRotation = node.rotation.size() == 0 ? glm::quat() :
        glm::quat((float)node.rotation[3], (float)node.rotation[0], (float)node.rotation[1], (float)node.rotation[2]);
    glm::vec3 localScale = node.scale.size() == 0 ? glm::vec3(1, 1, 1) :
        glm::vec3((float)node.scale[0], (float)node.scale[1], (float)node.scale[2]);

    SceneObject* sceneObject = new SceneObject(&Model::device, &Model::modelBindGroupLayout);
    sceneObject->setTranslation(localTranslation);
    sceneObject->setRotation(localRotation);
    sceneObject->setScale(localScale);

    if (node.mesh != -1) {
        const auto& mesh = model.meshes[node.mesh];
        for (const auto& primitive : mesh.primitives) {
            auto meshPrimitive = processPrimitive(primitive, model);
            sceneObject->addVisualObject(std::move(meshPrimitive));
        }
    }

    for (const auto& childIdx : node.children) {
        auto& childNode = model.nodes[childIdx];
        auto childResult = processNode(childNode, model);
        sceneObject->addChild(std::move(childResult));
    }

    return sceneObject;
}

Mesh* Model::processPrimitive(const tinygltf::Primitive& primitive, const tinygltf::Model& model) {
    std::cout << "processing primitive\n";

    // Extract data from POSITION attribute
    auto extractBufferData = [&](const std::string& attribute, size_t componentCount, const float*& data, size_t& count) {
        auto it = primitive.attributes.find(attribute);
        if (it != primitive.attributes.end()) {
            const auto& accessor = model.accessors[it->second];
            const auto& bufferView = model.bufferViews[accessor.bufferView];
            const auto& buffer = model.buffers[bufferView.buffer];
            data = reinterpret_cast<const float*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
            count = accessor.count * componentCount;
        }
    };

    const float* vertices = nullptr;
    size_t vertexCount = 0;
    extractBufferData("POSITION", 3, vertices, vertexCount);

    const float* normals = nullptr;
    size_t normalCount = 0;
    extractBufferData("NORMAL", 3, normals, normalCount);

    const float* uvs = nullptr;
    size_t uvCount = 0;
    extractBufferData("TEXCOORD_0", 2, uvs, uvCount);

    const uint32_t* indices = nullptr;
    size_t indexCount = 0;
    if (primitive.indices >= 0) {
        const auto& accessor = model.accessors[primitive.indices];
        const auto& bufferView = model.bufferViews[accessor.bufferView];
        const auto& buffer = model.buffers[bufferView.buffer];
        indices = reinterpret_cast<const uint32_t*>(&buffer.data[bufferView.byteOffset + accessor.byteOffset]);
        indexCount = accessor.count;
    }

    std::cout << "creating mesh\n";
    return new Mesh(vertices, vertexCount, indices, indexCount, normals, normalCount, uvs, uvCount,
        Model::textureView, Model::textureBindGroupLayout, Model::device, Model::sampler);
}
