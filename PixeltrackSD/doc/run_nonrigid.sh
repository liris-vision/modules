#!/bin/bash

./release/pixeltrack -s -b 146,110,66,38 ~/data/tracking/baseline/selected/cliff-dive1.avi
mv output.xml out_cliff-dive1.xml
./release/pixeltrack -s -b 156,96,48,36 ~/data/tracking/baseline/selected/motocross1.avi
mv output.xml out_motocross1.xml
./release/pixeltrack -s -b 448,186,25,14 ~/data/tracking/baseline/selected/skiing.avi
mv output.xml out_skiing.xml
./release/pixeltrack -s -b 355,191,18,35 ~/data/tracking/baseline/selected/mountain-bike.avi
mv output.xml out_mountain-bike.xml
./release/pixeltrack -s -b 144,90,16,29 ~/data/tracking/baseline/selected/cliff-dive2.avi
mv output.xml out_cliff-dive2.xml
./release/pixeltrack -s -b 405,370,24,70 ~/data/tracking/baseline/selected/volleyball.avi
mv output.xml out_volleyball.xml
./release/pixeltrack -s -b 169,226,28,46 ~/data/tracking/baseline/selected/motocross2.avi
mv output.xml out_motocross2.xml
./release/pixeltrack -s -b 230,70,76,87 ~/data/tracking/baseline/selected/transformer.avi
mv output.xml out_transformer.xml
./release/pixeltrack -s -b 179,65,14,52 ~/data/tracking/baseline/selected/diving.avi
mv output.xml out_diving.xml
./release/pixeltrack -s -b 144,95,27,35 ~/data/tracking/baseline/selected/high_jump.avi
mv output.xml out_high_jump.xml
./release/pixeltrack -s -b 169,70,21,90 ~/data/tracking/baseline/selected/gymnastics.avi
mv output.xml out_gymnastics.xml

