#!/bin/bash

echo "Testing loop detection..."

cd ../build

echo "=== Testing simple_loops.c ==="
./statik ../tests/simple_loops.c

echo ""
echo "Expected: 5 loops total (2 for, 1 while, 1 do-while, 1 for in helper)"