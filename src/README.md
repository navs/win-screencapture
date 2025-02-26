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