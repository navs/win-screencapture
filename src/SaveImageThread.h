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
            ScreenCapture::ImageBuffer image_buffer(buffer);
            image_queue.push({ image_buffer, width, height, filename });
            std::wcout << L"AddImage: " << filename << std::endl;
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
                    if (image_queue.empty()) break;
                    image_info = image_queue.front();
                    image_queue.pop();
                }
                SaveImageToFile(image_info.buffer, image_info.width, image_info.height, image_info.filename);
            }
        }
    
        void SaveImageToFile(const ScreenCapture::ImageBuffer& buffer, int width, int height, const std::wstring& filename) {
            //GUID CLSID_PngEncoder;
            //CLSIDFromString(L"{1B72AAE1-E766-4A07-8327-0E86B800BCF7}", &CLSID_PngEncoder);
    		GUID CLSID_PngEncoder = GUID_ContainerFormatPng;
    
            IWICImagingFactory* pFactory = nullptr;
            IWICBitmapEncoder* pEncoder = nullptr;
            IWICBitmapFrameEncode* pFrameEncode = nullptr;
            IWICStream* pStream = nullptr;
            IWICBitmap* pBitmap = nullptr;
    
            CoInitialize(nullptr);
            HRESULT hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_IWICImagingFactory, (LPVOID*)&pFactory);

            int step = 0;
    
            while (SUCCEEDED(hr)) {
                step = 1;
                hr = pFactory->CreateStream(&pStream);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 2;
                hr = pStream->InitializeFromFilename(filename.c_str(), GENERIC_WRITE);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 3;
                hr = pFactory->CreateEncoder(CLSID_PngEncoder, nullptr, &pEncoder);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 4;
                hr = pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 5;
                hr = pEncoder->CreateNewFrame(&pFrameEncode, nullptr);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 6;
                hr = pFrameEncode->Initialize(nullptr);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 7;
                hr = pFrameEncode->SetSize(width, height);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 8;
                WICPixelFormatGUID format = GUID_WICPixelFormat32bppBGRA;
                hr = pFrameEncode->SetPixelFormat(&format);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 9;
                hr = pFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, static_cast<UINT>(buffer.size()), const_cast<BYTE*>(buffer.data()), &pBitmap);
                // hr = pFactory->CreateBitmapFromMemory(width, height, GUID_WICPixelFormat32bppBGRA, width * 4, static_cast<UINT>(buffer.size()), (BYTE*)(buffer.data()), &pBitmap);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 10;
                hr = pFrameEncode->WriteSource(pBitmap, nullptr);
                if (FAILED(hr)) {
                    break;
                }
        
                step = 11;
                hr = pFrameEncode->Commit();
                if (FAILED(hr)) {
                    break;
                }
        
                step = 12;
                hr = pEncoder->Commit();
                break;
            }
    
            if (pBitmap) pBitmap->Release();
            if (pStream) pStream->Release();
            if (pFrameEncode) pFrameEncode->Release();
            if (pEncoder) pEncoder->Release();
            if (pFactory) pFactory->Release();
            CoUninitialize();
    
            if (FAILED(hr)) {
                std::wcerr << L"Failed to save image: [" << step << L"] " << hr << L":" << filename << std::endl;
            }
            else {
                std::wcout << L"Saved image: " << filename << std::endl;
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
