// Include the C++ wrapper instead of the raw header(s)
#define WEBGPU_CPP_IMPLEMENTATION

#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

#include <iostream>
#include <cassert>
#include <vector>
#include <array>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "utils.h"
//#include "build/SceneObject.h"
//#include "build/Model.h"

#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 720

using namespace wgpu;
using namespace std;
using namespace glm;


// We define a function that hides implementation-specific variants of device polling:
static void wgpuPollEvents([[maybe_unused]] Device device, [[maybe_unused]] bool yieldToWebBrowser) {
#if defined(WEBGPU_BACKEND_DAWN)
    device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
    device.poll(false);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(100);
    }
#endif
}

struct MyUniforms {
    mat4x4 projectionMatrix;
    mat4x4 viewMatrix;
    mat4x4 modelMatrix;
    vec4 color;
    float time;
    float padding[3]; // padding to make the size of the struct a multiple of 16 bytes
};

static_assert(sizeof(MyUniforms) % 16 == 0, "MyUniforms size must be a multiple of 16 bytes");

MyUniforms uniforms = {};

vec4 colorOne = { 0.0f, 1.0f, 0.4f, 1.0f };
vec4 colorTwo = { 1.0f, 1.0f, 1.0f, 0.7f };

/**
 * A structure that describes the data layout in the vertex buffer
 * We do not instantiate it but use it in `sizeof` and `offsetof`
 */
struct VertexAttributes {
    vec3 position;
    vec3 normal;
    vec3 color;
    vec2 uv;
};

class Application {
public:
    bool Initialize();	// Initialize the application and return true if successful
    void Terminate();	// Terminate the application
    void MainLoop();	// Run the main loop
    bool IsRunning();	// Return true if the application is running

private:
    TextureView GetNextSurfaceTextureView();	// Get the next surface texture view
    void InitializePipeline(BindGroupLayoutDescriptor* bindGroupLayoutDescriptor, 
        BindGroupLayout* bindGroupLayout);	// Initialize the pipeline
    void PlayWithBuffers();	// Play with buffers
    RequiredLimits GetRequiredLimits(Adapter adapter);	// Get the required limits
    bool InitializeBuffers(BindGroupLayout* bindGroupLayout);

private:
    GLFWwindow* window = nullptr;
    Device device = nullptr;
    Queue queue = nullptr;
    Surface surface = nullptr;
    TextureFormat surfaceFormat = TextureFormat::Undefined;
    RenderPipeline renderPipeline = nullptr;
    Buffer pointBuffer = nullptr;
    Buffer indexBuffer = nullptr;
    uint32_t indexCount = 0;
    Buffer uniformBuffer = nullptr;
    BindGroup bindGroup = nullptr;
    uint32_t uniformStride = 0;
    Texture depthTexture = nullptr;
    TextureView depthTextureView = nullptr;
    Texture colorTexture = nullptr;
    TextureView colorTextureView = nullptr;
    Sampler sampler = nullptr;

    mat4x4 S = glm::scale(mat4x4(1.0), vec3(2.0f));
    mat4x4 T1 = glm::translate(mat4x4(1.0), vec3(0.5, 0.0, 0.0));
    mat4x4 T2 = glm::translate(mat4x4(1.0), vec3(0.0, 0.0, -2.0));
    mat4x4 R1 = glm::rotate(mat4x4(1.0), 0.0f, vec3(0.0, 0.0, 1.0));
    mat4x4 R2 = glm::rotate(mat4x4(1.0), 0.0f, vec3(1.0, 0.0, 0.0));
};

int main() {
    Application app;

    if(!app.Initialize()) {
		return 1;
	}
    #ifdef __EMSCRIPTEN__
    auto emScriptCallback = [](void* arg) {
        Application* app = static_cast<Application*>(arg); //get the application

        app->MainLoop(); //run the main loop
        };

    emscripten_set_main_loop_arg(emScriptCallback, &app, 0, 1);
    #else // NOT __EMSCRIPTEN__
    while (app.IsRunning()) {
		app.MainLoop();
	}
    #endif // NOT __EMSCRIPTEN__

	app.Terminate();

	return 0;
}

