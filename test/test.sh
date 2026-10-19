#!/bin/sh

set -e

fail() {
    echo "$1" 1>&2
    exit 1
}

lap6() {
    command="$1"
    shift
    ../lap6 "$command" test.linc "$@" 2>/dev/null
}

cd `dirname "$0"`

#Test the assembler.
../lap6.py test.lap6 > output.oct
cmp test.oct output.oct || fail "Test failed."
echo "Assembler test succeeded."

#Create tape image.
lap6 mk
lap6 mx

#Save a manuscript.
echo "XYZ" | lap6 sm FOO
lap6 dx | grep "^FOO  * M " >/dev/null || fail "Test failed."
lap6 lo FOO && fail "Test failed."
lap6 am FOO | grep "^XYZ" >/dev/null || fail "Test failed."
lap6 rb 430 1 | od | head -1 |
  grep " 002065 \+005712 \+005354 \+005512 \+0077" >/dev/null ||
    fail "Test failed."
lap6 dm FOO
lap6 dx | grep "^FOO" >/dev/null && fail "Test failed."

#Bad manuscript input.
for i in '%' '/' '&' '(' ')' '~' '{' '}'; do
    echo "$i" | lap6 sm FOO && fail "Bad data $i not detected."
    lap6 dx | grep "^FOO" >/dev/null && fail "Test failed."
done

#Save a binary.
printf "\001\002\003\004" | lap6 sb BAR
lap6 dx | grep "^BAR  * B " >/dev/null || fail "Test failed."
lap6 lo BAR | od | grep " 001001 \+002003 " >/dev/null || fail "Test failed."

#Bad binary input.
printf "\000" | lap6 sb FOO && fail "Bad data not detected."
lap6 dx | grep "^FOO" >/dev/null && fail "Test failed."
printf "\000\020" | lap6 sb FOO && fail "Bad data not detected."
lap6 dx | grep "^FOO" >/dev/null && fail "Test failed."

echo "Tape filing test succeeded."

exit 0
