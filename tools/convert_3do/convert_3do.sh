set -x

datadir=$1

convert_anim() {
	fn=$( basename $1 )
	./convert_3do -anim $1
	ffmpeg -framerate 10 -pattern_type glob -i 'anim_*.tga' -c:v libx264 $fn.mp4
}

convert_cel() {
	./convert_3do -cel $1
}

convert_pal() {
	./convert_3do -pal $1
}

convert_text() {
	./convert_3do $1
}

# Cels/

# Cpak/

# Global/

convert_cel $datadir/Global/FLASHLOGO.cel
convert_cel $datadir/Global/Conrad.coded8
convert_cel $datadir/Global/Interplay.Coded6
convert_cel $datadir/Global/New3doLogo.Coded6

convert_anim $datadir/Global/Conrad.coded8
convert_anim $datadir/Global/Conrad.smlanim.Uncoded16

# Langs/

convert_text $datadir/Langs/JAPANESE.BIN

# Levels/

convert_pal $datadir/Levels/LEVEL1.PAL
convert_pal $datadir/Levels/LEVEL2.PAL
convert_pal $datadir/Levels/DT.PAL
convert_pal $datadir/Levels/LEVEL3.PAL
convert_pal $datadir/Levels/LEVEL4.PAL

# tunes/

#for fn in $datadir/tunes/*.Cpc; do
#done
