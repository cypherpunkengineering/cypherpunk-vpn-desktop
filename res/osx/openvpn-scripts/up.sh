#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"

CP_APP_PATH=`osascript -e 'POSIX path of (path to application id "com.cypherpunk.privacy.app")'`

CP_RESOURCES_PATH="${ARG_CP_PATH}/Contents/Resources"
LEASEWATCHER_PLIST_PATH="/Library/LaunchDaemons/com.cypherpunk.privacy.leasewatcher.plist"
#LEASEWATCHER_TEMPLATE_PATH="${CP_RESOURCES_PATH}/LeaseWatch3.plist"
REMOVE_LEASEWATCHER_PLIST="false"

nOptionIndex=1
nNameServerIndex=1
nWINSServerIndex=1
unset vForOptions
unset vDNS
unset vWINS
unset vOptions

scutil <<- EOF
	open
	d.init
	d.add PID # ${PPID}
	d.add Service ${PSID}
    d.add LeaseWatcherPlistPath "${LEASEWATCHER_PLIST_PATH}"
    d.add RemoveLeaseWatcherPlist "${REMOVE_LEASEWATCHER_PLIST}"
    d.add MonitorNetwork "${ARG_MONITOR_NETWORK_CONFIGURATION}"
    d.add RestoreOnDNSReset   "${ARG_RESTORE_ON_DNS_RESET}"
    d.add RestoreOnWINSReset  "${ARG_RESTORE_ON_WINS_RESET}"
	set State:/Network/OpenVPN

	# First, back up the device's current DNS and WINS configurations
    # Indicate 'no such key' by a dictionary with a single entry: "CypherpunkNoSuchKey : true"
    d.init
    d.add CypherpunkNoSuchKey true
    get State:/Network/Service/${PSID}/DNS
	set State:/Network/OpenVPN/OldDNS

    d.init
    d.add CypherpunkNoSuchKey true
    get State:/Network/Service/${PSID}/SMB
	set State:/Network/OpenVPN/OldSMB

	# Second, initialize the new DNS map
	d.init
	${HIDE_SNOW_LEOPARD}d.add DomainName ${domain}
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	${NO_SEARCH}d.add SearchDomains * ${SEARCH_DOMAIN}
	${HIDE_LEOPARD}d.add DomainName ${domain}
	set State:/Network/Service/${PSID}/DNS

	# Third, initialize the WINS map
	d.init
	${HIDE_SNOW_LEOPARD}${NO_WG}d.add Workgroup ${STATIC_WORKGROUP}
	${NO_WINS}d.add WINSAddresses * ${vWINS[*]}
	${HIDE_LEOPARD}${NO_WG}d.add Workgroup ${STATIC_WORKGROUP}
	set State:/Network/Service/${PSID}/SMB

	# Now, initialize the maps that will be compared against the system-generated map
	# which means that we will have to aggregate configurations of statically-configured
	# nameservers, and statically-configured search domains
	d.init
	${HIDE_SNOW_LEOPARD}d.add DomainName ${domain}
	${AGG_DNS}d.add ServerAddresses * ${ALL_DNS}
	${AGG_SEARCH}d.add SearchDomains * ${ALL_SEARCH}
	${HIDE_LEOPARD}d.add DomainName ${domain}
	set State:/Network/OpenVPN/DNS

	d.init
	${HIDE_SNOW_LEOPARD}${NO_WG}d.add Workgroup ${STATIC_WORKGROUP}
	${AGG_WINS}d.add WINSAddresses * ${ALL_WINS}
	${HIDE_LEOPARD}${NO_WG}d.add Workgroup ${STATIC_WORKGROUP}
	set State:/Network/OpenVPN/SMB

	# We're done
	quit
EOF

if ${ARG_MONITOR_NETWORK_CONFIGURATION} ; then
    if [ "${LEASEWATCHER_TEMPLATE_PATH}" != "" ] ; then
        sed -e "s|/Applications/Cypherpunk\ Privacy.app/Contents/Resources|${CP_RESOURCES_PATH}|g" "${LEASEWATCHER_TEMPLATE_PATH}" > "${LEASEWATCHER_PLIST_PATH}"
    fi
    launchctl load "${LEASEWATCHER_PLIST_PATH}"
fi

exit 0
