#!/bin/bash
TEST_DIR=~/src/mbasic/basic/dev/tests_with_results
PASS=0
FAIL=0

for bas in "$TEST_DIR"/*.bas; do
    name=$(basename "$bas" .bas)
    expected="$TEST_DIR/${name}.txt"
    if [ -f "$expected" ]; then
        actual=$(./mbasic "$bas" 2>&1 | sed 's/[[:space:]]*$//')
        expected_content=$(cat "$expected" | sed 's/[[:space:]]*$//')
        if [ "$actual" = "$expected_content" ]; then
            echo "PASS: $name"
            PASS=$((PASS + 1))
        else
            echo "FAIL: $name"
            FAIL=$((FAIL + 1))
        fi
    fi
done

echo "---"
echo "Passed: $PASS"
echo "Failed: $FAIL"
