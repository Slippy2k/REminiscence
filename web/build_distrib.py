#!/usr/bin/env python

VERSION = '0.3.0'

SDL_TARBALL = [ 'REminiscence-%s-sdl-win32.zip',
	(
		'../README',
		'../rs.exe'
	)
]

SRC_TARBALL = [ 'REminiscence-%s.tar.bz2',
	(
		'../Makefile',
		'../README',
		'../collision.cpp',
		'../cutscene.cpp',
		'../cutscene.h',
		'../file.cpp',
		'../file.h',
		'../fs.cpp',
		'../fs.h',
		'../game.cpp',
		'../game.h',
		'../graphics.cpp',
		'../graphics.h',
		'../intern.h',
		'../locale.cpp',
		'../locale.h',
		'../main.cpp',
		'../menu.cpp',
		'../menu.h',
		'../mixer.cpp',
		'../mixer.h',
		'../mod_player.cpp',
		'../mod_player.h',
		'../ogg_player.cpp',
		'../ogg_player.h',
		'../piege.cpp',
		'../resource.cpp',
		'../resource.h',
		'../scaler.cpp',
		'../scaler.h',
		'../seq_player.cpp',
		'../seq_player.h',
		'../sfx_player.cpp',
		'../sfx_player.h',
		'../staticres.cpp',
		'../systemstub.h',
		'../systemstub_sdl.cpp',
		'../unpack.cpp',
		'../unpack.h',
		'../util.cpp',
		'../util.h',
		'../video.cpp',
		'../video.h'
	)
]

DST_DIR = '.'

import os, tarfile, zipfile, hashlib

def print_md5(md5_file, filename):
	m = hashlib.new('md5')
	fp = file(filename, 'rb')
	m.update(fp.read())
	line = m.hexdigest() + ' ' + os.path.split(filename)[1] + '\n'
	md5_file.write(line)

def build_zip_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	zf = zipfile.ZipFile(file_path, 'w', zipfile.ZIP_DEFLATED)
	for entry_path in file_list:
		entry_name = os.path.split(entry_path)[1]
		zf.write(entry_path, entry_name)
	zf.close()

def build_bz2_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	tf = tarfile.open(file_path, 'w:bz2')
	for entry_path in file_list:
		entry_name = file_name[0:-8] + '/' + os.path.split(entry_path)[1]
		tf.add(entry_path, entry_name)
	tf.close()

md5_file = file('CHECKSUMS-%s.MD5' % VERSION, 'w')
for tarball in (SDL_TARBALL, SRC_TARBALL):
	file_name = tarball[0] % VERSION
	file_list = tarball[1]
	print "Generating '" + file_name + "'...",
	if file_name.endswith('.zip'):
		build_zip_tarball(file_name, file_list)
	elif file_name.endswith('.tar.bz2'):
		build_bz2_tarball(file_name, file_list)
	else:
		raise "Unhandled extension for tarball"
	print "Ok"
	print_md5(md5_file, file_name)

