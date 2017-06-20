#!/bin/bash -e
# @param String message - The message to log
logMessage()
{
	echo "${@}"
}

trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/bin:/sbin:/usr/sbin:/usr/bin"

readonly OUR_NAME=$(basename "${0}")

logMessage "**********************************************"
logMessage "Start of output from ${OUR_NAME}"

# Quick check - is the configuration there?
if ! scutil -w State:/Network/Cypherpunk &>/dev/null -t 1 ; then
	# Configuration isn't there, so we forget it
	logMessage "WARNING: No saved Cypherpunk DNS configuration found; not doing anything."
    logMessage "End of output from ${OUR_NAME}"
    logMessage "**********************************************"
	exit 0
fi

# NOTE: This script does not use any arguments passed to it by Cypherpunk, so it doesn't shift Cypherpunk options out of the argument list

# Get info saved by the up script
CYPHERPUNK_CONFIG="$( scutil <<-EOF
	open
	show State:/Network/Cypherpunk
	quit
EOF
)"

ARG_MONITOR_NETWORK_CONFIGURATION="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*MonitorNetwork :' | sed -e 's/^.*: //g')"
LEASEWATCHER_PLIST_PATH="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*LeaseWatcherPlistPath :' | sed -e 's/^.*: //g')"
REMOVE_LEASEWATCHER_PLIST="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RemoveLeaseWatcherPlist :' | sed -e 's/^.*: //g')"
PSID="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*Service :' | sed -e 's/^.*: //g')"
# Don't need: SCRIPT_LOG_FILE="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*ScriptLogFile :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_DNS_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnDNSReset :' | sed -e 's/^.*: //g')"
# Don't need: ARG_RESTORE_ON_WINS_RESET="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RestoreOnWINSReset :' | sed -e 's/^.*: //g')"
# Don't need: PROCESS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*PID :' | sed -e 's/^.*: //g')"
# Don't need: ARG_IGNORE_OPTION_FLAGS="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*IgnoreOptionFlags :' | sed -e 's/^.*: //g')"
ARG_TAP="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*IsTapInterface :' | sed -e 's/^.*: //g')"

bRouteGatewayIsDhcp="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*RouteGatewayIsDhcp :' | sed -e 's/^.*: //g')"
sTunnelDevice="$(echo "${CYPHERPUNK_CONFIG}" | grep -i '^[[:space:]]*TunnelDevice :' | sed -e 's/^.*: //g')"

if ${ARG_TAP} ; then
	if [ "$bRouteGatewayIsDhcp" == "true" ]; then
        # Issue warning if the primary service ID has changed
        PSID_CURRENT="$( scutil <<-EOF |
            open
            show State:/Network/Cypherpunk
            quit
EOF
grep Service | sed -e 's/.*Service : //'
)"
        if [ "${PSID}" != "${PSID_CURRENT}" ] ; then
            logMessage "WARNING: Ignoring change of Network Primary Service from ${PSID} to ${PSID_CURRENT}"
        fi

        # Remove leasewatcher
        if ${ARG_MONITOR_NETWORK_CONFIGURATION} ; then
            launchctl unload "${LEASEWATCHER_PLIST_PATH}"
            if ${REMOVE_LEASEWATCHER_PLIST} ; then
                rm -f "${LEASEWATCHER_PLIST_PATH}"
            fi
            logMessage "Cancelled monitoring of system configuration changes"

            # Indicate leasewatcher has been removed
            scutil <<-EOF
            open
            get State:/Network/Cypherpunk
            d.remove MonitorNetwork
            d.add MonitorNetwork        "false"
            set State:/Network/Cypherpunk
            quit
EOF
        fi

        # Release the DHCP lease
        if [ -z "$dev" ]; then
            # If $dev is not defined, then use TunnelDevice, which was set from $dev by up.sh
            # ($dev is not defined when this script is called from MenuController to clean up when Cypherpunk has crashed)
            if [ -n "${sTunnelDevice}" ]; then
                logMessage "ERROR: \$dev not defined; using TunnelDevice: ${sTunnelDevice}"
                set +e
                ipconfig set "${sTunnelDevice}" NONE 2>/dev/null
                set -e
                logMessage "Released the DHCP lease via ipconfig set \"${sTunnelDevice}\" NONE."
            else
                logMessage "WARNING: Cannot release the DHCP lease without \$dev or State:/Network/Cypherpunk/TunnelDevice being defined. Device may not have disconnected properly."
            fi
        else
            set +e
            ipconfig set "$dev" NONE 2>/dev/null
            set -e
            logMessage "Released the DHCP lease via ipconfig set \"$dev\" NONE."
        fi

        # Indicate the DHCP lease has been released
        scutil <<-EOF
        open
        get State:/Network/Cypherpunk
        d.remove TapDeviceSetNone
        d.add TapDeviceHasBeenSetNone "true"
        set State:/Network/Cypherpunk
        quit
EOF
    else
        logMessage "NOTE: No action by ${OUR_NAME} is needed because this TAP connection does not use DHCP via the TAP device."
	fi
else
    logMessage "No action by ${OUR_NAME} is needed because this is not a TAP connection."
fi

logMessage "End of output from ${OUR_NAME}"
logMessage "**********************************************"

exit 0
