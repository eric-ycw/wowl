#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#define SEARCH_DEPTH 5
#define TT_CLEAR_AGE 8
#define ASPIRATION_WINDOW 35
#define WIN_SCORE 999999

#include "Evaluation.h"
#include "Hash.h"

class Wowl {

public:

	int bestScore = -WIN_SCORE;
	sf::Vector2i bestMove;
	sf::Vector2i hashMove;

	Hash hashTable;

	/*EVALUATION*/
	int SEE(Board, int, int);

	/*MOVES*/
	void orderMoves(Board, std::vector<sf::Vector2i>&, int, int, U64);
	void resetKillerMoves();

	/*SEARCH*/
	void DLS(Board, int, int);
	int negaSearch(Board, int, int, int, int, int);
	int qSearch(Board, int, int, int);

	/*HASH TABLE*/
	void storeMoveInfo(U64, int, int, int, int);
	
	long perft(Board, int);

private:
	int estimate = 0;
	int killerMoves[2][SEARCH_DEPTH + 1];
	int negaNodes;
	int qSearchNodes;
	int captures = 0;
};

#endif