#include "config.h"
#include "path.h"
#include "win.h"

#include <shlwapi.h>

#pragma comment (lib, "shlwapi.lib")

const char PATH_SEPARATOR = '\\';
std::string g_argv0;
std::string g_daemon_path;
std::string g_client_executable;

static void SimplifyPathInPlace(std::string& path)
{
	size_t pos = 0;
	while (pos < path.size())
	{
		pos = path.find("\\..", pos);
		if (pos != path.npos)
		{
			if (pos > 0 && (pos + 3 == path.size() || path[pos + 3] == '\\'))
			{
				size_t slash = path.rfind('\\', pos - 1);
				if (slash != path.npos)
				{
					path.erase(slash, pos + 3 - slash);
					continue;
				}
			}
			pos += 3;
		}
	}
}
static std::string SimplifyPath(const std::string& path)
{
	std::string result = path;
	SimplifyPathInPlace(result);
	return std::move(result);
}
static std::string SimplifyPath(std::string&& path)
{
	SimplifyPathInPlace(path);
	return path;
}

void InitPaths(std::string argv0)
{
	g_argv0 = std::move(argv0);
	size_t last_slash = g_argv0.find_last_of(PATH_SEPARATOR);
	if (last_slash != std::string::npos)
		g_daemon_path = g_argv0.substr(0, last_slash);
	else
	{
		wchar_t cwd[MAX_PATH];
		g_daemon_path = convert<char>(_wgetcwd(cwd, MAX_PATH));
	}
	g_client_executable = g_daemon_path + "\\CypherpunkVPN.exe";
	if (!PathFileExists(convert<TCHAR>(g_client_executable).c_str()))
	{
#ifdef _DEBUG
		g_client_executable = SimplifyPath(g_daemon_path + "\\..\\..\\..\\..\\..\\client\\node_modules\\electron-prebuilt\\dist\\electron.exe");
		if (!PathFileExists(convert<TCHAR>(g_client_executable).c_str()))
#endif
		{
			g_client_executable = std::string();
			LOG(WARNING) << "Client executable not found";
		}
	}
}

std::string GetPath(PredefinedFile file)
{
	switch (file)
	{
	case DaemonExecutable: return g_argv0;
	case ClientExecutable: return g_client_executable;
	case OpenVPNExecutable: return GetPath(OpenVPNDir, "cypherpunkvpn-openvpn.exe");
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
	case BaseDir: return g_daemon_path;
	case LogDir: return GetPath(BaseDir, "logs");
	case OpenVPNDir: return GetPath(BaseDir, "openvpn" /*, IsWin64() ? "64" : "32"*/);
	case ProfileDir: return GetPath(BaseDir, "profiles");
	case TapDriverDir: return GetPath(BaseDir, "tap" /*, IsWin64() ? "64" : "32"*/);
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}
