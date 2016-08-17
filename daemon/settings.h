#pragma once

struct Settings
{
	struct Connection
	{
		std::string protocol;
		std::string remoteIP;
		unsigned int remotePort;
		unsigned int mtu;
		std::string cipher;
		std::vector<std::string> certificateAuthority;
		std::vector<std::string> certificate;
		std::vector<std::string> privateKey;

		bool runOpenVPNAsRoot;
	} connection;
};
