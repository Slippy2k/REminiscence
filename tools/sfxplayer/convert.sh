set -x
for num in 68 70 72 73 74 75 98 99; do
	filename=soundfx$num.raw
#	SDL_AUDIODRIVER=disk SDL_DISKAUDIOFILE=$filename ./sfxply $num
	./sfxply $num
	mv S16.raw $filename
	sox -r 22050 -e signed -b 16 -c 1 $filename soundfx$num.wav
	oggenc soundfx$num.wav
	rm -f $filename soundfx$num.wav
done

paulafreq=3546897
sox -r $(( $paulafreq /  538 )) -e signed -b 8 -c 1 sample0.raw sample0.wav
sox -r $(( $paulafreq / 1076 )) -e signed -b 8 -c 1 sample1.raw sample1.wav
sox -r $(( $paulafreq / 1076 )) -e signed -b 8 -c 1 sample2.raw sample2.wav
sox -r $(( $paulafreq /  538 )) -e signed -b 8 -c 1 sample3.raw sample3.wav
sox -r $(( $paulafreq /  135 )) -e signed -b 8 -c 1 sample4.raw sample4.wav
sox -r $(( $paulafreq / 1076 )) -e signed -b 8 -c 1 sample5.raw sample5.wav
sox -r $(( $paulafreq /  135 )) -e signed -b 8 -c 1 sample6.raw sample6.wav
sox -r $(( $paulafreq /  135 )) -e signed -b 8 -c 1 sample7.raw sample7.wav
sox -r $(( $paulafreq /  135 )) -e signed -b 8 -c 1 sample8.raw sample8.wav
