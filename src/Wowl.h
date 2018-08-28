#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#include "Evaluation.h"
#include "Hash.h"
#include <time.h>

class Wowl {

public:

	const Move NO_MOVE = Move(-9, -9);

	enum Search {
		MAX_SEARCH_DEPTH = 50,
		ASPIRATION_WINDOW = 50,
		NULL_MOVE_REDUCTION = 2,
		LMR_STARTING_MOVE = 4,
		LMR_STARTING_DEPTH = 3,
		HISTORY_MAX = 10000
	};

	Move bestMove;
	Move IDMoves[MAX_SEARCH_DEPTH];
	std::vector<U64> hashPosVec;
	std::vector<U64> tempHashPosVec;

	Hash tt;

	void initMVVLVA(const Board& b, const Evaluation& e);
	int SEE(Board&, const Evaluation&, int, int) const;
	bool checkThreefold(const U64) const;

	int pstScore(const Board&, Evaluation&, const Move&, const int);
	std::vector<int> scoreMoves(Board&, Evaluation& e, const std::vector<Move>&, const int, const int, const U64, const bool is_root);
	void pickNextMove(std::vector<Move>&, std::vector<int>&, int);
	void orderCaptures(Board&, std::vector<Move>&);
	void resetMoveHeuristics();
	void reduceHistory();
	
	bool timeOver(clock_t, double);
	int qSearch(Board&, Evaluation&, int, int, int);
	int negaSearch(Board&, int, int, int, int, int, bool, clock_t, double);
	void ID(Board&, const Evaluation&, int, int, double);

	int probeHashTable(const U64, int, int, int, int);
	void recordHash(const U64, int, int, int);
	void ageHash();

	void getPVLine(Board, U64);

	long perft(Board&, Evaluation&, int);

private:

	enum Score {
		WIN_SCORE = 1000000,
		DRAW_SCORE = 0,
	};

	enum HashConstants {
		TT_CLEAR_AGE = 6,
		VAL_UNKWOWN = 0
	};

	int MVVLVAScores[6][6];
	const int futilityMargin[5] = { 0, 300, 350, 600, 650 };

	int bestScore = 0;
	Move PVLine[MAX_SEARCH_DEPTH];
	Move killerMoves[2][MAX_SEARCH_DEPTH + 1];
	int historyMoves[2][6][120];
	int negaNodes, qSearchNodes;
	int failHigh, failHighFirst;
};

#endif