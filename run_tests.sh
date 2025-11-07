#!/usr/bin/env bash
set -e
echo "Compiling cms.c ..."
gcc -std=c99 -O2 cms.c -o cms

pass=0; fail=0
run_case () {
  name="$1"; infile="$2"
  echo ""; echo "=== $name ==="
  ./cms < "$infile" > "tests/$name.out" 2>&1 || true
  out="tests/$name.out"
  ok=0
  case "$name" in
    basic)
      grep -qi "AUTOSAVE is ON" "$out" && \
      grep -qi "successfully inserted" "$out" && \
      grep -qi "Search results" "$out" && \
      grep -qi "successfully updated" "$out" && \
      grep -qi "SUMMARY" "$out" && ok=1
      ;;
    delete_undo)
      grep -qi "successfully deleted" "$out" && \
      grep -qi "UNDO successful" "$out" && \
      grep -qi "record with ID=999001 is found" "$out" && ok=1
      ;;
    sort)
      grep -qi "Here are all the records" "$out" && ok=1
      ;;
    find)
      grep -qi "Search results" "$out" && ok=1
      ;;
  esac
  if [ $ok -eq 1 ]; then echo "[PASS] $name"; pass=$((pass+1)); else echo "[FAIL] $name"; fail=$((fail+1)); fi
}
run_case basic tests/basic.in
run_case delete_undo tests/delete_undo.in
run_case sort tests/sort.in
run_case find tests/find.in
echo ""; echo "Passed: $pass  Failed: $fail"
test $fail -eq 0
