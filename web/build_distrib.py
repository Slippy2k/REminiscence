#!/usr/bin/env python

SDL_TARBALL = [ 'REminiscence-%s-sdl2-win32.zip',
	(
		'CHANGES.txt',
		'../README.txt',
		'../README-modplug.txt',
		'../README-xbr.txt',
		'../scalers/scaler_nearest.dll',
		'../scalers/scaler_tv2x.dll',
		'../scalers/scaler_xbr.dll',
		'../icon.bmp',
		'../rs.cfg',
		'../rs.exe'
	)
]

SRC_TARBALL = [ 'REminiscence-%s.tar.bz2',
	(
		'CHANGES.txt',
		'../README.txt',
		'../rs.cfg',
		'../Makefile',
		'../collision.cpp',
		'../cpc_player.cpp',
		'../cpc_player.h',
		'../cutscene.cpp',
		'../cutscene.h',
		'../decode_mac.cpp',
		'../decode_mac.h',
		'../file.cpp',
		'../file.h',
		'../fs.cpp',
		'../fs.h',
		'../game.cpp',
		'../game.h',
		'../graphics.cpp',
		'../graphics.h',
		'../intern.h',
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
		'../protection.cpp',
		'../resource.cpp',
		'../resource.h',
		'../resource_aba.cpp',
		'../resource_aba.h',
		'../resource_mac.cpp',
		'../resource_mac.h',
		'../scaler.cpp',
		'../scaler.h',
		'../screenshot.cpp',
		'../screenshot.h',
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

SRC_DIR = '..'
DST_DIR = '.'

import hashlib
import os
import re
import tarfile
import tempfile
import zipfile

def get_version():
	f = file(os.path.join(SRC_DIR, 'README.txt'))
	for line in f.readlines():
		m = re.search('Release version: (\d+\.\d+\.\d+)', line.strip())
		if m:
			return m.group(1)
	return None

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
		entry_name = os.path.split(entry_path)[-1]
		zf.write(entry_path, entry_name)
	zf.close()

def fixup_makefile(filepath):
	# remove references to scalers
	temporary = tempfile.NamedTemporaryFile(delete=False)
	sourcefile = file(filepath, 'r')
	for line in sourcefile.readlines():
		ndx = line.find('-DUSE_STATIC_SCALER')
		if ndx != -1:
			line = line[:ndx] + line[ndx + 20:]
		elif line.startswith('SCALERS :='):
			continue
                temporary.write(line)
        temporary.flush()
        return temporary

def build_bz2_tarball(file_name, file_list):
	file_path = os.path.join(DST_DIR, file_name)
	tf = tarfile.open(file_path, 'w:bz2')
	for entry_path in file_list:
		entry_name = file_name[0:-8] + '/' + os.path.split(entry_path)[1]
		if entry_path == '../Makefile':
			fileobj = fixup_makefile(entry_path)
			tarinfo = tf.gettarinfo(name='Makefile', arcname=entry_name, fileobj=fileobj)
			fileobj.seek(0)
			tf.addfile(tarinfo, fileobj)
			continue
		tf.add(entry_path, entry_name)
	tf.close()

version = get_version()
md5_file = file('CHECKSUMS-%s.MD5' % version, 'w')
for tarball in (SDL_TARBALL, SRC_TARBALL):
	file_name = tarball[0] % version
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