bool Application::Initialize()
{
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return false;
    }

    //create a window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    this->window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "WebGPU", nullptr, nullptr);
    if (!this->window) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return false;
    }

    //get the descriptor
    InstanceDescriptor descriptor = Default;

    #ifdef WEBGPU_BACKEND_DAWN
    // Make sure the uncaptured error callback is called as soon as an error
    // occurs rather than at the next call to "wgpuDeviceTick".
    DawnTogglesDescriptor toggles;
    toggles.chain.next = nullptr;
    toggles.chain.sType = SType::DawnTogglesDescriptor;
    toggles.disabledToggleCount = 0;
    toggles.enabledToggleCount = 1;
    const char* toggleName = "enable_immediate_error_handling";
    toggles.enabledToggles = &toggleName;

    descriptor.nextInChain = &toggles.chain;
    #endif // WEBGPU_BACKEND_DAWN

    //create the instance
    #ifdef WEBGPU_BACKEND_EMSCRIPTEN
    Instance instance = createInstance(nullptr); // Emscripten doesn't support WGPUInstanceDescriptor
    #else
    Instance instance = createInstance(descriptor);
    #endif

    if (!instance) {
        cout << "Failed to create WebGPU instance" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }

    //get the surface
    this->surface = glfwGetWGPUSurface(instance, window);

    cout << "WebGPU instance created successfully and instance is " << instance << endl;

    cout << "Requesting adapter synchronously" << endl;

    RequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.powerPreference = PowerPreference::HighPerformance;
    options.compatibleSurface = this->surface;

    //Adapter adapter = requestAdapterSync(instance, &options); we do not need to use this function as we can use the following function
    Adapter adapter = instance.requestAdapter(options);

    //release the instance
    instance.release();

    if (!adapter) {
        cout << "Failed to get the adapter" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }
    cout << "Adapter obtained successfully and adapter is " << adapter << endl;

    //inspectAdapter(adapter);

    cout << "Requesting device synchronously" << endl;

    DeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = "My Device"; // anything works here, that's your call
    deviceDescriptor.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = "The default queue";

    auto onDeviceLost = [](WGPUDevice const */*device*/, WGPUDeviceLostReason reason, char const * message, void* /* pUserData */) {
        cout << "Device lost: reason " << reason;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };

    DeviceLostCallbackInfo deviceLostCallbackInfo = {};
    deviceLostCallbackInfo.callback = onDeviceLost;
    deviceLostCallbackInfo.mode = CallbackMode::AllowProcessEvents;

    deviceDescriptor.deviceLostCallbackInfo = deviceLostCallbackInfo;
   // deviceDescriptor.deviceLostCallback = onDeviceLost;// A function that is invoked whenever the device stops being available. (DEPRECATED)
   
    RequiredLimits requiredLimits = GetRequiredLimits(adapter);
    deviceDescriptor.requiredLimits = &requiredLimits;

    //this->device = requestDeviceSync(adapter, &deviceDescriptor); we do not need to use this function as we can use the following function
    this->device = adapter.requestDevice(deviceDescriptor);

    if (!this->device) {
        cout << "Failed to get the device" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }
    auto onDeviceError = [](ErrorType type, char const* message) {
        cout << "Uncaptured device error: type " << type;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };

    this->device.setUncapturedErrorCallback(onDeviceError);
    cout << "Device obtained successfully and device is " << this->device << endl;

    //inspectDevice(this->device);

    //create queue to pass commands to the device
    this->queue = this->device.getQueue();

    //auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
    //    cout << "Queued work finished with status: " << status << endl;
    //    };

    //wgpuQueueOnSubmittedWorkDone(this->queue, onQueueWorkDone, nullptr /* pUserData */);

    //configure the surface
    SurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = WINDOW_WIDTH;
    config.height = WINDOW_HEIGHT;

    this->surfaceFormat = this->surface.getPreferredFormat(adapter);
    config.format = surfaceFormat;

    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = TextureUsage::RenderAttachment;
    config.device = this->device;

    config.presentMode = PresentMode::Fifo;
    config.alphaMode = CompositeAlphaMode::Auto;

    this->surface.configure(config);

    //release the adapter
    adapter.release();

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    BindGroupLayout bindGroupLayout = nullptr;

    //initialize the pipeline
    this->InitializePipeline(&bindGroupLayoutDescriptor, &bindGroupLayout);

    //Experimenting with the buffers
    //this->PlayWithBuffers();
    if (!this->InitializeBuffers(&bindGroupLayout)) {
        return false;
    }

    return true;
}

void Application::Terminate()
{
    this->indexBuffer.destroy();
    this->indexBuffer.release();
    this->pointBuffer.destroy();
    this->pointBuffer.release();

    // Destroy the depth texture and its view
    this->depthTextureView.release();
    this->depthTexture.destroy();
    this->depthTexture.release();

    // Destroy the color texture
    this->colorTexture.destroy();
    this->colorTexture.release();

    //release the pipeline
    this->renderPipeline.release();

    //unconfigure the surface
    this->surface.unconfigure();

    //release queue
    this->queue.release();

    //release the surface
    this->surface.release();

    //release the device
    this->device.release();

    //destroy the window
    glfwDestroyWindow(this->window);

    glfwTerminate();
}

