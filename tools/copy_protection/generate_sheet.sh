set -x
python generate_sheet.py DOS
python generate_sheet.py SSI
python generate_sheet.py Amiga
for f in copy*png;
	do optipng -o9 -strip all $f
done
