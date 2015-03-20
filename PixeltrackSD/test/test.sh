#!/bin/bash

# $1: test name
# $2: test command
function doTest()
{
	echo "-------------------------------"
	echo "Running test '$1' ..."

	# clean previous results
	rm -f output.txt output.xml

	# run test command
	eval "$2"

	# compare test result with reference
	colordiff -u output.txt "$1_output.txt.ref" & colordiff -u output.xml "$1_output.xml.ref"
	if [ "$?" = "0" ]
	then
		result="passed"
	else
		result="FAILED"
	fi

	msg="* Test '$1' $result."
	echo "$msg"
	logs="${logs}${msg}\n"
}

# run tests
doTest "video" "../build/release/pixeltrack -s -f 10 -t 100 -b 5,80,10,20 ./video.avi"
doTest "cliff-dive1" "../build/release/pixeltrack -s -b 146,110,66,38 ./cliff-dive1.avi"
doTest "diving" "../build/release/pixeltrack -s -b 179,65,14,52 diving.avi"
doTest "faceocc2" "../build/release/pixeltrack -s -b 108,33,78,142 -f 4 faceocc2.avi"
doTest "girl" "../build/release/pixeltrack -s -b 56,24,34,58 girl.avi"
doTest "mountain-bike" "../build/release/pixeltrack -s -b 355,191,18,35 mountain-bike.avi"

# display results summary
echo
echo "******************************"
echo All tests summary:
echo -e "$logs"
echo Failed tests summary:
echo -e "$logs" | grep FAILED
echo
