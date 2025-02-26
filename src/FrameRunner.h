#ifndef __FRAME_RUNNER_H__
#define __FRAME_RUNNER_H__

#include <windows.h>
#include <strsafe.h>
#include <iostream>
#include <functional>

class FrameRunner {
private:
    HANDLE m_hTimer;
    int m_frameCount;

public:
    FrameRunner() : m_frameCount(0) {
        // 고해상도 타이머 생성
        m_hTimer = CreateWaitableTimer(NULL, TRUE, NULL);
        if (m_hTimer == NULL) {
            throw std::runtime_error("Failed to create timer");
        }
    }

    ~FrameRunner() {
        if (m_hTimer != NULL) {
            CloseHandle(m_hTimer);
        }
    }

    void CallbackWithTimestamp(SYSTEMTIME st, std::function<void(const wchar_t* timestamp)> callback) {
        WCHAR timestamp[MAX_PATH];
        StringCbPrintfW(timestamp, sizeof(timestamp),
            L"%04d%02d%02d_%02d%02d%02d_%03d",
            st.wYear, st.wMonth, st.wDay,
            st.wHour, st.wMinute, st.wSecond,
            m_frameCount);

        callback(timestamp);
    }

    void Run(int frames, std::function<void(const wchar_t* timestamp)> callback) {      
        double frameDelay = 1000.0 / frames;

        SYSTEMTIME lastTime;
        GetLocalTime(&lastTime);

        // 현재 프레임 번호는 currentTime 의 초 미만 소수점 * frames 를 정수로 캐스팅한 값으로
        m_frameCount = lastTime.wMilliseconds * frames / 1000;
        
        while (true) {
            // 현재 시간 측정
            SYSTEMTIME currentTime;
            GetLocalTime(&currentTime);
            
            // 현재 프레임 번호는 currentTime 의 초 미만 소수점 * frames 를 정수로 캐스팅한 값으로
            int nextFrameCount = currentTime.wMilliseconds * frames / 1000;
            if (nextFrameCount == m_frameCount) {
                int nextFrameMilliseconds = (nextFrameCount + 1) * 1000 / frames;
                // 프레임이 변경되지 않았으면 다음 프레임까지 대기 
                LARGE_INTEGER dueTime;
                dueTime.QuadPart = -1 * (int)(nextFrameMilliseconds - currentTime.wMilliseconds) * 10000;

                SetWaitableTimer(m_hTimer, &dueTime, 0, NULL, NULL, FALSE);
                WaitForSingleObject(m_hTimer, INFINITE);
                continue;
            }
            lastTime = currentTime;
            m_frameCount = nextFrameCount;
            CallbackWithTimestamp(currentTime, callback);
        }
    }
};

#endif // __FRAME_RUNNER_H__