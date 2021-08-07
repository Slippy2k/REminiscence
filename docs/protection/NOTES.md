
Flashback from Delphine Software came with a protection requiring the manual.
On start (and later in the game !), the player would have to lookup some symbols in ther manual provided with the game.

Patching the game executable to not display these screens is not really straightforward as the original programmers spent some effort protecting their game.

This document tries to list the part of the game engine to be patched to fully unprotect the game.



First of all, the executable set up a vector on the debug interrupt and calls it at regular interval in the timer interrupt.

```
seg000:7CC3 protection_vectorInt3 proc far          ; DATA XREF: protection_installVector+Eo
seg000:7CC3                                         ; sub_0_17CA3+1o
seg000:7CC3       inc _timer_counter
seg000:7CC7       inc _protection_randomVar2
seg000:7CCB       iret
seg000:7CCB protection_vectorInt3 endp

seg000:7C87 protection_installVector proc far       ; CODE XREF: snd_initSoundAndInput+33p
seg000:7C87       mov ax, 3503h
seg000:7C8A       int 21h                           ; DOS - 2+ - GET INTERRUPT VECTOR
seg000:7C8A                                         ; AL = interrupt number
seg000:7C8A                                         ; Return: ES:BX = value of interrupt vector
seg000:7C8C       mov _protection_prev_int3_vector_ptr, bx
seg000:7C90       mov _protection_prev_int3_vector_seg, es
seg000:7C94       push ds
seg000:7C95       mov dx, offset protection_vectorInt3
seg000:7C98       mov ax, cs
seg000:7C9A       mov ds, ax
seg000:7C9C       assume ds:seg000
seg000:7C9C       mov ax, 2503h
seg000:7C9F       int 21h 
```


```
seg000:7CEC timer_int8Vector proc far               ; DATA XREF: timer_sync+4Eo
seg000:7CEC       push ds
seg000:7CED       push ax
seg000:7CEE       mov ax, seg dseg
seg000:7CF1       mov ds, ax
seg000:7CF3       xor byte_1B73_3661C, 1
seg000:7CF8       jz loc_0_17D2A
seg000:7CFA       int 3                             ; Trap to Debugger

seg000:7A3C       mov al, 0A0h ; 'á'
seg000:7A3E       out 43h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7A40       mov ax, 0FFFFh
seg000:7A43       out 42h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7A45       nop
seg000:7A46       mov al, 0
seg000:7A48       out 43h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7A4A       in al, 40h                        ; Timer 8253-5 (AT: 8254.2).
seg000:7A4C       mov ah, al
seg000:7A4E       in al, 40h                        ; Timer 8253-5 (AT: 8254.2).
seg000:7A50       xchg ah, al
seg000:7A52       mov bx, 0FFFFh
seg000:7A55       sub bx, ax
seg000:7A57       mov ds:word_1B73_3664A, bx

seg000:7B56       mov bx, ds:word_1B73_3664A
seg000:7B5A       shr bx, 1
seg000:7B5C       mov al, 36h ; '6'
seg000:7B5E       out 43h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7B60       mov al, bl
seg000:7B62       out 40h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7B64       mov al, bh
seg000:7B66       out 40h, al                       ; Timer 8253-5 (AT: 8254.2).
seg000:7B68       push ds
seg000:7B69       mov dx, offset timer_int8Vector
seg000:7B6C       mov ax, cs
seg000:7B6E       mov ds, ax
seg000:7B70       assume ds:seg000
seg000:7B70       mov ax, 2508h
```

If any debugger was attached to the code, it would be called way too frequently. To continue with analysing, calls to int 3 would have to be nop'ed.

The protection first shows when starting the first level.


Find the call is relatively starigforward, as the 'PROTECTION' letter can be found in clear.

```
seg000:D7BE       mov di, offset _cut_textBuf
seg000:D7C1       mov byte ptr [di], 43h ; 'C'
seg000:D7C4       inc di
seg000:D7C5       mov byte ptr [di], 4Fh ; 'O'
seg000:D7C8       inc di
seg000:D7C9       mov byte ptr [di], 44h ; 'D'
seg000:D7CC       inc di
seg000:D7CD       mov byte ptr [di], 45h ; 'E'
seg000:D7D0       inc di
seg000:D7D1       mov byte ptr [di], 20h ; ' '
seg000:D7D4       inc di
```

