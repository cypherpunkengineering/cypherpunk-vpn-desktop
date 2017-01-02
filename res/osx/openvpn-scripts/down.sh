#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"

# Do something only if the server pushed something
if [ "$foreign_option_1" == "" ]; then
	exit 0
fi

# Get info saved by the up script
CYPHERPUNK_CONFIG="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/OpenVPN
	quit
EOF)"
ARG_MONITOR_NETWORK_CONFIGURATION="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*MonitorNetwork :' | sed -e 's/^.*: //g')"
LEASEWATCHER_PLIST_PATH="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*LeaseWatcherPlistPath :' | sed -e 's/^.*: //g')"
REMOVE_LEASEWATCHER_PLIST="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RemoveLeaseWatcherPlist :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_DNS_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnDNSReset :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_WINS_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnWINSReset :' | sed -e 's/^.*: //g')"
# Don't need: PROCESS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*PID :' | sed -e 's/^.*: //g')"

# Issue warning if the primary service ID has changed
PSID_CURRENT="$( (scutil | grep Service | sed -e 's/.*Service : //')<<- EOF
	open
	show State:/Network/OpenVPN
	quit
EOF)"

# Remove leasewatcher
if ${ARG_MONITOR_NETWORK_CONFIGURATION} ; then
    launchctl unload "${LEASEWATCHER_PLIST_PATH}"
    if ${REMOVE_LEASEWATCHER_PLIST} ; then
        rm -f "${LEASEWATCHER_PLIST_PATH}"
    fi
fi

# Restore configurations
DNS_OLD="$(/usr/sbin/scutil <<-EOF
    open
    show State:/Network/OpenVPN/OldDNS
    quit
EOF)"
WINS_OLD="$(/usr/sbin/scutil <<-EOF
    open
    show State:/Network/OpenVPN/OldSMB
    quit
EOF)"
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
        get State:/Network/OpenVPN/OldDNS
        set Setup:/Network/Service/${PSID}/DNS
        quit
EOF
fi

if [ "${WINS_OLD}" = "${CP_NO_SUCH_KEY}" ] ; then
    scutil <<- EOF
        open
        remove State:/Network/Service/${PSID}/SMB
        quit
EOF
else
    scutil <<- EOF
        open
        get State:/Network/OpenVPN/OldSMB
        set Setup:/Network/Service/${PSID}/SMB
        quit
EOF
fi

# Remove our system configuration data
scutil <<- EOF
	open
	remove State:/Network/OpenVPN/SMB
	remove State:/Network/OpenVPN/DNS
	remove State:/Network/OpenVPN/OldSMB
	remove State:/Network/OpenVPN/OldDNS
	remove State:/Network/OpenVPN
	quit
EOF

exit 0
