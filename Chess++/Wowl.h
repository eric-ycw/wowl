#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#define SEARCH_DEPTH 4
#define WIN_SCORE 999999

#include "Evaluation.h"

class Wowl {

public:

	sf::Vector2i bestMove;

	void orderMoves(Board, std::vector<sf::Vector2i>&);
	int negaMax(Board, int, int, int, int);

private:

};
#endif