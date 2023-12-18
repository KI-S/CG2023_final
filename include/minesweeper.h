#pragma once
#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <vector>

class Minesweeper {
private:
	int rows;
	int cols;
	int depths;
	int totalMines;
    int remainStepCount;

	/*
	* Unvisit: can only visit at this state
	* Flagged: player put flag, can't visit
	* Visited: player has visited this tile, player can see the content, can't visit
	*/
	enum TileStatus : int8_t { Unvisit, Flagged, Visited };
    /*
	* -1 : mine at here
	* else : record how many mines nearby
	*/
	std::vector<std::vector<std::vector<std::int8_t>>> board;
	std::vector<std::vector<std::vector<TileStatus>>> status;

public:
	Minesweeper(int rows, int cols, int depths, int mines);
	int selectTile(int row, int col, int depth);
	bool flagTile(int row, int col, int depth);
	
	std::vector<std::vector<std::vector<std::int8_t>>> getBoard();
    std::vector<std::vector<std::vector<TileStatus>>> getStatus();
	void printBoard();
    void printActualBoard();
    void printStatus();
    bool checkWin();

private:
	void traverseTile(int row, int col, int depth);
	bool onBoard(int row, int col, int depth);
	void placeMines();
};

#endif  // MINESWEEPER_H