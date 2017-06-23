#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"
readonly SCRIPT_LOG_FILE="/usr/local/cypherpunk/var/log/leasewatcher.log"

# check if configuration is present
if ! scutil -w State:/Network/Cypherpunk &>/dev/null -t 1 ; then
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

# if we have a process, get the three DNS configuration values:
# OLD is the pre-VPN value
# NOW is the current value
# GOOD is the expected (computed) post-VPN value

# wait for network to settle
sleep 0.2

# log date of notification
date >> "${SCRIPT_LOG_FILE}"

# start by assuming ths notification was not relevant
CHANGE_NOT_RELEVANT="true"

# check if we have a process
if (( M=${PROCESS:-0} )) ; then
    # scutil return value for a non-existant key
    SCUTIL_NO_SUCH_KEY="  No such key"

    # This is what up.sh stores for a non-existant key
    # DON'T CHANGE the indenting of the 2nd and 3rd lines; they are part of the string:
    CP_NO_SUCH_KEY="<dictionary> {
  CypherpunkNoSuchKey : true
}"

	# get PS network source address from scutil output
	PSSRC_NOW="$( (scutil | grep -A 1 Addresses | tail -1 | awk '{print $3}' )<<- EOF
	open
	show State:/Network/Service/${PSID}/IPv4
	quit
EOF
)"

    # check if the PSSRC has changed
	if [ "${PSSRC}" != "${PSSRC_NOW}" ] ; then
        CHANGE_NOT_RELEVANT="false"
        echo "The primary network source address change was detected:
***** Pre-VPN:
-${PSSRC}-
***** Currently:
-${PSSRC_NOW}-" >> "${SCRIPT_LOG_FILE}"
        # kill openvpn and exit
        echo "Sending SIGTERM to cypherpunk-privacy-openvpn PID ${PROCESS} to restart the connection" >> "${SCRIPT_LOG_FILE}"
        kill ${PROCESS}
        exit 0
	fi

	if [ "${USE_CYPHERPUNK_DNS}" = "true" ];then
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

	    # check if the DNS configuration has changed
		if [ "${DNS_GOOD}" != "${DNS_NOW}" ] ; then
	        CHANGE_NOT_RELEVANT="false"
	        echo "A network configuration change was detected" >> "${SCRIPT_LOG_FILE}"

			# print the 3 values to debug log
	        DNS_CHANGES_MSG="DNS configuration has changed:
	***** Pre-VPN:
				${DNS_OLD}
	***** Post-VPN:
				${DNS_GOOD}
	***** Currently:
				${DNS_NOW}
	"
	        echo "${DNS_CHANGES_MSG}" >> "${SCRIPT_LOG_FILE}"

			# if DNS changed to the pre-VPN value, restore to the post-VPN value
	        if [ "${DNS_NOW}" = "${DNS_OLD}" ] ; then
	            # it changed to the pre-VPN value
	            echo "Restoring DNS settings to the post-VPN value" >> "${SCRIPT_LOG_FILE}"
	            scutil <<-EOF
	                open
	                get State:/Network/Cypherpunk/DNS
	                set State:/Network/Service/${PSID}/DNS
	                quit
	EOF
			# if DNS changed to something other than the pre-VPN value, network must have changed
	        else
	            # kill openvpn and exit
	            echo "Sending SIGTERM to cypherpunk-privacy-openvpn PID ${PROCESS} to restart the connection" >> "${SCRIPT_LOG_FILE}"
	            kill ${PROCESS}
	            exit 0
	        fi
		fi
	fi

	# apparently the changes weren't relevant; just log and exit
    if ${CHANGE_NOT_RELEVANT} ; then
        echo "A system configuration change was ignored because it was not relevant" >> "${SCRIPT_LOG_FILE}"
    fi
fi

# done
exit 0
