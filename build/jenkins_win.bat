@echo off

pushd %~dp0
call win\jenkins.bat
popd
exit /b %errorlevel%
