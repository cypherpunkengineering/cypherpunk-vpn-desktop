#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"
readonly SCRIPT_LOG_FILE="/usr/local/cypherpunk/var/log/leasewatcher.log"

# check if configuration is present
if ! scutil -w State:/Network/Cypherpunk -t 1 >/dev/null 2>&1; then
	# if not, don't do anything
	exit 0
fi

# get saved configuration
CYPHERPUNK_CONFIG="$( scutil <<-EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"

# parse variables from configuration
PROCESS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*PID :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"
PSSRC="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*SourceAddress :' | sed -e 's/^.*: //g')"
USE_CYPHERPUNK_DNS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*UseCypherpunkDNS :' | sed -e 's/^.*: //g')"

# get cypherpunk DNS "state"
DNS_GOOD="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/Cypherpunk/DNS
	quit
EOF
)"
# get pre-cypherpunk DNS "state"
DNS_OLD="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/Cypherpunk/OldDNS
	quit
EOF
)"
# get current DNS "state"
DNS_NOW="$(/usr/sbin/scutil <<-EOF
	open
	show State:/Network/Global/DNS
	quit
EOF
)"
# if there is no such key, normalize the value
if [ "${DNS_NOW}" = "${SCUTIL_NO_SUCH_KEY}" ] ; then
	DNS_NOW="${CP_NO_SUCH_KEY}"
fi

# print the 3 values to debug log
DNS_CHANGES_MSG="DNS configuration:
***** Pre-VPN:
	${DNS_OLD}
***** Post-VPN:
	${DNS_GOOD}
***** Currently:
	${DNS_NOW}
"
echo "${DNS_CHANGES_MSG}"

# check if the DNS configuration has changed
if [ "${DNS_GOOD}" != "${DNS_NOW}" ] ; then
	echo "NETWORK CHANGE DETECTED!"
else
	echo "OK"
fi
