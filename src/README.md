# Screenshot2bmp

## Build Helps
### 자세한 빌드 출력을 보려면
MSBuild.exe ScreenCapture.vcxproj /verbosity:detailed

### 병렬 빌드 (예: 4개 코어 사용)
MSBuild.exe ScreenCapture.vcxproj /m:4

### 빌드 전 청소
MSBuild.exe ScreenCapture.vcxproj /t:Clean

### 리빌드 (Clean + Build)
MSBuild.exe ScreenCapture.vcxproj /t:Rebuild

## Issues
- Capture 타임이 0.03 초 이상 걸린다.
  - BitBlt 와 Save Time 이 0.012 초 이상 소요
<pre>
WindowTitle:Bubble
Capture Started. FPS=120
Found: (28, 51) (256 x 224 )
time:20250312_165456_078
GDI+ Startup Time: 0.0010082 sec
DC Creation Time: 0.0023248 sec
Bitmap Creation Time: 0.0016799 sec
BitBlt Time: 0.0126205 sec
Bitmap Creation Time: 0.0023767 sec
Save Time: 0.0127441 sec
GDI+ Shutdown Time: 0.0025364 sec
Capture Time: 0.0381966 sec
</pre>