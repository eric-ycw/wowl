#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#include "Evaluation.h"
#include "Hash.h"
#include <time.h>

class Wowl {

public:

	enum Search {
		MAX_SEARCH_DEPTH = 50,
		ASPIRATION_WINDOW = 35,
		NULL_MOVE_REDUCTION = 2,
		NO_MOVE = -9
	};

	sf::Vector2i bestMove;
	sf::Vector2i finalBestMove;
	sf::Vector2i hashMove;

	Hash hashTable;

	/*EVALUATION*/
	int SEE(Board, Evaluation&, int, int) const;

	/*MOVES*/
	void orderMoves(Board&, Evaluation&, std::vector<sf::Vector2i>&, int, U64);
	void resetKillerMoves();

	/*SEARCH*/
	void ID(Board, int, int, int);
	int negaSearch(Board, int, int, int, int, int);
	int qSearch(Board, Evaluation&, int, int, int);

	/*HASH TABLE*/
	int probeHashTable(U64, int, int, int);
	void recordHash(U64, int, int, int);
	
	long perft(Board, int);

private:

	enum Score {
		WIN_SCORE = 999999,
		DRAW_SCORE = 0,
	};

	enum HashConstants {
		TT_CLEAR_AGE = 0, 
		VAL_UNKWOWN = 0
	};

	int estimate = 0;
	int killerMoves[2][MAX_SEARCH_DEPTH + 1];
	int negaNodes;
	int qSearchNodes;
	int captures = 0;
};

#endif