#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
cd "$project_dir"
mkdir -p build/tests

passed=0
fail() {
  echo "FAIL: $*" >&2
  exit 1
}
pass() {
  passed=$((passed + 1))
  echo "ok $passed - $*"
}
expect_failure() {
  local name=$1
  shift
  if "$@" >build/tests/stdout 2>build/tests/stderr; then
    fail "$name unexpectedly succeeded"
  fi
  [[ -s build/tests/stderr ]] || fail "$name produced no diagnostic"
  pass "$name"
}

./scripts/compile.sh examples/add.wlp4 build/tests/add.asm >/dev/null
[[ -s build/tests/add.asm ]] || fail "successful compilation produced no assembly"
grep -q '^\.import print$' build/tests/add.asm || fail "assembly is missing runtime imports"
grep -q '^wain:$' build/tests/add.asm || fail "assembly is missing wain"
pass "source compiles through all four stages"

bin/wlp4scan < examples/add.wlp4 > build/tests/add.tokens
head -n 2 build/tests/add.tokens | grep -q $'^LONG long\nWAIN wain$' || fail "unexpected scanner output"
pass "scanner emits expected tokens"

bin/wlp4parse < build/tests/add.tokens > build/tests/add.tree
head -n 1 build/tests/add.tree | grep -q '^start BOF procedures EOF$' || fail "parser did not emit the start root"
grep -q '^main LONG WAIN' build/tests/add.tree || fail "parse tree is missing main"
pass "parser emits the expected full tree"

bin/wlp4type < build/tests/add.tree > build/tests/add.typed
grep -q '^ID sum : long$' build/tests/add.typed || fail "type annotation for sum is missing"
pass "type checker accepts the parser's start root and annotates identifiers"

expect_failure "lexical errors fail" ./scripts/compile.sh tests/cases/lexical-error.wlp4 build/tests/bad.asm
[[ ! -e build/tests/bad.asm ]] || fail "failed lexical compile left an output file"
expect_failure "syntax errors fail" ./scripts/compile.sh tests/cases/syntax-error.wlp4 build/tests/bad.asm
expect_failure "undeclared identifiers fail" ./scripts/compile.sh tests/cases/undeclared.wlp4 build/tests/bad.asm
expect_failure "incompatible types fail" ./scripts/compile.sh tests/cases/type-error.wlp4 build/tests/bad.asm

: > build/tests/empty
expect_failure "empty source fails without crashing" ./scripts/compile.sh build/tests/empty build/tests/bad.asm
printf 'start BOF procedures EOF\n' > build/tests/truncated.tree
expect_failure "truncated trees fail without crashing" bin/wlp4type < build/tests/truncated.tree

printf 'stale output\n' > build/tests/stale.asm
expect_failure "pipeline propagates failure" ./scripts/compile.sh tests/cases/type-error.wlp4 build/tests/stale.asm
[[ ! -e build/tests/stale.asm ]] || fail "failed pipeline left stale output"
pass "failed pipeline removes its requested output"

make -q bin/wlp4parse || fail "parser should initially be up to date"
# Some macOS filesystems and Make versions compare modification times at
# one-second resolution, so cross a timestamp boundary before touching.
sleep 1
touch src/wlp4data.h
if make -q bin/wlp4parse; then
  fail "header change did not invalidate parser"
fi
make bin/wlp4parse >/dev/null
pass "header dependency triggers parser rebuild"

make -j4 >/dev/null
make examples -j4 >/dev/null
pass "parallel build and example targets succeed"

echo "1..$passed"
