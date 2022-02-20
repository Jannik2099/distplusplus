#!/usr/bin/env bash
# shellcheck disable=SC2034

# helper values
DUMMY_FILE='{"name":"main.c","compressionType":0,"content":"aW50IG1haW4oKSB7fQ=="}'
DUMMY_FILE_ZSTD='{"name":"main.c","compressionType":1,"content":"KLUv/QRIaQAAaW50IG1haW4oKSB7fS9XBfA="}'
DUMMY_FILE_BAD_COMPRESSION='{"name":"main.c","compressionType":1,"content":"aW50IG1haW4oKSB7fQ=="}'

TEST="${1}"
NAME="${2}"
CLIENT="env LLVM_PROFILE_FILE=${NAME}-client.profraw ${3}"
SERVER="env LLVM_PROFILE_FILE=${NAME}-server.profraw ${4}"
SERVER_HELPER="env LLVM_PROFILE_FILE=/dev/null ${5}"
MINI_SERVER="env LLVM_PROFILE_FILE=/dev/null ${6}"
shift 6

SOCK="${PWD}/${NAME}.sock"
export DISTPLUSPLUS_LISTEN_ADDRESS="unix:${SOCK}"
export DISTPLUSPLUS_LOG_LEVEL="trace"
export DISTPLUSPLUS_STATE_DIR="${NAME}-statedir"
export DISTPLUSPLUS_CONFIG_FILE="${NAME}-config.toml"

echo "servers = [ \"${DISTPLUSPLUS_LISTEN_ADDRESS}\" ]" > "${DISTPLUSPLUS_CONFIG_FILE}"

# shellcheck source=/dev/null
source "${TEST}"
set -x
runTest
ret=$?
set +x
rm -f "${SOCK}"
rm -f "${DISTPLUSPLUS_CONFIG_FILE}"
exit ${ret}
