#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#define COLS 7 // amount of coloumns the board should have
#define ROWS 6 // amount of rows the board should have
#define NUM_ADJ_PIECES 4 // amount
#define PIECE_FALL_DELAY 70000 //170000 // piece fall delay in microseconds

typedef enum {EMPTY = 0, P1, P2} PIECE;

short game_over;

PIECE **init_board(void);
void free_board(PIECE **board);

void reset_board(PIECE **board);

void play_turn(PIECE **board, short player_num, short type);
int place_piece(PIECE **board, PIECE piece, int col);

int drop_row(PIECE **board, int col);

int check_win(PIECE **board, PIECE piece, int row, int col);
void win(PIECE piece);

void p_board(PIECE **board);
void p_solid_line(int length);
void p_space(int num);

void p_error(int error_code);

int main(void) {
    int turn;

    // array that stores the type that player 0 and 1 are. 0 is computer, 1 is person.
    short players[] = {0, 0};

    while (1) {
        // resets game
        PIECE **board = init_board();
        turn = 0;
        game_over = 0;
        
        printf("\033[2J"); // clears console
        p_board(board); // prints empty board

        srand(time(NULL));
        
        // runs game
        while (turn < COLS * ROWS && !game_over) {
            play_turn(board, (turn % 2) + 1, players[turn % 2]);
            turn++;
        }

        // if turn reached the max possible turns, trigger a draw since no one has won yet
        if (turn == COLS * ROWS) {
            printf("Turn: %d\n", turn);
            // winning on EMPTY calls tie
            win(EMPTY);
        }

        char response;

        // clears stdin
        while ((response = getchar()) != EOF && response != '\n');
        printf("Enter 'y' to play again or anything else to exit:\n");
        scanf("%c", &response);

        if (response == 'y' || response == 'Y') {
            printf("Continuing...\n");
        } else {
            printf("Exiting...\n");
            exit(0);
        }

        // clears stdin
        while ((response = getchar()) != EOF && response != '\n');

        // frees board from memory
        free_board(board);
    }

    return 0;
}

/* dynamically allocates and returns a new board, stored as a 2D array of the
 * the enum 'PIECE'. Call before starting a round.
 */
PIECE **init_board(void) {
    PIECE **board;

    // allocates memory for the double pointer
    assert((board = (PIECE **) malloc(ROWS * sizeof(PIECE *))) != NULL);

    // allocates memory for the cols (the pointers)
    for (int row = 0; row < ROWS; row++)
        assert((board[row] = (PIECE *) malloc(COLS * sizeof(PIECE *))) != NULL);

    // sets initial enum value of board
    reset_board(board);

    return board;
}

// frees board from memory
void free_board(PIECE **board) {
    for (int row = 0; row < ROWS; row++) {
        // frees rows
        free(board[row]);
    }

    // frees board
    free(board);
}

// sets all pieces in board to EMPTY
void reset_board(PIECE **board) {
    for (int row = 0; row < ROWS; row++) {
        for (int col = 0; col < COLS; col++) {
            board[row][col] = EMPTY;
        }
    }
}

// plays a turn for either player 1 or 2 of a specified type. 0 is computer, 1 is a real player.
// for player_num, 1 is P1 and 2 is P2.
void play_turn(PIECE **board, short player_num, short type) {
    int col = 0;
    int err_code;

    // handles real player who provides input
    if (type == 1) {
        if (player_num == 1)
            printf("1st player choose columm:\n");
        else
            printf("2nd player choose columm:\n");
        
        scanf("%d", &col);

        while((err_code = place_piece(board, player_num, col - 1)) != 0) {
            p_board(board);
            p_error(err_code);
            scanf("%d", &col);
        }
    // computer decision making
    } else if (type == 0) {
        int should_exit = 0;

        PIECE other_player;

        if (player_num == P1)
            other_player = P2;
        else
            other_player = P1;

        // checks every column to see if the computer can win
        for (col = 0; col < COLS && !should_exit; col++) {
            if (check_win(board, player_num, drop_row(board, col), col) == 1) {
                place_piece(board, player_num, col);
                should_exit = 1;
            }
        }

        if (!should_exit) {
            // block other player from winning if it hasn't gone yet and can
            for (col = 0; col < COLS && !should_exit; col++) {
                
                if (check_win(board, other_player, drop_row(board, col), col) == 1) {
                    place_piece(board, player_num, col);
                    should_exit = 1;
                }
            }

            if (!should_exit) {  
                short counter = 100;

                /* places a random piece in the remaining columns, but avoids going in spaces that would
                 * let the other player win
                 */
                do {
                    col = rand() % COLS;
                } while (drop_row(board, col) == -1 && (check_win(board, other_player, drop_row(board, col) - 1, col) == 1 
                    || counter-- > 0));
                
                place_piece(board, player_num, col);
                should_exit = 1;
            }
        }

    }
}

/* places a piece, specified by the enum value passed. P1 for player 1, and P2
 * for player 2. Calls error_func if inputted column would be out of bounds 
 */