void Application::MainLoop()
{
    TextureView targetView = this->GetNextSurfaceTextureView();
    if (!targetView) {
        return;
    }

    glfwPollEvents();

    //update the uniform buffer 
    uniforms.time = static_cast<float>(glfwGetTime());
    this->queue.writeBuffer(this->uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

    //uniforms.time = -newTime * 10;
    //this->queue.writeBuffer(this->uniformBuffer, this->uniformStride + offsetof(MyUniforms, time), &uniforms.time, sizeof(MyUniforms::time));

    float angle = uniforms.time;
    R1 = glm::rotate(mat4x4(1.0), angle, vec3(0.0, 0.0, 1.0));
    uniforms.modelMatrix = R1 * T1 * S;
    this->queue.writeBuffer(uniformBuffer, offsetof(MyUniforms, modelMatrix), &uniforms.modelMatrix, sizeof(MyUniforms::modelMatrix));

    CommandEncoderDescriptor commandEncoderDescriptor = {};
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label = "Command Encoder";

    CommandEncoder encoder = this->device.createCommandEncoder(commandEncoderDescriptor);

    RenderPassDescriptor renderPassDescriptor = {};
    renderPassDescriptor.nextInChain = nullptr;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.timestampWrites = nullptr; //do not measure the time

    RenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;

    renderPassColorAttachment.loadOp = LoadOp::Clear;
    renderPassColorAttachment.storeOp = StoreOp::Store;
    renderPassColorAttachment.clearValue = Color{ 0.2f, 0.2f, 0.2f, 1.0f }; //any value is fine

    #ifndef WEBGPU_BACKEND_WGPU
    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;  //because we do not use the depth buffer
    #endif // NOT WEBGPU_BACKEND_WGPU

    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &renderPassColorAttachment;

    //Add a depth/stencil attachment
    RenderPassDepthStencilAttachment renderPassDepthStencilAttachment = {};
    renderPassDepthStencilAttachment.view = this->depthTextureView;
    renderPassDepthStencilAttachment.depthClearValue = 1.0f;    //initial far value
    renderPassDepthStencilAttachment.depthLoadOp = LoadOp::Clear;
    renderPassDepthStencilAttachment.depthStoreOp = StoreOp::Store;
    renderPassDepthStencilAttachment.depthReadOnly = false; //turn off writing to the depth buffer globally

    //mandatory for depth/stencil attachment
    renderPassDepthStencilAttachment.stencilClearValue = 0;    //initial stencil value
    renderPassDepthStencilAttachment.stencilLoadOp = LoadOp::Undefined;
    renderPassDepthStencilAttachment.stencilStoreOp = StoreOp::Undefined;
    renderPassDepthStencilAttachment.stencilReadOnly = true;

    //constexpr auto NaNf = std::numeric_limits<float>::quiet_NaN();
    //renderPassDepthStencilAttachment.clearDepth = NaNf;


    renderPassDescriptor.depthStencilAttachment = &renderPassDepthStencilAttachment;

    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDescriptor);

    // Select which render pipeline to use
    renderPass.setPipeline(this->renderPipeline);
    renderPass.setVertexBuffer(0, this->pointBuffer, 0, this->pointBuffer.getSize());
    //uint must correspond to the index buffer data type
    renderPass.setIndexBuffer(this->indexBuffer, IndexFormat::Uint16, 0, this->indexBuffer.getSize()); 

    // dynamic offset for multiple uniform buffers
    //uint32_t dynamicOffset = 0;

    //dynamicOffset = 0 * this->uniformStride;

    // Set the binding group
    renderPass.setBindGroup(0, this->bindGroup, 0, nullptr);
    // Draw 1 instance of a 3-vertices shape
    renderPass.drawIndexed(this->indexCount, 1, 0, 0, 0);

    // Set the binding group with different dynamic offset
    //dynamicOffset = 1 * this->uniformStride;
    //renderPass.setBindGroup(0, this->bindGroup, 1, &dynamicOffset);
    // Draw 1 instance of a 3-vertices shape
    //renderPass.drawIndexed(this->indexCount, 1, 0, 0, 0);

    renderPass.end();
    renderPass.release();

    //encode and submit the render pass commands
    CommandBufferDescriptor commandBufferDescriptor = {};
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = "Command Buffer";
    CommandBuffer commandBuffer = encoder.finish(commandBufferDescriptor);

    //cout<<"Submitting the command buffer"<<endl;
    this->queue.submit(1, &commandBuffer);
    commandBuffer.release();

    //release the texture view
    targetView.release();

    //present the surface
    #ifndef __EMSCRIPTEN__
        this->surface.present();
    #endif // !__EMSCRIPTEN__

    #if defined(WEBGPU_BACKEND_DAWN)
            this->device.tick();
    #elif defined(WEBGPU_BACKEND_WGPU)
            wgpuDevicePoll(device, false, nullptr);
    #endif
}

