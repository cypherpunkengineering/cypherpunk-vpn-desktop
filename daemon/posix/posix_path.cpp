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
#include <sys/stat.h>

const char PATH_SEPARATOR = '/';
std::string g_argv0;
std::string g_daemon_path;

static bool g_is_installed = false;


void InitPaths(std::string argv0)
{
	g_argv0 = std::move(argv0);
	if (g_argv0.compare(0, 22, "/usr/local/cypherpunk/") == 0)
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

std::string GetPredefinedFile(PredefinedFile file, EnsureExistsTag ensure_path_exists)
{
	switch (file)
	{
	case DaemonExecutable: return g_argv0;
	case OpenVPNExecutable: return g_is_installed
		? "/usr/local/cypherpunk/bin/cypherpunk-privacy-openvpn"
#if OS_LINUX
		: GetPath(BaseDir, "daemon", "third_party", "openvpn_linux", "64", "openvpn");
#else
		: GetPath(BaseDir, "daemon", "third_party", "openvpn_osx", "openvpn");
#endif
	case ConfigFile: return GetPath(SettingsDir, ensure_path_exists, "config.json");
	case AccountFile: return GetPath(SettingsDir, ensure_path_exists, "account.json");
	case SettingsFile: return GetPath(SettingsDir, ensure_path_exists, "settings.json");
	default:
		LOG(ERROR) << "Unknown file";
		return std::string();
	}
}

std::string GetPredefinedDirectory(PredefinedDirectory dir)
{
	switch (dir)
	{
	case BaseDir: return g_is_installed ? "/usr/local/cypherpunk" : (g_daemon_path + "/../../..");
#if OS_LINUX
	case ScriptsDir: return g_is_installed ? GetPath(BaseDir, "etc", "scripts") : GetPath(BaseDir, "res", "linux", "openvpn-scripts");
#else
	case ScriptsDir: return g_is_installed ? GetPath(BaseDir, "etc", "scripts") : GetPath(BaseDir, "res", "osx", "openvpn-scripts");
#endif
	case SettingsDir: return g_is_installed ? GetPath(BaseDir, "etc") : g_daemon_path;
	case LogDir: return g_is_installed ? GetPath(BaseDir, "var", "log") : g_daemon_path;
	case ProfileDir: return g_is_installed ? GetPath(BaseDir, "etc", "profiles") : g_daemon_path;
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}

void RecursivelyCreateDirectory(std::string& path, size_t end)
{
	if (-1 == mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))
	{
		int error = errno;
		if (error == ENOENT && end > 0)
		{
			size_t p = path.rfind(PATH_SEPARATOR, end - 1);
			if (p != path.npos)
			{
				{
					path[p] = 0;
					FINALLY({ path[p] = PATH_SEPARATOR; });
					RecursivelyCreateDirectory(path, p);
				}
				POSIX_CHECK(mkdir, (path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH));
				return;
			}
		}
		if (error != EEXIST)
			THROW_POSIXEXCEPTION(error, mkdir);
	}
}

std::string& EnsurePathExists(std::string& path)
{
	RecursivelyCreateDirectory(path, path.size());
	return path;
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
	return result;
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
#ifdef HAVE_FSYNC
		POSIX_CHECK_IF_NONZERO(fsync, (POSIX_CHECK(fileno, (f))));
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
