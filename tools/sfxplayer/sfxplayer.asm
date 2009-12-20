CODE:000117E8                 addq.w  #1,(_snd_ticksCount).l
CODE:000117EE                 cmpi.w  #50,(_snd_ticksCount).l
CODE:000117F6                 bne.w   loc_11800
CODE:000117FA                 clr.w   (_snd_ticksCount).l
CODE:00011800 
CODE:00011800 loc_11800:                              ; CODE XREF: music_vector+1BEj
CODE:00011800                 move.w  #1,($DFF09C).l
CODE:00011808                 move.l  (_snd_ingameMusicToPlay).l,d0
CODE:0001180E                 beq.w   loc_11948
CODE:00011812                 movea.l d0,a0
CODE:00011814                 lea     (_sfx_periodTable).l,a2
CODE:0001181A                 movea.l (_snd_ingameMusicOffset).l,a1
CODE:00011820                 cmpa.l  #0,a1
CODE:00011826                 bne.w   loc_1183A
CODE:0001182A                 lea     $5E(a0),a1
CODE:0001182E                 clr.w   (_sfx_curOrder).l
CODE:00011834                 clr.w   (_sfx_orderDelay).l
CODE:0001183A 
CODE:0001183A loc_1183A:                              ; CODE XREF: music_vector+1EEj
CODE:0001183A                 move.w  (_sfx_orderDelay).l,d0
CODE:00011840                 beq.w   loc_11852
CODE:00011844                 subi.w  #1,d0
CODE:00011848                 move.w  d0,(_sfx_orderDelay).l
CODE:0001184E                 bra.w   loc_11948
CODE:00011852 ; ---------------------------------------------------------------------------
CODE:00011852 
CODE:00011852 loc_11852:                              ; CODE XREF: music_vector+208j
CODE:00011852                 move.w  $3E(a0),d0
CODE:00011856                 move.w  d0,(_sfx_orderDelay).l
CODE:0001185C                 moveq   #0,d0
CODE:0001185E                 move.b  (a1)+,d0
CODE:00011860                 beq.w   loc_1187A
CODE:00011864                 subq.w  #1,d0
CODE:00011866                 add.w   d0,d0
CODE:00011868                 move.w  d0,d1
CODE:0001186A                 move.w  $40(a0,d1.w),d1
CODE:0001186E                 add.w   d0,d0
CODE:00011870                 move.l  (a0,d0.w),d6
CODE:00011874                 move.l  d6,(dword_3048E).l
CODE:0001187A 
CODE:0001187A loc_1187A:                              ; CODE XREF: music_vector+228j
CODE:0001187A                 moveq   #0,d0
CODE:0001187C                 move.b  (a1)+,d0
CODE:0001187E                 beq.w   loc_1189A
CODE:00011882                 subq.w  #1,d0
CODE:00011884                 add.w   d1,d0
CODE:00011886                 add.w   d0,d0
CODE:00011888                 move.w  (a2,d0.w),d2
CODE:0001188C                 move.w  d2,(word_304A6).l
CODE:00011892                 moveq   #0,d0
CODE:00011894                 jsr     snd_startIngameMusicSample
CODE:0001189A 
CODE:0001189A loc_1189A:                              ; CODE XREF: music_vector+246j
CODE:0001189A                 moveq   #0,d0
CODE:0001189C                 move.b  (a1)+,d0
CODE:0001189E                 beq.w   loc_118B8
CODE:000118A2                 subq.w  #1,d0
CODE:000118A4                 add.w   d0,d0
CODE:000118A6                 move.w  d0,d1
CODE:000118A8                 move.w  $40(a0,d1.w),d1
CODE:000118AC                 add.w   d0,d0
CODE:000118AE                 move.l  (a0,d0.w),d6
CODE:000118B2                 move.l  d6,(dword_30492).l
CODE:000118B8 
CODE:000118B8 loc_118B8:                              ; CODE XREF: music_vector+266j
CODE:000118B8                 moveq   #0,d0
CODE:000118BA                 move.b  (a1)+,d0
CODE:000118BC                 beq.w   loc_118D8
CODE:000118C0                 subq.w  #1,d0
CODE:000118C2                 add.w   d1,d0
CODE:000118C4                 add.w   d0,d0
CODE:000118C6                 move.w  (a2,d0.w),d2
CODE:000118CA                 move.w  d2,(word_304A8).l
CODE:000118D0                 moveq   #1,d0
CODE:000118D2                 jsr     snd_startIngameMusicSample
CODE:000118D8 
CODE:000118D8 loc_118D8:                              ; CODE XREF: music_vector+284j
CODE:000118D8                 moveq   #0,d0
CODE:000118DA                 move.b  (a1)+,d0
CODE:000118DC                 beq.w   loc_118F6
CODE:000118E0                 subq.w  #1,d0
CODE:000118E2                 add.w   d0,d0
CODE:000118E4                 move.w  d0,d1
CODE:000118E6                 move.w  $40(a0,d1.w),d1
CODE:000118EA                 add.w   d0,d0
CODE:000118EC                 move.l  (a0,d0.w),d6
CODE:000118F0                 move.l  d6,(dword_30496).l
CODE:000118F6 
CODE:000118F6 loc_118F6:                              ; CODE XREF: music_vector+2A4j
CODE:000118F6                 moveq   #0,d0
CODE:000118F8                 move.b  (a1)+,d0
CODE:000118FA                 beq.w   loc_11918
CODE:000118FE                 subq.w  #1,d0
CODE:00011900                 add.w   d1,d0
CODE:00011902                 add.w   d0,d0
CODE:00011904                 move.w  (a2,d0.w),d2    ; period
CODE:00011908                 move.w  d2,(word_304AA).l
CODE:0001190E                 moveq   #2,d0
CODE:00011910                 moveq   #$40,d3 ; '@'   ; volume
CODE:00011912                 jsr     snd_startIngameMusicSample
CODE:00011918 
CODE:00011918 loc_11918:                              ; CODE XREF: music_vector+2C2j
CODE:00011918                 move.l  a1,(_snd_ingameMusicOffset).l
CODE:0001191E                 move.w  (_sfx_curOrder).l,d0
CODE:00011924                 addi.w  #1,d0
CODE:00011928                 cmp.w   $3C(a0),d0
CODE:0001192C                 blt.w   loc_1193E
CODE:00011930                 clr.l   (_snd_ingameMusicToPlay).l
CODE:00011936                 addi.w  #$14,(_sfx_orderDelay).l
CODE:0001193E 
CODE:0001193E loc_1193E:                              ; CODE XREF: music_vector+2F4j
CODE:0001193E                 move.w  d0,(_sfx_curOrder).l
CODE:00011944                 bra.w   loc_11982
CODE:00011948 ; ---------------------------------------------------------------------------
CODE:00011948 
CODE:00011948 loc_11948:                              ; CODE XREF: music_vector+1D6j
CODE:00011948                                         ; music_vector+216j
CODE:00011948                 tst.l   (_snd_ingameMusicToPlay).l
CODE:0001194E                 bne.w   loc_11982
CODE:00011952                 tst.l   (_snd_ingameMusicOffset).l
CODE:00011958                 beq.w   loc_11982
CODE:0001195C                 move.w  (_sfx_orderDelay).l,d0
CODE:00011962                 beq.w   loc_11974
CODE:00011966                 subi.w  #1,d0
CODE:0001196A                 move.w  d0,(_sfx_orderDelay).l
CODE:00011970                 bra.w   loc_11982
CODE:00011974 ; ---------------------------------------------------------------------------
CODE:00011974 
CODE:00011974 loc_11974:                              ; CODE XREF: music_vector+32Aj
CODE:00011974                 clr.l   (_snd_ingameMusicOffset).l
CODE:0001197A                 move.w  #7,($DFF096).l
CODE:00011982 
CODE:00011982 loc_11982:


CODE:0000B74A snd_startIngameMusicSample:             ; CODE XREF: music_vector+25Cp
CODE:0000B74A                                         ; music_vector+29Ap ...
CODE:0000B74A                 movem.l d1-a5,-(sp)
CODE:0000B74E                 moveq   #0,d1
CODE:0000B750                 bset    d0,d1
CODE:0000B752                 move.w  d1,($DFF096).l
CODE:0000B758                 lea     ($DFF0A0).l,a1
CODE:0000B75E                 lsl.w   #4,d0
CODE:0000B760                 adda.w  d0,a1
CODE:0000B762                 move.w  #600,d0
CODE:0000B766 
CODE:0000B766 loc_B766:                               ; CODE XREF: snd_startIngameMusicSample+1Cj
CODE:0000B766                 dbf     d0,loc_B766
CODE:0000B76A                 movea.l d6,a0
CODE:0000B76C                 moveq   #0,d0
CODE:0000B76E                 move.w  (a0)+,d0
CODE:0000B770                 move.w  (a0)+,d3
CODE:0000B772                 moveq   #0,d7
CODE:0000B774                 move.w  (a0)+,d7
CODE:0000B776                 move.w  (a0)+,d5
CODE:0000B778                 move.l  a0,d6
CODE:0000B77A                 clr.w   (a0)
CODE:0000B77C                 move.l  d6,(a1)         ; aud0lch
CODE:0000B77E                 lsr.l   #1,d0
CODE:0000B780                 move.w  d0,4(a1)        ; aud0len
CODE:0000B784                 move.w  d2,6(a1)        ; aud0per
CODE:0000B788                 move.w  d3,8(a1)        ; aud0vol
CODE:0000B78C                 ori.w   #-$8000,d1
CODE:0000B790                 move.w  d1,($DFF096).l
CODE:0000B796                 move.w  #$100,d0
CODE:0000B79A 
CODE:0000B79A loc_B79A:                               ; CODE XREF: snd_startIngameMusicSample+50j
CODE:0000B79A                 dbf     d0,loc_B79A
CODE:0000B79E                 lsr.w   #1,d5
CODE:0000B7A0                 add.l   d7,d6
CODE:0000B7A2 
CODE:0000B7A2 loc_B7A2:
CODE:0000B7A2                 move.l  d6,(a1)
CODE:0000B7A4                 move.w  d5,4(a1)
CODE:0000B7A8                 move.w  d2,6(a1)
CODE:0000B7AC                 move.w  d3,8(a1)
CODE:0000B7B0                 movem.l (sp)+,d1-a5
CODE:0000B7B4                 rts     