bool Application::IsRunning()
{
    return !glfwWindowShouldClose(this->window);
}

void Application::InitializePipeline(BindGroupLayoutDescriptor* bindGroupLayoutDescriptor, BindGroupLayout* bindGroupLayout)
{
    ShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", this->device);

    //now create the pipeline
    RenderPipelineDescriptor renderPipelineDescriptor = Default;

    //vertex pipeline state

    VertexBufferLayout vertexBufferLayout;
    vector<VertexAttribute> vertexAttributes(4);

    //position attribute
    vertexAttributes[0].shaderLocation = 0; //location in the shader (@location(0))
    vertexAttributes[0].format = VertexFormat::Float32x3; //XYZ position (3D)
    vertexAttributes[0].offset = offsetof(VertexAttributes, position); //offset in the vertex buffer

    //normal attribute
    vertexAttributes[1].shaderLocation = 1;
    vertexAttributes[1].format = VertexFormat::Float32x3;
    vertexAttributes[1].offset = offsetof(VertexAttributes, normal);

    //color attribute
    vertexAttributes[2].shaderLocation = 2;
    vertexAttributes[2].format = VertexFormat::Float32x3;
    vertexAttributes[2].offset = offsetof(VertexAttributes, color);

    // UV attribute
    vertexAttributes[3].shaderLocation = 3;
    vertexAttributes[3].format = VertexFormat::Float32x2;
    vertexAttributes[3].offset = offsetof(VertexAttributes, uv);

    vertexBufferLayout.attributeCount = static_cast<uint32_t>(vertexAttributes.size());
    vertexBufferLayout.attributes = vertexAttributes.data();
    vertexBufferLayout.stepMode = VertexStepMode::Vertex; //each vertex is a separate entity (not instanced)
    vertexBufferLayout.arrayStride = sizeof(VertexAttributes);

    renderPipelineDescriptor.vertex.bufferCount = 1;
    renderPipelineDescriptor.vertex.buffers = &vertexBufferLayout;


    renderPipelineDescriptor.vertex.module = shaderModule;
    renderPipelineDescriptor.vertex.entryPoint = "vs_main"; //vertex shader main
    renderPipelineDescriptor.vertex.constantCount = 0;
    renderPipelineDescriptor.vertex.constants = nullptr;

    //primitive pipeline state
    // 
    // Here we have to specify the type of primitives that will be drawn.
    // 
    // Each sequence of 3 vertices is considered as a triangle
    renderPipelineDescriptor.primitive.topology = PrimitiveTopology::TriangleList;

    // We'll see later how to specify the order in which vertices should be
    // connected. When not specified, vertices are considered sequentially.
    renderPipelineDescriptor.primitive.stripIndexFormat = IndexFormat::Undefined;

    // The face orientation is defined by assuming that when looking
    // from the front of the face, its corner vertices are enumerated
    // in the counter-clockwise (CCW) order.
    renderPipelineDescriptor.primitive.frontFace = FrontFace::CCW;

    // But the face orientation does not matter much because we do not
    // cull (i.e. "hide") the faces pointing away from us (which is often
    // used for optimization).
    renderPipelineDescriptor.primitive.cullMode = CullMode::None;

    //fragment pipeline state (this is optional)
    FragmentState fragmentState = {};
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main"; //fragment shader main
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    //stencil/depth pipeline state
    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less; //only blend the pixel if the depth is less than the existing depth
    depthStencilState.depthWriteEnabled = true; //write the depth value to the depth buffer every time a pixel is drawn
    TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
    depthStencilState.format = depthTextureFormat;

    //disable stencil
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    renderPipelineDescriptor.depthStencil = &depthStencilState;

    //Create depth texture
    TextureDescriptor depthTextureDescriptor = {};
    depthTextureDescriptor.dimension = TextureDimension::_2D;
    depthTextureDescriptor.format = depthTextureFormat;
    depthTextureDescriptor.mipLevelCount = 1;
    depthTextureDescriptor.sampleCount = 1;
    depthTextureDescriptor.size = { WINDOW_WIDTH, WINDOW_HEIGHT, 1 };
    depthTextureDescriptor.usage = TextureUsage::RenderAttachment;
    depthTextureDescriptor.viewFormatCount = 1;
    depthTextureDescriptor.viewFormats = (WGPUTextureFormat*) &depthTextureFormat;

    this->depthTexture = this->device.createTexture(depthTextureDescriptor);

    //Create depth texture view
    TextureViewDescriptor depthTextureViewDescriptor = {};
    depthTextureViewDescriptor.aspect = TextureAspect::DepthOnly;
    depthTextureViewDescriptor.baseArrayLayer = 0;
    depthTextureViewDescriptor.arrayLayerCount = 1;
    depthTextureViewDescriptor.baseMipLevel = 0;
    depthTextureViewDescriptor.mipLevelCount = 1;
    depthTextureViewDescriptor.dimension = TextureViewDimension::_2D;
    depthTextureViewDescriptor.format = depthTextureFormat;

    this->depthTextureView = this->depthTexture.createView(depthTextureViewDescriptor);

    //Create color texture
    TextureDescriptor textureDescriptor = {};
    textureDescriptor.dimension = TextureDimension::_2D;
    textureDescriptor.format = TextureFormat::RGBA8Unorm;
    textureDescriptor.mipLevelCount = 8;
    textureDescriptor.sampleCount = 1;
    textureDescriptor.size = { WINDOW_WIDTH, WINDOW_HEIGHT, 1 };
    textureDescriptor.usage = TextureUsage::TextureBinding | TextureUsage::CopyDst; //copy is needed to copy pixel data from c++ code
    //texture binding is needed to bind the texture to the shader
    textureDescriptor.viewFormatCount = 0;
    textureDescriptor.viewFormats = nullptr;

    this->colorTexture = this->device.createTexture(textureDescriptor);

    TextureViewDescriptor textureViewDesc;
    textureViewDesc.aspect = TextureAspect::All;
    textureViewDesc.baseArrayLayer = 0;
    textureViewDesc.arrayLayerCount = 1;
    textureViewDesc.baseMipLevel = 0;
    textureViewDesc.mipLevelCount = textureDescriptor.mipLevelCount;
    textureViewDesc.dimension = TextureViewDimension::_2D;
    textureViewDesc.format = textureDescriptor.format;
    this->colorTextureView = this->colorTexture.createView(textureViewDesc);

    //vector<uint8_t> pixels = createAmazingTexture(textureDescriptor);

    // Create a sampler
    SamplerDescriptor samplerDesc = {};
    samplerDesc.addressModeU = AddressMode::Repeat;
    samplerDesc.addressModeV = AddressMode::Repeat;
    samplerDesc.addressModeW = AddressMode::ClampToEdge;
    samplerDesc.magFilter = FilterMode::Linear;
    samplerDesc.minFilter = FilterMode::Linear;
    samplerDesc.mipmapFilter = MipmapFilterMode::Linear;
    samplerDesc.lodMinClamp = 0.0f;
    samplerDesc.lodMaxClamp = 8.0f;
    samplerDesc.compare = CompareFunction::Undefined;
    samplerDesc.maxAnisotropy = 1;
    this->sampler = this->device.createSampler(samplerDesc);

    // Arguments telling which part of the texture we upload to
    // (together with the last argument of writeTexture)
    ImageCopyTexture destination;
    destination.texture = this->colorTexture;
    destination.mipLevel = 0;
    destination.origin = { 0, 0, 0 };   //equivalent to offset arg in write buffer
    destination.aspect = TextureAspect::All;    //this only relevant for depth textures

    // Arguments telling how the C++ side pixel memory is laid out
    TextureDataLayout source;
    source.offset = 0;
    source.bytesPerRow = 4 * textureDescriptor.size.width;
    source.rowsPerImage = textureDescriptor.size.height;

    Extent3D mipLevelSize = textureDescriptor.size;
    std::vector<uint8_t> previousLevelPixels;
	for (uint32_t level = 0; level < textureDescriptor.mipLevelCount; ++level) {
        // Create image data for this mip level
        vector<uint8_t> pixels(4 * mipLevelSize.width * mipLevelSize.height);
        for (uint32_t i = 0; i < mipLevelSize.width; ++i) {
            for (uint32_t j = 0; j < mipLevelSize.height; ++j) {
                uint8_t* p = &pixels[4 * (j * mipLevelSize.width + i)];
                if (level == 0) {
                    p[0] = (i / 16) % 2 == (j / 16) % 2 ? 255 : 0; // r
                    p[1] = ((i - j) / 16) % 2 == 0 ? 255 : 0; // g
                    p[2] = ((i + j) / 16) % 2 == 0 ? 255 : 0; // b
                }
                else {
                    // Get the corresponding 4 pixels from the previous level
                    uint8_t* p00 = &previousLevelPixels[4 * ((2 * j + 0) * (2 * mipLevelSize.width) + (2 * i + 0))];
                    uint8_t* p01 = &previousLevelPixels[4 * ((2 * j + 0) * (2 * mipLevelSize.width) + (2 * i + 1))];
                    uint8_t* p10 = &previousLevelPixels[4 * ((2 * j + 1) * (2 * mipLevelSize.width) + (2 * i + 0))];
                    uint8_t* p11 = &previousLevelPixels[4 * ((2 * j + 1) * (2 * mipLevelSize.width) + (2 * i + 1))];
                    // Average
                    p[0] = (p00[0] + p01[0] + p10[0] + p11[0]) / 4;
                    p[1] = (p00[1] + p01[1] + p10[1] + p11[1]) / 4;
                    p[2] = (p00[2] + p01[2] + p10[2] + p11[2]) / 4;
                }
                p[3] = 255; // a
            }
        }
        destination.mipLevel = level;

        //compute the size of the texture
        source.bytesPerRow = 4 * mipLevelSize.width;
        source.rowsPerImage = mipLevelSize.height;

        this->queue.writeTexture(destination, pixels.data(), pixels.size(), source, mipLevelSize);

        //decrease the size for the next mip level
        mipLevelSize.width /= 2;
        mipLevelSize.height /= 2;
        previousLevelPixels = std::move(pixels);
	}

    //blend pipeline state
    BlendState blendState = {};

    blendState.color.srcFactor = BlendFactor::SrcAlpha;
    blendState.color.dstFactor = BlendFactor::OneMinusSrcAlpha;
    blendState.color.operation = BlendOperation::Add;

    blendState.alpha.srcFactor = BlendFactor::Zero;
    blendState.alpha.dstFactor = BlendFactor::One;
    blendState.alpha.operation = BlendOperation::Add;

    ColorTargetState colorTargetState = {};
    colorTargetState.format = surfaceFormat;
    colorTargetState.blend = &blendState;
    colorTargetState.writeMask = ColorWriteMask::All;

    fragmentState.targetCount = 1;  //we have only one output color for the render pass
    fragmentState.targets = &colorTargetState;

    renderPipelineDescriptor.fragment = &fragmentState;

    //multisample pipeline state
    renderPipelineDescriptor.multisample.count = 1;
    renderPipelineDescriptor.multisample.mask = ~0u; //all bits are enabled
    renderPipelineDescriptor.multisample.alphaToCoverageEnabled = false;

    // Create binding layouts

    vector<BindGroupLayoutEntry> bindingLayoutEntries(3, Default);

    // The uniform buffer binding that we already had
    BindGroupLayoutEntry& bindingLayout = bindingLayoutEntries[0];
    bindingLayout.binding = 0;//index as used in the @binding attribute in the shader
    bindingLayout.visibility = ShaderStage::Vertex | ShaderStage::Fragment;//both vertex and fragment shaders
    bindingLayout.buffer.type = BufferBindingType::Uniform;
    bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

    // The texture binding
    BindGroupLayoutEntry& textureBindingLayout = bindingLayoutEntries[1];
    // Setup texture binding
    textureBindingLayout.binding = 1;
    textureBindingLayout.visibility = ShaderStage::Fragment;
    textureBindingLayout.texture.sampleType = TextureSampleType::Float;
    textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

    // the sampler binding
    BindGroupLayoutEntry& samplerBindingLayout = bindingLayoutEntries[2];
    samplerBindingLayout.binding = 2;
    samplerBindingLayout.visibility = ShaderStage::Fragment;
    samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

    bindGroupLayoutDescriptor->label = "Bind Group Layout";
    bindGroupLayoutDescriptor->entryCount = (uint32_t)bindingLayoutEntries.size();
    bindGroupLayoutDescriptor->entries = bindingLayoutEntries.data();

    *bindGroupLayout = this->device.createBindGroupLayout(*bindGroupLayoutDescriptor);

    //pipeline layout
    PipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.label = "Pipeline Layout";
    pipelineLayoutDescriptor.bindGroupLayoutCount = 1;
    pipelineLayoutDescriptor.bindGroupLayouts = (WGPUBindGroupLayout*)bindGroupLayout;
    
    renderPipelineDescriptor.layout = this->device.createPipelineLayout(pipelineLayoutDescriptor);
    this->renderPipeline = this->device.createRenderPipeline(renderPipelineDescriptor);

    shaderModule.release();
}

