@echo off

setlocal

if exist "%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles(x86)%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe" set MSBUILD="%ProgramFiles%\MSBuild\14.0\bin\MSBuild.exe"
if exist "%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe" set ISCC="%ProgramFiles(x86)%\Inno Setup 5\ISCC.exe"
if exist "%ProgramFiles%\Inno Setup 5\ISCC.exe" set ISCC="%ProgramFiles%\Inno Setup 5\ISCC.exe"

set SIGNTOOL="%ProgramFiles(x86)%\Windows Kits\10\bin\x64\signtool.exe"

rem Allow user to locally specify parameters in an optional batch file
call config.bat 2> NUL
set errorlevel=0

set ENABLE_SIGNING=
if NOT [%SIGNTOOL%] == [] if exist %SIGNTOOL% if NOT [%CODESIGNCERT%] == [] if NOT [%CODESIGNPW%] == [] if exist %CODESIGNCERT% set ENABLE_SIGNING=1
if [%ENABLE_SIGNING%] == [] echo * No certificate specified (or signtool not found); building unsigned package...
if NOT [%ENABLE_SIGNING%] == [] echo * Using %CODESIGNCERT:"=% for signing...


pushd %~dp0

if not [%1] == [clean] goto skip_clean
echo * Cleaning...
rmdir /s /q ..\..\out\win 2>NUL
:skip_clean

cd ..

cd ..\client
echo * Updating Node modules...
call npm --loglevel=silent install
if errorlevel 1 goto error

call npm run build-version
if errorlevel 1 goto error
call npm run apply-version
if errorlevel 1 goto error

echo * Building client...
call npm --production run build
if errorlevel 1 goto error

echo * Rebuilding Electron modules...
if exist node_modules\nslog\build\Release\nslog.node del /f node_modules\nslog\build\Release\nslog.node
call node_modules\.bin\electron-rebuild.cmd --arch=ia32
if errorlevel 1 goto error

echo * Packaging Electron app...
call node_modules\.bin\electron-packager.cmd .\app\ CypherpunkPrivacy --overwrite --platform=win32 --arch=ia32 --icon=..\res\win\logo2.ico --out=..\out\win\client\ --prune --asar --version-string.FileDescription="Cypherpunk Privacy" --version-string.CompanyName="Cypherpunk Partners, slf." --version-string.LegalCopyright="Copyright (C) 2017 Cypherpunk Partners, slf. All rights reserved." --version-string.OriginalFilename="CypherpunkPrivacy.exe" --version-string.ProductName="Cypherpunk Privacy" --version-string.InternalName="CypherpunkPrivacy"
if errorlevel 1 goto error

cd ..\daemon
echo * Building 32-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=Win32
if errorlevel 1 goto error
echo * Building 64-bit service...
%MSBUILD% daemon.vcxproj /nologo /v:q /p:Configuration=Release /p:Platform=x64
if errorlevel 1 goto error

cd ..\build\win
echo * Creating installer...
if [%ENABLE_SIGNING%] == [] (
  %ISCC% /Q setup.iss
  if errorlevel 1 goto error
) else (
  %ISCC% /Q /DENABLE_SIGNING=1 /Sstandard="%SIGNTOOL:"=$q% sign /f %CODESIGNCERT:"=$q% /p %CODESIGNPW:"=$q% $f" setup.iss
  if errorlevel 1 goto error
)

echo.
echo Build successful!

:end
popd
endlocal
exit /b %errorlevel%

:error
echo.
echo Build failed with error %errorlevel%!
goto end
