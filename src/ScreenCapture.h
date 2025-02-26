// ScreenCapture.cpp
#ifndef __SCREEN_CAPTURE_H__
#define __SCREEN_CAPTURE_H__

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#pragma comment(lib, "gdiplus.lib")

class ScreenCapture {
public:
    static bool CaptureScreenRegion(int x, int y, int width, int height, const wchar_t* filename) {
        // GDI+ 초기화
        Gdiplus::GdiplusStartupInput gdiplusStartupInput;
        ULONG_PTR gdiplusToken;
        Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);

        bool result = false;
        
        // 화면 DC 얻기
        HDC screenDC = GetDC(NULL);
        HDC memDC = CreateCompatibleDC(screenDC);

        // 비트맵 생성
        HBITMAP hBitmap = CreateCompatibleBitmap(screenDC, width, height);
        HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

        // 화면 영역 복사
        BitBlt(memDC, 0, 0, width, height, screenDC, x, y, SRCCOPY);

        // Bitmap 객체 생성
        Gdiplus::Bitmap* bitmap = Gdiplus::Bitmap::FromHBITMAP(hBitmap, NULL);
        
        if (bitmap) {
            // CLSID 얻기
            CLSID encoderClsid;
            if (GetEncoderClsid(L"image/png", &encoderClsid)) {
                // 파일로 저장
                result = (bitmap->Save(filename, &encoderClsid, NULL) == Gdiplus::Ok);
            }
            delete bitmap;
        }

        // 리소스 해제
        SelectObject(memDC, hOldBitmap);
        DeleteObject(hBitmap);
        DeleteDC(memDC);
        ReleaseDC(NULL, screenDC);

        // GDI+ 종료
        Gdiplus::GdiplusShutdown(gdiplusToken);

        return result;
    }

private:
    static int GetEncoderClsid(const wchar_t* format, CLSID* pClsid) {
        UINT num = 0;
        UINT size = 0;

        Gdiplus::ImageCodecInfo* pImageCodecInfo = NULL;

        Gdiplus::GetImageEncodersSize(&num, &size);
        if (size == 0)
            return -1;

        pImageCodecInfo = (Gdiplus::ImageCodecInfo*)(malloc(size));
        if (pImageCodecInfo == NULL)
            return -1;

        Gdiplus::GetImageEncoders(num, size, pImageCodecInfo);

        for (UINT j = 0; j < num; ++j) {
            if (wcscmp(pImageCodecInfo[j].MimeType, format) == 0) {
                *pClsid = pImageCodecInfo[j].Clsid;
                free(pImageCodecInfo);
                return j;
            }
        }

        free(pImageCodecInfo);
        return -1;
    }
};

// main.cpp
//int main() {
//    ScreenCapture::CaptureScreenRegion(100, 100, 800, 600, L"capture.bmp");
//    return 0;
//}

#endif __SCREEN_CAPTURE_H__