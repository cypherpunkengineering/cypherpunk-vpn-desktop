#!/bin/bash -e

STUNNEL_PROFILE=$1
shift

echo "[STUNNEL] Starting with profile: $STUNNEL_PROFILE" 1>&2
"${BASH_SOURCE%/*}/macos/stunnel" "$STUNNEL_PROFILE" < /dev/null 2>&1 | sed 's/.*/[STUNNEL] &/' 1>&2 &
PID=$!
echo "[STUNNEL] Started with PID: $PID" 1>&2

sleep 2

function finish {
  echo "[STUNNEL] Issuing kill signal to PID $PID" 1>&2
  kill "$PID" || true
}
#trap 'exit' INT TERM
trap finish EXIT

"${BASH_SOURCE%/*}/../openvpn_osx/openvpn" "$@"
