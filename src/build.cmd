
chcp 65001

:::# Debug 빌드
:::MSBuild.exe ScreenCapture.vcxproj /p:Configuration=Debug /p:Platform=Win32

:::# Release 빌드
MSBuild.exe ScreenCapture.vcxproj /p:Configuration=Release /p:Platform=Win32 

::: Visual Studio 2022
:::"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ScreenCapture.vcxproj /p:Configuration=Debug /p:Platform=Win32