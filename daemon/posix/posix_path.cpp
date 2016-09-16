#include "config.h"
#include "path.h"
#include "posix.h"
#include "logger.h"

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <system_error>

const char PATH_SEPARATOR = '/';
std::string g_argv0;
std::string g_daemon_path;

static bool g_is_installed = false;


void InitPaths(std::string argv0)
{
	g_argv0 = std::move(argv0);
	if (g_argv0.compare(0, 15, "/usr/local/bin/") == 0)
		g_is_installed = true;
	size_t last_slash = g_argv0.find_last_of(PATH_SEPARATOR);
	if (last_slash != std::string::npos)
		g_daemon_path = g_argv0.substr(0, last_slash);
	else
	{
		char cwd[PATH_MAX];
		g_daemon_path = getcwd(cwd, PATH_MAX);
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
	case BaseDir: return g_is_installed ? "/Applications/CypherpunkVPN.app" : (g_daemon_path + "/../..");
	case LogDir: return g_is_installed ? "/tmp" : g_daemon_path;
	case ProfileDir: return g_is_installed ? "/tmp" : g_daemon_path;
	case ScriptsDir: return g_is_installed ? GetPath(BaseDir, "Contents/Resources/scripts") : GetPath(BaseDir, "res", "osx", "openvpn-scripts");
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}

std::string ReadFile(const std::string& path)
{
	std::string result;
	FILE* f = fopen(path.c_str(), "r");
	if (f == NULL)
		THROW_POSIXEXCEPTION(errno, fopen);
	{
		FINALLY({ fclose(f); });
		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		result.resize(size);
		if (size != fread(&result[0], 1, size, f))
			THROW_POSIXEXCEPTION(EIO, fread);
	}
	return std::move(result);
}

void WriteFile(const std::string& path, const char* text, size_t length)
{
	const std::string tmp = path + ".new";
	FILE* f = fopen(tmp.c_str(), "w");
	if (f == NULL)
		THROW_POSIXEXCEPTION(errno, fopen);
	{
		FINALLY({ fclose(f); });
		if (length != fwrite(text, 1, length, f))
			THROW_POSIXEXCEPTION(EIO, fwrite);
		if (0 != fflush(f))
			THROW_POSIXEXCEPTION(errno, fflush);
		int fd = fileno(f);
		if (-1 == (fd = fileno(f)))
			THROW_POSIXEXCEPTION(errno, fileno);
#ifdef HAVE_FSYNC
		if (0 != fsync(fd))
			THROW_POSIXEXCEPTION(errno, fsync);
#endif
	}
	if (0 != rename(tmp.c_str(), path.c_str()))
		THROW_POSIXEXCEPTION(errno, rename);
}
