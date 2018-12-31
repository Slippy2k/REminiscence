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


Missing background music

The code seems to explicitely disable cutscene, sound sfx and level background music when playing back 'demo' (attrack mode)

.text:00008B28                 LDR     R0, =_demoNumTestPtr
.text:00008B2C                 LDR     R0, [R0]
.text:00008B30                 TEQ     R0, #0
.text:00008B34                 BNE     loc_8B40
.text:00008B38                 MOV     R0, #0
.text:00008B3C                 BL      playCutscene

.text:00008BBC                 ADR     R0, aBootLevelsLe_5 ; "$boot/Levels/LEVEL1.CT2"
.text:00008BC0                 BL      load_file
.text:00008BC4                 LDR     R1, =unk_26600
.text:00008BC8                 STR     R0, [R1]
.text:00008BCC                 LDR     R0, =_demoNumTestPtr
.text:00008BD0                 LDR     R0, [R0]
.text:00008BD4                 TEQ     R0, #0
.text:00008BD8                 BEQ     loc_8BE4
.text:00008BDC                 MOV     R0, #2
.text:00008BE0                 BL      play_music

.text:00008E48                 ADR     R0, aBootLevelsDt_c ; "$boot/Levels/DT.CT"
.text:00008E4C                 BL      load_file
.text:00008E50                 LDR     R1, =unk_26600
.text:00008E54                 STR     R0, [R1]
.text:00008E58                 LDR     R0, =_demoNumTestPtr
.text:00008E5C                 LDR     R0, [R0]
.text:00008E60                 TEQ     R0, #0
.text:00008E64                 BEQ     loc_8E70
.text:00008E68                 MOV     R0, #4
.text:00008E6C                 BL      play_music
.text:00008E70

.text:0000DD64                 LDR     R0, =_demoNumTestPtr
.text:0000DD68                 LDR     R0, [R0]
.text:0000DD6C                 TEQ     R0, #0
.text:0000DD70                 BEQ     loc_DD80
.text:0000DD74                 MOV     R0, #1
.text:0000DD78                 LDMDB   R11, {R4-R8,R11,SP,PC}
.text:0000DD78 ; ---------------------------------------------------------------------------
.text:0000DD7C off_DD7C        DCD _demoNumTestPtr     ; DATA XREF: play_sound+30r
.text:0000DD80 ; ---------------------------------------------------------------------------
.text:0000DD80
.text:0000DD80 loc_DD80                                ; CODE XREF: play_sound+3Cj
.text:0000DD80                 CMP     R4, #67

