#!/bin/sh

do_file () {
	fn=$( basename $1 )
	./dpoly2bmp $1 > $fn.TXT
	rm -f $fn.mp4
	ffmpeg -framerate 10 -pattern_type glob -i '*.TGA' -c:v libx264 $fn.mp4
	convert -delay 10 -loop 0 -layers Optimize *TGA $fn.gif
	rm -f *.BMP *.TGA *.RGBA *.TXT
}

do_file CAILLOU-F.SET
do_file MEMOTECH.SET
do_file MEMOSTORY0.SET
do_file MEMOSTORY1.SET
do_file MEMOSTORY2.SET
do_file MEMOSTORY3.SET
do_file TAKEMecha1-F.set
do_file files/5_TAKEMA/TAKEMADO.SET
do_file files/6_JUPITE/JUPITER2.SET
do_file files/ALIMPONT.SET
do_file files/CARTE02.SET
do_file files/CARTEID_.SET
do_file files/CDFILES_.SET
do_file files/ESPIONSV.SET
do_file files/GENBRIDG.SET
do_file files/GENERATO.SET
do_file files/GIVE_ID_.SET
do_file files/HOLOCUBE.SET
do_file files/HOLOSEQ_.SET
do_file files/SERRUREI.SET
do_file files/TELETRAN.SET
