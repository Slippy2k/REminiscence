
# Flashback 3DO

## CDROM

## Files

*.CT

Compressed with bytekiller ?

## Rendering

Flashback was originally developed for the Megadrive and its rendering core is based on
a tile based hardware. The background are actually made up of several tiles (of different
resolutions, eg. 128x44, 16x16) composed when loading. This is very efficient on hardwrae
supporting these. Load the graphics to the dedicated memory and load the attributes to the
other dedicated memory.

As the PC does not have this kind of hardware acceleration, the PC version comes with pre-processed
level backgrounds. The .MAP files contains all of the backgrounds bitmaps (256x224) of a given level.

The 3DO does not come with preprocessed backkground bitmaps (as in the PC version), but composes
in software the background bitmaps by reading the .LEV files and then decoding the tiles found
in .BNQ and .SGD files.

The executable relies on default .cel files as place-holders where the software decoded graphics
are rendered to.

## Differences with Amiga/PC

### Texts

On Amiga/PC, the in-game texts are stored in the executable. The 3DO reworked that and all
texts are now read from external files.

### Cutscenes

This version includes CG cutscenes, replacing the original polygons based.
The quality is similar to the PC CD-ROM release.

Japanese subtitles are present and can be rendered on top of the video.
The .DAT files contains the timing information (frame number) indicating
when the bitmaps need to be rendered on top of the video. The bitmaps are stored in .SUB.

### Sounds

The PC version used Fibonnaci-delta encoded samples at 6000Hz. The Amiga
version was not compressed and played at higger sample rate (~8000Hz).

The 3DO version comes with uncompressed 8 bits mono samples at 8000Hz.

## Unused assets

### Menu animation

The Conrad animation played in the main menu comes with two encodings.
One 'Uncoded16' and another one 'coded8'.

Conrad.coded8 is only loaded in Conrad.smlanim.Uncoded16 fails

```
.text:00009C4C     ADR     R0, aBootGlobalConr ; "$boot/Global/Conrad.smlanim.Uncoded16"
.text:00009C50     MOV     R1, #0x40000
.text:00009C54     BL      load_cel_anim
...
.text:00009C60     TEQ     R0, #0
.text:00009C64     BNE     return
.text:00009C68     ADR     R0, aBootGlobalCo_0 ; "$boot/Global/Conrad.coded8"
.text:00009C6C     MOV     R1, #0x40000
.text:00009C70     BL      load_cel_anim
...
.text:00009C7C     TEQ     R0, #0
.text:00009C80     BNE     return
.text:00009C84     ADR     R0, aCanTLoadConrad ; "Can't load Conrad animation\n"
.text:00009C88     SVC     0x1000E
```

### Level music

Background level music can be found in the tunes/ directory. The music are only played when
the game is in demo mode. The code has explicit checks to condition the playback of the cutscene,
the sounds and the music if a demo file (DEMOx.TEST) has been loaded.

```
.text:00008B28     LDR     R0, =_demoNumTestPtr
.text:00008B2C     LDR     R0, [R0]
.text:00008B30     TEQ     R0, #0
.text:00008B34     BNE     loc_8B40
.text:00008B38     MOV     R0, #0
.text:00008B3C     BL      playCutscene

.text:00008BBC     ADR     R0, aBootLevelsLe_5 ; "$boot/Levels/LEVEL1.CT2"
.text:00008BC0     BL      load_file
...
.text:00008BCC     LDR     R0, =_demoNumTestPtr
.text:00008BD0     LDR     R0, [R0]
.text:00008BD4     TEQ     R0, #0
.text:00008BD8     BEQ     loc_8BE4
.text:00008BDC     MOV     R0, #2
.text:00008BE0     BL      play_music
```



---


Cels/

Cpak/
	*.sub - japanese subtitles

Demo/

Global/
	Conrad.coded8
	Conrad.smlanim.Uncoded16 - Menu animation
	FLASHLOGO.cel - logo
	Interplay.Coded6 - splash screen
	New3doLogo.Coded6
	GLOBAL3D.ANI
	GLOBAL3D.OBJ - .OBJ endian

Langs/
	The original engine would include the languages inside the executable itself.

Levels/
	same as on PC
		need to generate the bitmap
			the PC version would generate a .MAP files from .MBK/.LEV/.SGD so as to preload

samples/

tunes/
	play in demo mode (but no sfx), but no in regular playback

LaunchMe
	04: BL SelfRelocCode NOP if the image is not self-relocating
	change 0xEB 0x00 0x99 0xF8 to a NOP

	try changing the '=_demoNumTestPtr' checks to enable music background during playback


