#!/usr/bin/env bash

function runTest() {
    (sleep 1; ${SERVER} --compress THISCOMPRESSIONDOESNOTEXIST) &
    PID=$!
    sleep 3
    kill ${PID}
    wait ${PID}
    RET=$?
    if [[ "${RET}" == "1" ]]; then
        return 0
    fi
    return 1
}
