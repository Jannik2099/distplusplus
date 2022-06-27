#!/usr/bin/env bash

function runTest() {
    export DISTPLUSPLUS_COMPRESS="zstd"
    ${MINI_SERVER} &
    PID=$!
    sleep 1

    SRCFILE="${NAME}.c"
    echo 'int main() {}' > "${SRCFILE}"
    ${CLIENT} "cc" "-c" "${SRCFILE}"
    RET=$?
    kill ${PID}
    rm "${SRCFILE}"
    if [[ "${RET}" == "0" ]]; then
        return 0
    fi
    return 1
}
