#include <iostream>
#include <vector>

#include <webgpu/webgpu.hpp>

#include <GLFW/glfw3.h>
#include <glfw3webgpu.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "utils.h"

#ifdef __EMSCRIPTEN__
#  include <emscripten.h>
#endif // __EMSCRIPTEN__

using namespace std;
using namespace wgpu;
using namespace glm;

struct MyUniforms {
    mat4x4 projectionMatrix;
    mat4x4 viewMatrix;
    mat4x4 modelMatrix;
    vec4 color;
    float time;
    float padding[3]; // padding to make the size of the struct a multiple of 16 bytes
};

static_assert(sizeof(MyUniforms) % 16 == 0, "MyUniforms size must be a multiple of 16 bytes");

class Application {
public:
    bool Initialize(uint16 windowWidth, uint16 windowHeight);	// Initialize the application and return true if successful
    void Terminate();	// Terminate the application
    void MainLoop();	// Run the main loop
    bool IsRunning();	// Return true if the application is running

private:
    void wgpuPollEvents(bool yieldToWebBrowser);	// Poll events
    TextureView GetNextSurfaceTextureView();	// Get the next surface texture view
    void PlayWithBuffers();	// Play with buffers
    RequiredLimits GetRequiredLimits(Adapter adapter);	// Get the required limits

    bool initWindowAndDevice(uint16 windowWidth, uint16 windowHeight);
    void terminateWindowAndDevice();

    bool initDepthBuffer();
    void terminateDepthBuffer();

    bool initRenderPipeline();
    void terminateRenderPipeline();

    bool initTexture();
    void terminateTexture();

    bool initGeometry();
    void terminateGeometry();

    bool initUniforms();
    void terminateUniforms();

    bool initBindingGroup();
    void terminateBindingGroup();

private:
    std::unique_ptr<wgpu::ErrorCallback> onDeviceError = nullptr;

    //window and device variables
    uint16 windowWidth = 0;
    uint16 windowHeight = 0;
    GLFWwindow* window = nullptr;
    Instance instance = nullptr;
    Device device = nullptr;
    Queue queue = nullptr;
    Surface surface = nullptr;
    TextureFormat surfaceFormat = TextureFormat::Undefined;

    //depth buffer variables
    TextureFormat depthTextureFormat = TextureFormat::Depth24Plus;
    Texture depthTexture = nullptr;
    TextureView depthTextureView = nullptr;

    //render pipeline variables
    BindGroupLayout bindGroupLayout = nullptr;
    RenderPipeline renderPipeline = nullptr;
    ShaderModule shaderModule = nullptr;

    //texture variables
    Sampler sampler = nullptr;
    Texture imageTexture = nullptr;
    TextureView imageTextureView = nullptr;

    //geometry variables
    Buffer pointBuffer = nullptr;
    Buffer indexBuffer = nullptr;
    uint32_t indexCount = 0;

    //uniforms variables
    MyUniforms uniforms;
    Buffer uniformBuffer = nullptr;
    uint32_t uniformStride = 0;

    //binding group variables
    BindGroup bindGroup = nullptr;

    //main loop variables
    mat4x4 S = glm::scale(mat4x4(1.0), vec3(2.0f));
    mat4x4 T1 = glm::translate(mat4x4(1.0), vec3(0.5, 0.0, 0.0));
    mat4x4 T2 = glm::translate(mat4x4(1.0), vec3(0.0, 0.0, -2.0));
    mat4x4 R1 = glm::rotate(mat4x4(1.0), 0.0f, vec3(0.0, 0.0, 1.0));
    mat4x4 R2 = glm::rotate(mat4x4(1.0), 0.0f, vec3(1.0, 0.0, 0.0));
};



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
