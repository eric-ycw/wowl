#ifndef BOARD_INCLUDED
#define BOARD_INCLUDED

//Board
#define BOARD_SIZE 8
//Pieces
#define WP 1
#define WN 2
#define WB 3
#define WR 4
#define WQ 5
#define WK 6
#define BP -1
#define BN -2
#define BB -3
#define BR -4
#define BQ -5
#define BK -6
//Turn
#define WHITE 1
#define BLACK -1

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <SFML\System\Vector2.hpp>

class Board 
{
	friend class Evaluation;
	friend class Wowl;

public:

	/*CONVERTERS*/
	sf::Vector2f toCoord(char, char);
	int convertCoord(sf::Vector2f);
	sf::Vector2i convertCoord(int);
	std::string toNotation(sf::Vector2f vec);
	int to64Coord(int);

	/*VECTORS*/
	void reserveVectors();

	/*SETTERS*/
	void setPosition(std::vector<std::string>);
	void setPosition();
	void setPositionFromArray();

	/*GETTERS*/
	int getTurn();
	int getSquarePiece(int);

	/*LEGALITY*/
	bool checkLegal(int, int);
	void getLegalMoves();

	/*ATTACKS*/
	bool checkAttack(int, int, int[]);

	/*KING*/
	bool checkKing(int, int[]);
	bool checkMoveCheck(int, int);
	std::tuple<bool, bool, bool, bool> checkCastling();

	/*MOVES*/
	void move(int, int);
	void move(int, int, int[]);
	void undo();
	void tempUndo();
	void specialMoves(int, int, int, int[]);

	/*BOARD*/
	void resetBoard();
	void outputBoard();
	void copyBoard();

private:

	/*VARIABLES*/
	int turn;
	bool castled[2] = { false, false };

	int mailbox[120] = {
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, BR, BN, BB, BQ, BK, BB, BN, BR, -9,
		-9, BP, BP, BP, BP, BP, BP, BP, BP, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9, WP, WP, WP, WP, WP, WP, WP, WP, -9,
		-9, WR, WN, WB, WQ, WK, WB, WN, WR, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9
	};

	const int start[120] = {
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, BR, BN, BB, BQ, BK, BB, BN, BR, -9,
		-9, BP, BP, BP, BP, BP, BP, BP, BP, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9, WP, WP, WP, WP, WP, WP, WP, WP, -9,
		-9, WR, WN, WB, WQ, WK, WB, WN, WR, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9
	};

	const int mailbox64[8][8] = {
	{ 21, 22, 23, 24, 25, 26, 27, 28 },
	{ 31, 32, 33, 34, 35, 36, 37, 38 },
	{ 41, 42, 43, 44, 45, 46, 47, 48 },
	{ 51, 52, 53, 54, 55, 56, 57, 58 },
	{ 61, 62, 63, 64, 65, 66, 67, 68 },
	{ 71, 72, 73, 74, 75, 76, 77, 78 },
	{ 81, 82, 83, 84, 85, 86, 87, 88 },
	{ 91, 92, 93, 94, 95, 96, 97, 98 }
	};

	int temp[120] = {
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, BR, BN, BB, BQ, BK, BB, BN, BR, -9,
		-9, BP, BP, BP, BP, BP, BP, BP, BP, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9,  0,  0,  0,  0,  0,  0,  0,  0, -9,
		-9, WP, WP, WP, WP, WP, WP, WP, WP, -9,
		-9, WR, WN, WB, WQ, WK, WB, WN, WR, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9,
		-9, -9, -9, -9, -9, -9, -9, -9, -9, -9
	};

	std::vector<sf::Vector2i> moveVec;
	std::vector<sf::Vector2i> legalMoveVec;
	std::vector<sf::Vector2i> attackMoveVec;
};
#endif