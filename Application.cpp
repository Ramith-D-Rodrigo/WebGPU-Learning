#include "Application.h"

bool Application::Initialize(uint16 width, uint16 height)
{
    if(!initWindowAndDevice(width, height)) return false;
    if(!initDepthBuffer()) return false;
    if(!initRenderPipeline()) return false;
    if(!initTextureSampler()) return false;
    if(!initTexture()) return false;
    if(!initUniforms()) return false;
    if(!initScene()) return false;

    return true;
}

void Application::Terminate()
{
    terminateScene();
    terminateUniforms();
    terminateTexture();
    terminateTextureSampler();
    terminateRenderPipeline();
    terminateDepthBuffer();
    terminateWindowAndDevice();
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

    mat4 rootModelMatrix = mat4(1.0f);
    this->renderScene(renderPass, &rootModelMatrix, this->scene);

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

bool Application::initWindowAndDevice(uint16 width, uint16 height) {
    if (!glfwInit()) {
        cout << "Failed to initialize GLFW" << endl;
        return false;
    }

    //create a window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    this->window = glfwCreateWindow(width, height, "WebGPU", nullptr, nullptr);

    this->windowWidth = width;
    this->windowHeight = height;

    if (!this->window) {
        cout << "Failed to create window" << endl;
        glfwTerminate();
        return false;
    }

    //get the surface
    this->surface = glfwGetWGPUSurface(this->instance, this->window);

    //instance descriptor
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
    this->instance = createInstance(descriptor);
#endif

    if (!this->instance) {
        cout << "Failed to create WebGPU instance" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }
    cout << "WebGPU instance created successfully and instance is " << this->instance << endl;

    cout << "Requesting adapter synchronously" << endl;

    RequestAdapterOptions options = {};
    options.nextInChain = nullptr;
    options.powerPreference = PowerPreference::HighPerformance;
    options.compatibleSurface = this->surface;

    //Adapter adapter = requestAdapterSync(instance, &options); we do not need to use this function as we can use the following function
    Adapter adapter = this->instance.requestAdapter(options);

    if (!adapter) {
        cout << "Failed to get the adapter" << endl;
        glfwDestroyWindow(this->window);
        return false;
    }
    cout << "Adapter obtained successfully and adapter is " << adapter << endl;

    //get the surface format
    this->surfaceFormat = this->surface.getPreferredFormat(adapter);

    cout << "Requesting device synchronously" << endl;

    DeviceDescriptor deviceDescriptor = {};
    deviceDescriptor.nextInChain = nullptr;
    deviceDescriptor.label = "My Device"; // anything works here, that's your call
    deviceDescriptor.requiredFeatureCount = 0; // we do not require any specific feature
    deviceDescriptor.defaultQueue.nextInChain = nullptr;
    deviceDescriptor.defaultQueue.label = "The default queue";

    auto onDeviceLost = [](WGPUDevice const*/*device*/, WGPUDeviceLostReason reason, char const* message, void* /* pUserData */) {
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
    auto onDeviceErrorFunc = [](ErrorType type, char const* message) {
        cout << "Uncaptured device error: type " << type;
        if (message) cout << " (" << message << ")";
        cout << endl;
    };

    this->onDeviceError = this->device.setUncapturedErrorCallback(onDeviceErrorFunc);
    cout << "Device obtained successfully and device is " << this->device << endl;
    
    //inspectDevice(this->device);

    //create queue to pass commands to the device
    this->queue = this->device.getQueue();

    //configure the surface
    SurfaceConfiguration config = {};
    config.nextInChain = nullptr;
    config.width = this->windowWidth;
    config.height = this->windowHeight;
    config.format = this->surfaceFormat;

    config.viewFormatCount = 0;
    config.viewFormats = nullptr;

    config.usage = TextureUsage::RenderAttachment;
    config.device = this->device;

    config.presentMode = PresentMode::Fifo;
    config.alphaMode = CompositeAlphaMode::Auto;

    this->surface.configure(config);

    adapter.release();

    return this->device && this->queue;
}

void Application::terminateWindowAndDevice()
{
    this->onDeviceError.release();
	this->queue.release();
	this->device.release();
    this->surface.unconfigure();
	this->surface.release();
    this->instance.release();
    //destroy the window
    glfwDestroyWindow(this->window);

    glfwTerminate();
}

bool Application::initDepthBuffer()
{
    //Create depth texture
    TextureDescriptor depthTextureDescriptor = {};
    depthTextureDescriptor.dimension = TextureDimension::_2D;
    depthTextureDescriptor.format = this->depthTextureFormat;
    depthTextureDescriptor.mipLevelCount = 1;
    depthTextureDescriptor.sampleCount = 1;
    depthTextureDescriptor.size = { this->windowWidth, this->windowHeight, 1 };
    depthTextureDescriptor.usage = TextureUsage::RenderAttachment;
    depthTextureDescriptor.viewFormatCount = 1;
    depthTextureDescriptor.viewFormats = (WGPUTextureFormat*)&this->depthTextureFormat;

    this->depthTexture = this->device.createTexture(depthTextureDescriptor);

    //Create depth texture view
    TextureViewDescriptor depthTextureViewDescriptor = {};
    depthTextureViewDescriptor.aspect = TextureAspect::DepthOnly;
    depthTextureViewDescriptor.baseArrayLayer = 0;
    depthTextureViewDescriptor.arrayLayerCount = 1;
    depthTextureViewDescriptor.baseMipLevel = 0;
    depthTextureViewDescriptor.mipLevelCount = 1;
    depthTextureViewDescriptor.dimension = TextureViewDimension::_2D;
    depthTextureViewDescriptor.format = this->depthTextureFormat;

    this->depthTextureView = this->depthTexture.createView(depthTextureViewDescriptor);

    return this->depthTexture && this->depthTextureView;
}

void Application::terminateDepthBuffer()
{
    this->depthTextureView.release();
	this->depthTexture.destroy();
	this->depthTexture.release();
}

bool Application::initRenderPipeline()
{
    this->shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wgsl", this->device);

    //now create the pipeline
    RenderPipelineDescriptor renderPipelineDescriptor = Default;

    //vertex pipeline state

    //instead of one vertex buffer, we can have multiple vertex buffers (e.g. position, normal, color, etc.)
    vector<VertexBufferLayout> vertexBufferLayouts(3);

    //position attribute
    VertexAttribute positionAttribute;
    positionAttribute.shaderLocation = 0; //location in the shader (@location(0))
    positionAttribute.format = VertexFormat::Float32x3; //XYZ position (3D)
    positionAttribute.offset = 0;

    //normal attribute
    VertexAttribute normalAttribute;
    normalAttribute.shaderLocation = 1;
    normalAttribute.format = VertexFormat::Float32x3;
    normalAttribute.offset = 0;

    // UV attribute
    VertexAttribute uvAttribute;
    uvAttribute.shaderLocation = 2;
    uvAttribute.format = VertexFormat::Float32x2;
    uvAttribute.offset = 0;

    //create the vertex buffer layout

    //position buffer layout
    VertexBufferLayout positionBufferLayout;
    positionBufferLayout.attributeCount = 1;
    positionBufferLayout.attributes = &positionAttribute;
    positionBufferLayout.stepMode = VertexStepMode::Vertex; //each vertex is a separate entity (not instanced)
    positionBufferLayout.arrayStride = sizeof(vec3);

    //normal buffer layout
    VertexBufferLayout normalBufferLayout;
    normalBufferLayout.attributeCount = 1;
    normalBufferLayout.attributes = &normalAttribute;
    normalBufferLayout.stepMode = VertexStepMode::Vertex;
    normalBufferLayout.arrayStride = sizeof(vec3);

    // UV buffer layout
    VertexBufferLayout uvBufferLayout;
    uvBufferLayout.attributeCount = 1;
    uvBufferLayout.attributes = &uvAttribute;
    uvBufferLayout.stepMode = VertexStepMode::Vertex;
    uvBufferLayout.arrayStride = sizeof(vec2);

    vertexBufferLayouts[0] = positionBufferLayout;
    vertexBufferLayouts[1] = normalBufferLayout;
    vertexBufferLayouts[2] = uvBufferLayout;

    renderPipelineDescriptor.vertex.bufferCount = vertexBufferLayouts.size();
    renderPipelineDescriptor.vertex.buffers = vertexBufferLayouts.data();
    renderPipelineDescriptor.vertex.module = this->shaderModule;
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
    fragmentState.module = this->shaderModule;
    fragmentState.entryPoint = "fs_main"; //fragment shader main
    fragmentState.constantCount = 0;
    fragmentState.constants = nullptr;

    //stencil/depth pipeline state
    DepthStencilState depthStencilState = Default;
    depthStencilState.depthCompare = CompareFunction::Less; //only blend the pixel if the depth is less than the existing depth
    depthStencilState.depthWriteEnabled = true; //write the depth value to the depth buffer every time a pixel is drawn
    depthStencilState.format = this->depthTextureFormat;

    //disable stencil
    depthStencilState.stencilReadMask = 0;
    depthStencilState.stencilWriteMask = 0;

    renderPipelineDescriptor.depthStencil = &depthStencilState;

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
    //First group is camera
    BindGroupLayoutEntry cameraBindGroupLayoutEntry = {};
    cameraBindGroupLayoutEntry.binding = 0;
    cameraBindGroupLayoutEntry.visibility = ShaderStage::Vertex;
	cameraBindGroupLayoutEntry.buffer.type = BufferBindingType::Uniform;
    cameraBindGroupLayoutEntry.buffer.minBindingSize = sizeof(CameraUniform);

    BindGroupLayoutDescriptor cameraBindGroupLayoutDescriptor = {};
    cameraBindGroupLayoutDescriptor.label = "Camera Bind Group Layout";
    cameraBindGroupLayoutDescriptor.entryCount = 1;
    cameraBindGroupLayoutDescriptor.entries = &cameraBindGroupLayoutEntry;

    this->cameraBindGroupLayout = this->device.createBindGroupLayout(cameraBindGroupLayoutDescriptor);

    //Second group is model matrix
    BindGroupLayoutEntry modelMatrixBindGroupLayoutEntry = {};
    modelMatrixBindGroupLayoutEntry.binding = 0;
    modelMatrixBindGroupLayoutEntry.visibility = ShaderStage::Vertex;
    modelMatrixBindGroupLayoutEntry.buffer.type = BufferBindingType::Uniform;
    modelMatrixBindGroupLayoutEntry.buffer.minBindingSize = sizeof(glm::mat4);

    BindGroupLayoutDescriptor modelMatrixBindGroupLayoutDescriptor = {};
    modelMatrixBindGroupLayoutDescriptor.label = "Model Matrix Bind Group Layout";
    modelMatrixBindGroupLayoutDescriptor.entryCount = 1;
    modelMatrixBindGroupLayoutDescriptor.entries = &modelMatrixBindGroupLayoutEntry;

    this->modelMatrixBindGroupLayout = this->device.createBindGroupLayout(modelMatrixBindGroupLayoutDescriptor);

    //Third group is the texture
    vector<BindGroupLayoutEntry> bindGroupLayoutEntries(2);

    //texture
    BindGroupLayoutEntry textureBindingLayout;
    textureBindingLayout.binding = 0;
    textureBindingLayout.visibility = ShaderStage::Fragment;
    textureBindingLayout.texture.sampleType = TextureSampleType::Float;
    textureBindingLayout.texture.viewDimension = TextureViewDimension::_2D;

    //sampler
    BindGroupLayoutEntry samplerBindingLayout;
    samplerBindingLayout.binding = 1;
    samplerBindingLayout.visibility = ShaderStage::Fragment;
    samplerBindingLayout.sampler.type = SamplerBindingType::Filtering;

    bindGroupLayoutEntries[0] = textureBindingLayout;
    bindGroupLayoutEntries[1] = samplerBindingLayout;

    BindGroupLayoutDescriptor bindGroupLayoutDescriptor = {};
    bindGroupLayoutDescriptor.label = "Bind Group Layout";
    bindGroupLayoutDescriptor.entryCount = static_cast<uint32_t>(bindGroupLayoutEntries.size());
    bindGroupLayoutDescriptor.entries = bindGroupLayoutEntries.data();

    this->textureBindGroupLayout = this->device.createBindGroupLayout(bindGroupLayoutDescriptor);
    vector<BindGroupLayout> bindGroupLayouts = { cameraBindGroupLayout, modelMatrixBindGroupLayout, textureBindGroupLayout };

    //pipeline layout
    PipelineLayoutDescriptor pipelineLayoutDescriptor = {};
    pipelineLayoutDescriptor.label = "Pipeline Layout";
    pipelineLayoutDescriptor.bindGroupLayoutCount = static_cast<uint32_t>(bindGroupLayouts.size());
    pipelineLayoutDescriptor.bindGroupLayouts = (WGPUBindGroupLayout*)bindGroupLayouts.data();

    renderPipelineDescriptor.layout = this->device.createPipelineLayout(pipelineLayoutDescriptor);
    this->renderPipeline = this->device.createRenderPipeline(renderPipelineDescriptor);

    return this->renderPipeline != nullptr;
}

void Application::terminateRenderPipeline()
{
    this->renderPipeline.release();
	this->shaderModule.release();
	this->cameraBindGroupLayout.release();
    this->modelMatrixBindGroupLayout.release();
    this->textureBindGroupLayout.release();
}

bool Application::initTextureSampler()
{
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
    this->sampler = device.createSampler(samplerDesc);

    return this->sampler != nullptr;
}

void Application::terminateTextureSampler()
{
    this->sampler.release();
}

bool Application::initTexture()
{
    this->imageTexture = loadTexture(RESOURCE_DIR "/image.png", this->device, &this->imageTextureView);
    if (!this->imageTexture) {
        cout << "Failed to load the texture" << endl;
        return false;
    }

    return this->imageTexture && this->imageTextureView;
}

void Application::terminateTexture()
{
	this->imageTextureView.release();
    this->imageTexture.destroy();
	this->imageTexture.release();
}

bool Application::initScene()
{
    this->scene = new SceneObject(&device, &modelMatrixBindGroupLayout);

    cout << "Loading the model" << endl;

     SceneObject* object = Model::LoadModel("D:\\Uni\\3D Models\\models\\base_sponza\\NewSponza_Main_glTF_003.gltf",
         device, textureBindGroupLayout, modelMatrixBindGroupLayout, imageTextureView, sampler);
    if (!object) {
        cout<<"Failed to load the model"<<endl;
        delete this->scene;
        return false;
    }
    this->scene->addChild(object);

    cout << "Model loaded successfully" << endl;

    return this->scene != nullptr;
}

void Application::terminateScene()
{
    delete this->scene;
    this->scene = nullptr;
}

bool Application::initUniforms()
{
    //create the uniform buffers

    SupportedLimits limits;
    this->device.getLimits(&limits);

    //camera uniform buffer
    this->cameraUniformStride = ceilToNextMultiple((uint32_t)sizeof(CameraUniform), (uint32_t)limits.limits.minUniformBufferOffsetAlignment);

    BufferDescriptor bufferDescriptor = Default;
    bufferDescriptor.label = "Uniform Buffer";
    bufferDescriptor.size = sizeof(CameraUniform);
    bufferDescriptor.usage = BufferUsage::Uniform | BufferUsage::CopyDst;
    bufferDescriptor.mappedAtCreation = false;

    this->cameraUniformBuffer = this->device.createBuffer(bufferDescriptor);
    //// Set the projection matrix
    float ratio = this->windowWidth / (float)this->windowHeight;
    float focalLength = 2.0;
    float nearView = 0.01f;
    float farView = 100.0f;
    float fov = 2 * glm::atan(1 / focalLength);
    cameraUniform.projectionMatrix = glm::perspectiveZO(fov, ratio, nearView, farView); //REMEMBER THIS!!!
    cameraUniform.viewMatrix = glm::lookAt(vec3(0.0, 0.0, -50.0), vec3(0.0, 0.0, 0.0), vec3(0.0, 1.0, 0.0));

    this->queue.writeBuffer(this->cameraUniformBuffer, 0, &cameraUniform, sizeof(CameraUniform));

    //create the camera bind group
    BindGroupEntry cameraBindGroupEntry = {};
    cameraBindGroupEntry.binding = 0;
    cameraBindGroupEntry.buffer = this->cameraUniformBuffer;
    cameraBindGroupEntry.offset = 0;
    cameraBindGroupEntry.size = sizeof(CameraUniform);

    BindGroupDescriptor cameraBindGroupDescriptor = {};
    cameraBindGroupDescriptor.label = "Camera Bind Group";
    cameraBindGroupDescriptor.layout = this->cameraBindGroupLayout;
    cameraBindGroupDescriptor.entryCount = 1;
    cameraBindGroupDescriptor.entries = &cameraBindGroupEntry;

    this->cameraBindGroup = this->device.createBindGroup(cameraBindGroupDescriptor);

    return this->cameraUniformBuffer;
}

void Application::terminateUniforms()
{
    this->cameraUniformBuffer.destroy();
    this->cameraUniformBuffer.release();

    this->cameraUniformStride = 0;
}

void Application::renderScene(RenderPassEncoder renderPass, mat4* parentModelMatrix, SceneObject* renderingObject)
{
    mat4 worldModelMatrix = *parentModelMatrix * renderingObject->calculateModelMatrix();
    renderingObject->writeModelUniformBuffer(this->queue, &worldModelMatrix);
   
    vector<Mesh*> meshes = renderingObject->getVisualObjects();

    for (int i = 0; i < meshes.size(); i++) {
		Buffer vertexBuffer = meshes[i]->getVertexBuffer();
        Buffer indexBuffer = meshes[i]->getIndexBuffer();
        Buffer normalBuffer = meshes[i]->getNormalBuffer();
        Buffer uvBuffer = meshes[i]->getUVBuffer();

        // Set the vertex buffer
        renderPass.setVertexBuffer(0, vertexBuffer, 0, vertexBuffer.getSize());
        renderPass.setVertexBuffer(1, normalBuffer, 0, normalBuffer.getSize());
        renderPass.setVertexBuffer(2, uvBuffer, 0, uvBuffer.getSize());

        //uint must correspond to the index buffer data type
        renderPass.setIndexBuffer(indexBuffer, meshes[i]->getIndexFormat(), 0, indexBuffer.getSize());

        renderPass.setBindGroup(0, this->cameraBindGroup, 0, nullptr);
        renderPass.setBindGroup(1, renderingObject->getModelBindGroup(), 0, nullptr);
        renderPass.setBindGroup(2, meshes[i]->getTextureBindGroup(), 0, nullptr);

        renderPass.drawIndexed((uint32_t)meshes[i]->getNumIndices(), 1, 0, 0, 0);
	}

    //now render children
    vector<SceneObject*> children = renderingObject->getChildren();
    for (int i = 0; i < children.size(); i++) {
		renderScene(renderPass, &worldModelMatrix, children[i]);
	}
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

RequiredLimits Application::GetRequiredLimits(Adapter adapter)
{
    //first get the adapter supported limits
    SupportedLimits supportedLimits;
    adapter.getLimits(&supportedLimits);

    RequiredLimits requiredLimits = Default;

    requiredLimits.limits = supportedLimits.limits;
    return requiredLimits;
}

// We define a function that hides implementation-specific variants of device polling:
void Application::wgpuPollEvents(bool yieldToWebBrowser) {
#if defined(WEBGPU_BACKEND_DAWN)
    this->device.tick();
#elif defined(WEBGPU_BACKEND_WGPU)
    device.poll(false);
#elif defined(WEBGPU_BACKEND_EMSCRIPTEN)
    if (yieldToWebBrowser) {
        emscripten_sleep(100);
    }
#endif

    //to avoid the warning
    if(yieldToWebBrowser) {
	}
}