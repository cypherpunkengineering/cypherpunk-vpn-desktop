#pragma once

#include <string>

enum PredefinedDirectory
{
	BaseDir,		// Base directory of config files etc. (e.g. C:\Program Files\Cypherpunk VPN\ on Windows)
	LogDir,			// Directory where to write log file(s)
	OpenVPNDir,		// Directory where OpenVPN binary sits
	ProfileDir,		// Directory where temporary OpenVPN profile files are written
	ScriptsDir,		// Directory where OpenVPN hook scripts are
#ifdef WIN32
	TapDriverDir,
#endif
};
enum PredefinedFile
{
	DaemonExecutable,
	OpenVPNExecutable,
	OpenVPNOutLog,
	OpenVPNErrLog,
#ifdef WIN32
	TapInstallExecutable,
#endif
};

extern void InitPaths(std::string argv0);
extern std::string GetPath(PredefinedFile file);
extern std::string GetPath(PredefinedDirectory dir);

extern const char PATH_SEPARATOR;

template<typename First>
static inline First&& CombinePath(First&& first) { return std::forward<First>(first); }
template<typename First, typename Next, typename... Rest>
static inline std::string CombinePath(First&& first, Next&& next, Rest&&... rest)
{
	std::string result(std::forward<First>(first));
	if (!result.empty() && *result.crbegin() != PATH_SEPARATOR)
		result += PATH_SEPARATOR;
	result += std::forward<Next>(next);
	return CombinePath(result, std::forward<Rest>(rest)...);
}

template<typename... Args>
static inline std::string GetPath(PredefinedDirectory root, Args&&... components)
{
	std::string result = GetPath(root);
	return CombinePath(result, std::forward<Args>(components)...);
}
