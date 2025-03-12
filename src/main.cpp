#include <Windows.h>
#include <iostream>
#include <locale>
#include <string>
#include <chrono>
#include "Util.h"
#include "ScreenCapture.h"
#include "FrameRunner.h"
#include "SaveImageThread.h"

int main(int argc, char* argv[]) {

    CoInitialize(nullptr);

    // 한글 출력을 위한 locale 설정 
    std::locale::global(std::locale("kor"));

    if (argc < 2) {
        std::wcerr << L"Usage: " << argv[0] << L" <window title> [frameRate]" << std::endl;
        return 1;   
    }

    // argv[1] 를 문자열로 받아서 wchat_t* 로 변환
    std::wstring thisTitle = Util::ToWString(argv[0]);
    std::wstring windowTitle = Util::ToWString(argv[1]);
    std::wcout << "WindowTitle:" << windowTitle << std::endl;   

    // argv[2] 을 정수값으로 parsing 해서 frameRate 로 사용
    int frameRate = 30;
    if (argc > 2) {
        frameRate = std::stoi(argv[2]);
    }

    std::wcout << L"Capture Started. FPS=" << frameRate << std::endl;
    wchar_t fileName[MAX_PATH];
    
    auto windowInfo = Util::FindTargetWindow(windowTitle.c_str(), thisTitle.c_str());
    if (windowInfo.found) {
        int width = windowInfo.rect.right - windowInfo.rect.left;
        int height = windowInfo.rect.bottom - windowInfo.rect.top;

        ScreenCapture screenCapture;
        SaveImageThread saveImageThread;

        // Start Thread
        saveImageThread.Start();

        auto captureCallback = [&](const ScreenCapture::ImageBuffer& buffer, int width, int height, const std::wstring& filename) {
            saveImageThread.AddImage(buffer, width, height, filename);  
        };

        std::wcout << "Found: (" << windowInfo.rect.left << ", " << windowInfo.rect.top << ") (" << width << " x " << height << " )" << std::endl;
        try {
            FrameRunner runner;
            // 3프레임당 timestamp 출력
            auto capture = [&](const wchar_t* timestamp) -> bool {
                // Capture 에 걸린 시간 출력
                auto before = std::chrono::system_clock::now();

                // std::wcout << L"time:" << timestamp << std::endl;
                // fileName 을 timestamp + ".png" 로 저장
                StringCbPrintfW(fileName, sizeof(fileName), L"%s.png", timestamp);
                auto fileNameStr = std::wstring(fileName);

                for (int i = 0; i < 3; i++) {
                    bool success = screenCapture.CaptureScreenRegion(windowInfo.rect.left, windowInfo.rect.top, width, height, fileNameStr, captureCallback);
                    if (success) {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::microseconds(1));
                }

                auto after = std::chrono::system_clock::now();
                std::chrono::duration<double> elapsed = after - before;
                std::wcout << L"Capture Time: " << elapsed.count() << L" sec" << std::endl;

                //return false; // TEST -// dont continue
                return true;
            };
            FrameRunner frameRunner;
            frameRunner.Run(frameRate, capture);

        } catch (const std::exception& e) {
            std::wcerr << L"Error: " << e.what() << std::endl;
        }
        saveImageThread.Stop();

    }   else {
        std::wcerr << L"Window not found" << std::endl;
    }

    std::wcout << L"Capture Finished." << std::endl;
    CoUninitialize();
    return 0;
}


