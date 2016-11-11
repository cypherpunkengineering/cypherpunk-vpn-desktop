#include "config.h"
#include "path.h"
#include "posix.h"
#include "logger.h"

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <errno.h>
#include <system_error>
#include <fcntl.h>

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
#if OS_LINUX
	case OpenVPNExecutable: return g_is_installed ? "/usr/local/bin/cypherpunkvpn-openvpn" : GetPath(BaseDir, "daemon", "third_party", "openvpn_linux", "openvpn"); // Exists in /usr/local/bin, callable anywhere
#else
	case OpenVPNExecutable: return g_is_installed ? "/usr/local/bin/cypherpunkvpn-openvpn" : GetPath(BaseDir, "daemon", "third_party", "openvpn_osx", "openvpn"); // Exists in /usr/local/bin, callable anywhere
#endif
	case SettingsFile: return GetPath(SettingsDir, "settings.json");
	default:
		LOG(ERROR) << "Unknown file";
		return std::string();
	}
}

std::string GetPath(PredefinedDirectory dir)
{
	switch (dir)
	{
#if OS_LINUX
	case BaseDir: return g_is_installed ? "/usr/local/bin/CypherpunkVPN" : (g_daemon_path + "/../..");
	case ScriptsDir: return g_is_installed ? "/usr/local/libexec/CypherpunkVPN/scripts" : GetPath(BaseDir, "res", "osx", "openvpn-scripts");
	case SettingsDir: return g_is_installed ? "/usr/local/etc/CypherpunkVPN" : g_daemon_path;
#else
	case BaseDir: return g_is_installed ? "/Applications/CypherpunkVPN.app" : (g_daemon_path + "/../..");
	case ScriptsDir: return g_is_installed ? GetPath(BaseDir, "Contents/Resources/scripts") : GetPath(BaseDir, "res", "osx", "openvpn-scripts");
	case SettingsDir: return g_is_installed ? "/Library/Application Support/CypherpunkVPN" : g_daemon_path;
#endif
	case LogDir: return g_is_installed ? "/tmp" : g_daemon_path;
	case ProfileDir: return g_is_installed ? "/tmp" : g_daemon_path;
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}

std::string ReadFile(const std::string& path)
{
	std::string result;
	FILE* f = POSIX_CHECK_IF_NULL(daemon_fopen, (path.c_str(), "r"));
	{
		FINALLY({ daemon_fclose(f); });
		fseek(f, 0, SEEK_END);
		long size = ftell(f);
		fseek(f, 0, SEEK_SET);
		result.resize(size);
		if (size != fread(&result[0], 1, size, f))
			THROW_POSIXEXCEPTION(EIO, fread);
	}
	return std::move(result);
}

void WriteFile(const std::string& path, const char* text, size_t length)
{
	const std::string tmp = path + ".new";
	FILE* f = POSIX_CHECK_IF_NULL(daemon_fopen, (tmp.c_str(), "w"));
	{
		FINALLY({ daemon_fclose(f); });
		if (length != fwrite(text, 1, length, f))
			THROW_POSIXEXCEPTION(EIO, fwrite);
		POSIX_CHECK_IF_NONZERO(fflush, (f));
		int fd = POSIX_CHECK(fileno, (f));
#ifdef HAVE_FSYNC
		POSIX_CHECK_IF_NONZERO(fsync, (fd));
#endif
	}
	POSIX_CHECK_IF_NONZERO(rename, (tmp.c_str(), path.c_str()));
}

FILE* daemon_fopen(const char* filename, const char* mode)
{
	FILE* file = fopen(filename, mode);
	if (file)
	{
		int fd = fileno(file);
		if (fd != -1)
		{
			int flags = fcntl(fd, F_GETFD);
			if (flags != -1)
			{
				// Set the close-on-exec flag on the file so it's not inherited by forked processes
				fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
			}
		}
	}
	return file;
}

int daemon_fclose(FILE* file)
{
	return fclose(file);
}
