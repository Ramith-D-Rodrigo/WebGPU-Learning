#include <iostream>
#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include "utils.h"

using namespace std;

class Application {
public:
    bool Initialize();	// Initialize the application and return true if successful
    void Terminate();	// Terminate the application
    void MainLoop();	// Run the main loop
    bool IsRunning();	// Return true if the application is running

private:
    WGPUTextureView GetNextSurfaceTextureView();	// Get the next surface texture view

private:
    GLFWwindow* window;
    WGPUDevice device;
    WGPUQueue queue;
    WGPUSurface surface;

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
    WGPUInstanceDescriptor descriptor = {};
    descriptor.nextInChain = nullptr;

    #ifdef WEBGPU_BACKEND_DAWN
    // Make sure the uncaptured error callback is called as soon as an error
    // occurs rather than at the next call to "wgpuDeviceTick".
    WGPUDawnTogglesDescriptor toggles;
    toggles.chain.next = nullptr;
    toggles.chain.sType = WGPUSType_DawnTogglesDescriptor;
    toggles.disabledToggleCount = 0;
    toggles.enabledToggleCount = 1;
    const char* toggleName = "enable_immediate_error_handling";
    toggles.enabledToggles = &toggleName;

    descriptor.nextInChain = &toggles.chain;
    #endif // WEBGPU_BACKEND_DAWN

    //create the instance
    #ifdef WEBGPU_BACKEND_EMSCRIPTEN
    WGPUInstance instance = wgpuCreateInstance(nullptr); // Emscripten doesn't support WGPUInstanceDescriptor
    #else
    WGPUInstance instance = wgpuCreateInstance(&descriptor);
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

    WGPURequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.powerPreference = WGPUPowerPreference_HighPerformance;
    options.compatibleSurface = this->surface;

    WGPUAdapter adapter = requestAdapterSync(instance, &options);

    //release the instance
    wgpuInstanceRelease(instance);

    if (!adapter) {
        cout << "Failed to get the adapter" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }
    cout << "Adapter obtained successfully and adapter is " << adapter << endl;

    inspectAdapter(adapter);

    cout << "Requesting device synchronously" << endl;

    WGPUDeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = "My Device"; // anything works here, that's your call
    deviceDescriptor.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDescriptor.requiredLimits = nullptr; // we do not require any specific limit
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = "The default queue";
    deviceDescriptor.deviceLostCallback = // A function that is invoked whenever the device stops being available.
        [](WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
        cout << "Device lost: reason " << reason;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };

    this->device = requestDeviceSync(adapter, &deviceDescriptor);

    auto onDeviceError = [](WGPUErrorType type, char const* message, void* /* pUserData */) {
        cout << "Uncaptured device error: type " << type;
        if (message) cout << " (" << message << ")";
        cout << endl;
        };
    wgpuDeviceSetUncapturedErrorCallback(this->device, onDeviceError, nullptr /* pUserData */);

    if (!this->device) {
        cout << "Failed to get the device" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }

    cout << "Device obtained successfully and device is " << this->device << endl;

    inspectDevice(this->device);

    //create queue to pass commands to the device
    this->queue = wgpuDeviceGetQueue(this->device);

    auto onQueueWorkDone = [](WGPUQueueWorkDoneStatus status, void* /* pUserData */) {
        cout << "Queued work finished with status: " << status << endl;
        };
    wgpuQueueOnSubmittedWorkDone(this->queue, onQueueWorkDone, nullptr /* pUserData */);

    //configure the surface
    WGPUSurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = 800;
    config.height = 600;

    WGPUTextureFormat surfaceFormat = wgpuSurfaceGetPreferredFormat(this->surface, adapter);
    config.format = surfaceFormat;

    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = WGPUTextureUsage_RenderAttachment;
    config.device = this->device;

    config.presentMode = WGPUPresentMode_Fifo;
    config.alphaMode = WGPUCompositeAlphaMode_Auto;

    wgpuSurfaceConfigure(this->surface, &config);

    //release the adapter
    wgpuAdapterRelease(adapter);

