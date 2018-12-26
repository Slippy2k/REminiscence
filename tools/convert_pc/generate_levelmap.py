
import os
import sys
from PIL import Image

W = 256
H = 224

h = 0
w = 0
y = 0

count = 0
f = file(sys.argv[1])
name = sys.argv[2]
for line in f.readlines():
	if count == 0:
		assert line.startswith('levelMap')
		h,w = line[9:].split()
		h = int(h)
		w = int(w)
		im = Image.new("RGB", (W * w, H * h))
	else:
		for x in range(0, w):
			num = line[x * 4 + 1:x * 4 + 3]
			if num != '  ':
				fname = 'out/%s-%02d.png' % (name, int(num, 16))
				if os.path.exists(fname):
					im.paste(Image.open(fname), (x * W, y))
		y += H
	count += 1

fname = os.path.splitext(sys.argv[1])[0] + '.png'
im.save(fname)
