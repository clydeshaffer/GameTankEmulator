# Example assembly programs for the GameTank

Here I'll provide some code examples for the GameTank system.
The information within will be cumulative, meaning that higher-numbered examples may include features from lower-numbered examples without the explanation repeated.

0. Drawing pixels by direct video memory write

1. Draw a colored rectangle using the blitter

2. Load sprite data and draw it using the blitter

3. Write a game loop that renders once per VSync

4. Read input from gamepads

5. Playing sounds and music

6. Using a 2MB cartridge

7. Accessing extended RAM and VRAM



This code was written for vasm by Dr. Volker Barthelmann availble at http://www.compilers.de/vasm.html

The examples that use sprites refer to a sprite sheet in the assets folder, compressed with zopfli.

zlib6502 by Pitor Fusik is used to extract the data. It is included here as a precompiled blob with the following assumptions:

| **Use**            | **Mapping**     |
|--------------------|-----------------|
| INFLATE code       | 0xE000 - 0xE1FC |
| reserved RAM       | 0x200 - 0x04FD  |
| reserved Zero Page | 0xF0 - 0xF3     |