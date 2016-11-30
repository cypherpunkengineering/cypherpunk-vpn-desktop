@echo off

setlocal

pushd %~dp0

set BUILD_NUMBER=00000%BUILD_NUMBER%
set BUILD_NUMBER=%BUILD_NUMBER:~-5%

git submodule update --recursive --init
if %errorlevel% neq 0 goto error

cmd /c build.bat clean
if %errorlevel% neq 0 goto error

echo signing build
"C:\Program Files (x86)\Windows Kits\10\bin\x64\signtool" sign /debug /f "C:\cypherpunk\cyp-codesign.p12" /p %CODESIGNPW% cypherpunk-*.exe

cd ..\..\out\win
scp -scp -P 92 -i "%USERPROFILE%\.ssh\pscp.ppk" cypherpunk-*.exe "upload@builds-upload.cypherpunk.engineering:/data/builds/"
if %errorlevel% equ 0 goto upload_success
echo * Warning: failed to upload build
set errorlevel=0
:upload_success

:end
popd
exit /b %errorlevel%

:error
echo.
echo Build failed with error %errorlevel%!
goto end
