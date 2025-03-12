#ifndef __SAVE_IMAGE_THREAD_H__
#define __SAVE_IMAGE_THREAD_H__

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
#include <queue>
#include <sstream>
#include <string>
#include <locale>
#include <codecvt>
#include "ScreenCapture.h""

class SaveImageThread {
    public:
        SaveImageThread() : running(false), image_count(0) {}
    
        void Start() {
            running = true;
            worker_thread = std::thread(&SaveImageThread::Run, this);
        }
    
        void Stop() {
            running = false;
            condition.notify_one();
            if (worker_thread.joinable()) {
                worker_thread.join();
            }
        }
    
        void AddImage(const ScreenCapture::ImageBuffer& buffer, int width, int height, const std::wstring& filename) {
            std::unique_lock<std::mutex> lock(mutex);
            image_queue.push({ buffer, width, height, filename });
            condition.notify_one();
        }
    
    private:
        struct ImageInfo {
            ScreenCapture::ImageBuffer buffer;
            int width;
            int height;
            std::wstring filename;
        };
    
        void Run() {
            while (running) {
                ImageInfo image_info;
                {
                    std::unique_lock<std::mutex> lock(mutex);
                    condition.wait(lock, [this] { return !image_queue.empty() || !running; });
                    if (!running && image_queue.empty()) break;
                    image_info = image_queue.front();
                    image_queue.pop();
                }
    
                SaveImageToFile(image_info.buffer, image_info.width, image_info.height, image_info.filename);
            }
        }
    
        void SaveImageToFile(const ScreenCapture::ImageBuffer& buffer, int width, int height, const std::wstring& filename) {
            GUID CLSID_PngEncoder;
            CLSIDFromString(L"{1B72AAE1-E766-4A07-8327-0E86B800BCF7}", &CLSID_PngEncoder);
    
            IWICImagingFactory* pFactory = nullptr;
            IWICBitmapEncoder* pEncoder = nullptr;
            IWICBitmapFrameEncode* pFrameEncode = nullptr;
            IWICStream* pStream = nullptr;
            IWICBitmap* pBitmap = nullptr;
    
            CoInitialize(nullptr);
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pFactory);
    
            if (SUCCEEDED(hr)) {
                hr = pFactory->CreateStream(&pStream);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pStream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pFactory->CreateEncoder(CLSID_PngEncoder, nullptr, &pEncoder);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pFrameEncode->Initialize(nullptr);
            }
    
            if (SUCCEEDED(hr)) {
                hr = pFrameEncode->SetSize(width, height);
            }
    
            WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
            if (SUCCEEDED(hr)) {
                hr = pFrameEncode->SetPixelFormat(&format);
            }
    
    
            if (SUCCEEDED(hr))
            {
                //hr = pFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, static_cast<UINT>(buffer.size()), reinterpret_cast<BYTE*>(buffer.data()), &pBitmap);
                hr = pFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, static_cast<UINT>(buffer.size()), (BYTE*)(buffer.data()), &pBitmap);
            }
    
            if (SUCCEEDED(hr))
            {
                hr = pFrameEncode->WriteSource(pBitmap, nullptr);
            }
    
    
            if (SUCCEEDED(hr)) {
                hr = pFrameEncode->Commit();
            }
    
            if (SUCCEEDED(hr)) {
                hr = pEncoder->Commit();
            }
    
            if (pBitmap) pBitmap->Release();
            if (pStream) pStream->Release();
            if (pFrameEncode) pFrameEncode->Release();
            if (pEncoder) pEncoder->Release();
            if (pFactory) pFactory->Release();
            CoUninitialize();
    
            if (FAILED(hr)) {
                std::cerr << "Failed to save image: " << filename.c_str() << std::endl;
            }
            else {
                std::cout << "Saved image: " << filename.c_str() << std::endl;
            }
        }
    
    private:
        std::thread worker_thread;
        bool running;
        std::mutex mutex;
        std::condition_variable condition;
        std::queue<ImageInfo> image_queue;
        int image_count;
    };


#endif // __SAVE_IMAGE_THREAD_H__
