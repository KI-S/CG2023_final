#include "minesweeper.h"
#include <cstdlib>
#include <ctime>
#include <iostream>


Minesweeper::Minesweeper(int rows, int cols, int depths, int mines) : rows(rows), cols(cols), depths(depths), totalMines(mines) {
  board.resize(rows, std::vector<std::vector<std::int8_t>>(cols, std::vector<std::int8_t>(depths, 0)));
  status.resize(rows, std::vector<std::vector<TileStatus>>(cols, std::vector<TileStatus>(depths, Unvisit)));
  remainStepCount = rows * cols * depths - mines;
  flgLose = false;
  placeMines();
}

bool Minesweeper::onBoard(int row, int col, int depth) {
  if (0 <= row && row < rows && 0 <= col && col < cols && 0 <= depth && depth < depths) {
    return true;
  }
  return false;
}

void Minesweeper::printBoard() {
  std::cout << "board:\n";
  for (int k = 0; k < depths; ++k) {
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        if (status[i][j][k] == Visited)
          std::cout << static_cast<int>(board[i][j][k]) << " ";
        else if (status[i][j][k] == Flagged)
          std::cout << "F ";
        else
          std::cout << "? ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

void Minesweeper::printActualBoard() {
  std::cout << "actual board:\n";
  for (int k = 0; k < depths; ++k) {
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        std::cout << static_cast<int>(board[i][j][k]) << " ";
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

void Minesweeper::printStatus() {
  std::cout << "stats:\n";
  for (int k = 0; k < depths; ++k) {
    for (int i = 0; i < rows; ++i) {
      for (int j = 0; j < cols; ++j) {
        switch (status[i][j][k]) {
          case Unvisit: std::cout << "0 "; break;
          case Flagged: std::cout << "F "; break;
          case Visited: std::cout << "1 "; break;
          default: std::cout << "Bugged "; break;
        }
      }
      std::cout << std::endl;
    }
    std::cout << std::endl;
  }
}

bool Minesweeper::checkWin() {
  return remainStepCount == 0;
}

bool Minesweeper::checkLose() {
    return flgLose;
}

int Minesweeper::selectTile(int row, int col, int depth) { 
  if (status[row][col][depth] == Unvisit) {
    // if player select mine
    if (board[row][col][depth] == -1) {
      // lose
      flgLose = true;
      return 0;
    }
    status[row][col][depth] = Visited;
    if (board[row][col][depth] == 0) {
      traverseTile(row, col, depth);
    }
    return 1;
  }
  return -1;
}

void Minesweeper::traverseTile(int row, int col, int depth) {
  for (int i = row - 1; i <= row + 1; i++) {
    for (int j = col - 1; j <= col + 1; j++) {
      for (int k= depth - 1; k <= depth + 1; k++) {
        if (onBoard(i, j, k) && status[i][j][k] == Unvisit) {
          status[i][j][k] = Visited;
          if (board[i][j][k] == 0) {
            traverseTile(i, j, k);
          }
        }
      }
    }
  }
}

bool Minesweeper::flagTile(int row, int col, int depth) {
  if (status[row][col][depth] != Visited) {
    if (status[row][col][depth] != Flagged) {
      status[row][col][depth] = Flagged;
    } else {
      status[row][col][depth] = Unvisit;
    }
    return true;
  }
  return false;
}

void Minesweeper::placeMines() {
  srand(static_cast<unsigned int>(time(nullptr)));

  int minesPlaced = 0;
  while (minesPlaced < totalMines) {
    int randomRow = rand() % rows;
    int randomCol = rand() % cols;
    int randomDepth = rand() % depths;

    if (board[randomRow][randomCol][randomDepth] != -1) {
      board[randomRow][randomCol][randomDepth] = -1;
      ++minesPlaced;
      for (int i = (randomRow - 1); i <= (randomRow + 1); ++i) {
        for (int j = (randomCol - 1); j <= (randomCol + 1); ++j) {
          for (int k = (randomDepth - 1); k <= (randomDepth + 1); ++k) {
            if (onBoard(i, j, k) && board[i][j][k] != -1) {
              board[i][j][k]++;
            }
          }
        }
      }
    }
  }
}

std::vector<std::vector<std::vector<std::int8_t>>> Minesweeper::getBoard() {
  return board;
}

std::vector<std::vector<std::vector<Minesweeper::TileStatus>>> Minesweeper::getStatus() {
  return status;
}
