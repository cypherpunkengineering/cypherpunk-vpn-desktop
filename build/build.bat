@echo off

if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe"

pushd %~dp0

cd ..\daemon
echo Building 32-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=Win32
if %errorlevel% neq 0 goto error
echo Building 64-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=x64
if %errorlevel% neq 0 goto error

cd ..\client
echo Updating Node modules...
call npm --loglevel=silent install
if %errorlevel% neq 0 goto error
echo Rebuilding Electron modules...
del node_modules\nslog\build\Release\nslog.node
call node_modules\.bin\electron-rebuild.cmd --arch=ia32
if %errorlevel% neq 0 goto error
echo Packaging Electron app...
call node_modules\.bin\electron-packager.cmd .\ cyphervpn --overwrite --platform=win32 --arch=ia32 --icon=app\img\logo.ico --out=..\out\client\ --prune --asar
if %errorlevel% neq 0 goto error

echo Build successful!

:end
popd
exit /b %errorlevel%

:error
echo Build failed with error %errorlevel%!
goto end
