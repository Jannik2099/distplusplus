#!/usr/bin/env bash

function runTest() {
	${SERVER} &
	PID=$!
	sleep 1

	export DISTPLUSPLUS_LISTEN_ADDRESS="unix:/home/jannik/Dokumente/projects/distplusplus/build/test.sock"
	UUID=$(${SERVER_HELPER} "Reservation" "{}")
#int main() {} in base64 is aW50IG1haW4oKSB7fQ==
	FILE='{"name":"main.c","content":"aW50IG1haW4oKSB7fQ=="}'
	JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
	JSON=$(printf "${JSON_FMT}" "${UUID}" 'cc' "${FILE}" "$(pwd)" '["-c"]')
	${SERVER_HELPER} "CompileRequest" "${JSON}"
	ret=$?
	kill ${PID}

	return ${ret}
}
