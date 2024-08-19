#include <ArrayList.h>
#include <ESP32Servo.h>
  #define dirPin 4
  #define stepPin 2
  #define stepsPerRevolution 100
// https://roboticsproject.readthedocs.io/en/latest/ConnectFourAlgorithm.html
//servo 0, photoresistors A0-A6, dirPin 4, stepPin 2
Servo servo;
const int ROW_COUNT = 6;
const int COLUMN_COUNT = 7;
int presistors[7] = {A0, A1, A2, A3, A4, A5, A6};
int game_over = 0;
int turn = 0;
int position = 0;
int previous = 0;

// pieces decided ahead of time:
// int 1 for red pieces
// int 2 for yellow pieces
int Player_1 = 0;
int Player_2 = 1;

int Piece_1 = 1;
int Piece_2 = 2;

int WINDOW_LENGTH = 4;

int board[6][7] = {{0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0},
                   {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0},
                   {0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 0, 0}};

int window[4] = {0, 0, 0, 0};

// initializing 6x7 board as an array of 0s
void create_board(int board[6][7]) {
  for (int i = 0; i < 6; i++) {
    for (int j = 0; j < 7; j++) {
      board[i][j] = 0;
    }
  }
}

void pretty_print_board(int board[6][7]) {

  Serial.println(" 0  1  2  3  4  5  6");

  for (int i = 5; i >= 0; i--) {
    String row_str = "";

    for (int j = 0; j < 7; j++) {
      int token = board[i][j];

      if (token == 1) {
        row_str += " R ";
      } else if (token == 2) {
        row_str += " Y ";
      } else {
        row_str += "   ";
      }
    }

    Serial.println(row_str);
  }

  // Add a delay before printing the board again (simulate board refresh rate)
  delay(1000);
}

void drop_piece(int board[6][7], int row, int col, int piece) {
  board[row][col] = piece;
}

int is_valid_location(int board[6][7], int col) {
  return board[ROW_COUNT - 1][col] == 0;
}

int get_next_open_row(int board[6][7], int col) {
  for (int r = 0; r < ROW_COUNT; r++) {
    if (board[r][col] == 0) {
      return r;
    }
  }
  return -1; // returns -1 if there are no moves left that are possible
}

int count(int windows[4], int piece) {
  int count = 0;
  for (int i = 0; i < 4; i++) {
    if (windows[i] == piece) {
      count++;
    }
  }
  return count;
}

void printAL(ArrayList<int> temp) {
    for (int i = 0; i < temp.size(); i++) {
      Serial.print(temp.get(i) + " ");
    }
}

ArrayList<int> get_valid_locations(int board[6][7]) {
  ArrayList<int> valid_locations;
  for (int col = 0; col < COLUMN_COUNT; col++) {
    if (is_valid_location(board, col)) {
      valid_locations.add(col);
    }
  }
  // printAL(valid_locations);

  return valid_locations;
}

int evaluate_window(int window[4], int piece) {
  int score = 0;
  int opp_piece = Piece_1;
  if (piece == Piece_1) {
    opp_piece = Piece_2;
  }

  if (count(window, piece) == 4) {
    score += 100;
  } else if (count(window, piece) == 3 && count(window, 0) == 1) {
    score += 5;
  } else if (count(window, piece) == 2 && count(window, 0) == 2) {
    score += 2;
  } else if (count(window, opp_piece) == 3 && count(window, 0) == 1) {
    score -= 4;
  }
  return score;
}

int score_position(int board[ROW_COUNT][COLUMN_COUNT], int piece) {
  int score = 0;

  // Score centre column
  int centre_count = 0;
  for (int r = 0; r < ROW_COUNT; r++) {
    centre_count += (board[r][COLUMN_COUNT / 2] == piece) ? 1 : 0;
  }
  score += centre_count * 3;

  // Score horizontal positions
  for (int r = 0; r < ROW_COUNT; r++) {
    for (int c = 0; c <= COLUMN_COUNT - WINDOW_LENGTH; c++) {
      // Create a horizontal window of 4
      int window[WINDOW_LENGTH] = {board[r][c], board[r][c + 1], board[r][c + 2], board[r][c + 3]};
      score += evaluate_window(window, piece);
    }
  }

  // Score vertical positions
  for (int c = 0; c < COLUMN_COUNT; c++) {
    for (int r = 0; r <= ROW_COUNT - WINDOW_LENGTH; r++) {
      // Create a vertical window of 4
      int window[WINDOW_LENGTH] = {board[r][c], board[r + 1][c], board[r + 2][c], board[r + 3][c]};
      score += evaluate_window(window, piece);
    }
  }

  // Score positive diagonals
  for (int r = 0; r <= ROW_COUNT - WINDOW_LENGTH; r++) {
    for (int c = 0; c <= COLUMN_COUNT - WINDOW_LENGTH; c++) {
      // Create a positive diagonal window of 4
      int window[WINDOW_LENGTH] = {board[r][c], board[r + 1][c + 1], board[r + 2][c + 2], board[r + 3][c + 3]};
      score += evaluate_window(window, piece);
    }
  }

  // Score negative diagonals
  for (int r = WINDOW_LENGTH - 1; r < ROW_COUNT; r++) {
    for (int c = 0; c <= COLUMN_COUNT - WINDOW_LENGTH; c++) {
      // Create a negative diagonal window of 4
      int window[WINDOW_LENGTH] = {board[r][c], board[r - 1][c + 1], board[r - 2][c + 2], board[r - 3][c + 3]};
      score += evaluate_window(window, piece);
    }
  }

  return score;
}

