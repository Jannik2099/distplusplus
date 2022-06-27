#!/usr/bin/env bash

function runTest() {
    ${SERVER} --compress zstd &
    PID=$!
    sleep 1

    UUID=$(${SERVER_HELPER} "Reservation" "{}")
    JSON_FMT='{"uuid":"%s","compiler":"%s","inputFile":%s,"cwd":"%s","argument":%s}'
    JSON=$(printf "${JSON_FMT}" "${UUID}" 'cc' "${DUMMY_FILE}" "$(pwd)" '["-c"]')
    ${SERVER_HELPER} "CompileRequest" "${JSON}"
    RET=$?
    if [[ "${RET}" != "0" ]]; then
        echo "SERVER_HELPER failed unexpectedly"
        return ${RET}
    fi
    
    (sleep 1; kill ${PID}) &
    wait ${PID}
    RET=$?
    if [[ "${RET}" == "128" ]]; then
        echo "SERVER died before being terminated"
        return ${RET}
    fi
    
    return 0
}
