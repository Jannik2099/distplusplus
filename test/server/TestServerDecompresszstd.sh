#!/usr/bin/env bash

function runTest() {
    ${SERVER} &
    PID=$!
    sleep 1

    UUID=$(${SERVER_HELPER} "Reservation" "{}")
    JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
    JSON=$(printf "${JSON_FMT}" "${UUID}" 'cc' "${DUMMY_FILE_ZSTD}" "$(pwd)" '["-c"]')
    ${SERVER_HELPER} "CompileRequest" "${JSON}"
    ret=$?
    kill ${PID}

    return ${ret}
}
