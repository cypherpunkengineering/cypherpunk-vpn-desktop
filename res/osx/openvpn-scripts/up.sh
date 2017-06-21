#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"

LEASEWATCHER_PLIST_PATH="/Library/LaunchDaemons/com.cypherpunk.privacy.leasewatcher.plist"

ARG_RESTORE_ON_DNS_RESET="false"

OSVER="$(sw_vers | grep 'ProductVersion:' | grep -o '10\.[0-9]*')"

trim() {
	echo ${@}
}

nOptionIndex=1
nNameServerIndex=1
unset vForOptions
unset vDNS
unset vOptions

# wait for network to settle
sleep 2

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
if [ -z "${ALL_DNS}" ] ; then
	AGG_DNS="#"
fi

scutil <<- EOF
	open

	# Store our variables for the other scripts (leasewatch, down, etc.) to use
	d.init
	# The '#' in the next line does NOT start a comment; it indicates to scutil that a number follows it (as opposed to a string or an array)
	d.add PID # ${PPID}
	d.add Service ${PSID}
	d.add LeaseWatcherPlistPath "${LEASEWATCHER_PLIST_PATH}"
	d.add RestoreOnDNSReset   "${ARG_RESTORE_ON_DNS_RESET}"
	set State:/Network/Cypherpunk

	# First, back up the device's current DNS configurations
	# Indicate 'no such key' by a dictionary with a single entry: "CypherpunkNoSuchKey : true"
	# If there isn't a key, "CypherpunkNoSuchKey : true" won't be removed.
	# If there is a key, "CypherpunkNoSuchKey : true" will be removed and the key's contents will be used
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Service/${PSID}/DNS
	set State:/Network/Cypherpunk/OldDNS

	d.init
	d.add CypherpunkNoSuchKey true
	get Setup:/Network/Service/${PSID}/DNS
	set State:/Network/Cypherpunk/OldDNSSetup

	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Service/${PSID}/SMB
	set State:/Network/Cypherpunk/OldSMB

	# Second, initialize the new DNS map
	d.init
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set State:/Network/Service/${PSID}/DNS

	# set the Setup: as well
	d.init
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set Setup:/Network/Service/${PSID}/DNS

	# We're done
	quit
EOF

# wait for settings to propagate
sleep 1

scutil <<- EOF
	open

	# Now, initialize the maps that will be compared against the system-generated map
	# which means that we will have to aggregate configurations of statically-configured
	# nameservers
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Global/DNS
	set State:/Network/Cypherpunk/DNS

	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Global/SMB
	set State:/Network/Cypherpunk/SMB

	# We're done
	quit
EOF

launchctl load "${LEASEWATCHER_PLIST_PATH}"

exit 0