int place_piece(PIECE **board, PIECE piece, int col) {
    int error_code = 0;
    int cur_row = -1;

    // checks if valid colomun to place is
    if (col >= 0 && col < COLS) {

        // moves a piece down the board in the column until the next cell on the board is not empty
        while (cur_row < ROWS - 1 && board[cur_row + 1][col] == EMPTY)  {
            if (cur_row >= 0) {
                board[cur_row][col] = EMPTY;
            }
            
            board[cur_row + 1][col] = piece;

            // "animates" piece by printing and sleeping for a delay as it falls
            // only does this if PIECE_FALL_DELAY > 0
            if (PIECE_FALL_DELAY > 0) {
                p_board(board);
                printf("\n");
                usleep(PIECE_FALL_DELAY);
            }
            
            cur_row++;            
        } 

        // error code 2 if col full
        if (cur_row == -1) {
            error_code = 2;
        } else
            // prints the board once if delay is 0, since it wasn't printed as pieces fell
            if (PIECE_FALL_DELAY == 0) {
                p_board(board);
                printf("\n");
            }

            // checks for a win where the piece was p
            if(check_win(board, piece, cur_row, col) == 1) {
                win(piece);
            }

    // error code 1 if invalid col
    } else {
        error_code = 1;
    }
    
    return error_code;
}

// returns the row num (0 indexed) where the piece would fall, if column full returns -1;
int drop_row(PIECE **board, int col) {
    int cur_row = -1;

    while (cur_row < ROWS - 1 && board[cur_row + 1][col] == EMPTY) {
        cur_row++;
    }

    return cur_row;
}

// checks if there is NUM_ADJ_PIECES exist in a row from the piece at (col, row) where (0,0) is the top left
// corner. Returns 1 if the piece ENUM at that position is part of a winning line else, returns 0. Returns
// -1 on error 
int check_win(PIECE **board, PIECE piece, int row, int col) {
    int has_won = -1;
    int num_in_row = 1;

    if (piece != EMPTY && 0 <= row && row < ROWS && 0 <= col && col < COLS) {
        has_won = 0;
        
        // CHECKS VERTICALLY DOWN
        for (int i = 1; i + row < ROWS && board[i + row][col] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
            num_in_row++;
        }

        // CHECKS VERTICALLY UP
        for (int i = 1; row - i >= 0 && board[row - i][col] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
            num_in_row++;
        }

        if (num_in_row >= NUM_ADJ_PIECES) {
            has_won = 1;

        } else {
            num_in_row = 1;

            // CHECKS HORIZTONALLY RIGHT
            for (int i = 1; i + col < COLS && board[row][col + i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                num_in_row++;
            }

            for (int i = 1; col - i >= 0 && board[row][col - i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                num_in_row++;
            }

            // CHECKS HORIZONTALLY LEFT
            if (num_in_row >= NUM_ADJ_PIECES) {
                has_won = 1;

            } else {
                num_in_row = 1;

                // DOWN RIGHT
                for (int i = 1; col + i < COLS && row + i < ROWS && board[row + i][col + i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                    num_in_row++;
                }

                // UP LEFT
                for (int i = 1; col - i >= 0 && row - i >= 0 && board[row - i][col - i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                    num_in_row++;
                }

                if (num_in_row >= NUM_ADJ_PIECES) {
                    has_won = 1;

                } else {
                    num_in_row = 1;
                    
                    // DOWN LEFT
                    for (int i = 1; col - i >= 0 && row + i < ROWS && board[row + i][col - i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                           num_in_row++;
                    }

                    // UP RIGHT
                    for (int i = 1; col + i < COLS && row - i >= 0 && board[row - i][col + i] == piece && num_in_row < NUM_ADJ_PIECES; i++) {
                        num_in_row++;
                    }

                    if (num_in_row >= NUM_ADJ_PIECES) {
                        has_won = 1;
                    }
                }
            }
        }
    }

    return has_won;
}

// function called on win, where piece is the ENUM type of the piece of the player.
// if called on EMPTY, means draw
void win(PIECE piece) {
    switch (piece)
    {
    case P1: 
        printf("- - - - P1 WON - - - -\n");
        break;

    case P2:
        printf("- - - - P2 WON - - - -\n");
        break;

    case EMPTY:
        printf("- - - - DRAW - - - -\n");
        break;

    default:
        break;
    }

    game_over = 1;
}

// prints board in console
void p_board(PIECE **board) {
    // prints character that clears console
    printf("\033[2J");

    // labels each column with their respective number (non-0 indexed)
    for (int col = 1; col <= COLS; col++) {
        printf("%3d ", col);
    }
    printf("\n");

    // prints the cells of the board. O is printed for P1, X is printed for P2, and nothing is printed if EMPTY
    for (int row = 0; row < ROWS; row++) {
        p_solid_line(COLS);

        for (int col = 0; col < COLS; col++) {
            switch (board[row][col])
            {
            case EMPTY:
                printf("|   ");
                break;
            case P1:
                printf("| O ");
                break;
            case P2:
                printf("| X ");
                break;
            default:
                break;
            }
        }

        printf("|\n");
    }

    p_solid_line(COLS);

    // prints "legs" of board
    printf("|");
    for (int i = 0; i < COLS - 1; i++) {
        printf("    ");
    }
    printf("   |\n");

    printf("|");
    for (int i = 0; i < COLS - 1; i++) {
        printf("    ");
    }
    printf("   |\n");
}

// prints solid line in format "+---+---+" where length corresponds to then number of "---" in the line.
void p_solid_line(int length) {
    for (int i = 0; i < length; i++) {
        printf("+---");
    }

    printf("+\n");
}

// prints num amount of newlines
void p_space(int num) {
    for (int i = 0; i < num; i++)
        printf("\n");
}

// prints to re-choose a number between possible range of cols.
void p_error(int error_code) {
    switch (error_code)
    {
    case 1:
        printf("Invaild column to place piece in. Please pick a number form 1 - %d.\n", COLS);
        break;
    case 2:
        printf("Column full. Please choose a different column 1 - %d.\n", COLS);
    default:
        break;
    }
    
}


