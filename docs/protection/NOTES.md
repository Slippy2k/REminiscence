
Flashback from Delphine Software came with a protection requiring the manual.
On start (and later in the game !), the player would have to lookup some symbols in ther manual provided with the game.

Patching the game executable to not display these screens is not really straightforward as the original programmers spent some effort protecting their game.

This document tries to list the part of the game engine to be patched to fully unprotect the game.



First of all, the executable set up a vector on the debug interrupt and calls it at regular interval in the timer interrupt.

```
```

If any debugger was attached to the code, it would be called way too frequently. To continue with analysing, calls to int 3 would have to be nop'ed.

The protection first shows when starting the first level.


Find the call is relatively starigforward, as the 'PROTECTION' letter can be found in clear.

A first approach would nop the calls to the protection.

But simply removing the call, would result in the inventory nagging the game cracker.


The protection screen actually sets a flag when exiting with symbols matching.
That flag has to be set.

The text cannot be found in clear in the executable as it is xor'ed.



The second protection screen is called later in the game when switching room by using a teleporter or a taxi.
Internally, the game engine has jump table for the game objects. There is one opcode when Conrad changing rooms.

This opcode contains the protection screen, duplicated and this time, and the protection letters are obfuscated with xor.
If the symbols entered do not match, the game continues but patches the jump table opcode with a 'nop', rendering the
teleporter unsable.

If however the symbols match, the game engine replaces the changeRoom opcode with one that does not have the protection call.
One way to patch the game here is to directly patch the jump table with the correct opcode.



There is a third protection in the game engine. Later in the game, Conrad could fall from lift and ground. This is also done
by patching the jump table. Internally, there are several opcodes to check if Conrad can stand on a room area. The engine would swap
the down and up grid opcodes. The game has two counters that should always be in sync.

One way to patch the game is to simply nop the opcodes exchanges.

