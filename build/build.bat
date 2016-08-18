@echo off

if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe"

pushd %~dp0

pushd ..\daemon
echo Building 32-bit service...
%MSBUILD% daemon.vcxproj /p:Configuration=Release /p:Platform=Win32
if %errorlevel% neq 0 exit /b %errorlevel%
echo Building 64-bit service...
%MSBUILD% daemon.vcxproj /p:Configuration=Release /p:Platform=x64
if %errorlevel% neq 0 exit /b %errorlevel%
popd

pushd ..\client
echo Updating Node modules...
call npm install
if %errorlevel% neq 0 exit /b %errorlevel%
echo Rebuilding Electron modules...
del node_modules\nslog\build\Release\nslog.node
call node_modules\.bin\electron-rebuild.cmd --arch=ia32
if %errorlevel% neq 0 exit /b %errorlevel%
echo Packaging Electron app...
call node_modules\.bin\electron-packager.cmd .\ cyphervpn --overwrite --platform=win32 --arch=ia32 --icon=app\img\logo.ico --out=out\ --prune --asar
if %errorlevel% neq 0 exit /b %errorlevel%
popd

popd
