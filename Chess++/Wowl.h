#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#define SEARCH_DEPTH 3
#define WIN_SCORE 999999

#include "Evaluation.h"

class Wowl {

public:

	sf::Vector2i bestMove;

	void orderMoves(Board);
	int negaMax(Board, int, int, int, int);

private:

};
#endif