TextureView Application::GetNextSurfaceTextureView()
{
    SurfaceTexture surfaceTexture;
    this->surface.getCurrentTexture(&surfaceTexture);

    if (surfaceTexture.status != SurfaceGetCurrentTextureStatus::Success) {
        return nullptr;
    }

    Texture texture = surfaceTexture.texture;

    TextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = texture.getFormat();
    viewDescriptor.dimension = TextureViewDimension::_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = TextureAspect::All;

    TextureView targetView = texture.createView(viewDescriptor);

    return targetView;
}

void Application::PlayWithBuffers() {
    //create the first buffer
    const int BUFFER_SIZE = 16;

    BufferDescriptor bufferDescriptor = {};
    bufferDescriptor.label = "Buffer 1";
    bufferDescriptor.usage = BufferUsage::CopyDst | BufferUsage::CopySrc;   //we will copy data to and from this buffer
    bufferDescriptor.size = BUFFER_SIZE;
    bufferDescriptor.mappedAtCreation = false;

    Buffer buffer1 = this->device.createBuffer(bufferDescriptor);

    //create the second buffer
    bufferDescriptor.label = "Buffer 2";
    bufferDescriptor.usage = BufferUsage::CopyDst | BufferUsage::MapRead; //mapped for reading

    Buffer buffer2 = this->device.createBuffer(bufferDescriptor);

    //write input data
    //create an array for example
    vector<uint8_t> data(BUFFER_SIZE);
    for (size_t i = 0; i < data.size(); i++) {
        data[i] = static_cast<uint8_t>(i);
    }

    this->queue.writeBuffer(buffer1, 0, data.data(), BUFFER_SIZE); //copying data from RAM to VRAM

    //encode and submit the buffer to buffer copy commands
    CommandEncoder encoder = this->device.createCommandEncoder(Default);
    encoder.copyBufferToBuffer(buffer1, 0, buffer2, 0, BUFFER_SIZE);
    CommandBuffer commandBuffer = encoder.finish(Default);
    encoder.release();
    this->queue.submit(1, &commandBuffer);
    commandBuffer.release();

    //read the buffer data back

    struct Context {
        bool ready;
        Buffer buffer;
        int bufferSize;
    };

    auto onBuffer2Mapped = [](WGPUBufferMapAsyncStatus status, void* userData) { // callback function to be called when the buffer is mapped
        //re interpret the user data to bool
        Context* context = reinterpret_cast<Context*>(userData);
        context->ready = true;

        cout << "Buffer 2 mapped with status: " << status << endl;
        if (status != WGPUBufferMapAsyncStatus_Success) {
            cout << "Buffer 2 mapping failed" << endl;
            return;
        }
        //read the buffer data
        uint8_t* data = (uint8_t*)(context->buffer.getConstMappedRange(0, context->bufferSize));
        //we are using const mapped range because we are not going to modify the data (because we have set the buffer usage to MapRead)
        //we can use mapped range if the buffer usage is set to MapWrite

        //print the data
        cout << "Buffer 2 data: ";
        for (size_t i = 0; i < context->bufferSize; i++) {
            cout << static_cast<int>(data[i]) << " ";
        }

        //unmap the buffer
        context->buffer.unmap();
        };

    Context context = { false, buffer2, BUFFER_SIZE };

    wgpuBufferMapAsync(buffer2, MapMode::Read, 0, BUFFER_SIZE, onBuffer2Mapped, (void*)&context);


    while (!context.ready) {
        wgpuPollEvents(this->device, true);
    }

    //release the buffers
    buffer1.release();
    buffer2.release();
}

