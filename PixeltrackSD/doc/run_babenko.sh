#!/bin/bash

./release/pixeltrack -s -b 163,73,59,80 ~/data/tracking/baseline/selected/david_indoor.avi
mv output.xml out_david_indoor.xml
./release/pixeltrack -s -b 122,53,51,57 ~/data/tracking/baseline/selected/sylvester.avi
mv output.xml out_sylvester.xml
./release/pixeltrack -s -b 56,24,34,58 ~/data/tracking/baseline/selected/girl.avi
mv output.xml out_girl.xml
./release/pixeltrack -s -b 139,104,79,116 ~/data/tracking/baseline/selected/faceocc.avi
mv output.xml out_faceocc.xml
./release/pixeltrack -s -b 108,33,78,142 -f 4 ~/data/tracking/baseline/selected/faceocc2.avi
mv output.xml out_faceocc2.xml
./release/pixeltrack -s -b 153,79,18,39 ~/data/tracking/baseline/selected/coke11.avi
mv output.xml out_coke11.xml
./release/pixeltrack -s -b 108,33,78,142 ~/data/tracking/baseline/selected/tiger1.avi
mv output.xml out_tiger1.xml
./release/pixeltrack -s -b 21,43,28,27 ~/data/tracking/baseline/selected/tiger2.avi
mv output.xml out_tiger2.xml
