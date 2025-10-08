#!/bin/sh

fail() {
    echo "$1" 1>&2
    exit 1
}

cd `dirname "$0"`
../lap6.py test.lap6 > output.oct
cmp test.oct output.oct || fail "Test failed."
echo "Test succeeded."
exit 0
