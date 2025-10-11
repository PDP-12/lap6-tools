#!/bin/sh

fail() {
    echo "$1" 1>&2
    exit 1
}

lap6() {
    ../lap6 "$1" test.linc "$2" 2>/dev/null
}

cd `dirname "$0"`
../lap6.py test.lap6 > output.oct
cmp test.oct output.oct || fail "Test failed."
echo "Assembler test succeeded."

lap6 mk
lap6 mx
echo "XYZ" | lap6 sm FOO
lap6 dx | grep "FOO  * M " >/dev/null || fail "Test failed."
lap6 lo FOO && fail "Test failed."
lap6 am FOO | grep "XYZ" >/dev/null || fail "Test failed."
lap6 dm FOO
lap6 dx | grep "FOO" >/dev/null && fail "Test failed."
printf "\001\002\003\004" | lap6 sb BAR
lap6 dx | grep "BAR  * B " >/dev/null || fail "Test failed."
lap6 lo BAR | od | grep " 001001 002003 " >/dev/null || fail "Test failed."
echo "Tape filing test succeeded."

exit 0
