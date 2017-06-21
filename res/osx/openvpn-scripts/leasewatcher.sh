#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"

# Quick check - is the configuration there?
if ! scutil -w State:/Network/Cypherpunk &>/dev/null -t 1 ; then
	# Configuration isn't there, so we forget it
	exit 0
fi

CYPHERPUNK_CONFIG="$( scutil <<-EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"

SCRIPT_LOG_FILE="/tmp/leasewatcher.log"
PROCESS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*PID :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"

# If we have a process, then we check the DNS and WINS status...
# ..._OLD is the pre-VPN value
# ..._NOW is the current value
# ..._GOOD is the expected (computed) post-VPN value

# wait for network to settle
sleep 0.5

if (( M=${PROCESS:-0} )) ; then
    # This is what scutil returns for a non-existant key
    SCUTIL_NO_SUCH_KEY="  No such key"
    # This is what up.sh stores into State:/Network/Cypherpunk/OldDNS and State:/Network/Cypherpunk/OldSMB for a non-existant key
    # DON'T CHANGE the indenting of the 2nd and 3rd lines; they are part of the string:
    CP_NO_SUCH_KEY="<dictionary> {
  CypherpunkNoSuchKey : true
}"
	# What's the correct DNS info?
	DNS_GOOD="$(/usr/sbin/scutil <<-EOF
		open
		show State:/Network/Cypherpunk/DNS
		quit
EOF
)"
    # What's the old DNS info?
    DNS_OLD="$(/usr/sbin/scutil <<-EOF
        open
        show State:/Network/Cypherpunk/OldDNS
        quit
EOF
)"
	# What's the current DNS info?
	DNS_NOW="$(/usr/sbin/scutil <<-EOF
		open
		show State:/Network/Global/DNS
		quit
EOF
)"
    # Make it match CP_NO_SUCH_KEY if there is no such key
	if [ "${DNS_NOW}" = "${SCUTIL_NO_SUCH_KEY}" ] ; then
        DNS_NOW="${CP_NO_SUCH_KEY}"
    fi
	# What's the correct WINS info?
    WINS_GOOD="$(/usr/sbin/scutil <<-EOF
		open
		show State:/Network/Cypherpunk/SMB
		quit
EOF
)"
    # What's the old WINS info?
    WINS_OLD="$(/usr/sbin/scutil <<-EOF
        open
        show State:/Network/Cypherpunk/OldSMB
        quit
EOF
)"
	# What's the current WINS info?
	WINS_NOW="$(/usr/sbin/scutil <<-EOF
		open
		show State:/Network/Global/SMB
		quit
EOF
)"
    # Make it match CP_NO_SUCH_KEY if there is no such key
	if [ "${WINS_NOW}" = "${SCUTIL_NO_SUCH_KEY}" ] ; then
        WINS_NOW="${CP_NO_SUCH_KEY}"
    fi

    # If the DNS configuration has changed
    # Then if it is the way it was pre-VPN
    #      Then if OK to do so, restore to the post-VPN configuration
    #      Otherwise restart the connection
    # If the WINS configuration has changed
    # Then if it is the way it was pre-VPN
    #      Then if OK to do so, restore to the post-VPN configuration
    #      Otherwise restart the connection
    NOTHING_DISPLAYED="true"
	if [ "${DNS_GOOD}" != "${DNS_NOW}" ] ; then
        NOTHING_DISPLAYED="false"
        echo "A network configuration change was detected" >> "${SCRIPT_LOG_FILE}"

        DNS_CHANGES_MSG="			DNS configuration has changed:
			--- BEGIN EXPECTED DNS CFG ---
			${DNS_GOOD}
			---- END EXPECTED DNS CFG ----
			--- BEGIN CURRENT DNS CFG ---
			${DNS_NOW}
			---- END CURRENT DNS CFG ----
			--- BEGIN PRE-VPN DNS CFG ---
			${DNS_OLD}
			---- END PRE-VPN DNS CFG ----"
        echo "${DNS_CHANGES_MSG}" >> "${SCRIPT_LOG_FILE}"
        if [ "${DNS_NOW}" = "${DNS_OLD}" ] ; then
            # DNS changed, but to the pre-VPN settings
            echo "Restoring the expected DNS settings." >> "${SCRIPT_LOG_FILE}"
            scutil <<-EOF
                open
                get State:/Network/Cypherpunk/DNS
                set State:/Network/Service/${PSID}/DNS
                quit
EOF
        else
            # DNS changed, but not to the pre-VPN settings
            echo "Sending USR1 to Cypherpunk (process ID ${PROCESS}) to restart the connection." >> "${SCRIPT_LOG_FILE}"
            # sleep so log message is displayed before we start getting log messages from Cypherpunk about the restart
            sleep 0.2
            kill -USR1 ${PROCESS}
            # We're done here, so no need to wait around.
            exit 0
        fi
	fi

    if ${NOTHING_DISPLAYED} ; then
        echo "A system configuration change was ignored because it was not relevant" >> "${SCRIPT_LOG_FILE}"
    fi
fi

exit 0
