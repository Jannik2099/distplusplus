#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1
	kill ${PID}
}
