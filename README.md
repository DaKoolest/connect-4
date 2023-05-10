## Overview

This program runs connect 4 with visuals in your terminal. Allows for 2 players to play connect 4 in the same terminal, where input is sent to stdin and output is printed to stdout. Players can either be set to be a player that proivdes input in real time and a computer, that will automatically make moves. The game draws a traditonal connect 4 board and supports animated pieces falling.

## Usage

First clone the repository in the desired directory using
``` sh
git clone https://github.com/DaKoolest/connect-4.git
```
Next, using your C compiler of choice, compile 'connect-four.c' (note, the file was programmed in C17)
``` sh
gcc connect-4.c -o connect-4.x
```
Then, just run the executable via your command line. If you wish to change the number of rows and columns of the board, the amount of pieces needed to get in a row to win, or the rate at which pieces fall, you can edit the constants defined at the top of `connect-4.c`. To change the type of player assigned to P1 and P2, you must modify the declaration of `short players[]` in `int main(void)`. The first element corresponds to the type of the first player, and similarly the second corresponds to the type of the second. `0` refers to a computer, and `1` refers to a player that inputs moves in the command line.


