# Copyright 2017 NXP
gnuplot ./demo.pg > c.png
convert.im6 white.png result_so_far.ppm -gravity SouthEast -composite result_so_far2.ppm
convert.im6 c.png result_so_far2.ppm +append result.png
convert.im6 -type truecolor -format bmp -define bmp:format=bmp3 -transpose -rotate 270 result.png result.bmp
./fb_chess 0 24 if=result.bmp

