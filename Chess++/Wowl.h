#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#define SEARCH_DEPTH 4
#define WIN_SCORE 999999

#include "Evaluation.h"

class Wowl {

public:

	sf::Vector2i bestMove;
	sf::Vector2i priorityMove;

	void orderMoves(Board, std::vector<sf::Vector2i>&);

	/*SEARCH*/
	void IID(Board, std::vector<sf::Vector2i>&, int, int color);
	int negaMax(Board, int, int, int, int, int);

	void findBestMove(Board, int, int);
	
	long perft(Board, int);

private:

};
#endif