int winning_move(int board[6][7], int piece) {

  // horizontal location check
  for (int c = 0; c < COLUMN_COUNT - 3; c++) {
    for (int r = 0; r < ROW_COUNT; r++) {
      if (board[r][c] == piece && board[r][c + 1] == piece &&
          board[r][c + 2] == piece && board[r][c + 3] == piece) {
        return 1;
      }
    }
  }

  // vertical location check
  for (int c = 0; c < COLUMN_COUNT; c++) {
    for (int r = 0; r < ROW_COUNT - 3; r++) {
      if (board[r][c] == piece && board[r + 1][c] == piece &&
          board[r + 2][c] == piece && board[r + 3][c] == piece) {
        return 1;
      }
    }
  }

  // positive diagonal location check
  for (int c = 0; c < COLUMN_COUNT - 3; c++) {
    for (int r = 0; r < ROW_COUNT - 3; r++) {
      if (board[r][c] == piece && board[r + 1][c + 1] == piece &&
          board[r + 2][c + 2] == piece && board[r + 3][c + 3] == piece) {
        return 1;
      }
    }
  }

  // check negative location check
  for (int c = 0; c < COLUMN_COUNT - 3; c++) {
    for (int r = 3; r < ROW_COUNT; r++) {
      if (board[r][c] == piece && board[r - 1][c + 1] == piece &&
          board[r - 2][c + 2] == piece && board[r - 3][c + 3] == piece) {
        return 1;
      }
    }
  }
  return 0;
}

int is_terminal_node(int board[6][7]) {
  return (winning_move(board, Piece_1) || winning_move(board, Piece_2) || get_valid_locations(board).size() == 0);
}

int pick_best_move(int board[6][7], int piece) {
  ArrayList<int> valid_locations = get_valid_locations(board);
  // printAL(valid_locations);
  int best_score = -1000;
  int best_col = valid_locations.get(0);
  for (int col = 0; col < valid_locations.size(); col++) {
    int row = get_next_open_row(board, valid_locations.get(col));
    int temp_board[6][7];
    memcpy(temp_board, board, sizeof(board));

    drop_piece(temp_board, row, valid_locations.get(col), piece);
    int score = score_position(temp_board, piece);

    if (score > best_score) {
      best_score = score;
      best_col = valid_locations.get(col);
    }
  }
  return best_col;
}

int minimax(int board[ROW_COUNT][COLUMN_COUNT], int depth, int alpha, int beta, bool maximisingPlayer) {
    // Get valid locations to play
    int valid_locations[COLUMN_COUNT]; // Assuming COLS is defined
    ArrayList<int> num_valid_locations = get_valid_locations(board);

    // Check if terminal node reached
    bool is_terminal = is_terminal_node(board);
    if (depth == 0 || is_terminal) {
        if (is_terminal) {
            // Check if bot wins
            if (winning_move(board, Piece_2)) {
                return 10000000; // Bot wins
            }
            // Check if human wins
            else if (winning_move(board, Piece_1)) {
                return -10000000; // Human wins
            }
            else {
                return 0; // Draw
            }
        }
        else {
            return score_position(board, Piece_2); // Return bot's score
        }
    }

    if (maximisingPlayer) {
        int value = -1000000000; // Negative infinity
        int column = valid_locations[random(0, num_valid_locations.size())]; // Random column
        for (int i = 0; i < num_valid_locations.size(); i++) {
            int col = valid_locations[i];
            int row = get_next_open_row(board, col);
            int b_copy[ROW_COUNT][COLUMN_COUNT];
            memcpy(b_copy, board, sizeof(b_copy)); // Create a copy of the board
            drop_piece(b_copy, row, col, Piece_2); // Drop piece
            int new_score = minimax(b_copy, depth - 1, alpha, beta, false);
            if (new_score > value) {
                value = new_score;
                column = col;
            }
            alpha = max(alpha, value);
            if (alpha >= beta) {
                break;
            }
        }
        return column;
    }
    else {
        int value = 1000000000; // Positive infinity
        int column = valid_locations[random(0, num_valid_locations.size())]; // Random column
        for (int i = 0; i < num_valid_locations.size(); i++) {
            int col = valid_locations[i];
            int row = get_next_open_row(board, col);
            int b_copy[ROW_COUNT][COLUMN_COUNT];
            memcpy(b_copy, board, sizeof(b_copy)); // Create a copy of the board
            drop_piece(b_copy, row, col, Piece_1); // Drop piece
            int new_score = minimax(b_copy, depth - 1, alpha, beta, true);
            if (new_score < value) {
                value = new_score;
                column = col;
            }
            beta = min(beta, value);
            if (alpha >= beta) {
                break;
            }
        }
        return column;
    }
}

