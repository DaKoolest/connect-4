#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

typedef enum {EMPTY = 0, P1, P2} PIECE;

int board_cols = 6; // amount of coloumns the board should have
int board_rows = 7; // amount of rows the board should have
int pieces_to_win = 4; // amount of adjacent alike pieces to win
int piece_fall_delay = 70000; // piece fall delay in microseconds
bool game_over;

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

char *str_lower(const char *string);

int main(int argc, char *argv[]) {
    int turn;

    // array that stores the type that player 0 and 1 are. 0 is computer, 1 is person.
    short players[] = {1, 0};

    // start at 1 to ignore program name argument
    for (int arg = 1; arg < argc; arg += 2) {
        if (strcmp(argv[arg], "--help") == 0) {
            printf("Usage: %s [options]\n\
Options:\n\
  --help                Display this information.\n\
  --player1 <option>    Either select human (1) or computer (0) as player 1.\n\
  --player2 <option>    Either select human (1) or computer (0) as player 2.\n\
  --cols <num>          Set the number of columns on the board.\n\
  --rows <num>          Set the number of rows on the board.\n\
  --win <num>           Set the number of adjacent pieces to win.\n\
  --falldelay <millis>  Set the fall delay of pieces in milliseconds.\n\
\n\
If no arguments are supplied, the default options are given as described in\n\
the source file.\n", argv[0]);
            exit(0);
        }

        if (arg + 1 >= argc) {
            fprintf(stderr, "Required argument not provided.\n");
            exit(1);
        }

        char *param_arg = str_lower(argv[arg + 1]);

        if (strcmp(argv[arg], "--player1") == 0) {
            if (strcmp(param_arg, "0") == 0 || strcmp(param_arg, "com") == 0
                    || strcmp(param_arg, "computer") == 0 || strcmp(param_arg, "cpu") == 0) {
                players[0] = 0;
            } else if (strcmp(param_arg, "1") == 0 || strcmp(param_arg, "person") == 0
                    || strcmp(param_arg, "human") == 0) {
                players[0] = 1;
            } else {
                fprintf(stderr, "Invalid player1 argument.\n");
                exit(1);
            }
        } else if (strcmp(argv[arg], "--player2") == 0) {
            if (strcmp(param_arg, "0") == 0 || strcmp(param_arg, "com") == 0
                    || strcmp(param_arg, "computer") == 0 || strcmp(param_arg, "cpu") == 0) {
                players[1] = 0;
            } else if (strcmp(param_arg, "1") == 0 || strcmp(param_arg, "person") == 0
                    || strcmp(param_arg, "human") == 0) {
                players[1] = 1;
            } else {
                fprintf(stderr, "Invalid player2 argument.\n");
                exit(1);
            }
        } else if (strcmp(argv[arg], "--cols") == 0) {
            board_cols = atoi(param_arg);
        } else if (strcmp(argv[arg], "--rows") == 0) {
            board_rows = atoi(param_arg);
        } else if (strcmp(argv[arg], "--win") == 0) {
            pieces_to_win = atoi(param_arg);
        } else if (strcmp(argv[arg], "--falldelay") == 0) {
            piece_fall_delay = atoi(param_arg) * 1000; // falldelay argument in milliseconds
        } else {
            fprintf(stderr, "Invalid parameter.\n");
            exit(1);
        }
    }


    while (1) {
        // resets game
        PIECE **board = init_board();
        turn = 0;
        game_over = false;
        
        printf("\033[2J"); // clears console
        p_board(board); // prints empty board

        srand(time(NULL));
        
        // runs game
        while (turn < board_cols * board_rows && !game_over) {
            play_turn(board, (turn % 2) + 1, players[turn % 2]);
            turn++;
        }

        // if turn reached the max possible turns, trigger a draw since no one has won yet
        if (turn == board_cols * board_rows) {
            printf("Turn: %d\n", turn);
            // winning on EMPTY calls tie
            win(EMPTY);
        }

        int response;

        // clears stdin
        while ((response = getchar()) != EOF && response != '\n');

        printf("Enter 'y' to play again or anything else to exit:\n");
        response = getchar();

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
    assert((board = (PIECE **) malloc(board_rows * sizeof(PIECE *))) != NULL);

    // allocates memory for the cols (the pointers)
    for (int row = 0; row < board_rows; row++)
        assert((board[row] = (PIECE *) malloc(board_cols * sizeof(PIECE *))) != NULL);

    // sets initial enum value of board
    reset_board(board);

    return board;
}

// frees board from memory
void free_board(PIECE **board) {
    for (int row = 0; row < board_rows; row++) {
        // frees rows
        free(board[row]);
    }

    // frees board
    free(board);
}

// sets all pieces in board to EMPTY
void reset_board(PIECE **board) {
    for (int row = 0; row < board_rows; row++) {
        for (int col = 0; col < board_cols; col++) {
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

        int digit;
        bool is_num = true;
        while ((digit = getchar()) != EOF && digit != '\n') {
            // if digit is 0-9, and stdin is number, shift digits to
            // left by 1 and add new digit. otherwise, if not line feed,
            // carriage return, or null string, stdin is not a number,
            // so set num to 0.
            if (is_num && '0' <= digit && digit <= '9') {
                col = 10*col + digit - '0';
            } else if (is_num && digit != 10 && digit != 13) {
                col = 0;
                is_num = false;
            }

        }

        while((err_code = place_piece(board, player_num, col - 1)) != 0) {
            p_board(board);
            p_error(err_code);

            col = 0;
            is_num = true;
            while (read(0, &digit, 1) > 0 && digit != 10) {
                // if digit is 0-9, and stdin is number, shift digits to
                // left by 1 and add new digit. otherwise, if not line feed,
                // carriage return, or null string, stdin is not a number,
                // so set num to 0.
                if (is_num && '0' <= digit && digit <= '9') {
                    col = 10*col + digit - '0';
                } else if (is_num && digit != 10 && digit != 13) {
                    col = 0;
                    is_num = false;
                }

            }
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
        for (col = 0; col < board_cols && !should_exit; col++) {
            if (check_win(board, player_num, drop_row(board, col), col) == 1) {
                place_piece(board, player_num, col);
                should_exit = 1;
            }
        }

        if (!should_exit) {
            // block other player from winning if it hasn't gone yet and can
            for (col = 0; col < board_cols && !should_exit; col++) {
                
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
                    col = rand() % board_cols;
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
    if (col >= 0 && col < board_cols) {

        // moves a piece down the board in the column until the next cell on the board is not empty
        while (cur_row < board_rows - 1 && board[cur_row + 1][col] == EMPTY)  {
            if (cur_row >= 0) {
                board[cur_row][col] = EMPTY;
            }
            
            board[cur_row + 1][col] = piece;

            // "animates" piece by printing and sleeping for a delay as it falls
            // only does this if piece_fall_delay > 0
            if (piece_fall_delay > 0) {
                p_board(board);
                printf("\n");
                usleep(piece_fall_delay);
            }
            
            cur_row++;            
        } 

        // error code 2 if col full
        if (cur_row == -1) {
            error_code = 2;
        } else {
            // prints the board once if delay is 0, since it wasn't printed as pieces fell
            if (piece_fall_delay == 0) {
                p_board(board);
                printf("\n");
            }

            // checks for a win where the piece was p
            if(check_win(board, piece, cur_row, col) == 1) {
                win(piece);
            }
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

    while (cur_row < board_rows - 1 && board[cur_row + 1][col] == EMPTY) {
        cur_row++;
    }

    return cur_row;
}

// checks if there is pieces_to_win exist in a row from the piece at (col, row) where (0,0) is the top left
// corner. Returns 1 if the piece ENUM at that position is part of a winning line else, returns 0. Returns
// -1 on error 
int check_win(PIECE **board, PIECE piece, int row, int col) {
    int has_won = -1;
    int num_in_row = 1;

    if (piece != EMPTY && 0 <= row && row < board_rows && 0 <= col && col < board_cols) {
        has_won = 0;
        
        // CHECKS VERTICALLY DOWN
        for (int i = 1; i + row < board_rows && board[i + row][col] == piece && num_in_row < pieces_to_win; i++) {
            num_in_row++;
        }

        // CHECKS VERTICALLY UP
        for (int i = 1; row - i >= 0 && board[row - i][col] == piece && num_in_row < pieces_to_win; i++) {
            num_in_row++;
        }

        if (num_in_row >= pieces_to_win) {
            has_won = 1;

        } else {
            num_in_row = 1;

            // CHECKS HORIZTONALLY RIGHT
            for (int i = 1; i + col < board_cols && board[row][col + i] == piece && num_in_row < pieces_to_win; i++) {
                num_in_row++;
            }

            for (int i = 1; col - i >= 0 && board[row][col - i] == piece && num_in_row < pieces_to_win; i++) {
                num_in_row++;
            }

            // CHECKS HORIZONTALLY LEFT
            if (num_in_row >= pieces_to_win) {
                has_won = 1;

            } else {
                num_in_row = 1;

                // DOWN RIGHT
                for (int i = 1; col + i < board_cols && row + i < board_rows && board[row + i][col + i] == piece && num_in_row < pieces_to_win; i++) {
                    num_in_row++;
                }

                // UP LEFT
                for (int i = 1; col - i >= 0 && row - i >= 0 && board[row - i][col - i] == piece && num_in_row < pieces_to_win; i++) {
                    num_in_row++;
                }

                if (num_in_row >= pieces_to_win) {
                    has_won = 1;

                } else {
                    num_in_row = 1;
                    
                    // DOWN LEFT
                    for (int i = 1; col - i >= 0 && row + i < board_rows && board[row + i][col - i] == piece && num_in_row < pieces_to_win; i++) {
                           num_in_row++;
                    }

                    // UP RIGHT
                    for (int i = 1; col + i < board_cols && row - i >= 0 && board[row - i][col + i] == piece && num_in_row < pieces_to_win; i++) {
                        num_in_row++;
                    }

                    if (num_in_row >= pieces_to_win) {
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

    game_over = true;
}

// prints board in console
void p_board(PIECE **board) {
    // prints character that clears console
    printf("\033[2J");

    // labels each column with their respective number (non-0 indexed)
    for (int col = 1; col <= board_cols; col++) {
        printf("%3d ", col);
    }
    printf("\n");

    // prints the cells of the board. O is printed for P1, X is printed for P2, and nothing is printed if EMPTY
    for (int row = 0; row < board_rows; row++) {
        p_solid_line(board_cols);

        for (int col = 0; col < board_cols; col++) {
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

    p_solid_line(board_cols);

    // prints "legs" of board
    printf("|");
    for (int i = 0; i < board_cols - 1; i++) {
        printf("    ");
    }
    printf("   |\n");

    printf("|");
    for (int i = 0; i < board_cols - 1; i++) {
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
        printf("Invaild column to place piece in. Please pick a number from 1 - %d.\n", board_cols);
        break;
    case 2:
        printf("Column full. Please choose a different column from 1 - %d.\n", board_cols);
    default:
        break;
    }
    
}

char *str_lower(const char *string) {
    int string_length = strlen(string);
    char *lower = malloc(string_length + 1); // +1 for null char
    if (!lower) {
        return NULL;
    }

    for (int i = 0; i < string_length; i++) {
        lower[i] = tolower(string[i]);
    }

    return lower;
}

