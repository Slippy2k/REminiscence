DATADIR=~/Data/flashback/data_fr/

do_levelmap () {
	name=$1
	./convert_data --levelmap $DATADIR/$name.CT > $name.txt
	python generate_levelmap.py $name.txt $name
}

do_levelmap LEVEL1
do_levelmap LEVEL2
do_levelmap LEVEL3
do_levelmap LEVEL4
do_levelmap LEVEL5