RequiredLimits Application::GetRequiredLimits(Adapter adapter)
{
    //first get the adapter supported limits
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    RequiredLimits requiredLimits = Default;

    //set the required limits
    requiredLimits.limits.maxVertexAttributes = 4;
    requiredLimits.limits.maxVertexBuffers = 1; //for now 1
    requiredLimits.limits.maxBufferSize = 16 * sizeof(VertexAttributes);
    requiredLimits.limits.maxVertexBufferArrayStride = sizeof(VertexAttributes);
    requiredLimits.limits.minStorageBufferOffsetAlignment = supportedLimits.limits.minStorageBufferOffsetAlignment;
    requiredLimits.limits.minUniformBufferOffsetAlignment = supportedLimits.limits.minUniformBufferOffsetAlignment;
    requiredLimits.limits.maxInterStageShaderComponents = 8; //3 for vertex and 3 for fragment (both normal and color)
    requiredLimits.limits.maxBindGroups = 1; //for now 1
    requiredLimits.limits.maxUniformBuffersPerShaderStage = 1; //for now 1
    requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4 * sizeof(float);
    //requiredLimits.limits.maxDynamicUniformBuffersPerPipelineLayout = 1; //for now 1

    requiredLimits.limits.maxTextureDimension1D = WINDOW_HEIGHT;
    requiredLimits.limits.maxTextureDimension2D = WINDOW_WIDTH;
    requiredLimits.limits.maxTextureArrayLayers = 1;
    requiredLimits.limits.maxSampledTexturesPerShaderStage = 1;
    requiredLimits.limits.maxSamplersPerShaderStage = 1;

    return requiredLimits;
}

