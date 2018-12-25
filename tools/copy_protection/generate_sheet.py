
import os
import sys
from PIL import Image

CHAR_W = 8
CHAR_H = 8

SHAPES_COUNT = 30
CODES_COUNT = 5

WORDS_COUNT = 40

SHEET_W = (16 + 128 + 16 + CHAR_W * 16 + 16)
SHEET_H = SHAPES_COUNT * (16 + 128 + 16)

class Font(object):
	def __init__(self, fname):
		self.data = file(fname).read()
	def drawChar(self, c, img, x, y):
		glyph = (ord(c) - 32) * CHAR_W * CHAR_H / 2
		for j in range(0, CHAR_H):
			for i in range(0, CHAR_W, 2):
				color = ord(self.data[glyph])
				glyph += 1
				if (color >> 4) == 15:
					img.putpixel((x + i, y + j), (255, 255, 255))
				if (color & 15) == 15:
					img.putpixel((x + i + 1, y + j), (255, 255, 255))
	def drawString(self, s, img, x, y):
		for i, c in enumerate(s):
			self.drawChar(c, img, x, y)
			x += CHAR_W

class Codes(object):
	def __init__(self, fname):
		count = 0
		self.shapes = []
		for line in file(fname).readlines():
			if line.startswith('Shape'):
				assert (count % 6) == 0
				self.shapes.append([])
			else:
				line = line.strip()
				# assert line.startswith('code')
				self.shapes[len(self.shapes) - 1].append(line)
			count += 1
	def getCode(self, shape, code):
		return self.shapes[shape][code]

type = sys.argv[1]

fnt = Font('FB_TXT.FNT')

if type == 'DOS' or type == 'Amiga':

	img = Image.new('RGB', [SHEET_W, SHEET_H], 0x000000)
	fnt = Font('FB_TXT.FNT')

	codes = Codes('%s_shapes.txt' % type.lower())

	y = 16
	for shape in range(SHAPES_COUNT):
		shape_img = Image.open(os.path.join(type, 'shape_%02d.bmp' % shape))
		# crop (64, 48, 128, 128);
		img.paste(shape_img, (16 - 64, y - 48))
		ymargin = (128 - CODES_COUNT * (4 + CHAR_H)) / 2
		for code in range(CODES_COUNT):
			s = codes.getCode(shape, code)
			xs = 16 + 128 + 16
			ys = y + ymargin + code * (4 + CHAR_H)
			fnt.drawString(s, img, xs, ys)
		y += 16 + 128 + 16

	img.save('copy_protection_%s.png' % type.lower())

elif type == 'SSI':

	words = []
	for line in file('dos_ssi.txt').readlines():
		if line.startswith('page'):
			words.append(line.strip())
	assert(len(words) == WORDS_COUNT)

	img = Image.new('RGB', [256 + 128, 224 + WORDS_COUNT * (4 + CHAR_H)], 0x000000)
	screenshot_img = Image.open('fbe_000.png')
	img.paste(screenshot_img, (64, 0))
	y = 224
	for word in words:
		xs = 0
		ys = y
		fnt.drawString(word, img, 0, y)
		y += 4 + CHAR_H

	img.save('copy_protection_ssi.png')