A first approach would nop the calls to the protection.

```
seg000:13F4       call load_level_data
...
seg000:1401       call protection_screen1
```

But simply removing the call, would result in the inventory nagging the game cracker.


The protection screen actually sets a flag when exiting with symbols matching.

That flag has to be set.

The text cannot be found in clear in the executable as it is xor'ed.

```
dseg:137A _crackerStringData db 19h, 8, 1Bh, 19h, 11h, 1Fh, 8, 67h, 18h, 16h, 1Bh, 13h, 8, 1Fh, 1Bh, 0Fh, 5Ah                  ; == "CRACKER=BLAIREAU"

seg000:2AEC       cmp _protection_screenCounter, 0
seg000:2AF1       jnz loc_0_12B18
seg000:2AF3       cld
seg000:2AF4       push ds
seg000:2AF5       pop es
seg000:2AF6       assume es:dseg
seg000:2AF6       mov si, offset _crackerStringData
seg000:2AF9       mov di, offset _textBuffer
seg000:2AFC       mov bx, di
seg000:2AFE loc_0_12AFE:
seg000:2AFE       lodsb
seg000:2AFF       xor al, 5Ah
seg000:2B01       stosb
seg000:2B02       cmp al, 0
seg000:2B04       jnz loc_0_12AFE
seg000:2B06       push 0E4h ; 'S'
seg000:2B09       push 0C1h ; '-'
seg000:2B0C       push 41h ; 'A'
seg000:2B0E       push ds
seg000:2B0F       push bx
seg000:2B10       nop
seg000:2B11       push cs
seg000:2B12       call near ptr draw_string
seg000:2B15       add sp, 0Ah
seg000:2B18 loc_0_12B18: 
```



The second protection screen is called later in the game when switching room by using a teleporter or a taxi.
Internally, the game engine has jump table for the game objects. There is one opcode when Conrad changing rooms.

This opcode contains the protection screen, duplicated and this time, and the text 'code' letters are obfuscated.

```
seg000:6E07       mov di, offset _textBuffer
seg000:6E0A       mov byte ptr [di], 40h ; '@'
seg000:6E0D       add byte ptr [di], 3
seg000:6E10       mov byte ptr [di+1], 4Ch ; 'L'
seg000:6E14       add byte ptr [di+1], 3
seg000:6E18       mov byte ptr [di+2], 41h ; 'A'
seg000:6E1C       add byte ptr [di+2], 3
seg000:6E20       mov byte ptr [di+3], 42h ; 'B'
seg000:6E24       add byte ptr [di+3], 3
seg000:6E28       mov byte ptr [di+4], 1Dh
seg000:6E2C       add byte ptr [di+4], 3
```

If the symbols entered do not match, the game continues but patches the jump table opcode with a 'nop', rendering the
teleporter unsable.

If however the symbols match, the game engine replaces the changeRoom opcode with one that does not have the protection call.
One way to patch the game here is to directly patch the jump table with the correct opcode.

```
seg000:21DF       mov si, offset _pge_opcodeTable
seg000:21E2       add si, 104h                      ; si = 0xD37 (o_protection_screen())
seg000:21E6       mov [bp+prot_op1], si
seg000:21E9       mov si, offset pge_op_changeRoom
seg000:21EC       mov [bp+prot_op2], si

seg000:2234 loc_0_12234:                            ; CODE XREF: pge_prepare+1Cj
seg000:2234       mov bx, [bp+prot_op1]
seg000:2237       mov ax, [bp+prot_op2]
seg000:223A       cmp _protection_unkVar2, 0
seg000:223F       jnz loc_0_12247
seg000:2241       cmp [bx], ax
seg000:2243       jz loc_0_12247
seg000:2245       mov [bx], ax
```



There is a third protection in the game engine. Later in the game, Conrad could fall from lift and ground. This is also done
by patching the jump table. Internally, there are several opcodes to check if Conrad can stand on a room area. The engine would swap
the down and up grid opcodes. The game has two counters that should always be in sync.

One way to patch the game is to simply nop the opcodes exchanges.

