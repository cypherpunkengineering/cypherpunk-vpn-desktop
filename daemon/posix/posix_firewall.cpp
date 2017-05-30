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
	if (0 != logged_system("grep -qF com.cypherpunk.privacy /etc/pf.conf"))
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
	// install exemptions first

	// ipv4 exemptions
	logged_system("iptables --new-chain cypherpunk.100.exemptLAN");
	logged_system("iptables -F cypherpunk.100.exemptLAN");
	logged_system("iptables -A cypherpunk.100.exemptLAN -d 10.0.0.0/8 -j ACCEPT");
	logged_system("iptables -A cypherpunk.100.exemptLAN -d 172.16.0.0/12 -j ACCEPT");
	logged_system("iptables -A cypherpunk.100.exemptLAN -d 192.168.0.0/16 -j ACCEPT");
	logged_system("iptables -A cypherpunk.100.exemptLAN -d 224.0.0.0/4 -j ACCEPT");
	logged_system("iptables -A cypherpunk.100.exemptLAN -d 255.255.255.255/32 -j ACCEPT");

	// ipv6 exemptions
	logged_system("ip6tables --new-chain cypherpunk.100.exemptLAN");
	logged_system("ip6tables -F cypherpunk.100.exemptLAN");
	logged_system("ip6tables -A cypherpunk.100.exemptLAN -d fc00::/7 -j ACCEPT");
	logged_system("ip6tables -A cypherpunk.100.exemptLAN -d fe80::/10 -j ACCEPT");
	logged_system("ip6tables -A cypherpunk.100.exemptLAN -d ff00::/8 -j ACCEPT");

	// install killswitch last

	// ipv4 killswitch
	logged_system("iptables --new-chain cypherpunk.500.killswitch");
	logged_system("iptables -F cypherpunk.500.killswitch");
	logged_system("iptables -A cypherpunk.500.killswitch -o lo+ -j ACCEPT");
	logged_system("iptables -A cypherpunk.500.killswitch -m owner --gid-owner cypherpunk -j ACCEPT");
	logged_system("iptables -A cypherpunk.500.killswitch -p udp --sport 68 --dport 67 -j ACCEPT");
	logged_system("iptables -A cypherpunk.500.killswitch ! -o tun+ -j DROP");

	// ipv6 killswitch
	logged_system("ip6tables --new-chain cypherpunk.500.killswitch");
	logged_system("ip6tables -F cypherpunk.500.killswitch");
	logged_system("ip6tables -A cypherpunk.500.killswitch -o lo+ -j ACCEPT");
	logged_system("ip6tables -A cypherpunk.500.killswitch -m owner --gid-owner cypherpunk -j ACCEPT");
	logged_system("ip6tables -A cypherpunk.500.killswitch -p udp --sport 546 --dport 547 -j ACCEPT");
	logged_system("ip6tables -A cypherpunk.500.killswitch ! -o tun+ -j DROP");
#endif
}

void firewall_uninstall()
{
#ifdef OS_OSX
	if (0 == logged_system("grep -qF com.cypherpunk.privacy /etc/pf.conf"))
	{
		// Strip out added lines and reload
		LOG(INFO) << "Uninstalling PF anchors";
		// FIXME: doesnt seem to work
		logged_system("sed -i '' '/com\\.cypherpunk\\.privacy/d' /etc/pf.conf && pfctl -q -f /etc/pf.conf");
	}
#elif OS_LINUX
	logged_system("iptables --delete-chain cypherpunk.100.exemptLAN");
	logged_system("ip6tables --delete-chain cypherpunk.100.exemptLAN");
	logged_system("iptables --delete-chain cypherpunk.500.killswitch");
	logged_system("ip6tables --delete-chain cypherpunk.500.killswitch");
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

void firewall_log_status()
{
	// FIXME: This doesn't help; it doesn't actually print the stdout, it merely executes the call. Replace with something that actually works if needed.
#ifdef OS_OSX
	logged_system("pfctl -sr");
	logged_system("pfctl -sr -a com.cypherpunk.privacy");
	logged_system("pfctl -sr -a com.cypherpunk.privacy/100.killswitch");
	logged_system("pfctl -sr -a com.cypherpunk.privacy/200.exemptLAN");
#elif OS_LINUX
#endif
}

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
	logged_system("iptables -A OUTPUT -j cypherpunk." + anchor);
	logged_system("ip6tables -A OUTPUT -j cypherpunk." + anchor);
#endif
}

void firewall_disable_anchor(const std::string& anchor)
{
#ifdef OS_OSX
	logged_system("pfctl -q -a com.cypherpunk.privacy/" + anchor + " -F rules");
#elif OS_LINUX
	logged_system("iptables -D OUTPUT -j cypherpunk." + anchor);
	logged_system("ip6tables -D OUTPUT -j cypherpunk." + anchor);
#endif
}

bool firewall_anchor_enabled(const std::string& anchor)
{
#ifdef OS_OSX
	return 0 == logged_system("pfctl -q -a com.cypherpunk.privacy/" + anchor + " -s rules 2>/dev/null | grep -q .");
#elif OS_LINUX
	return (
		0 == logged_system("iptables -L OUTPUT 2>/dev/null | grep -q cypherpunk." + anchor) &&
		0 == logged_system("ip6tables -L OUTPUT 2>/dev/null | grep -q cypherpunk." + anchor)
	);
#endif
}

void firewall_set_anchor_enabled(const std::string& anchor, bool enable)
{
	firewall_log_status();
	bool currently_enabled = firewall_anchor_enabled(anchor);
	if (!enable != !currently_enabled)
	{
		if (enable)
			firewall_enable_anchor(anchor);
		else
			firewall_disable_anchor(anchor);
	}
	firewall_log_status();
}