int get_Pin(const int presistors[7]) {
  for (int i = 0; i < 7; i++) {
    if (analogRead(presistors[i]) > 8) {
      return i;
    }
  }
  return -1;
}

void setup() {
    Serial.begin(9600);
    create_board(board);
    myservo.setPeriodHertz(50);
    myservo.attach(14);
    pinMode(stepPin, OUTPUT);
    pinMode(dirPin, OUTPUT);
    randomSeed(analogRead(5));
  }

void loop() {
  int col;
  int row;
  while (!game_over) {
    if (turn == Player_1) {
      while (1) {
        int col = get_Pin(presistors); // Read the integer input from serial
        if (col != -1) {
          break;
        }
      }

      // Now 'col' contains the valid column value selected by the user
      Serial.print("Selected column: ");
      Serial.println(col);

      // Here you can perform further actions with the selected column value

      // Add a delay to avoid continuous prompt
      delay(1000);

      if (is_valid_location(board, col)) {
        row = get_next_open_row(board, col);
        drop_piece(board, row, col, Piece_1);
      }
}


        if (winning_move(board, Piece_1)) {
          game_over = 1;
          Serial.println("Human Wins!");
        }
      }
      turn = Player_2;
    }

    if ((turn == Player_2) && !game_over) {
      for (position = 60; position >= 15; position -= 1) {
            servo.write(position);
            delay(250);
          }
      int col, minimax_score;
      minimax_score = -1000000;

      // Calculate the best move using minimax algorithm
      for (int c = 0; c < 7; c++) {
        if (is_valid_location(board, c)) {
          int row = get_next_open_row(board, c);
          char original_piece = board[row][c];
          board[row][c] = Piece_2;

          int score = minimax(board, 4, -1000000, 1000000, 1);
          // int score = pick_best_move(board, Piece_2); // Depth 4 for minimax

          board[row][c] = original_piece;

          if (score > minimax_score) {
            minimax_score = score;
            col = c;
          }
        }
      }

      if (is_valid_location(board, col)) {
        int row = get_next_open_row(board, col);
          digitalWrite(dirPin, HIGH);
          for (int i = 0; i < col * stepsPerRevolution; i++) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(100);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(100);
          }
          delay(250);
          for (position = 15; position <= 60; position += 1) {
            servo.write(position);
            delay(250);
          }
          drop_piece(board, row, col, Piece_2);

        int position = 0;
        switch (col) {
          case 1:
    // do something when var equals 1
            position = 165;
            break;
          case 2:
    // do something when var equals 2
            position = 340;
            break;
          case 3:
    // do something when var equals 3
            position = 520;
            break;
          case 4:
    // do something when var equals 4
            position = 692;
            break;
          case 5:
    // do something when var equals 5
            position = 868;
            break;
          case 6:
    // do something when var equals 6
            position = 1035;
            break;          
          default:
    // if nothing else matches, do the default
    // default is optional
            position = 0;
            break;
        }

          delay(2000);
          for (position = 60; position >= 15; position -= 1) {
            servo.write(position);
            delay(250);
          }
          delay(250);
          digitalWrite(dirPin, LOW);
          for (int i = 0; i < col * stepsPerRevolution; i++) {
            digitalWrite(stepPin, HIGH);
            delayMicroseconds(100);
            digitalWrite(stepPin, LOW);
            delayMicroseconds(100);
          }
          delay(250);

        if (winning_move(board, Piece_2)) {
          game_over = true;
          Serial.println("AI Wins!");
        }
      }
        turn = Player_1;
      }
    Serial.println("");
    pretty_print_board(board);
    Serial.println("");

    if (game_over) {
      Serial.println("Game finished!");
    }
  }

}