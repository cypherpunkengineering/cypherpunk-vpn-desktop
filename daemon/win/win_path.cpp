#include "config.h"
#include "path.h"
#include "win.h"

#include <tchar.h>
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
					pos = slash;
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
	case SettingsFile: return GetPath(SettingsDir, "settings.json");
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
	case SettingsDir: return GetPath(BaseDir);
	default:
		LOG(ERROR) << "Unknown path";
		return std::string();
	}
}

std::string ReadFile(const std::string& path)
{
	auto file = Win32Handle::Wrap(WIN_CHECK_IF_INVALID(CreateFile, (convert<TCHAR>(path).c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL)));
	LARGE_INTEGER size;
	WIN_CHECK_IF_FALSE(GetFileSizeEx, (file, &size));
	if (size.HighPart != 0)
		THROW_WIN32EXCEPTION(ERROR_FILE_TOO_LARGE, ReadFile);
	std::string result;
	result.resize((size_t)size.LowPart);
	DWORD read = 0;
	WIN_CHECK_IF_FALSE(ReadFile, (file, &result[0], size.LowPart, &read, NULL));
	if (read != size.LowPart)
		THROW_WIN32EXCEPTION(ERROR_READ_FAULT, ReadFile);
	return std::move(result);
}

void WriteFile(const std::string& path, const char* text, size_t length)
{
	if (length > (DWORD)0xFFFFFFFF)
		THROW_WIN32EXCEPTION(ERROR_FILE_TOO_LARGE, WriteFile);
	const std::tstring tmp = convert<TCHAR>(path) + _T(".new");
	{
		auto file = Win32Handle::Wrap(WIN_CHECK_IF_INVALID(CreateFile, (tmp.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL)));
		DWORD written = 0;
		WIN_CHECK_IF_FALSE(WriteFile, (file, text, (DWORD)length, &written, NULL));
		if (written != length)
			THROW_WIN32EXCEPTION(ERROR_WRITE_FAULT, WriteFile);
		WIN_CHECK_IF_FALSE(FlushFileBuffers, (file));
	}
	WIN_CHECK_IF_FALSE(MoveFileEx, (tmp.c_str(), convert<TCHAR>(path).c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH));
}

FILE* daemon_fopen(const char* filename, const char* mode)
{
	return fopen(filename, mode);
}

int daemon_fclose(FILE* file)
{
	return fclose(file);
}
