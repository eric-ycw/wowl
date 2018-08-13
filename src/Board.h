#ifndef BOARD_INCLUDED
#define BOARD_INCLUDED

#include <iostream>
#include <string>
#include <vector>
#include <tuple>
#include <algorithm>
#include <assert.h>
#include <SFML\System\Vector2.hpp>

struct Move {
	int from;
	int to;

	Move() {}
	Move(int a, int b) : from(a), to(b) {}

	bool operator==(const Move& m) const {
		return (from == m.from && to == m.to);
	}

	bool operator!=(const Move& m) const {
		return (from != m.from || to != m.to);
	}
};

class Board 
{
	friend class Evaluation;
	friend class Wowl;
	friend class Hash;

public:

	Board() { reserveVectors(); }

	enum Piece { WP = 1, WN = 2, WB = 3, WR = 4, WQ = 5, WK = 6, 
				 BP = -1, BN = -2, BB = -3, BR = -4, BQ = -5, BK = -6 };

	enum Color { WHITE = 1, BLACK = -1 };

	enum Size { BOARD_SIZE = 8 };

	std::tuple<bool, bool, bool, bool> castlingRights;

	sf::Vector2f toCoord(char, char);
	int convertCoord(sf::Vector2f);
	sf::Vector2i convertCoord(int);
	int to64Coord(int) const;
	std::string toNotation(int) const;

	void reserveVectors();

	void setPosition(std::vector<std::string>);
	void setPosition();

	int getTurn() const;
	int getSquarePiece(int) const;

	bool checkLegal(int, int);
	bool checkLegalPawn(int, int, int) const;
	bool checkLegalKing(int, int, int);
	void getLegalMoves();
	void getCaptures();

	bool checkAttack(int, int, const int[]) const;
	bool checkAttackPawn(int, int, const int[], int) const;
	bool checkAttackKnight(int, int, const int[]) const;
	bool checkAttackBishop(int, int, const int[]) const;
	bool checkAttackRook(int, int, const int[]) const;
	bool checkAttackQueen(int, int, const int[]) const;
	bool checkAttackKing(int, int, const int[]) const;
	std::tuple<int, int> getSmallestAttacker(int, int);

	void setKingSquare(int[]);
	bool inCheck(int, int[]);
	bool checkMoveCheck(int, int);
	std::tuple<bool, bool, bool, bool> checkCastling();

	void move(int, int);
	void undo();
	void nullMove();
	void specialMoves(int, int, int, int[]);
	void setEnPassantSquare();

	void resetBoard();
	void outputBoard() const;

private:

	int turn;
	bool castled[2] = { false, false };
	int epSquare;
	int kingSquareWhite, kingSquareBlack;
	int pieceMoves[3][10] = {
		{-10, -20, -11, -9, 0, 0, 0, 0, 0, 0},     //Pawn
		{-21, -19, -12, -8, 21, 19, 12, 8, 0, 0},  //Knight
		{1, -1, 10, -10, 11, 9, -11, -9, 2, -2}     //King
	};

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

	std::vector<Move> legalMoveVec;
	std::vector<Move> captureVec;
	std::vector<Move> moveVec;
};

#endif