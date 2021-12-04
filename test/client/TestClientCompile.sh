#!/usr/bin/env bash

function runTest() {
	${MINI_SERVER} &
	PID=$!
	sleep 1

	SRCFILE="${NAME}.c"
	echo 'int main() {}' > "${SRCFILE}"
	${CLIENT} "cc" "-c" "${SRCFILE}"
	RET=$?
	kill ${PID}
	if [[ "${RET}" == "0" ]]; then
		return 0
	fi
	return 1
}
