#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1

	UUID=$(${SERVER_HELPER} "Reservation" "{}")
#int main() {} in base64 is aW50IG1haW4oKSB7fQ==
	FILE='{"name":"main.c","content":"aW50IG1haW4oKSB7fQ=="}'
	JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
	JSON=$(printf "${JSON_FMT}" "${UUID}" '/I/AM/A/BAD/COMPILER' "${FILE}" "$(pwd)" '["-c"]')
	RET=$(${SERVER_HELPER} "CompileRequest" "${JSON}")
	kill ${PID}
	if [[ "${RET}" == "INVALID_ARGUMENT" ]]; then
		return 0
	fi
	return 1
}
