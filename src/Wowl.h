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
		ASPIRATION_WINDOW = 36,
		NULL_MOVE_REDUCTION = 2,
		LMR_STARTING_MOVE = 3,
		LMR_STARTING_DEPTH = 3,
		NO_MOVE = -9
	};

	Move bestMove, finalBestMove, hashMove;
	std::vector<U64> hashPosVec;
	std::vector<U64> tempHashPosVec;

	Hash hashTable;

	void initMVVLVA(const Board& b, const Evaluation& e);
	int SEE(Board, Evaluation&, int, int) const;
	bool checkThreefold(const U64) const;

	void staticEvalOrdering(Board&, Evaluation&, std::vector<Move>&, int, int, int);
	std::vector<int> scoreMoves(Board&, const std::vector<Move>&, const int, const U64);
	void pickNextMove(std::vector<Move>&, const std::vector<int>&, int);
	void orderCaptures(Board&, std::vector<Move>&);
	void resetMoveHeuristics();
	
	int qSearch(Board, Evaluation&, int, int, int);
	int negaSearch(Board, int, int, int, int, int, bool);
	void ID(Board&, const Evaluation&, int, int, int);
	int MTDf(Board, int, int, int);

	int probeHashTable(const U64, int, int, int, int);
	void recordHash(const U64, int, int, int);
	void ageHash();
	
	long perft(Board, int);

private:

	enum Score {
		WIN_SCORE = 999999,
		DRAW_SCORE = 0,
	};

	enum HashConstants {
		TT_CLEAR_AGE = 12, 
		VAL_UNKWOWN = 0
	};

	int MVVLVAScores[6][6];
	const int futilityMargin[2] = { 300, 560 };

	int bestScore = 0;
	Move killerMoves[2][MAX_SEARCH_DEPTH + 1];
	int historyMoves[2][6][120];
	int negaNodes, qSearchNodes;
	double fh, fhf;
	int captures = 0;
};

#endif