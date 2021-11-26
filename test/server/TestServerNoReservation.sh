#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1

#int main() {} in base64 is aW50IG1haW4oKSB7fQ==
	FILE='{"name":"main.c","content":"aW50IG1haW4oKSB7fQ=="}'
	JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
	JSON=$(printf "${JSON_FMT}" "I_HAVE_NO_UUID" 'cc' "${FILE}" "$(pwd)" '["-c"]')
	RET=$(${SERVER_HELPER} "CompileRequest" "${JSON}")
	kill ${PID}
	if [[ "${RET}" == "FAILED_PRECONDITION" ]]; then
		return 0
	fi
	return 1
}
