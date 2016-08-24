#include "config.h"
#include "path.h"
#include "win.h"

const char PATH_SEPARATOR = '\\';
std::string g_argv0;
std::string g_argv0_path;

void InitPaths(std::string argv0)
{
	g_argv0 = std::move(argv0);
	size_t last_slash = g_argv0.find_last_of(PATH_SEPARATOR);
	if (last_slash != std::string::npos)
		g_argv0_path = g_argv0.substr(0, last_slash);
	else
	{
		wchar_t cwd[MAX_PATH];
		g_argv0_path = convert<char>(_wgetcwd(cwd, MAX_PATH));
	}
}

std::string GetPath(PredefinedFile file)
{
	switch (file)
	{
	case DaemonExecutable: return g_argv0;
	case OpenVPNExecutable: return GetPath(OpenVPNDir, "cypherpunk-openvpn.exe");
	case TapInstallExecutable: return GetPath(TapDriverDir, "tapinstall.exe");
	default:
		LOG(ERROR) << "Unknown file";
		return std::string();
	}
}

std::string GetPath(PredefinedDirectory dir)
{
	switch (dir)
	{
	case BaseDir: return g_argv0_path;
	case LogDir: return GetPath(BaseDir, "logs");
	case OpenVPNDir: return GetPath(BaseDir, "openvpn" /*, IsWin64() ? "64" : "32"*/);
	case TapDriverDir: return GetPath(BaseDir, "tap" /*, IsWin64() ? "64" : "32"*/);
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}
