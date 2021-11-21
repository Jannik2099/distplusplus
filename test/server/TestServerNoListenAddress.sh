#!/usr/bin/env bash

function runTest() {
	unset DISTPLUSPLUS_LISTEN_ADDRESS
	${SERVER}
	if [[ $? != 0 ]]; then
		return 0
	fi
	return 1
}
