@echo off

setlocal

pushd %~dp0

set BUILD_NUMBER=00000%BUILD_NUMBER%
set BUILD_NUMBER=%BUILD_NUMBER:~-5%

git submodule update --recursive --init
if %errorlevel% neq 0 goto error

cmd /c build.bat clean
if %errorlevel% neq 0 goto error

cd ..\..\out\win
scp -scp -P 92 -i "%USERPROFILE%\.ssh\pscp.ppk" cypherpunk-privacy-*.exe "upload@10.111.52.44:/data/builds/"
if %errorlevel% neq 0 goto error

:end
popd
exit /b %errorlevel%

:error
echo.
echo Build failed with error %errorlevel%!
goto end
