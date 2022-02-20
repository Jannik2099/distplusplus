#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1

	JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
	JSON=$(printf "${JSON_FMT}" "I_HAVE_NO_UUID" 'cc' "${DUMMY_FILE}" "$(pwd)" '["-c"]')
	RET=$(${SERVER_HELPER} "CompileRequest" "${JSON}")
	kill ${PID}
	if [[ "${RET}" == "FAILED_PRECONDITION" ]]; then
		return 0
	fi
	return 1
}
