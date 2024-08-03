//#include "utils.h"
//
//
//void inspectAdapter(Adapter adapter)
//{
//    #ifndef __EMSCRIPTEN__
//    SupportedLimits supportedLimits = {};
//    supportedLimits.nextInChain = nullptr;
//
//    #ifdef WEBGPU_BACKEND_DAWN
//    bool success = adapter.getLimits(&supportedLimits) == Status::Success;
//    #else
//    bool success = adapter.getLimits(&supportedLimits)
//    #endif
//
//    if (success) {
//        cout << "Adapter limits:" << endl;
//        cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << endl;
//        cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << endl;
//        cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << endl;
//        cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << endl;
//    }
//    #endif // NOT __EMSCRIPTEN__
//
//    vector<FeatureName*> features;
//
//    size_t featuresCount = adapter.enumerateFeatures(nullptr);  //first call to get the count
//
//    //allocate memory for the features
//    features.resize(featuresCount);
//
//    adapter.enumerateFeatures(*features.data()); //second call to get the features
//
//    //print the features
//    cout << "Adapter features: " << endl;
//    cout << hex; //set hex
//    for (auto feature : features) {
//        cout << " - 0x" << *feature << endl;
//    }
//
//    cout << dec; //restore decimal
//
//    AdapterProperties properties = Default;
//
//    adapter.getProperties(&properties);
//    cout << "Adapter properties:" << endl;
//    cout << " - vendorID: " << properties.vendorID << endl;
//    if (properties.vendorName) {
//        cout << " - vendorName: " << properties.vendorName << endl;
//    }
//    if (properties.architecture) {
//        cout << " - architecture: " << properties.architecture << endl;
//    }
//    cout << " - deviceID: " << properties.deviceID << endl;
//    if (properties.name) {
//        cout << " - name: " << properties.name << endl;
//    }
//    if (properties.driverDescription) {
//        cout << " - driverDescription: " << properties.driverDescription << endl;
//    }
//    cout << hex;
//    cout << " - adapterType: 0x" << properties.adapterType << endl;
//    cout << " - backendType: 0x" << properties.backendType << endl;
//    cout << dec; // Restore decimal numbers
//}
//
//
//Adapter requestAdapterSync(Instance instance, RequestAdapterOptions const* options) {
//    struct UserData {
//        Adapter adapter = nullptr;
//        bool requestEnded = false;
//    };
//
//    UserData userData;
//
//    auto onAdapterRequestEnded = [&userData](RequestAdapterStatus status, Adapter adapter, char const* message) {
//        if (status == RequestAdapterStatus::Success) {
//            userData.adapter = adapter;
//        }
//        else {
//            cout << "Could not get the WebGPU adapter! " << message << endl;
//        }
//        userData.requestEnded = true;
//    };
//
//    instance.requestAdapter(*options);
//
//    #ifdef __EMSCRIPTEN__
//    while (!userData.requestEnded) {
//        emscripten_sleep(100);
//    }
//    #endif // __EMSCRIPTEN__
//
//    assert(userData.requestEnded);
//
//    return userData.adapter;
//}
//
///**
// * Utility function to get a WebGPU device, so that
// *     WGPUAdapter device = requestDeviceSync(adapter, options);
// * is roughly equivalent to
// *     const device = await adapter.requestDevice(descriptor);
// * It is very similar to requestAdapter
// */
//Device requestDeviceSync(Adapter adapter, DeviceDescriptor const* descriptor) {
//    struct UserData {
//        Device device = nullptr;
//        bool requestEnded = false;
//    };
//    UserData userData;
//
//    auto onDeviceRequestEnded = [&userData](RequestDeviceStatus status, Device device, char const* message) {
//        if (status == WGPURequestDeviceStatus_Success) {
//            userData.device = device;
//        }
//        else {
//            cout << "Could not get WebGPU device: " << message << endl;
//        }
//        userData.requestEnded = true;
//        };
//
//    adapter.requestDevice(*descriptor, onDeviceRequestEnded);
//
//    #ifdef __EMSCRIPTEN__
//    while (!userData.requestEnded) {
//        emscripten_sleep(100);
//    }
//    #endif // __EMSCRIPTEN__
//
//    assert(userData.requestEnded);
//
//    return userData.device;
//}
//
//void inspectDevice(Device device) {
//    vector<FeatureName*> features;
//
//    size_t featuresCount = device.enumerateFeatures(nullptr);  //first call to get the count
//        
//    //allocate memory for the features
//    features.resize(featuresCount);
//    device.enumerateFeatures(*features.data()); //second call to get the features
//
//    cout << "Device features: " << endl;
//    cout << hex; //set hex
//    for (auto feature : features) {
//        cout << " - 0x" << *feature << endl;
//    }
//    cout << dec; //restore decimal
//
//    SupportedLimits supportedLimits = Default;
//
//    #ifdef WEBGPU_BACKEND_DAWN
//    bool success = device.getLimits(&supportedLimits) == Status::Success;
//    #else
//    bool success = device.getLimits(&supportedLimits);
//    #endif
//
//    if (success) {
//        cout << "Device limits:" << endl;
//        cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << endl;
//        cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << endl;
//        cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << endl;
//        cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << endl;
//    }
//
//}