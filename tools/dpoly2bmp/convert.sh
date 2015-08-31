#!/bin/sh

do_file () {
	fn=$1
	./dpoly2bmp $fn
	rm -f $fn.mp4
	ffmpeg -framerate 10 -pattern_type glob -i '*.PNG' -c:v libx264 $fn.mp4
	rm -f *.BMP *.PNG *.RGBA
}

do_file CAILLOU-F.SET
do_file MEMOTECH.SET
do_file MEMOSTORY0.SET
do_file MEMOSTORY1.SET
do_file MEMOSTORY2.SET
do_file MEMOSTORY3.SET
do_file JUPITERStation1.set
do_file TAKEMecha1-F.set
