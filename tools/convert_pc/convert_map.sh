#set -x

DIR=~/Data/flashback/data_us/
OUT=out/

./map2bmp $DIR/MENU1.MAP $DIR/MENU1.PAL $OUT
./map2bmp $DIR/MENU2.MAP $DIR/MENU2.PAL $OUT
./map2bmp $DIR/MENU3.MAP $DIR/MENU3.PAL $OUT
./map2bmp $DIR/INSTRU_E.MAP $DIR/INSTRU_E.PAL $OUT

./map2bmp $DIR/LEVEL1.MAP $DIR/LEVEL1.PAL $OUT
./map2bmp $DIR/LEVEL2.MAP $DIR/LEVEL2.PAL $OUT
./map2bmp $DIR/LEVEL3.MAP $DIR/LEVEL3.PAL $OUT
./map2bmp $DIR/LEVEL4.MAP $DIR/LEVEL4.PAL $OUT
./map2bmp $DIR/LEVEL5.MAP $DIR/LEVEL5.PAL $OUT

for f in $OUT/*bmp; do optipng -quiet $f; done
rm $OUT/*bmp
