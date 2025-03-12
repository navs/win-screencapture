#ifndef __SCREENCAPTURE_H__
#define __SCREENCAPTURE_H__

#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <wincodec.h>
#include <fstream>
#include <iomanip>
#include <string>
#include <functional>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "windowscodecs.lib")

class ScreenCapture {
public:
    using ImageBuffer = std::vector<unsigned char>;
    using CaptureCallback = std::function<void(const ImageBuffer&, int, int, const std::wstring&)>;

    ScreenCapture() : device(nullptr), context(nullptr), output(nullptr), output1(nullptr), deskDupl(nullptr), deskDuplAcquired(false), width(0), height(0), format(DXGI_FORMAT_B8G8R8A8_UNORM) {
        if (!Initialize()) {
            std::cerr << "ScreenCapture initialization failed." << std::endl;
        }
    }

    ~ScreenCapture() {
        Cleanup();
    }

    bool CaptureScreenRegion(int x, int y, int w, int h, const std::wstring& filename, const CaptureCallback& callback) {
        if (!deskDupl) {
            std::cerr << "Desktop Duplication not initialized." << std::endl;
            return false;
        }

        DXGI_OUTDUPL_FRAME_INFO frameInfo;
        IDXGIResource* desktopResource = nullptr;
        HRESULT hr = deskDupl->AcquireNextFrame(0, &frameInfo, &desktopResource);

        if (FAILED(hr)) {
            // Handle errors such as DXGI_ERROR_WAIT_TIMEOUT (no new frame)
            std::cerr << "Failed to acquire next frame." << std::endl;
            return false;
        }

        ID3D11Texture2D* acquiredTexture = nullptr;
        hr = desktopResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&acquiredTexture);
        desktopResource->Release();

        if (FAILED(hr)) {
            std::cerr << "Failed to get texture from acquired resource." << std::endl;
            deskDupl->ReleaseFrame();
            return false;
        }

        D3D11_TEXTURE2D_DESC textureDesc;
        acquiredTexture->GetDesc(&textureDesc);

        ID3D11Texture2D* subResourceTexture = nullptr;
        D3D11_TEXTURE2D_DESC subResourceTextureDesc = textureDesc;
        subResourceTextureDesc.Width = w;
        subResourceTextureDesc.Height = h;
        subResourceTextureDesc.Usage = D3D11_USAGE_STAGING;
        subResourceTextureDesc.BindFlags = 0;
        subResourceTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        subResourceTextureDesc.MiscFlags = 0;

        hr = device->CreateTexture2D(&subResourceTextureDesc, nullptr, &subResourceTexture);
        if (FAILED(hr)) {
            std::cerr << "Failed to create subresource texture." << std::endl;
            acquiredTexture->Release();
            deskDupl->ReleaseFrame();
            return false;
        }

        D3D11_BOX region;
        region.left = x;
        region.top = y;
        region.right = x + w;
        region.bottom = y + h;
        region.front = 0;
        region.back = 1;

        context->CopySubresourceRegion(subResourceTexture, 0, 0, 0, 0, acquiredTexture, 0, &region);


        D3D11_MAPPED_SUBRESOURCE mappedResource;
        hr = context->Map(subResourceTexture, 0, D3D11_MAP_READ, 0, &mappedResource);
        if (FAILED(hr)) {
            std::cerr << "Failed to map subresource texture." << std::endl;
            acquiredTexture->Release();
            subResourceTexture->Release();
            deskDupl->ReleaseFrame();
            return false;
        }

        ImageBuffer buffer(w * h * 4);
        memcpy(buffer.data(), mappedResource.pData, buffer.size());
        context->Unmap(subResourceTexture, 0);

        acquiredTexture->Release();
        subResourceTexture->Release();
        deskDupl->ReleaseFrame();

        if (IsFrameEmpty(buffer, w, h)) {
            std::cerr << "Empty frame captured." << std::endl;
            return false;
        }

        callback(buffer, w, h, filename);
        return true;
    }

private:
    bool Initialize() {
        D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
        HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &featureLevel, 1, D3D11_SDK_VERSION, &device, nullptr, &context);
        if (FAILED(hr)) {
            std::cerr << "D3D11CreateDevice failed." << std::endl;
            return false;
        }

        IDXGIDevice* dxgiDevice = nullptr;
        hr = device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if (FAILED(hr)) {
            std::cerr << "Failed to get DXGI device." << std::endl;
            return false;
        }

        IDXGIAdapter* dxgiAdapter = nullptr;
        hr = dxgiDevice->GetParent(__uuidof(IDXGIAdapter), (void**)&dxgiAdapter);
        dxgiDevice->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get DXGI adapter." << std::endl;
            return false;
        }

        IDXGIOutput* dxgiOutput = nullptr;
        hr = dxgiAdapter->EnumOutputs(0, &dxgiOutput);
        dxgiAdapter->Release();
        if (FAILED(hr)) {
            std::cerr << "Failed to get DXGI output." << std::endl;
            return false;
        }

        hr = dxgiOutput->QueryInterface(__uuidof(IDXGIOutput1), (void**)&output1);
        if (FAILED(hr))
        {
            dxgiOutput->Release();
            std::cerr << "IDXGIOutput1 is required for screen capture" << std::endl;
            return false;
        }
        output = dxgiOutput;

        hr = output1->DuplicateOutput(device, &deskDupl);
        if (FAILED(hr)) {
            output->Release();
            std::cerr << "Failed to get duplicate output." << std::endl;
            return false;
        }

        deskDuplAcquired = true;
        return true;
    }

    void Cleanup() {
        if (deskDupl) {
            deskDupl->Release();
            deskDupl = nullptr;
        }
        if (output1) {
            output1->Release();
            output1 = nullptr;
        }
        if (output) {
            output->Release();
            output = nullptr;
        }
        if (context) {
            context->Release();
            context = nullptr;
        }
        if (device) {
            device->Release();
            device = nullptr;
        }
    }

    bool IsFrameEmpty(const ScreenCapture::ImageBuffer& buffer, int width, int height) {
        // Check if the buffer is empty or has zero dimensions
        if (buffer.empty() || width <= 0 || height <= 0) {
            return true;
        }
    
        // Assuming the buffer is BGRA32 format (4 bytes per pixel)
        size_t pixelCount = width * height;
        const unsigned char* pixelData = buffer.data();
    
        // Iterate through all pixels and check if they are all black (0, 0, 0, 0)
        for (size_t i = 0; i < pixelCount; ++i) {
            size_t pixelIndex = i * 4;
            unsigned char blue = pixelData[pixelIndex];
            unsigned char green = pixelData[pixelIndex + 1];
            unsigned char red = pixelData[pixelIndex + 2];
            unsigned char alpha = pixelData[pixelIndex + 3];
    
            // If any pixel is not fully transparent black, the frame is not empty
            if (blue != 0 || green != 0 || red != 0 || alpha != 0) {
                return false;
            }
        }
    
        // All pixels are black, so the frame is considered empty
        return true;
    }

private:
    ID3D11Device* device;
    ID3D11DeviceContext* context;
    IDXGIOutput* output;
    IDXGIOutput1* output1;
    IDXGIOutputDuplication* deskDupl;
    bool deskDuplAcquired;
    int width;
    int height;
    DXGI_FORMAT format;
};

#endif // __SCREENCAPTURE_H__