#pragma once

#include <string>
#include <cstdio>

enum PredefinedDirectory
{
	BaseDir,		// Base directory of config files etc. (e.g. C:\Program Files\Cypherpunk Privacy\ on Windows, /usr/local/cypherpunk on macOS/Linux)
	LogDir,			// Directory where to write log file(s)
	OpenVPNDir,		// Directory where OpenVPN binary sits
	ProfileDir,		// Directory where temporary OpenVPN profile files are written
	ScriptsDir,		// Directory where OpenVPN hook scripts are
	SettingsDir,	// Directory where daemon and connection settings are read/written
#ifdef WIN32
	TapDriverDir,
#endif
};
enum PredefinedFile
{
	DaemonExecutable,
	ClientExecutable,
	OpenVPNExecutable,
	ConfigFile,
	AccountFile,
	SettingsFile,
#ifdef WIN32
	TapInstallExecutable,
#endif
};
enum EnsureExistsTag
{
	EnsureExists = true,
	DontEnsureExists = false,
};

extern void InitPaths(std::string argv0);
extern std::string GetPredefinedFile(PredefinedFile file, EnsureExistsTag ensure_path_exists = DontEnsureExists);
extern std::string GetPredefinedDirectory(PredefinedDirectory dir);
extern std::string& EnsurePathExists(std::string& path);
extern std::string ReadFile(const std::string& path);
extern void WriteFile(const std::string& path, const char* text, size_t length);

extern FILE* daemon_fopen(const char* filename, const char* mode);
extern int daemon_fclose(FILE* file);

extern const char PATH_SEPARATOR;

static inline std::string EnsurePathExists(std::string&& path) { return std::move(EnsurePathExists(static_cast<std::string&>(path))); }
static inline void WriteFile(const std::string& path, const std::string& text) { WriteFile(path, text.c_str(), text.size()); }

namespace impl {
	template<typename T>
	static inline std::string& AppendPathSegment(std::string& path, T&& segment)
	{
		if (!path.empty() && *path.crbegin() != PATH_SEPARATOR)
			path += PATH_SEPARATOR;
		path += std::forward<T>(segment);
		return path;
	}
	static inline std::string& AppendPathSegment(std::string& path, EnsureExistsTag t)
	{
		if (t) EnsurePathExists(path);
		return path;
	}
}

/*
template<bool ensure_exists = false>
static inline std::string CombinePath(std::string path)
{
	return path;
}
template<bool ensure_exists = false, typename Next, typename... Rest>
static inline std::string CombinePath(std::string path, Next&& next, Rest&&... rest)
{
	impl::AppendPathSegment(path, std::forward<Next>(next));
	return CombinePath(std::move(path), std::forward<Rest>(rest)...);
}
*/


static inline std::string GetPath(std::string path)
{
	return path;
}
template<typename Next, typename... Rest>
static inline std::string GetPath(std::string path, Next&& next, Rest&&... rest)
{
	impl::AppendPathSegment(path, std::forward<Next>(next));
	return GetPath(std::move(path), std::forward<Rest>(rest)...);
}
template<typename... Args>
static inline std::string GetPath(PredefinedDirectory root, Args&&... components)
{
	return GetPath(GetPredefinedDirectory(root), std::forward<Args>(components)...);
}

template<typename Last>
static inline std::string GetFile(std::string path, Last&& last)
{
	impl::AppendPathSegment(path, std::forward<Last>(last));
	return path;
}
static inline std::string GetFile(std::string path, EnsureExistsTag t);
template<typename Last>
static inline std::string GetFile(std::string path, Last&& last, EnsureExistsTag t)
{
	return GetFile(std::move(path), t, std::forward<Last>(last));
}
template<typename Next, typename... Rest>
static inline std::string GetFile(std::string path, Next&& next, Rest&&... rest)
{
	impl::AppendPathSegment(path, std::forward<Next>(next));
	return GetFile(std::move(path), std::forward<Rest>(rest)...);
}
template<typename... Args>
static inline std::string GetFile(PredefinedDirectory root, Args&&... components)
{
	return GetFile(GetPredefinedDirectory(root), std::forward<Args>(components)...);
}
static inline std::string GetFile(PredefinedFile file, EnsureExistsTag t = DontEnsureExists)
{
	return GetPredefinedFile(file, t);
}
