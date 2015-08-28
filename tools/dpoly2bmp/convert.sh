#!/bin/sh

do_file () {
	fn=$1
	./dpoly2bmp $fn
	ffmpeg -framerate 10 -pattern_type glob -i '*.PNG' -c:v libx264 $fn.mp4
	rm -f *.BMP *.PNG *.RGBA
}

do_file CAILLOU-F.SET
do_file JUPITERStation1.set
do_file TAKEMecha1-F.set
