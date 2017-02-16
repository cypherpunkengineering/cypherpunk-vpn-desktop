@echo off

setlocal

pushd %~dp0

set BUILD_NUMBER=00000%BUILD_NUMBER%
set BUILD_NUMBER=%BUILD_NUMBER:~-5%

set CODESIGNCERT="C:\cypherpunk\cyp-codesign.p12"

git submodule update --recursive --init
if %errorlevel% neq 0 goto error

cmd /c build.bat clean
if %errorlevel% neq 0 goto error

echo cd to output dir
cd ..\..\out\win

echo Save build artifacts
for %%f in (cypherpunk-*.exe) do set ARTIFACT=%%~nxf
echo Uploading build to builds repo...
scp -scp -P 92 -i "%USERPROFILE%\.ssh\pscp.ppk" "%ARTIFACT%" "upload@builds-upload.cypherpunk.engineering:/data/builds/"
echo Uploading build to GCS bucket...
call "C:\Program Files (x86)\Google\Cloud SDK\google-cloud-sdk\bin\gsutil" cp "%ARTIFACT%" gs://builds.cypherpunk.com/builds/windows/
echo Sending notification to slack...
"C:\windows\system32\curl" -X POST --data "payload={\"text\": \"cypherpunk-privacy-windows build %BUILD_NUMBER% is now available from https://download.cypherpunk.com/builds/windows/%ARTIFACT%\"}" https://hooks.slack.com/services/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

echo done

if %errorlevel% equ 0 goto upload_success
echo * Warning: failed to upload build
set errorlevel=0
:upload_success

:end
popd
endlocal
exit /b %errorlevel%

:error
echo.
echo Build failed with error %errorlevel%!
goto end
