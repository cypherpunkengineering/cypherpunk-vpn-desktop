#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"

CP_APP_PATH=`osascript -e 'POSIX path of (path to application id "com.cypherpunk.privacy.app")'`

CP_RESOURCES_PATH="${ARG_CP_PATH}/Contents/Resources"
LEASEWATCHER_PLIST_PATH="/Library/LaunchDaemons/com.cypherpunk.privacy.leasewatcher.plist"
REMOVE_LEASEWATCHER_PLIST="false"

ARG_MONITOR_NETWORK_CONFIGURATION="true"
ARG_RESTORE_ON_DNS_RESET="false"

OSVER="$(sw_vers | grep 'ProductVersion:' | grep -o '10\.[0-9]*')"

trim() {
	echo ${@}
}

case "${OSVER}" in
	10.4 | 10.5 )
		HIDE_SNOW_LEOPARD=""
		HIDE_LEOPARD="#"
		;;
	10.6 | 10.7 )
		HIDE_SNOW_LEOPARD="#"
		HIDE_LEOPARD=""
		;;
esac

nOptionIndex=1
nNameServerIndex=1
unset vForOptions
unset vDNS
unset vOptions

# what's the rush?
#sleep 2

while vForOptions=foreign_option_$nOptionIndex; [ -n "${!vForOptions}" ]; do
	{
	vOptions[nOptionIndex-1]=${!vForOptions}
	case ${vOptions[nOptionIndex-1]} in
		*DNS*)
			vDNS[nNameServerIndex-1]="$(trim "${vOptions[nOptionIndex-1]//dhcp-option DNS /}")"
			let nNameServerIndex++
			;;
	esac
	let nOptionIndex++
	}
done

PSID=$( (scutil | grep PrimaryService | sed -e 's/.*PrimaryService : //')<<- EOF
	open
	show State:/Network/Global/IPv4
	quit
EOF
)

STATIC_DNS_CONFIG="$( (scutil | sed -e 's/^[[:space:]]*[[:digit:]]* : //g' | tr '\n' ' ')<<- EOF
	open
	show Setup:/Network/Service/${PSID}/DNS
	quit
EOF
)"
if echo "${STATIC_DNS_CONFIG}" | grep -q "ServerAddresses" ; then
	readonly STATIC_DNS="$(trim "$( echo "${STATIC_DNS_CONFIG}" | sed -e 's/^.*ServerAddresses[^{]*{[[:space:]]*\([^}]*\)[[:space:]]*}.*$/\1/g' )")"
fi
if echo "${STATIC_DNS_CONFIG}" | grep -q "SearchDomains" ; then
	readonly STATIC_SEARCH="$(trim "$( echo "${STATIC_DNS_CONFIG}" | sed -e 's/^.*SearchDomains[^{]*{[[:space:]]*\([^}]*\)[[:space:]]*}.*$/\1/g' )")"
fi

if [ ${#vDNS[*]} -eq 0 ] ; then
	DYN_DNS="false"
	ALL_DNS="${STATIC_DNS}"
elif [ -n "${STATIC_DNS}" ] ; then
	case "${OSVER}" in
		10.6 | 10.7 )
			# Do nothing - in 10.6 we don't aggregate our configurations, apparently
			DYN_DNS="false"
			ALL_DNS="${STATIC_DNS}"
			;;
		10.4 | 10.5 )
			DYN_DNS="true"
			# We need to remove duplicate DNS entries, so that our reference list matches MacOSX's
			SDNS="$(echo "${STATIC_DNS}" | tr ' ' '\n')"
			(( i=0 ))
			for n in "${vDNS[@]}" ; do
				if echo "${SDNS}" | grep -q "${n}" ; then
					unset vDNS[${i}]
				fi
				(( i++ ))
			done
			if [ ${#vDNS[*]} -gt 0 ] ; then
				ALL_DNS="$(trim "${STATIC_DNS}" "${vDNS[*]}")"
			else
				DYN_DNS="false"
				ALL_DNS="${STATIC_DNS}"
			fi
			;;
	esac
else
	DYN_DNS="true"
	ALL_DNS="$(trim "${vDNS[*]}")"
fi
readonly DYN_DNS ALL_DNS

if ! ${DYN_DNS} ; then
	NO_DNS="#"
fi
if [ -z "${STATIC_WORKGROUP}" ] ; then
	NO_WG="#"
fi
if [ -z "${ALL_DNS}" ] ; then
	AGG_DNS="#"
fi
if [ -z "${ALL_SEARCH}" ] ; then
	AGG_SEARCH="#"
fi

scutil <<- EOF
	open
	d.init
	d.add PID # ${PPID}
	d.add Service ${PSID}
	d.add LeaseWatcherPlistPath "${LEASEWATCHER_PLIST_PATH}"
	d.add RemoveLeaseWatcherPlist "${REMOVE_LEASEWATCHER_PLIST}"
	d.add MonitorNetwork "${ARG_MONITOR_NETWORK_CONFIGURATION}"
	d.add RestoreOnDNSReset   "${ARG_RESTORE_ON_DNS_RESET}"
	set State:/Network/OpenVPN

	# First, back up the device's current DNS configurations
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
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set State:/Network/Service/${PSID}/DNS

	# set the Setup: as well
	d.init
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set Setup:/Network/Service/${PSID}/DNS

	# Now, initialize the maps that will be compared against the system-generated map
	# which means that we will have to aggregate configurations of statically-configured
	# nameservers
	d.init
	${AGG_DNS}d.add ServerAddresses * ${ALL_DNS}
	set State:/Network/OpenVPN/DNS

	# We're done
	quit
EOF

if ${ARG_MONITOR_NETWORK_CONFIGURATION} ; then
	launchctl load "${LEASEWATCHER_PLIST_PATH}"
fi

exit 0
