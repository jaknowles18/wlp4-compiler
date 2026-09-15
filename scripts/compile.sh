#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 2 ]]; then
  echo "usage: $0 INPUT.wlp4 OUTPUT.asm" >&2
  exit 2
fi

project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
input=$1
output=$2

if [[ ! -f "$input" ]]; then
  echo "compile: input file not found: $input" >&2
  exit 2
fi

mkdir -p "$(dirname "$output")" "$project_dir/build"
rm -f "$output"
work_dir=$(mktemp -d "$project_dir/build/.compile.XXXXXX")
trap 'rm -rf "$work_dir"' EXIT

"$project_dir/bin/wlp4scan" < "$input" > "$work_dir/tokens"
"$project_dir/bin/wlp4parse" < "$work_dir/tokens" > "$work_dir/tree"
"$project_dir/bin/wlp4type" < "$work_dir/tree" > "$work_dir/typed-tree"
"$project_dir/bin/wlp4gen" < "$work_dir/typed-tree" > "$work_dir/output.asm"

mv "$work_dir/output.asm" "$output"
echo "compiled $input -> $output"
