#ifndef WOWL_INCLUDED
#define WOWL_INCLUDED

#include "Evaluation.h"
#include "Hash.h"
#include <time.h>

class Wowl {

public:

	static constexpr int maxSearchDepth = 50;

	const Move NO_MOVE = Move(-9, -9);

	Move bestMove;
	Move IDMoves[maxSearchDepth];
	std::vector<U64> hashPosVec;
	std::vector<U64> tempHashPosVec;

	Hash tt;

	bool checkThreefold(const U64) const;

	void initMVVLVA(const Board& b, const Evaluation& e);
	int SEE(Board&, const Evaluation&, int, int) const;

	std::vector<int> scoreMoves(Board&, Evaluation& e, const std::vector<Move>&, const int, const int, const U64, const bool isRoot);
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

	long perft(Board&, Evaluation&, int, int);

private:

	static constexpr int mateScore = 100000;

	static constexpr int historyMax = 10000;

	static constexpr int futilityMargin[6] = { 0, 240, 300, 480, 550, 720 };
	static constexpr int deltaMargin = 125;

	static constexpr int ttAgeLimit = 6;
	static constexpr int NO_ENTRY = mateScore * 2;

	int bestScore = 0;
	int pvCounter = 0;
	Move hashMove;
	Move PVLine[maxSearchDepth];
	Move killerMoves[2][maxSearchDepth + 1];
	int MVVLVAScores[6][6];
	int historyMoves[2][6][120];
	int negaNodes, qSearchNodes;
	int failHigh, failHighFirst;
};

#endif