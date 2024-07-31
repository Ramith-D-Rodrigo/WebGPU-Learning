#include "utils.h"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include "main.h"
#endif // __EMSCRIPTEN__

#include <vector>
#include <cassert>
#include <iostream>


using namespace std;

void inspectAdapter(WGPUAdapter adapter)
{
    #ifndef __EMSCRIPTEN__
    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

    #ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits) == WGPUStatus_Success;
    #else
    bool success = wgpuAdapterGetLimits(adapter, &supportedLimits);
    #endif

    if (success) {
        cout << "Adapter limits:" << endl;
        cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << endl;
        cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << endl;
        cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << endl;
        cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << endl;
    }
    #endif // NOT __EMSCRIPTEN__

    vector<WGPUFeatureName> features;

    size_t featuresCount = wgpuAdapterEnumerateFeatures(adapter, nullptr);  //first call to get the count

    //allocate memory for the features
    features.resize(featuresCount);

    wgpuAdapterEnumerateFeatures(adapter, features.data()); //second call to get the features

    //print the features
    cout << "Adapter features: " << endl;
    cout << hex; //set hex
    for (auto feature : features) {
        cout << " - 0x" << feature << endl;
    }

    cout << dec; //restore decimal

    WGPUAdapterProperties properties = {};
    properties.nextInChain = nullptr;
    wgpuAdapterGetProperties(adapter, &properties);
    cout << "Adapter properties:" << endl;
    cout << " - vendorID: " << properties.vendorID << endl;
    if (properties.vendorName) {
        cout << " - vendorName: " << properties.vendorName << endl;
    }
    if (properties.architecture) {
        cout << " - architecture: " << properties.architecture << endl;
    }
    cout << " - deviceID: " << properties.deviceID << endl;
    if (properties.name) {
        cout << " - name: " << properties.name << endl;
    }
    if (properties.driverDescription) {
        cout << " - driverDescription: " << properties.driverDescription << endl;
    }
    cout << hex;
    cout << " - adapterType: 0x" << properties.adapterType << endl;
    cout << " - backendType: 0x" << properties.backendType << endl;
    cout << dec; // Restore decimal numbers
}


WGPUAdapter requestAdapterSync(WGPUInstance instance, WGPURequestAdapterOptions const* options) {
    struct UserData {
        WGPUAdapter adapter = nullptr;
        bool requestEnded = false;
    };

    UserData userData;

    auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status, WGPUAdapter adapter, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestAdapterStatus_Success) {
            userData.adapter = adapter;
        }
        else {
            cout << "Could not get the WebGPU adapter! " << message << endl;
        }
        userData.requestEnded = true;
        };


    wgpuInstanceRequestAdapter(instance, options, onAdapterRequestEnded, (void*)&userData);

    #ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
    #endif // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.adapter;
}

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDeviceSync(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDeviceSync(WGPUAdapter adapter, WGPUDeviceDescriptor const* descriptor) {
    struct UserData {
        WGPUDevice device = nullptr;
        bool requestEnded = false;
    };
    UserData userData;

    auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status, WGPUDevice device, char const* message, void* pUserData) {
        UserData& userData = *reinterpret_cast<UserData*>(pUserData);
        if (status == WGPURequestDeviceStatus_Success) {
            userData.device = device;
        }
        else {
            cout << "Could not get WebGPU device: " << message << endl;
        }
        userData.requestEnded = true;
        };

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    #ifdef __EMSCRIPTEN__
    while (!userData.requestEnded) {
        emscripten_sleep(100);
    }
    #endif // __EMSCRIPTEN__

    assert(userData.requestEnded);

    return userData.device;
}

void inspectDevice(WGPUDevice device) {
    vector<WGPUFeatureName> features;

    size_t featuresCount = wgpuDeviceEnumerateFeatures(device, nullptr);  //first call to get the count

    //allocate memory for the features
    features.resize(featuresCount);
    wgpuDeviceEnumerateFeatures(device, features.data());

    cout << "Device features: " << endl;
    cout << hex; //set hex
    for (auto feature : features) {
        cout << " - 0x" << feature << endl;
    }
    cout << dec; //restore decimal

    WGPUSupportedLimits supportedLimits = {};
    supportedLimits.nextInChain = nullptr;

    #ifdef WEBGPU_BACKEND_DAWN
    bool success = wgpuDeviceGetLimits(device, &supportedLimits) == WGPUStatus_Success;
    #else
    bool success = wgpuDeviceGetLimits(device, &supportedLimits);
    #endif

    if (success) {
        cout << "Device limits:" << endl;
        cout << " - maxTextureDimension1D: " << supportedLimits.limits.maxTextureDimension1D << endl;
        cout << " - maxTextureDimension2D: " << supportedLimits.limits.maxTextureDimension2D << endl;
        cout << " - maxTextureDimension3D: " << supportedLimits.limits.maxTextureDimension3D << endl;
        cout << " - maxTextureArrayLayers: " << supportedLimits.limits.maxTextureArrayLayers << endl;
    }

}