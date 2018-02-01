#include "config.h"
#include "path.h"
#include "posix.h"
#include "logger.h"

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
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
	if (!g_daemon_path.empty() && g_daemon_path[0])
	{
		if (char* real = realpath(g_daemon_path.c_str(), NULL))
		{
			g_daemon_path = real;
			free(real);
		}
	}
}

bool IsInstalled()
{
	return g_is_installed;
}

std::string GetPredefinedFile(PredefinedFile file, EnsureExistsTag ensure_path_exists)
{
	switch (file)
	{
	case DaemonExecutable: return g_argv0;
	case OpenVPNExecutable: return g_is_installed ? GetPath(OpenVPNDir, "cypherpunk-privacy-openvpn") : GetPath(OpenVPNDir, "openvpn");
	case ConfigFile: return GetPath(SettingsDir, ensure_path_exists, "config.json");
	case AccountFile: return GetPath(SettingsDir, ensure_path_exists, "account.json");
	case SettingsFile: return GetPath(SettingsDir, ensure_path_exists, "settings.json");
	case DaemonPortFile: return g_is_installed ? GetPath(BaseDir, "var", ensure_path_exists, "daemon.lock") : GetPath(g_daemon_path, "daemon.lock");
	case LocalSocketFile: return g_is_installed ? GetPath(BaseDir, "var", ensure_path_exists, "daemon.socket") : GetPath(g_daemon_path, "daemon.socket");
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
	case ScriptsDir: return g_is_installed
		? GetPath(BaseDir, "etc", "scripts")
		: GetPath(BaseDir, "res",
#if OS_LINUX
			"linux", "openvpn-scripts"
#else
			"osx", "openvpn-scripts"
#endif
		);
	case SettingsDir: return g_is_installed ? GetPath(BaseDir, "etc") : g_daemon_path;
	case LogDir: return g_is_installed ? GetPath(BaseDir, "var", "log") : g_daemon_path;
	case ProfileDir: return g_is_installed ? GetPath(BaseDir, "etc", "profiles") : g_daemon_path;
	case OpenVPNDir: return g_is_installed
		? GetPath(BaseDir, "bin")
		: GetPath(BaseDir, "daemon", "third_party",
#if OS_LINUX
			"openvpn_linux", "64"
#else
			"openvpn_osx"
#endif
		);
	default:
		LOG(ERROR) << "Unknown path: " << dir;
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
	int flags = O_CLOEXEC | O_NOFOLLOW;
	if (mode) for (const char* m = mode; *m; m++)
	{
		switch (*m)
		{
			case 'r':
				if (flags & (O_RDONLY | O_WRONLY | O_RDWR)) return NULL;
				flags |= O_RDONLY;
				break;
			case 'w':
				if (flags & (O_RDONLY | O_WRONLY | O_RDWR)) return NULL;
				flags |= O_WRONLY | O_CREAT | O_TRUNC;
				break;
			case 'a':
				if (flags & (O_RDONLY | O_WRONLY | O_RDWR)) return NULL;
				flags |= O_WRONLY | O_CREAT | O_APPEND;
				break;
			case '+':
				if ((flags & (O_RDONLY | O_WRONLY | O_RDWR)) == 0) return NULL;
				flags &= ~(O_RDONLY | O_WRONLY | O_RDWR);
				flags |= O_RDWR;
				break;
			case 'x':
				flags |= O_EXCL;
				break;
		}
	}

	int fd = open(filename, flags, 0644);
	if (fd < 0) return NULL;

	return fdopen(fd, mode);
}

int daemon_fclose(FILE* file)
{
	return fclose(file);
}

int daemon_unlink(const char* filename)
{
	return unlink(filename);
}
