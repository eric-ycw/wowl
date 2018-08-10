#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#include "Evaluation.h"
#include "Hash.h"
#include <time.h>

class Wowl {

public:

	enum Search {
		MIN_SEARCH_DEPTH = 5,
		MAX_SEARCH_DEPTH = 50,
		ASPIRATION_WINDOW = 35,
		NULL_MOVE_REDUCTION = 2,
		LMR_STARTING_MOVE = 5,
		LMR_STARTING_DEPTH = 3,
		NO_MOVE = -9
	};

	Move bestMove, finalBestMove, hashMove;
	std::vector<U64> hashPosVec;
	std::vector<U64> tempHashPosVec;

	Hash hashTable;

	/*EVALUATION*/
	int SEE(Board, Evaluation&, int, int) const;
	bool checkThreefold(U64) const;

	/*MOVES*/
	std::vector<Move> IIDmoveOrdering(Board&, Evaluation&, std::vector<Move>, int, int, int);
	void orderMoves(Board&, Evaluation&, std::vector<Move>&, int, U64, int, int);
	void resetMoveHeuristics();

	/*SEARCH*/
	int qSearch(Board, Evaluation&, int, int, int);
	int negaSearch(Board, int, int, int, int, int, bool);
	void ID(Board&, int, int, int);
	int MTDf(Board, int, int, int);

	/*HASH TABLE*/
	int probeHashTable(U64, int, int, int, int);
	void recordHash(U64, int, int, int);
	void ageHash();
	
	long perft(Board, int);

private:

	enum Score {
		WIN_SCORE = 999999,
		DRAW_SCORE = 0,
	};

	enum HashConstants {
		TT_CLEAR_AGE = 8, 
		VAL_UNKWOWN = 0
	};

	int estimate = 0;
	Move killerMoves[2][MAX_SEARCH_DEPTH + 1];
	int historyMoves[2][120][120];
	int negaNodes, qSearchNodes;
	int captures = 0;
};

#endif