
# Flashback Protection Notes

Being released in the early 90s, the developers from [Delphine Software](https://www.mobygames.com/company/delphine-software-international) protected the game code to prevent piracy.

On start (and later in the game !), the player would have to lookup some symbols in the manual provided with the game.

This document tries to list the game engine routines related to the game protection.

The addresses and instructions are based on the disassembly of the French DOS version executable.



## Anti debugging


The game executable sets up its own vector routine for the [interrupt 3](https://en.wikipedia.org/wiki/INT_%28x86_instruction%29%23INT3).

```
seg000:7CC3 isr_int3
seg000:7CC3       inc _timer_counter
seg000:7CC7       inc _protection_symbol_rand_var
seg000:7CCB       iret

seg000:7C95       mov dx, offset isr_int3
seg000:7C98       mov ax, cs
seg000:7C9A       mov ds, ax
seg000:7C9C       mov ax, 2503h
seg000:7C9F       int 21h
```

That interrupt is itself triggered from the [timer vector](https://stanislavs.org/helppc/8253.html).

```
seg000:7CEC isr_int8 proc far               ; DATA XREF: timer_sync+4Eo
seg000:7CF8       jz loc_0_17D2A
seg000:7CFA       int 3                             ; Trap to Debugger

seg000:7B69       mov dx, offset isr_int8
seg000:7B6C       mov ax, cs
seg000:7B6E       mov ds, ax
seg000:7B70       mov ax, 2508h
```

If a debugger was attached to the program, it would be called (too) frequently.


## Protection Screen 1

The protection first shows when starting the game after loading the level data files.

![fb-protection-symbol](https://imgur.com/TtV1ey8)

```
seg000:13F4       call load_level_data
...
seg000:1401       call protection_screen1
```

The call is relatively straightforward to locate as the 'CODE' letters can be found in clear in the code.

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

The function call should not simply be bypassed, as the protection rountine sets two flags.

The first flag is set after the screen is displayed (eg. the function was actually called).

```
seg000:D86E       inc _protection_screen1_shown_flag
```

And the second flag is set when the code entered by the player matches the symbol.

```
seg000:D986       mov _protection_screen1_input_flag, 0FFh
```

The first flag is checked in the inventory and nags the game cracker.

![fb-protection-inventory-cracker](https://imgur.com/X36fD1V)

The text cannot be found in clear in the executable as it is xor'ed.

```
; CRACKER=BLAIREAU
dseg:137A _cracker_data db 19h, 8, 1Bh, 19h, 11h, 1Fh, 8, 67h, 18h, 16h, 1Bh, 13h, 8, 1Fh, 1Bh, 0Fh, 5Ah
```

```
seg000:2AEC       cmp _protection_screen1_shown_flag, 0
seg000:2AF1       jnz .L1
...
seg000:2AF6       mov si, offset _cracker_data
seg000:2AF9       mov di, offset _text_buffer
seg000:2AFC       mov bx, di
seg000:2AFE .L2:
seg000:2AFE       lodsb
seg000:2AFF       xor al, 5Ah
seg000:2B01       stosb
seg000:2B02       cmp al, 0
seg000:2B04       jnz .L2
seg000:2B06       push 0E4h ; color
seg000:2B09       push 0C1h ; y_pos
seg000:2B0C       push 41h  ; x_pos
seg000:2B0E       push ds
seg000:2B0F       push bx
seg000:2B10       nop
seg000:2B11       push cs
seg000:2B12       call near ptr draw_string
seg000:2B15       add sp, 0Ah
seg000:2B18 .L1:
```

The second flag conditions the game cutscenes.

![fb-protection-cutscene-holocube](https://imgur.com/uvC2cHc)

Cutscenes are skipped if it is not set.

```
seg000:14DB       cmp _demo_mode_flag, 0
seg000:14E0       jnz .L1
seg000:14E2       cmp _protection_screen1_input_flag, 0
seg000:14E7       jz loc_0_114F2
seg000:14E9 .L1:
seg000:14E9       push 1 ; load_map_flag
seg000:14EB       push cs
seg000:14EC       call near ptr cut_play
seg000:14EF       add sp, 2
```


## Protection Screen 2

The second protection screen is shown later in the game when switching rooms by using a teleporter or taking a taxi.

![fb-protection-taxi-1](https://imgur.com/UmPQfsv) ![fb-protection-taxi-2](https://imgur.com/7Mq3nS4)

Internally, the game engine has an opcode table for the game objects.
One of these opcodes (0x82) handles moving an object to a different room.

Initially, the table is initialized with a different routine than the `change_room` opcode.

```
dseg:0C33 _pge_opcode_tbl dw offset pge_op_0x00
...
dseg:0D37       dw offset pge_op_0x82_protection_screen2
dseg:0D39       dw offset pge_op_0x83_has_inventory_item
dseg:0D3B       dw offset pge_op_0x84_change_level
```

This `protection_screen2' opcode contains another protection screen, also requiring to lookup the symbols in the manual.
The code is duplicated from the first screen routine and this time, the 'code' letters are obfuscated.

```
seg000:6E07       mov di, offset _text_buffer
seg000:6E0A       mov byte ptr [di], 40h
seg000:6E0D       add byte ptr [di], 3
seg000:6E10       mov byte ptr [di+1], 4Ch
seg000:6E14       add byte ptr [di+1], 3
seg000:6E18       mov byte ptr [di+2], 41h
seg000:6E1C       add byte ptr [di+2], 3
seg000:6E20       mov byte ptr [di+3], 42h
seg000:6E24       add byte ptr [di+3], 3
seg000:6E28       mov byte ptr [di+4], 1Dh
seg000:6E2C       add byte ptr [di+4], 3
```

If the code entered by the player matches the symbol, the game engine updates the opcodes table with the real change_room opcode,
calls it and finally sets a flag.

```
seg000:1448       mov si, offset pge_op_0x82_protection_screen2 ; &_pge_opcode_tbl[0x82]
seg000:144B       mov _protection_pge_opcode_tbl_0x82, si
...
seg000:6D33       mov si, offset _pge_opcode_tbl
seg000:6D36       add si, 104h ; 0x82
seg000:6D3A       mov [bp-2], si

; update the opcodes table
seg000:6FF8       mov si, [bp-2]
seg000:6FFB       mov bx, _protection_pge_opcode_tbl_0x82
seg000:6FFF       mov [si], bx

; calls the original opcode
seg000:700D       call bx

seg000:702E       mov _protection_screen2_input_flag, 0FFh
```

However, if the symbols entered do not match after 3 tries, the game continues but patches the opcode 0x82 table with a 'nop',
rendering the teleporter and taxi unsable.

```
seg000:6D33       mov si, offset _pge_opcode_tbl
seg000:6D36       add si, 104h ; 0x82
seg000:6D3A       mov [bp-2], si
...
seg000:706C       mov bx, [bp-2]
seg000:706F       mov ax, offset pge_op_nop
seg000:7072       mov [bx], ax

seg000:7096       mov _protection_screen2_input_flag, 0
```

Similar to the first protection screen, there is flag that is set when the protection screen exits.
This is used to swap again the opcode 0x82 with the protection screen routine.

```
seg000:21DF       mov si, offset _pge_opcode_tbl
seg000:21E2       add si, 104h ; &_pge_opcode_tbl[0x82]
seg000:21E6       mov [bp+prot_op1], si
seg000:21E9       mov si, offset pge_op_protection_screen2
seg000:21EC       mov [bp+prot_op2], si

seg000:2234       mov bx, [bp+prot_op1]
seg000:2237       mov ax, [bp+prot_op2]
seg000:223A       cmp _protection_screen2_input_flag, 0
seg000:223F       jnz .L1
seg000:2241       cmp [bx], ax
seg000:2243       jz .L1
seg000:2245       mov [bx], ax
seg000:2247 .L1:
```


## Room Grid

The third protection found in the game engine is related to the room grid. Conrad would sometimes fall randomly.

![fb-protection-grid-fall](https://imgur.com/kYJK0Z4)

Internally, each object movement in a room is restricted to a grid of 16 horizontal by 3 vertical cells (256 / 16 and 224 / 72).

![fb-protection-grid-room](https://imgur.com/6pgs040)

The attribute of each cell (eg. collide, walk) can be queried by some of the opcodes.

The engine increments two counters and if they ran out of sync, the grid opcodes 2d (x+2,y+1) and 2u (x+2,y-1) are swapped.

```
seg000:4828       mov bx, offset _pge_opcode_tbl
seg000:482B       add bx, 34h
seg000:482E       mov [bp+op_0x1A], bx ; &_pge_opcode_tbl[0x1A]

seg000:489F       cmp _protection_grid_sync_counter1, 4
seg000:48A4       jl .L1
seg000:48A6       mov al, _protection_grid_sync_counter2
seg000:48A9       sub al, 10
seg000:48AB       cmp _protection_grid_sync_counter1, al
seg000:48AF       jl .L1
seg000:48B1       mov si, [bp+op_0x1A]
seg000:48B4       mov ax, [si]                      ; opcode_0x1A - pge_op_0x1A_not_collide_2u
seg000:48B6       mov bx, [si+4]                    ; opcode_0x1C - pge_op_0x1C_not_collide_2d
seg000:48B9       mov [si+4], ax
seg000:48BC       mov [si], bx
seg000:48BE       add _protection_grid_sync_counter2, 12
seg000:48C3 .L1:
```
