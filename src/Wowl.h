#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#define SEARCH_DEPTH 6
#define TT_CLEAR_AGE 6
#define ASPIRATION_WINDOW 50
#define DELTA 980
#define WIN_SCORE 999999

#include "Evaluation.h"
#include "Hash.h"

class Wowl {

public:

	sf::Vector2i bestMove;
	sf::Vector2i priorityMove;
	sf::Vector2i hashMove;

	Hash hashTable;

	/*EVALUATION*/
	int SEE(Board&, int, int);

	void orderMoves(Board, std::vector<sf::Vector2i>&, int, int, U64);

	/*SEARCH*/
	void ID(Board, int, int);
	int negaSearch(Board, int, int, int, int, int);
	int qSearch(Board, int, int, int);
	
	long perft(Board, int);

private:
	int estimate = 0;
	int nodes = 0;
};
#endif