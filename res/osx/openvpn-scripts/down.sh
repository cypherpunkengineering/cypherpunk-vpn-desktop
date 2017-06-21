#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"

# Get info saved by the up script
CYPHERPUNK_CONFIG="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"
ARG_MONITOR_NETWORK_CONFIGURATION="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*MonitorNetwork :' | sed -e 's/^.*: //g')"
LEASEWATCHER_PLIST_PATH="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*LeaseWatcherPlistPath :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_DNS_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnDNSReset :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_SMB_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnSMBReset :' | sed -e 's/^.*: //g')"
# Don't need: PROCESS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*PID :' | sed -e 's/^.*: //g')"

# Issue warning if the primary service ID has changed
PSID_CURRENT="$( (scutil | grep Service | sed -e 's/.*Service : //')<<- EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"

# Remove leasewatcher
if ${ARG_MONITOR_NETWORK_CONFIGURATION} ; then
    launchctl unload "${LEASEWATCHER_PLIST_PATH}"
fi

# Restore configurations
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
CP_NO_SUCH_KEY="<dictionary> {
  CypherpunkNoSuchKey : true
}"

if [ "${DNS_OLD}" = "${CP_NO_SUCH_KEY}" ] ; then
    scutil <<- EOF
        open
        remove State:/Network/Service/${PSID}/DNS
        quit
EOF
else
    scutil <<- EOF
        open
        get State:/Network/Cypherpunk/OldDNS
        set State:/Network/Service/${PSID}/DNS
        quit
EOF
fi

if [ "${DNS_OLD_SETUP}" = "${CP_NO_SUCH_KEY}" ] ; then
	echo "DEBUG: Removing 'Setup:' DNS key"
	scutil <<-EOF
		open
		remove Setup:/Network/Service/${PSID}/DNS
		quit
EOF
else
	echo "DEBUG: Restoring 'Setup:' DNS key"
	scutil <<-EOF
		open
		get State:/Network/Cypherpunk/OldDNSSetup
		set Setup:/Network/Service/${PSID}/DNS
		quit
EOF
fi

if [ "${SMB_OLD}" = "${CP_NO_SUCH_KEY}" ] ; then
    scutil <<- EOF
        open
        remove State:/Network/Service/${PSID}/SMB
        quit
EOF
else
    scutil <<- EOF
        open
        get State:/Network/Cypherpunk/OldSMB
        set Setup:/Network/Service/${PSID}/SMB
        quit
EOF
fi

# Remove our system configuration data
scutil <<- EOF
	open
	remove State:/Network/Cypherpunk/SMB
	remove State:/Network/Cypherpunk/DNS
	remove State:/Network/Cypherpunk/OldSMB
	remove State:/Network/Cypherpunk/OldDNS
	remove State:/Network/Cypherpunk/OldDNSSetup
	remove State:/Network/Cypherpunk
	quit
EOF

exit 0
