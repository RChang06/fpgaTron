# Embedded C Programmming on RISC-V

This lab is done **INDIVIDUALLY**.

## Getting Started with Hardware

The following instructions are adapted from Lab 0 for use with C.

Connect your FPGA board to your laptop and a VGA monitor.  There are VGA
monitors in the lab with a BLUE connector on the end. If you do not have a VGA
monitor at home, you can use CPUlator to get started (see below), but to you
will be marked with things running on your FPGA board.

In Windows, go to **Start** --> **Nios V Command Shell** and run the following commands on your laptop:

    make DE10-Lite
    make GDB_SERVER

(alternatively, use `make DE1-SoC`)

The first command transmits a Nios V computer system to your DE10-Lite board.
The second command will start a program on your laptop that you need to leave running -- it
continuously communicates between your Nios V computer system and your laptop.
If you exit the GDB server by typing Q, the commands below will not longer work.
Sometimes things get confused and you need to stop everything, including the GDB server,
and start over. Sometimes the GDB server can't find the JTAG server to start -- try starting Quartus
and then re-run the command.

Again, go to **Start** --> **Nios V Command Shell** and run the following commands on your laptop:

    make TERMINAL

This launches a program to listen and display any characters sent by your Nios V computer over the JTAG UART.
Leave this program running to capture everything printed by your program. Control-C will exit.

You can merge windows by dragging the tab for the Nios V Command Shell window over the tab from the first window.

Again, go to **Start** --> **Nios V Command Shell** and run the following commands on your laptop:

    make COMPILE
    make RUN

The first command compiles your program, `vga.c` into an object file,
`vga.c.o`, and then linking it into an executable, `vga.elf`.  The second
command transfers the `vga.elf` executable to your Nios V computer and runs it.

This Nios V program will print the message "start" to the TERMINAL window you started earlier, then it will
fill the VGA screen with colour bar pattern, then it will print the message "done".

You can edit your vga.c program, save it, then run `make COMPILE` and `make RUN` again.
Hint: the easiest way to edit is with `notepad vga.c`, but you can also right-click the
file and use notepad++ or launch WSL and run vim.


### Using GDB

After `make COMPILE`, you can also launch gdb to debug your program:

    make GDB_CLIENT

Note: for some reason, GDB is not working with C code. Use CPUlator instead.


### Using CPUlator

Instead of using GDB and using your FPGA board, you can run things on CPUlator.
This tool, written by a former UBC Master's student / UofT PhD student Henry Wong,
perfectly emulates the DE1-SoC Computer System with RISC-V as well as several
other computers and processors. Henry runs this website on his own.

http://cpulator.01xz.net

To get started:
* select RISC-V RV32 under Architecture
* select RISC-V RV32 DE1-SoC under System
* select Go
* in the Editor window, change Language from RV32 to C

All of your source code has to appear in the one edit window (as a single file).
For this lab, first paste the contents of `vga.c` into the Editor window. Then,
find the line that says `#include "address_map_niosv.h` and delete it; in its place,
paste the entire contents of the file `address_map_niosv.h`.

**WARNING** since CPUlator is a website, pressing REFRESH on the web page might
cause your program to disappear. Save if locally using the `File --> Save ...` feature.
Likewise, you can load it back again using `File --> Load...`.

## Lab 9 Requirements

### Tron Light Cycle Game

Modify vga.c into a C program that plays the Tron light cycle on your FPGA
board.  Requirements for the game are described below.

The game is played with 2 players. One player must be a human, the other must
be a robot (described below).  You must determine which controls to use for the
human. (Optionally, you may allow a second human to force the robot to make a
turn.)

The game starts with an all-black screen. A white border should be drawn around
the perimeter of the screen, forming the gameplay area. The game designer can
add fixed obstacles (white). Obstacles may be single pixels, rectangles, lines,
or any other shape that you wish.  Collision with the border or an obstacle
results in death.

You can determine collision by reading the pixel colour from the VGA buffer at
the desired location, eg `pixel_t p = *pVGA;` will read the pixel colour at
location [0,0] into the variable p.

Each player controls a light cycle which starts at a fixed position roughly
halfway down the screen and one-third of the way from the left and right edges.
The light cycle is a single pixel that constantly moves across the screen and
leaves behind a coloured light trace. The player controls whether the light
cycle is moving up, down, left, or right, but it must always move by one pixel
after every time delay. The light trace is an electrified wall that kills any
player upon collision (including yourself!).

The winner is the first player to reach a score of 9. For each player, you must
always display the current score (0-9) using one 7-segment display each. A player
scores one point by surviving longer than the other player; it is possible that both
players die at the same time, in which case neither player scores a point. After a
complete game (reaching 9), the screen must fill up with the light of the winning
player.

For a simple robot player, have it look one and two pixels ahead. If it detects
anything other than an unclaimed pixel (still black), it first attempts to turn
LEFT. If the pixel to the LEFT is not black, then it attempts to turn RIGHT. If
it cannot turn LEFT or RIGHT, then it continues straight. You may modify this
behavior to make your robot player better.

This game is deliberately open-ended to allow you to either simplify or be
creative as you see fit.

It is recommended that you start with one working player (either human or robot),
then add the second player, then add the score-keeping and display.


### Switching between CPUlator (DE1-SoC) and DE10-Lite

If you have a DE1-SoC, or are running in CPUlator, modify the beginning of
`address_map_niosv.h` so it reads `#define DE10LITE 0`. If you switch from
CPUlator back to DE10-Lite hardware, be sure to change the 0 back to a 1.

### Deadline

Due to the late release of this lab, you may get it marked at the same time as
Lab 10. However, it is strongly advised that you attempt to get it done this
week.

### Submission Requirements

Submit your entire program, as a single C source file. However, please rename
your source file to end in `.txt` so that Canvas can actually display its contents
instead of forcing a file download.