bool Application::InitializeBuffers(BindGroupLayout* bindGroupLayout)
{
    // Define point data
    // The de-duplicated list of point positions
    vector<float> pointData;
    // Define index data
    // This is a list of indices referencing positions in the pointData
    vector<uint16_t> indexData;

  //  SceneObject* object = Model::LoadModel("D:\\Uni\\3D Models\\models\\base_sponza\\NewSponza_Main_glTF_003.gltf");
  //  if (!object) {
  //      cout<<"Failed to load the model"<<endl;
		//return false;
  //  }

    bool status = loadGeometry(RESOURCE_DIR "/pyramid.txt", pointData, indexData, 8);
    if (!status) {
        cout<<"Failed to load the geometry"<<endl;
		return false;
    }

    this->indexCount = static_cast<uint32_t>(indexData.size());

    BufferDescriptor bufferDescriptor = {};
    bufferDescriptor.label = "Vertex Buffer";
    bufferDescriptor.size = pointData.size() * sizeof(float);
    bufferDescriptor.usage = BufferUsage::Vertex | BufferUsage::CopyDst; //must add vertex usage
    bufferDescriptor.mappedAtCreation = false;
    this->pointBuffer = this->device.createBuffer(bufferDescriptor);

    this->queue.writeBuffer(this->pointBuffer, 0, pointData.data(), bufferDescriptor.size);

    ////create the normal buffer
    //bufferDescriptor.label = "Normal Buffer";

    //create the index buffer
    bufferDescriptor.label = "Index Buffer";
    bufferDescriptor.size = indexData.size() * sizeof(uint32_t);
    bufferDescriptor.usage = BufferUsage::Index | BufferUsage::CopyDst; //must add index usage
    this->indexBuffer = this->device.createBuffer(bufferDescriptor);

    bufferDescriptor.size = (bufferDescriptor.size + 3) & ~3; // round up to the next multiple of 4

    this->queue.writeBuffer(this->indexBuffer, 0, indexData.data(), bufferDescriptor.size);

    //create the uniform buffer
    SupportedLimits limits;
    this->device.getLimits(&limits);
    this->uniformStride = ceilToNextMultiple((uint32_t)sizeof(MyUniforms), (uint32_t)limits.limits.minUniformBufferOffsetAlignment);

    bufferDescriptor.label = "Uniform Buffer";
    bufferDescriptor.size = sizeof(MyUniforms) + this->uniformStride;
    bufferDescriptor.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    this->uniformBuffer = this->device.createBuffer(bufferDescriptor);

    constexpr float PI = 3.14159265358979323846f;

    float angle1 = (float)glfwGetTime();
    float angle2 = 3.0f * PI / 4.0;
    vec3 focalPoint(0.0, 0.0, -2.0);

    this->S = glm::scale(mat4x4(1.0), vec3(1.2f));
    this->T1 = glm::translate(mat4x4(1.0), vec3(0.5, 0.0, 0.0));
    this->R1 = glm::rotate(mat4x4(1.0), angle1, vec3(0.0, 0.0, 1.0));
    uniforms.modelMatrix = R1 * T1 * S;

    this->R2 = glm::rotate(mat4x4(1.0), -angle2, vec3(1.0, 0.0, 0.0));
    this->T2 = glm::translate(mat4x4(1.0), -focalPoint);
    uniforms.viewMatrix = T2 * R2;

    // Set the projection matrix
    float ratio = WINDOW_WIDTH / WINDOW_HEIGHT;
    float focalLength = 2.0;
    float nearView = 0.01f;
    float farView = 100.0f;
    float fov = 2 * glm::atan(1 / focalLength);
    uniforms.projectionMatrix = glm::perspective(fov, ratio, nearView, farView);


    uniforms.time = 1.0f;
    uniforms.color = colorOne;
    this->queue.writeBuffer(this->uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

    //uniforms.time = -1.0f;
    //uniforms.color = colorTwo;
    //this->queue.writeBuffer(this->uniformBuffer, this->uniformStride, &uniforms, sizeof(MyUniforms));

    // Create a binding
    vector<BindGroupEntry> bindings(3);
    // The index of the binding (the entries in bindGroupDesc can be in any order)
    bindings[0].binding = 0;
    // The buffer it is actually bound to
    bindings[0].buffer = this->uniformBuffer;
    // We can specify an offset within the buffer, so that a single buffer can hold
    // multiple uniform blocks.
    bindings[0].offset = 0;
    // And we specify again the size of the buffer.
    bindings[0].size = sizeof(MyUniforms);


    bindings[1].binding = 1;
    bindings[1].textureView = this->colorTextureView;

    bindings[2].binding = 2;
    bindings[2].sampler = this->sampler;

    // A bind group contains one or multiple bindings
    BindGroupDescriptor bindGroupDesc;
    bindGroupDesc.layout = *bindGroupLayout;
    // There must be as many bindings as declared in the layout!
    bindGroupDesc.entryCount = (uint32_t) bindings.size();
    bindGroupDesc.entries = bindings.data();
    this->bindGroup = device.createBindGroup(bindGroupDesc);

    cout<<"Buffers initialized successfully"<<endl;

    return true;
}
