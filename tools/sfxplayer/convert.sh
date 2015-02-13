set -x
for num in 68 70 72 73 74 75; do
	filename=sfx$num.raw
#	SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=$filename ./sfxply $num
	./sfxply $num
	mv out.raw $filename
	sox -r 11025 -e signed -b 8 -c 1 $filename sfx$num.wav
	oggenc sfx$num.wav
	rm -f $filename sfx$num.wav
done
