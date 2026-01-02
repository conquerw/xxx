#!/bin/bash

if [ $# -ne 1 ]; then
	echo "please input one para"
	exit 1
else
	echo "the first para: $1"
fi

sudo rm -fr perf.data

PROCESS="event_send"

PID=$(ps -ef | grep "$PROCESS" | grep -v grep | awk '{print $2}')

sudo perf record -F 99 --call-graph fp -p "$PID"

sudo perf script -i perf.data > $1.perf

../../FlameGraph/stackcollapse-perf.pl $1.perf > $1.floded

../../FlameGraph/flamegraph.pl $1.floded > $1.svg
