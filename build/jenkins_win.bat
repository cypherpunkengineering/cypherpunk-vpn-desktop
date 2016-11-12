git submodule update --recursive --init
cd build/win/
cmd /c build.bat
cd ..\..\
copy "out\win\setup\CypherpunkPrivacy.exe" "cypherpunk-vpn-windows-%BUILD_NUMBER%.exe"
scp -scp -P 92 -i "%USERPROFILE%\.ssh\pscp.ppk" "cypherpunk-vpn-windows-%BUILD_NUMBER%.exe" "upload@10.111.52.44:/data/builds/"
