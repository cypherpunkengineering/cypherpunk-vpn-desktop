#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"

LEASEWATCHER_PLIST_PATH="/Library/LaunchDaemons/com.cypherpunk.privacy.leasewatcher.plist"

trim() {
	echo ${@}
}

# don't touch any DNS settings if no DNS servers are provided from OpenVPN
if [ "$foreign_option_1" == "" ]; then
	exit 0
fi

# wait for network to settle
sleep 0.5

# parse DNS servers from OpenVPN's foreign_option_* variables
nOptionIndex=1
nNameServerIndex=1
unset vForOptions
unset vDNS
unset vOptions
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

# get PSID from scutil output
PSID=$( (scutil | grep PrimaryService | sed -e 's/.*PrimaryService : //')<<- EOF
	open
	show State:/Network/Global/IPv4
	quit
EOF
)

# get PS source address from scutil output
PSSRC="$( (scutil | grep -A 1 Addresses | tail -1 | awk '{print $3}' )<<- EOF
	open
	show State:/Network/Service/${PSID}/IPv4
	quit
EOF
)"

# get DNS from scutil output
STATIC_DNS_CONFIG="$( (scutil | sed -e 's/^[[:space:]]*[[:digit:]]* : //g' | tr '\n' ' ')<<- EOF
	open
	show Setup:/Network/Service/${PSID}/DNS
	quit
EOF
)"

# check if static DNS servers are configured
if echo "${STATIC_DNS_CONFIG}" | grep -q "ServerAddresses" ; then
	readonly STATIC_DNS="$(trim "$( echo "${STATIC_DNS_CONFIG}" | sed -e 's/^.*ServerAddresses[^{]*{[[:space:]]*\([^}]*\)[[:space:]]*}.*$/\1/g' )")"
fi

# check if static DNS search domains are configured
if echo "${STATIC_DNS_CONFIG}" | grep -q "SearchDomains" ; then
	readonly STATIC_SEARCH="$(trim "$( echo "${STATIC_DNS_CONFIG}" | sed -e 's/^.*SearchDomains[^{]*{[[:space:]]*\([^}]*\)[[:space:]]*}.*$/\1/g' )")"
fi

# evaluate to use static DNS or dynamic DNS
if [ ${#vDNS[*]} -eq 0 ] ; then
	DYN_DNS="false"
	ALL_DNS="${STATIC_DNS}"
elif [ -n "${STATIC_DNS}" ] ; then
	DYN_DNS="false"
	ALL_DNS="${STATIC_DNS}"
else
	DYN_DNS="true"
	ALL_DNS="$(trim "${vDNS[*]}")"
fi
readonly DYN_DNS ALL_DNS

# comment out lines below depending on above logic
if ! ${DYN_DNS} ; then
	NO_DNS="#"
fi
if [ -z "${ALL_DNS}" ] ; then
	AGG_DNS="#"
fi

# first scutil configuration
scutil <<- EOF
	open

##### store cypherpunk variables for other scripts (leasewatch, down, etc.) to use

	d.init
	# the '#' in the next line is not a comment; it indicates to scutil that a number follows it
	d.add PID # ${PPID}
	d.add Service ${PSID}
	d.add SourceAddress ${PSSRC}
	d.add LeaseWatcherPlistPath "${LEASEWATCHER_PLIST_PATH}"
	set State:/Network/Cypherpunk

##### backup the current DNS configurations

	# Indicate 'no such key' by a dictionary with a single entry: "CypherpunkNoSuchKey : true"
	# If there isn't a key, "CypherpunkNoSuchKey : true" won't be removed.
	# If there is a key, "CypherpunkNoSuchKey : true" will be removed and the key's contents will be used

	# backup DNS "state"
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Service/${PSID}/DNS
	set State:/Network/Cypherpunk/OldDNS

	# backup DNS "setup"
	d.init
	d.add CypherpunkNoSuchKey true
	get Setup:/Network/Service/${PSID}/DNS
	set State:/Network/Cypherpunk/OldDNSSetup

	# backup SMB "state"
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Service/${PSID}/SMB
	set State:/Network/Cypherpunk/OldSMB

##### set the cypherpunk VPN provided DNS configuration

	# set DNS "state"
	d.init
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set State:/Network/Service/${PSID}/DNS

	# set DNS "setup"
	d.init
	${NO_DNS}d.add ServerAddresses * ${vDNS[*]}
	set Setup:/Network/Service/${PSID}/DNS

	# done
	quit
EOF

# wait for settings to propagate to "Global"
sleep 0.25

# second scutil configuration
scutil <<- EOF
	open

##### store values for leasewatcher/down to compare against
##### remember to aggregate configurations of statically-configured nameservers later

	# set the DNS map
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Global/DNS
	set State:/Network/Cypherpunk/DNS

	# set the SMB map
	d.init
	d.add CypherpunkNoSuchKey true
	get State:/Network/Global/SMB
	set State:/Network/Cypherpunk/SMB

	# We're done
	quit
EOF

# load leasewatcher plist
launchctl load "${LEASEWATCHER_PLIST_PATH}"

exit 0
