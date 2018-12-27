
# Files

.POL contains shapes polygons and palette colors.
.CMD contains bytecode.

The Amiga version bundles the two files into a single file (.CMP) compressed with bytekiller.

CMD contains several animations (for example, OBJECT)

All data is indexed, so a .POL file will contain 4 offsets table :
* shape offset
* shape data
* vertices offset
* vertices data

This allows to reduce and re-use vertices data.

# Opcodes

The cutscene player routine handles 14 opcodes.

Num | Name | Parameters | Description
--- | ---- | ---------- | ---
0   | markCurPos           | | _delay = 5, update palette, flip graphics buffer
1   | refreshScreen        | flag: byte            | set _clearScreen, if 0 clear by copying _pageC and use 2nd palette, otherwise clear with 0xC0 and use first palette
2   | waitForSync          | delay: byte           | _delay = n * 4
3   | drawShape            | num: word             | if _clearScreen != 0, page1 is copied to pageC
4   | setPalette           | num: byte, slot: byte | Load 16 colors palette (444)
5   | markCurPos           | | same as 0x00
6   | drawCaptionString    | num: word             | Draw string
7   | nop                  | |
8   | skip3                | | ignored 3 bytes
9   | refreshAll           | | same as 0x00 + updateKeys (0x0E)
10  | drawShapeScale       | |
11  | drawShapeScaleRotate | |
12  | drawCreditsText      | | _delay = 10, update palette, copy page0 to page1
13  | drawStringAtPos      | num: word, x: byte, y: byte |
14  | handleKeys           | |

# Rendering

page0 is front buffer, page1 is back buffer, pageC is auxiliary buffer
shapes and texts are drawn to page1

3 primitives
- polygons (n points)
- pixels
- ellipses

3 frame buffers
- 2 for double buffering
- 1 for temporary storage

2 palettes of 16 colors

alpha blending

transformations
- scaling
- rotation
- vertical and horizontal stretching

# Copy protection

# Differences between Amiga and PC

## Colors

espions
taxi

## Bugs

espions

## Texts ?
