#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"

flushDNSCache()
{
	set +e
	/usr/bin/dscacheutil -flushcache 2>/dev/null
	/usr/sbin/discoveryutil udnsflushcaches 2>/dev/null
	/usr/sbin/discoveryutil mdnsflushcache 2>/dev/null
	/usr/bin/killall -HUP mDNSResponder
	set -e
}

# @param String list - list of network service names, output from disable_ipv6()
# sets all network services in list to ipv6 mode = automatic
restore_ipv6() {
	[ "$1" = "" ] && return

	printf %s "$1
" | \
	while IFS= read -r ripv6_service ; do
		networksetup -setv6automatic "$ripv6_service"
		echo "Re-enabled IPv6 (automatic) for '$ripv6_service'"
	done
}

# check if configuration is present
if ! scutil -w State:/Network/Cypherpunk &>/dev/null -t 1 ; then
	# if not, don't do anything
	exit 0
fi

# get saved configuration
CYPHERPUNK_CONFIG="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"

# parse variables from configuration
LEASEWATCHER_PLIST_PATH="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*LeaseWatcherPlistPath :' | sed -e 's/^.*: //g')"
USE_CYPHERPUNK_DNS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*UseCypherpunkDNS :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"
# Note: '\n' was translated into '\t', so we translate it back (it was done because grep and sed only work with single lines)
sRestoreIpv6Services="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreIpv6Services :' | sed -e 's/^.*: //g' | tr '\t' '\n')"

# "grep" will return error status (1) if no matches are found, so don't fail if not found
set +e

# parse current PSID
PSID_CURRENT="$( scutil <<-EOF |
	open
	show State:/Network/Cypherpunk
	quit
EOF
grep 'Service : ' | sed -e 's/.*Service : //'
)"

# resume abort on error
set -e

# log a warning if the primary service ID has changed
if [ "${PSID}" != "${PSID_CURRENT}" ] ; then
	echo "down.sh: ignoring change of network primary service from ${PSID} to ${PSID_CURRENT}"
fi

# unload leasewatcher plist
launchctl unload "${LEASEWATCHER_PLIST_PATH}" || true

# don't modify the indentation below
CP_NO_SUCH_KEY="<dictionary> {
  CypherpunkNoSuchKey : true
}"

if [ "${USE_CYPHERPUNK_DNS}" = "true" ];then
	# get previously saved DNS and SMB configurations
	DNS_OLD="$(/usr/sbin/scutil <<-EOF
		open
		show State:/Network/Cypherpunk/OldDNS
		quit
	EOF
	)"
	SMB_OLD="$( scutil <<-EOF
		open
		show State:/Network/Cypherpunk/OldSMB
		quit
	EOF
	)"
	DNS_OLD_SETUP="$( scutil <<-EOF
		open
		show State:/Network/Cypherpunk/OldDNSSetup
		quit
	EOF
	)"

	# if DNS "state" was empty, remove empty tag
	if [ "${DNS_OLD}" = "${CP_NO_SUCH_KEY}" ] ; then
		scutil <<- EOF
			open
			remove State:/Network/Service/${PSID}/DNS
			quit
	EOF
	# otherwise, restore the old DNS "state"
	else
		scutil <<- EOF
			open
			get State:/Network/Cypherpunk/OldDNS
			set State:/Network/Service/${PSID}/DNS
			quit
	EOF
	fi

	# if DNS "setup" was empty, remove empty tag
	if [ "${DNS_OLD_SETUP}" = "${CP_NO_SUCH_KEY}" ] ; then
		echo "down.sh: Removing 'Setup:' DNS key"
		scutil <<-EOF
			open
			remove Setup:/Network/Service/${PSID}/DNS
			quit
	EOF

	# otherwise, restore the old DNS "setup"
	else
		echo "down.sh: Restoring 'Setup:' DNS key"
		scutil <<-EOF
			open
			get State:/Network/Cypherpunk/OldDNSSetup
			set Setup:/Network/Service/${PSID}/DNS
			quit
	EOF
	fi

	# if SMB "state" was empty, remove empty tag
	if [ "${SMB_OLD}" = "${CP_NO_SUCH_KEY}" ] ; then
		scutil <<- EOF
			open
			remove State:/Network/Service/${PSID}/SMB
			quit
	EOF
	# otherwise, restore the old SMB "state"
	else
		scutil <<- EOF
			open
			get State:/Network/Cypherpunk/OldSMB
			set State:/Network/Service/${PSID}/SMB
			quit
	EOF
	fi

	# flush DNS cache
	flushDNSCache
fi

# re-enable ipv6 on devices that we disabled it on
restore_ipv6 "$sRestoreIpv6Services"

# cleanup cypherpunk configuration values
scutil >/dev/null <<- EOF
	open
	remove State:/Network/Cypherpunk/SMB
	remove State:/Network/Cypherpunk/DNS
	remove State:/Network/Cypherpunk/OldSMB
	remove State:/Network/Cypherpunk/OldDNS
	remove State:/Network/Cypherpunk/OldDNSSetup
	remove State:/Network/Cypherpunk
	quit
EOF

# done
exit 0
