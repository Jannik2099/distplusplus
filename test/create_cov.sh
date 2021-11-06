#!/usr/bin/env bash
if [ "$(basename "$(realpath .)")" != "build" ]; then
	echo "error: this script is meant to be run in the build dir"
	exit 1
fi

mapfile -t objs_pre < <(find . -path './test' -prune -o -path './protos' -prune -o -name '*.o' -print)
objs=()
for obj in "${objs_pre[@]}"; do
	objs+=("-object")
	objs+=("${obj}")
done
mapfile -t profraws < <(find . -name '*.profraw')

llvm-profdata merge -sparse "${profraws[@]}" -o coverage.profdata
llvm-cov report --ignore-filename-regex='(third_party\/.*|protos\/.*)' -instr-profile coverage.profdata "${objs[@]}" > coverage.txt
llvm-cov show --format=html --ignore-filename-regex='(third_party\/.*|protos\/.*)' -instr-profile coverage.profdata "${objs[@]}" > coverage.html