    return true;
}

void Application::Terminate()
{
    //unconfigure the surface
    wgpuSurfaceUnconfigure(this->surface);

    //release the surface
    wgpuSurfaceRelease(this->surface);

    //release queue
    wgpuQueueRelease(this->queue);

    //release the device
    wgpuDeviceRelease(this->device);

    //destroy the window
    glfwDestroyWindow(this->window);

    glfwTerminate();
}

void Application::MainLoop()
{
    WGPUTextureView targetView = this->GetNextSurfaceTextureView();
    if (!targetView) {
        return;
    }

    glfwPollEvents();

    WGPUCommandEncoderDescriptor commandEncoderDescriptor = {};
    commandEncoderDescriptor.nextInChain = nullptr;
    commandEncoderDescriptor.label = "Command Encoder";

    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(this->device, &commandEncoderDescriptor);

    WGPURenderPassDescriptor renderPassDescriptor = {};
    renderPassDescriptor.nextInChain = nullptr;
    renderPassDescriptor.depthStencilAttachment = nullptr;
    renderPassDescriptor.timestampWrites = nullptr; //do not measure the time

    WGPURenderPassColorAttachment renderPassColorAttachment = {};
    renderPassColorAttachment.view = targetView;
    renderPassColorAttachment.resolveTarget = nullptr;

    renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
    renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
    renderPassColorAttachment.clearValue = WGPUColor{ 0.5f, 0.8f, 0.0f, 1.0f }; //any value is fine

    renderPassColorAttachment.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;  //because we do not use the depth buffer

    renderPassDescriptor.colorAttachmentCount = 1;
    renderPassDescriptor.colorAttachments = &renderPassColorAttachment;

    WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDescriptor);
    wgpuRenderPassEncoderEnd(renderPass);
    wgpuRenderPassEncoderRelease(renderPass);

    //encode and submit the render pass commands
    WGPUCommandBufferDescriptor commandBufferDescriptor = {};
    commandBufferDescriptor.nextInChain = nullptr;
    commandBufferDescriptor.label = "Command Buffer";
    WGPUCommandBuffer commandBuffer = wgpuCommandEncoderFinish(encoder, &commandBufferDescriptor);

    cout<<"Submitting the command buffer"<<endl;
    wgpuQueueSubmit(this->queue, 1, &commandBuffer);
    wgpuCommandBufferRelease(commandBuffer);

    //release the texture view
    wgpuTextureViewRelease(targetView);

    //present the surface
    #ifndef __EMSCRIPTEN__
        wgpuSurfacePresent(this->surface);
    #endif // !__EMSCRIPTEN__

    #if defined(WEBGPU_BACKEND_DAWN)
            wgpuDeviceTick(device);
    #elif defined(WEBGPU_BACKEND_WGPU)
            wgpuDevicePoll(device, false, nullptr);
    #endif
}

bool Application::IsRunning()
{
    return !glfwWindowShouldClose(this->window);
}

WGPUTextureView Application::GetNextSurfaceTextureView()
{
    WGPUSurfaceTexture surfaceTexture;
    wgpuSurfaceGetCurrentTexture(this->surface, &surfaceTexture);

    if (surfaceTexture.status != WGPUSurfaceGetCurrentTextureStatus_Success) {
        return nullptr;
    }

    WGPUTextureViewDescriptor viewDescriptor;
    viewDescriptor.nextInChain = nullptr;
    viewDescriptor.label = "Surface texture view";
    viewDescriptor.format = wgpuTextureGetFormat(surfaceTexture.texture);
    viewDescriptor.dimension = WGPUTextureViewDimension_2D;
    viewDescriptor.baseMipLevel = 0;
    viewDescriptor.mipLevelCount = 1;
    viewDescriptor.baseArrayLayer = 0;
    viewDescriptor.arrayLayerCount = 1;
    viewDescriptor.aspect = WGPUTextureAspect_All;
    WGPUTextureView targetView = wgpuTextureCreateView(surfaceTexture.texture, &viewDescriptor);

    return targetView;
}
