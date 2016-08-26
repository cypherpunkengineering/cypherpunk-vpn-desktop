#include <unistd.h>
#include <limits.h>
#include "config.h"
#include "path.h"
#include "logger.h"

const char PATH_SEPARATOR = '/';
std::string g_argv0;
std::string g_argv0_path;

static bool g_is_installed = false;


void InitPaths(std::string argv0)
{
	g_argv0 = std::move(argv0);
	if (g_argv0.compare(0, 15, "/usr/local/bin/") == 0)
		g_is_installed = true;
	size_t last_slash = g_argv0.find_last_of(PATH_SEPARATOR);
	if (last_slash != std::string::npos)
		g_argv0_path = g_argv0.substr(0, last_slash);
	else
	{
		char cwd[PATH_MAX];
		g_argv0_path = getcwd(cwd, PATH_MAX);
	}
}

std::string GetPath(PredefinedFile file)
{
	switch (file)
	{
	case DaemonExecutable: return g_argv0;
	case OpenVPNExecutable: return g_is_installed ? "/usr/local/bin/cypherpunkvpn-openvpn" : GetPath(BaseDir, "daemon", "third_party", "openvpn_osx", "openvpn"); // Exists in /usr/local/bin, callable anywhere
	default:
		//LOG(ERROR) << "Unknown file";
		return std::string();
	}
}

std::string GetPath(PredefinedDirectory dir)
{
	switch (dir)
	{
	case BaseDir: return g_is_installed ? "/Applications/CypherpunkVPN.app" : (g_argv0_path + "/../..");
	case LogDir: return g_is_installed ? "/tmp" : g_argv0_path;
	case ProfileDir: return g_is_installed ? "/tmp" : g_argv0_path;
	case ScriptsDir: return g_is_installed ? GetPath(BaseDir, "scripts") : GetPath(BaseDir, "res", "osx", "openvpn-scripts");
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}
