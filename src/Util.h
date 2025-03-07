#ifndef __UTIL_H__
#define __UTIL_H__

#include <Windows.h>
#include <iostream>
#include <string>
#include <codecvt>

// ScreenCapture 클래스에 추가할 함수들
struct WindowInfo {
    HWND hwnd;
    const wchar_t* partialTitle;
    const wchar_t* thisTitle;
    RECT rect;
    bool found;
};

class Util {
public:

static WindowInfo FindTargetWindow(const wchar_t* partialTitle, const wchar_t* thisTitle) {
    WindowInfo info = { NULL, NULL, NULL, {0, 0, 0, 0}, false };
    info.partialTitle = partialTitle;
    info.thisTitle = thisTitle;
    
    // 현재 활성화된 모든 창을 순회하면서 찾기
    EnumWindows([](HWND hwnd, LPARAM lParam) -> BOOL {
        WindowInfo* info = reinterpret_cast<WindowInfo*>(lParam);
        const wchar_t* searchTitle = info->partialTitle;
        const wchar_t* thisTitle = info->thisTitle;

        // 창 제목 가져오기
        wchar_t title[1024];
        int len = GetWindowTextW(hwnd, title, sizeof(title)/sizeof(wchar_t));

//        std::wcout << L"Len: " <<  len << std::endl;
//        std::wcout << L"Title: " << title << L":" << std::endl;
//        std::wcout << L"PartialTitle: " <<  info->partialTitle << std::endl;

        // thisTitle 이 포함되어 있는지 확인
        if (wcsstr(title, thisTitle) != nullptr) {
            return TRUE; // 계속 검색
        }

        // 창이 보이는 상태인지 확인
        if (!IsWindowVisible(hwnd)) {
            return TRUE; // 계속 검색
        }

        // 현재 프로세스의 창인지 확인
        DWORD pid;
        GetWindowThreadProcessId(hwnd, &pid);
        if (pid == GetCurrentProcessId()) {
            return TRUE; // 계속 검색
        }

        // 창 제목에 검색어가 포함되어 있는지 확인
        if (wcsstr(title, searchTitle) != nullptr) {
            // 창의 실제 영역 가져오기 (테두리, 캡션 등 포함)
            RECT rect;
            GetWindowRect(hwnd, &rect);
            
            // 창이 최소화되어 있는지 확인
            if (!IsIconic(hwnd)) {
                info->hwnd = hwnd;
                info->rect = rect;
                info->found = true;
                return FALSE; // 검색 중단
            }
        }
        return TRUE; // 계속 검색
    }, reinterpret_cast<LPARAM>(&info));

    if (info.found) {
        // 클라이언트 영역만 가져오기 (테두리, 캡션 등 제외)
        RECT clientRect;
        GetClientRect(info.hwnd, &clientRect);
        
        // 클라이언트 영역의 스크린 좌표 계산
        POINT pt = {clientRect.left, clientRect.top};
        ClientToScreen(info.hwnd, &pt);
        info.rect.left = pt.x;
        info.rect.top = pt.y;
        
        pt.x = clientRect.right;
        pt.y = clientRect.bottom;
        ClientToScreen(info.hwnd, &pt);
        info.rect.right = pt.x;
        info.rect.bottom = pt.y;
    }
    
    return info;
}

// 사용 예시:
// auto windowInfo = ScreenCapture::FindTargetWindow(L"Notepad");
// if (windowInfo.found) {
//     int width = windowInfo.rect.right - windowInfo.rect.left;
//     int height = windowInfo.rect.bottom - windowInfo.rect.top;
//     CaptureScreenRegion(windowInfo.rect.left, windowInfo.rect.top, 
//                        width, height, L"window_capture.bmp");
// }

static std::wstring ToWString(const char* str)
{
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

}; // Util

#endif // __UTIL_H__