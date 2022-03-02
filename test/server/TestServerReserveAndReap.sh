#!/usr/bin/env bash

function runTest() {
    ${SERVER} &
    PID=$!
    sleep 1
    ${SERVER_HELPER} "Reservation" "{}"
    kill ${PID}
}
