#!/bin/bash -e

sleep 60 &
PID=$!

trap '{ echo "****** Success processing $1 script"; kill $PID; exit 0; }' SIGTERM
trap '{ echo "****** Failure processing $1 script"; kill $PID; exit 1; }' SIGINT

echo "****** DAEMON $$ $@"
wait

echo "****** Timeout waiting for $1 script"
exit 2
