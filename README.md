## Overview

This program runs connect 4 with visuals in your terminal. Allows for 2 players to play connect 4 in the same terminal, where input is sent to stdin and output is printed to stdout. Players can either be set to be a player that provides input in real time or a computer that will automatically make moves. The game draws a traditional connect 4 board and supports animated pieces falling.

## Usage

First clone the repository in the desired directory using
``` sh
git clone https://github.com/DaKoolest/connect-4.git
```
Next, using your C compiler of choice, compile 'connect-four.c' (note: the file was programmed in C17; the command below is for macOS)
``` sh
gcc connect-4.c -o connect-4.x
```
Then, just run the executable via your command line. If you wish to change some of the default parameters, run the executable with the `--help` to see which ones can be changed.


