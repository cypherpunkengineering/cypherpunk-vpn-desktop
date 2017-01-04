#include "config.h"
#include "logger.h"
#include "posix.h"
#include <cstdio>
#include <cstdlib>
#include <string>


static int logged_system(const std::string& cmd)
{
	LOG(VERBOSE) << "Executing shell command: " << cmd;
	return system(cmd.c_str());
}

void firewall_install()
{
#ifdef OS_OSX
	if (0 != logged_system("grep -F com.cypherpunk.privacy /etc/pf.conf"))
	{
		LOG(INFO) << "Installing PF anchors";
		// Need to install anchors
		FILE* f = fopen("/etc/pf.conf", "a");
		fputs("\n"
			"anchor \"com.cypherpunk.privacy/*\"\n"
			"load anchor \"com.cypherpunk.privacy\" from \"/usr/local/cypherpunk/etc/pf.anchors/com.cypherpunk.privacy\"\n"
			, f);
		fclose(f);
		// Reload root pf.conf file
		LOG(INFO) << "Reloading PF root configuration";
		logged_system("pfctl -q -f /etc/pf.conf");
	}
#elif OS_LINUX
#endif
}

void firewall_uninstall()
{
#ifdef OS_OSX
	if (0 == logged_system("grep -F com.cypherpunk.privacy /etc/pf.conf"))
	{
		// Strip out added lines and reload
		LOG(INFO) << "Uninstalling PF anchors";
		logged_system("sed -i '' '/com\\.cypherpunk\\.privacy/d' /etc/pf.conf && pfctl -q -f /etc/pf.conf");
	}
#elif OS_LINUX
#endif
}

/*
std::string firewall_enable()
{
#ifdef OS_OSX
	std::string token;
	bool successful;
	FILE* p = POSIX_CHECK_IF_NULL(popen("pfctl -a com.cypherpunk.privacy -E 2>&1", "r"));
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;
	while ((len = getline(&line, &cap, p)) > 0)
	{
		if (0 == strncmp('Token : ', line, 8))
		{
			const char* nl = strchr(line, '\n');
			if (nl) token = std::string(line + 8, nl);
			else    token = std::string(line + 8);
		}
		else if (0 == strncmp('pf enabled', line, 10))
		{
			successful = true;
		}
	}
	free(line);
	pclose(p); // ignore exit code, we inspect only stderr
	if (!token.empty() && successful)
		return token;
	THROW_POSIXEXCEPTION(EPERM, pfctl);
#elif OS_LINUX
#endif
}

void firewall_disable(const std::string& token)
{
#ifdef OS_OSX
	bool successful = false;
	FILE* p = POSIX_CHECK_IF_NULL(popen(("pfctl -a com.cypherpunk.privacy -X " + token + " 2>&1").c_str(), "r"));
	char* line = NULL;
	size_t cap = 0;
	ssize_t len;
	while ((len = getline(&line, &cap, p)) > 0)
	{
		if (0 == strncmp('pf disabled', line, 11) ||
		    0 == strncmp('disable request successful', line, 26))
		{
			successful = true;
		}
	}
	free(line);
	pclose(p);
	if (!successful)
		THROW_POSIXEXCEPTION(EINVAL, pfctl);
#elif OS_LINUX
#endif
}
*/

void firewall_ensure_enabled()
{
#ifdef OS_OSX
	logged_system("test -f /usr/local/cypherpunk/etc/pf.anchors/pf.token && pfctl -q -s References | grep -qFf /usr/local/cypherpunk/etc/pf.anchors/pf.token || pfctl -a com.cypherpunk.privacy -E 2>&1 | grep -F 'Token : ' | cut -c9- > /usr/local/cypherpunk/etc/pf.anchors/pf.token");
#elif OS_LINUX
#endif
}

void firewall_ensure_disabled()
{
#ifdef OS_OSX
	logged_system("test -f /usr/local/cypherpunk/etc/pf.anchors/pf.token && pfctl -q -a com.cypherpunk.privacy -X `cat /usr/local/cypherpunk/etc/pf.anchors/pf.token` && rm /usr/local/cypherpunk/etc/pf.anchors/pf.token");
#elif OS_LINUX
#endif
}

void firewall_enable_anchor(const std::string& anchor)
{
#ifdef OS_OSX
	logged_system("pfctl -q -a com.cypherpunk.privacy/" + anchor + " -f /usr/local/cypherpunk/etc/pf.anchors/com.cypherpunk.privacy." + anchor);
#elif OS_LINUX
#endif
}

void firewall_disable_anchor(const std::string& anchor)
{
#ifdef OS_OSX
	logged_system("pfctl -q -a com.cypherpunk.privacy/" + anchor + " -F rules");
#elif OS_LINUX
#endif
}

bool firewall_anchor_enabled(const std::string& anchor)
{
#ifdef OS_OSX
	return 0 == logged_system("pfctl -q -a com.cypherpunk.privacy/" + anchor + " -s rules 2>/dev/null | grep -q .");
#elif OS_LINUX
#endif
}

void firewall_set_anchor_enabled(const std::string& anchor, bool enable)
{
#ifdef OS_OSX
	bool currently_enabled = firewall_anchor_enabled(anchor);
	if (!enable != !currently_enabled)
	{
		if (enable)
			firewall_enable_anchor(anchor);
		else
			firewall_disable_anchor(anchor);
	}
#elif OS_LINUX
#endif
}
