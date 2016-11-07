@echo off

if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe" set ISCC="%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 5\ISCC.exe" set ISCC="%ProgramFiles%\Inno Setup 5\ISCC.exe"

pushd %~dp0

cd ..\..\daemon
echo * Building 32-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=Win32
if %errorlevel% neq 0 goto error
echo * Building 64-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=x64
if %errorlevel% neq 0 goto error

cd ..\client
echo * Updating Node modules...
call npm --loglevel=silent install
if %errorlevel% neq 0 goto error

echo * Building client...
call npm --production run build
if %errorlevel% neq 0 goto error

echo * Rebuilding Electron modules...
del node_modules\nslog\build\Release\nslog.node
call node_modules\.bin\electron-rebuild.cmd --arch=ia32
if %errorlevel% neq 0 goto error

echo * Packaging Electron app...
call node_modules\.bin\electron-packager.cmd .\app\ CypherpunkVPN --overwrite --platform=win32 --arch=ia32 --icon=..\res\win\logo2.ico --out=..\out\win\client\ --prune --asar --version-string.FileDescription="Cypherpunk VPN" --version-string.CompanyName="Cypherpunk Partners LLC" --version-string.LegalCopyright="Copyright (C) 2016 Cypherpunk Partners LLC. All rights reserved." --version-string.OriginalFilename="CypherpunkVPN.exe" --version-string.ProductName="Cypherpunk VPN" --version-string.InternalName="CypherpunkVPN"
if %errorlevel% neq 0 goto error

cd ..\build\win
echo * Creating installer...
%ISCC% /Q setup.iss
if %errorlevel% neq 0 goto error

echo.
echo Build successful!

:end
popd
exit /b %errorlevel%

:error
echo.
echo Build failed with error %errorlevel%!
goto end
