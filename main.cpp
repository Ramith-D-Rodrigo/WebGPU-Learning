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

using namespace wgpu;
using namespace std;

class Application {
public:
    bool Initialize();	// Initialize the application and return true if successful
    void Terminate();	// Terminate the application
    void MainLoop();	// Run the main loop
    bool IsRunning();	// Return true if the application is running

private:
    TextureView GetNextSurfaceTextureView();	// Get the next surface texture view

private:
    GLFWwindow* window;
    Device device = nullptr;
    Queue queue = nullptr;
    Surface surface = nullptr;

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
    this->window = glfwCreateWindow(800, 600, "WebGPU", nullptr, nullptr);
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
    deviceDescriptor.requiredLimits = nullptr; // we do not require any specific limit
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = "The default queue";

    auto onDeviceLost = [](WGPUDeviceLostReason reason, char const * message, void* /* pUserData */) {
        cout << "Device lost: reason " << reason;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };

    deviceDescriptor.deviceLostCallback = onDeviceLost;// A function that is invoked whenever the device stops being available.
    //this->device = requestDeviceSync(adapter, &deviceDescriptor); we do not need to use this function as we can use the following function
    this->device = adapter.requestDevice(deviceDescriptor);

    auto onDeviceError = [](ErrorType type, char const* message) {
        cout << "Uncaptured device error: type " << type;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };
    this->device.setUncapturedErrorCallback(onDeviceError);

    if (!this->device) {
        cout << "Failed to get the device" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }

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
    config.width = 800;
    config.height = 600;

    TextureFormat surfaceFormat = this->surface.getPreferredFormat(adapter);
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

    return true;
}

void Application::Terminate()
{
    //unconfigure the surface
    this->surface.unconfigure();

    //release the surface
    this->surface.release();

    //release queue
    this->queue.release();

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
    renderPassColorAttachment.clearValue = Color{ 0.5f, 0.8f, 0.0f, 1.0f }; //any value is fine

    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;  //because we do not use the depth buffer

    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &renderPassColorAttachment;

    RenderPassEncoder renderPass = encoder.beginRenderPass(renderPassDescriptor);
    renderPass.end();
    renderPass.release();

    //encode and submit the render pass commands
    CommandBufferDescriptor commandBufferDescriptor = {};
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = "Command Buffer";
    CommandBuffer commandBuffer = encoder.finish(commandBufferDescriptor);

    cout<<"Submitting the command buffer"<<endl;
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
