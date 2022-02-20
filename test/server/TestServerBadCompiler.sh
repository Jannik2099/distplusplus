#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1

	UUID=$(${SERVER_HELPER} "Reservation" "{}")
	JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
	JSON=$(printf "${JSON_FMT}" "${UUID}" '/I/AM/A/BAD/COMPILER' "${DUMMY_FILE}" "$(pwd)" '["-c"]')
	RET=$(${SERVER_HELPER} "CompileRequest" "${JSON}")
	kill ${PID}
	if [[ "${RET}" == "INVALID_ARGUMENT" ]]; then
		return 0
	fi
	return 1
}
