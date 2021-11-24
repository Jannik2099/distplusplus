#!/usr/bin/env bash
# shellcheck disable=SC2034

TEST="${1}"
NAME="${2}"
CLIENT="env LLVM_PROFILE_FILE=${NAME}-client.profraw ${3}"
SERVER="env LLVM_PROFILE_FILE=${NAME}-server.profraw ${4}"
SERVER_HELPER="env LLVM_PROFILE_FILE=/dev/null ${5}"
shift 5

SOCK="${PWD}/${NAME}.sock"
export DISTPLUSPLUS_LISTEN_ADDRESS="unix:${SOCK}"
export DISTPLUSPLUS_LOG_LEVEL="trace"

# shellcheck source=/dev/null
source "${TEST}"
set -x
runTest
ret=$?
set +x
rm -f "${SOCK}"
exit ${ret}
