# Dune
Dune is our version of the popular mobile games, [Dune!](https://play.google.com/store/apps/details?id=io.voodoo.dune&hl=en_CA&gl=US) and [Tiny Wings](https://apps.apple.com/ca/app/tiny-wings/id417817520), for the [DE1-SoC](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836) board.  The objective is to ride through a series of undulating hills and achieve the maximum possible score before crashing.

The game was coded at a very low-level using C, and writes directly to the board's registers to control the double-buffered VGA display, HEX displays, switches, and PS/2 keyboard interface.  Multiple techniques and computer graphics algorithms were used to update the frames efficiently, and a custom physics engine was written to control the player's character.

The technical highlights of our project are as follows:
* Bresenham's line and circle algorithms for accurate and fast graphics
* Efficient techniques for updating the dunes and the frames
* Double buffering of the VGA display and proper syncing
* PS/2 keyboard interface to accept control inputs from the player
* Custom physics engine
* Custom game fonts

## Gameplay footage
The gameplay footage can be found here: https://youtu.be/EgDXtpAh_6s. 
[![here.](https://img.youtube.com/vi/EgDXtpAh_6s/maxresdefault.jpg)](https://youtu.be/EgDXtpAh_6s)

The video footage looks a bit choppy due to the recording software.  The actual gameplay on a physical board is quite smooth.

## Installation

```bash
git clone https://github.com/adamw8/Dune-DE1_SoC.git
```
Since the emulator for the [DE1-SoC](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836) board does not support multiple C files, all the code for Dune is in `dune.c`.

## Usage

The code was designed to be run on a physical [DE1-SoC](https://www.terasic.com.tw/cgi-bin/page/archive.pl?Language=English&No=836) board, however it can also be emulated online.

1. Got to https://cpulator.01xz.net/?sys=arm-de1soc.  This is an online emulator for the DE1_SoC board.
2. At the top of the central panel, there is a `Language` drop down labelled `ARMv7`. Change the language to `C`.
3. In the top toolbar, select `File`->`Open` and select `dune.c`.
4. Press `F5` to compile and load the code.
5. Press `F3` to start the program.
6. Scroll down in the right hand panel until you see the VGA display and the PS/2 keyboard interface.  `Note:` The size of the VGA display can be increased by clicking the small drop down arrow next to the `VGA pixel buffer` label.
7. Follow the instructions on the screen to play.  Provide keyboard inputs into the PS/2 keyboard interface.

## Contributors
This project was written entirely by [Adam Wei](https://github.com/adamw8) and [Hao Xiang Yang](https://github.com/hxyang123).
