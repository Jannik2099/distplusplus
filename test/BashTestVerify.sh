#!/usr/bin/env bash

TEST="${1}"

source "${TEST}"

declare -F runTest || exit 1
