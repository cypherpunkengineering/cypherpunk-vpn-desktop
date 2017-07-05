#!/bin/bash -e
trap "" TSTP
trap "" HUP
trap "" INT
export PATH="/usr/local/sbin:/usr/local/bin:/sbin:/bin:/usr/sbin:/usr/bin"

LEASEWATCHER_PLIST_PATH="/Library/LaunchDaemons/com.cypherpunk.privacy.leasewatcher.plist"

trim() {
	echo ${@}
}

flushDNSCache()
{
	set +e
	/usr/bin/dscacheutil -flushcache 2>/dev/null
	/usr/sbin/discoveryutil udnsflushcaches 2>/dev/null
	/usr/sbin/discoveryutil mdnsflushcache 2>/dev/null
	set -e
}

disable_ipv6() {
# for each enabled network service that has ipv6 = automatic, set to ipv6 = off
# outputs one network service per line, for use by down.sh

	# Get list of services and remove the first line which contains a heading
	dipv6_services="$( networksetup  -listallnetworkservices | sed -e '1,1d')"

	# Go through the list disabling IPv6 for enabled services, and outputting lines with the names of the services
	printf %s "$dipv6_services
" | \
	while IFS= read -r dipv6_service ; do

		# If first character of a line is an asterisk, the service is disabled, so we skip it
		if [ "${dipv6_service:0:1}" != "*" ] ; then
			dipv6_ipv6_status="$( networksetup -getinfo "$dipv6_service" | grep 'IPv6: ' | sed -e 's/IPv6: //')"
			if [ "$dipv6_ipv6_status" = "Automatic" ] ; then
				networksetup -setv6off "$dipv6_service"
				echo "$dipv6_service"
			fi
		fi

	done
}

# check if DNS servers were provided via OpenVPN variables
if [ "$foreign_option_1" == "" ]; then
	USE_CYPHERPUNK_DNS="false"
else
	USE_CYPHERPUNK_DNS="true"

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
fi

ipv6_disabled_services=""
ipv6_disabled_services="$( disable_ipv6 )"
if [ "$ipv6_disabled_services" != "" ] ; then
	printf %s "$ipv6_disabled_services
" | \
	while IFS= read -r dipv6_service ; do
		echo "Disabled IPv6 for '$dipv6_service'"
	done
fi
readonly ipv6_disabled_services
# Note '\n' is translated into '\t' so it is all on one line, because grep and sed only work with single lines
readonly ipv6_disabled_services_encoded="$( echo "$ipv6_disabled_services" | tr '\n' '\t' )"

# wait for network to settle
sleep 0.2

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

if [ "${USE_CYPHERPUNK_DNS}" = "true" ];then
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
fi

# save previous DNS values
SCUTIL_NO_SUCH_KEY="  No such key"
DNS_BEFORE="$( scutil <<-EOF
	open
	show State:/Network/Global/DNS
	quit
EOF
)"

# first scutil configuration
scutil <<- EOF
	open

##### store cypherpunk variables for other scripts (leasewatch, down, etc.) to use

	d.init
	# the '#' in the next line is not a comment; it indicates to scutil that a number follows it
	d.add PID # ${PPID}
	d.add Service ${PSID}
	d.add SourceAddress ${PSSRC}
	d.add UseCypherpunkDNS ${USE_CYPHERPUNK_DNS}
	d.add LeaseWatcherPlistPath "${LEASEWATCHER_PLIST_PATH}"
	d.add RestoreIpv6Services "$ipv6_disabled_services_encoded"
	set State:/Network/Cypherpunk

	# done
	quit
EOF

if [ "${USE_CYPHERPUNK_DNS}" = "true" ];then
	SCUTIL_NO_SUCH_KEY="  No such key"
	DNS_NOW="$( scutil <<-EOF
		open
		show State:/Network/Global/DNS
		quit
	EOF
	)"

	# second scutil configuration
	scutil >/dev/null <<- EOF
		open

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
	ATTEMPTS_MAX=20
	ATTEMPTS=0
	while true;do
		# bail out after X attempts
		ATTEMPTS=$(($ATTEMPTS + 1))
		if [ "${ATTEMPTS}" -gt "${ATTEMPTS_MAX}" ];then
			echo "Max attempts reached!"
			exit 1
		fi

		# get current DNS settings
		DNS_AFTER="$( scutil <<-EOF
			open
			show State:/Network/Global/DNS
			quit
		EOF
		)"

		# check if propagated
		if [ "${DNS_BEFORE}" != "${DNS_AFTER}" ];then
			echo "DNS configuration propagated successfully after ${ATTEMPTS} attempts."
			break
		fi

		# not propagated yet, wait a while and try again
		sleep 0.1
	done

	# flush DNS cache so it uses new settings
	flushDNSCache

	# third scutil configuration
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
fi

# finally, load leasewatcher plist
if [[ $0 == /usr/local/cypherpunk/* ]] ;
then
	launchctl load "${LEASEWATCHER_PLIST_PATH}"
fi

exit 0
