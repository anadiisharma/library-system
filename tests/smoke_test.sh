#!/bin/sh
# Smoke test: compile-and-run check used by the Jenkins "Build and Test" stage.
# Starts the server on a spare port using a throw-away copy of the data, then checks the API.
set -e
BIN="$1"; [ -n "$BIN" ] || BIN=./library
WORK=$(mktemp -d)
cp "$BIN" "$WORK/library"; cp books.csv "$WORK/"; cp -r web "$WORK/web"
cd "$WORK"
PORT=18080 APP_VERSION=test ./library >server.log 2>&1 &
PID=$!
trap 'kill $PID 2>/dev/null; rm -rf "$WORK"' EXIT
for i in 1 2 3 4 5 6 7 8 9 10; do
  curl -fs http://127.0.0.1:18080/healthz >/dev/null 2>&1 && break
  sleep 1
done
fail() { echo "TEST FAILED: $1"; cat server.log; exit 1; }

curl -fs http://127.0.0.1:18080/healthz | grep -q ok            || fail "health check"
BOOKS=$(curl -fs http://127.0.0.1:18080/api/books)
echo "$BOOKS" | grep -q '"title":"Steve Jobs"'                  || fail "catalog did not load books.csv"
echo "$BOOKS" | grep -q 'Guns,Germs,and Steel'                  || fail "quoted CSV title not parsed"
curl -fs -X POST "http://127.0.0.1:18080/api/issue?bid=12"  | grep -q '"ok":true'    || fail "issue available book"
curl -fs -X POST "http://127.0.0.1:18080/api/issue?bid=12"  | grep -q '"ok":false'   || fail "double issue must be refused"
curl -fs -X POST "http://127.0.0.1:18080/api/return?bid=12" | grep -q '"fine":0'     || fail "on-time return must have no fine"
curl -fs http://127.0.0.1:18080/ | grep -q "Central Library"    || fail "web page not served"
echo "ALL TESTS PASSED"
