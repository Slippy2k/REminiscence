#!/usr/bin/env python

VERSION = '0.1.0'

SDL_TARBALL = [ 'fb-%s-gl-win32.zip',
	(
		'README.TXT',
		'../../../README-SDL',
		'../../SDL.dll',
		'../../zlib1.dll',
		'../../fb.dll',
		'../../fb.exe'
	)
]

DST_DIR = '.'

import os, zipfile

def build_zip_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	zf = zipfile.ZipFile(file_path, 'w', zipfile.ZIP_DEFLATED)
	for entry_path in file_list:
		entry_name = os.path.split(entry_path)[1]
		zf.write(entry_path, entry_name)
	zf.close()

tarball = SDL_TARBALL
file_name = tarball[0] % VERSION
file_list = tarball[1]
print "Generating '" + file_name + "'...",
if file_name.endswith('.zip'):
	build_zip_tarball(file_name, file_list)
else:
	raise "Unhandled extension for tarball"
print "Ok"

