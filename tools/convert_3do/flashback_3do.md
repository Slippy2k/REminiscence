Cels/

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
		even SGD

samples/

tunes/
	play in demo mode (but no sfx), but no in regular playback


LaunchMe

	04: BL SelfRelocCode NOP if the image is not self-relocating
	change 0xEB 0x00 0x99 0xF8 to a NOP


Conrad.coded8 is only loaded if Conrad.smlanim.Uncoded16 fails

.text:00009C4C     ADR     R0, aBootGlobalConr ; "$boot/Global/Conrad.smlanim.Uncoded16"
.text:00009C50     MOV     R1, #0x40000
.text:00009C54     BL      load_cel_anim
.text:00009C58     LDR     R1, =0x26038
.text:00009C5C     STR     R0, [R1]
.text:00009C60     TEQ     R0, #0
.text:00009C64     BNE     return
.text:00009C68     ADR     R0, aBootGlobalCo_0 ; "$boot/Global/Conrad.coded8"
.text:00009C6C     MOV     R1, #0x40000
.text:00009C70     BL      load_cel_anim
.text:00009C74     LDR     R1, =0x26038
.text:00009C78     STR     R0, [R1]
.text:00009C7C     TEQ     R0, #0
.text:00009C80     BNE     return
.text:00009C84     ADR     R0, aCanTLoadConrad ; "Can't load Conrad animation\n"
.text:00009C88     SVC     0x1000E
.text:00009C8C return

