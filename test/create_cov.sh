#!/usr/bin/env bash
if [ "$(basename "$(realpath .)")" != "build" ]; then
    echo "error: this script is meant to be run in the build dir"
    exit 1
fi

LLVM_PROFDATA=${LLVM_PROFDATA:="llvm-profdata"}
LLVM_COV=${LLVM_COV:="llvm-cov"}

mapfile -t objs_pre < <(find . -path './test' -prune -o -path './protos' -prune -o -name '*.o' -print)
objs=()
for obj in "${objs_pre[@]}"; do
    objs+=("-object")
    objs+=("${obj}")
done
mapfile -t profraws < <(find . -name '*.profraw')

${LLVM_PROFDATA} merge -sparse "${profraws[@]}" -o coverage.profdata
${LLVM_COV} report --ignore-filename-regex='(third_party\/.*|protos\/.*)' -instr-profile coverage.profdata "${objs[@]}" > coverage.txt
${LLVM_COV} show --format=html --ignore-filename-regex='(third_party\/.*|protos\/.*)' -instr-profile coverage.profdata "${objs[@]}" > coverage.html
${LLVM_COV} export -format lcov --ignore-filename-regex='(third_party\/.*|protos\/.*)' -instr-profile coverage.profdata "${objs[@]}" > coverage-lcov.txt
