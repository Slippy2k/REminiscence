#set -x

DIR=~/Data/flashback/data_us/

./map2bmp $DIR/MENU1.MAP $DIR/MENU1.PAL
./map2bmp $DIR/MENU2.MAP $DIR/MENU2.PAL
./map2bmp $DIR/MENU3.MAP $DIR/MENU3.PAL
./map2bmp $DIR/INSTRU_F.MAP $DIR/INSTRU_F.PAL
./map2bmp $DIR/INSTRU_E.MAP $DIR/INSTRU_E.PAL

./map2bmp $DIR/LEVEL1.MAP $DIR/LEVEL1.PAL
./map2bmp $DIR/LEVEL2.MAP $DIR/LEVEL2.PAL
./map2bmp $DIR/LEVEL3.MAP $DIR/LEVEL3.PAL
./map2bmp $DIR/LEVEL4.MAP $DIR/LEVEL4.PAL
./map2bmp $DIR/LEVEL5.MAP $DIR/LEVEL5.PAL

mv $DIR/*bmp out/
for f in out/*bmp; do optipng $f; done
rm out/*bmp
