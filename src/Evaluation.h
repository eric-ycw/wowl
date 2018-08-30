#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

class Evaluation {

	friend class Wowl;

public:

	double phase;

	double getPhase(const Board&);
	int blockedPawns(const Board&);
	int doubledAndIsolatedPawns(const Board&, int);
	int connectedPawns(const Board&, int);
	int backwardPawns(const Board&, int);
	int passedPawns(const Board&, int);

	int knightOutpost(const Board&, int, int);
	int rookBehindPassed(const Board&, int, int);
	int trappedRook(const Board&, int, int);

	int flipTableValue(int) const;
	int PST(const Board&, int, int);

	int mobilityKnight(const Board&, int, int);
	int mobilitySlider(const Board&, int, int, int);

	int spaceArea(const Board&, int);

	int kingShelter(Board&, int);
	int kingDangerProximity(Board&, int);

	int pawnPushThreat(const Board&, int, int);
	int pawnAttackThreat(const Board&, int, int);

	int piecesEval(const Board&, int);

	bool attackedByEnemyPawn(const Board&, int, int);
	int isOpenFile(const Board&, int);
	int isPassed(const Board&, int, int);

	int totalEvaluation(Board&, int, int[]);
	int lazyEvaluation(const Board&, int);

private:

	enum pawnStructValue {
		DOUBLED_P_PENALTY = -12, ISOLATED_P_PENALTY = -8,
		SUPPORTED_P_BONUS = 12, PHALANX_P_BONUS = 8,
		BACKWARD_P_PENALTY = -14,
		PASSED_P_BONUS = 3
	};

	enum piecesBonusValue {
		MINOR_BEHIND_PAWN_BONUS = 12,
		KNIGHT_OUTPOST_BONUS = 24,
		BISHOP_PAIR_BONUS = 35,
		ROOK_BEHIND_PASSED_P_BONUS = 30,
		TRAPPED_ROOK_PENALTY = -12
	};

	enum kingAttackerValue {
		KNIGHT_KING_ATTACKER = 50,
		BISHOP_KING_ATTACKER = 35,
		ROOK_KING_ATTACKER = 30,
		QUEEN_KING_ATTACKER = 12
	};

	enum positionValue {
		CLOSED_POSITION_BONUS = 3,
		R_OPEN_FILE_BONUS = 20,
		K_OPEN_FILE_PENALTY = -28, K_P_SHIELD_PENALTY = -24
	};

	enum threatValue {
		PAWN_PUSH_THREAT_PENALTY = -28,
		PAWN_ATTACK_THREAT_PENALTY = -80
	};

	const int kingRing[25] = {
		0, 1, -1, 10, -10, 11, 9, -11, 9,
		2, -2, 20, -20, 22, 18, -22, 18,
		12, 8, -12, -8, 21, -21, 19, -19
	};

	const int pieceValues[6]
	{
		120, 480, 512, 802, 1520, 40000
	};
	const int knightMobilityTable[9]
	{
		-50, -35, -10, -2, 5, 10, 15, 20, 25
	};

	const int bishopMobilityTable[14]
	{
		-35, -20, -5, 0, 5, 10, 15, 20, 24, 28, 30, 34, 38, 42
	};

	const int rookMobilityTable[15]
	{
		-25, -18, -12, -8, -4, 2, 7, 12, 16, 20, 23, 26, 29, 31, 34
	};

	const int queenMobilityTable[28]
	{
		-30, -15, -10, -5, 0, 5, 9, 12, 15, 18, 21, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 45, 45, 46, 46, 47, 47
	};

	const int pawnTable[64]
	{
		99, 99, 99, 99, 99, 99, 99, 99,
		40, 50, 50, 50, 50, 50, 50, 40,
		25, 30, 35, 45, 45, 35, 30, 25,
		10, 10, 20, 25, 25, 20, 10, 10,
		 0,  0, 17, 22, 22, 17,  0,  0,
		 0,  0, 10, 12, 12, 10,  0,  0,
		 0,  5,  5,  0,  0,  5,  5,  0,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	const int knightTable[64]
	{
		-90,-50,-30,-20,-20,-30,-50,-90,
		-50,-15,  0,  5,  5,  0,-15,-50,
		 -5, 20, 40, 45, 45, 40, 20, -5,
		-20, 10, 20, 40, 40, 20, 10,-20,
		-20, 10, 15, 40, 40, 15, 10,-20,
		-40,-12,  0,  5,  5,  0,-12,-40,
		-50,-25,-12, -5, -5,-12,-25,-50,
		-80,-30,-30,-30,-30,-30,-30,-80,
	};
	const int bishopTable[64]
	{
		-25,-10,-15,-25,-25,-15,-10,-20,
		-20, 12,  5,  0,  0,  5, 12,-20,
		-15, 10,  8,  0,  0,  8, 10,-15,
		-10, 20, 15,  5,  5, 15, 20,-10,
		 -5, 20, 15,  8,  8, 15, 20, -5,
		 -5, 20, 15,  8,  8, 15, 20, -5,
		-15, 20, 10,  0,  0, 10, 20,-15,
		-25,-10,-20,-25,-25,-20,-10,-25,
	};
	const int rookTable[64]
	{
		-20,-10, -5,  0,  0, -5,-10,-20,
		 -8,  5, 10, 15, 15, 10,  5, -8,
		-15, -5,  0,  0,  0,  0, -5,-15,
		-15, -5,  0,  0,  0,  0, -5,-15,
		-15, -5,  0,  0,  0,  0, -5,-15,
		-15, -8,  0,  0,  0,  0, -8,-15,
		-15, -8,  0,  0,  0,  0, -8,-15,
		-15,-12,-12, -8, -8,-12,-12,-15
	};
	const int queenTable[64]
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		 -5,  3,  3,  3,  3,  3,  3, -5,
		  0,  5,  5,  5,  5,  5,  5,  0,
		  0,  5,  8, 10, 10,  8,  5,  0,
		  0,  5,  8, 10, 10,  8,  5,  0,
		  0,  5,  8,  8,  8,  8,  5,  0,
		 -5,  5,  8,  8,  8,  8,  5, -5,
		-20,-10,-10,  0,  0,-10,-10,-20
	};
	const int kingTable[64]
	{
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-80,-80,-80,-80,-80,-80,-80,-80,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-30,-70,-70,-50,-50,-70,-70,-30,
		  0,  0,-30,-50,-50,-30,  0,  0,
		  0,  0,-15,-30,-30,-15,  0,  0
	};
	const int kingEndTable[64]
	{
		 0, 20, 30, 30, 30, 30, 20,  0,
		20, 40, 50, 50, 50, 50, 40, 20,
		40, 60, 65, 65, 65, 65, 60, 40,
		45, 60, 60, 70, 70, 60, 60, 45,
		45, 60, 60, 65, 65, 60,-60, 45,
		40, 50, 60, 60, 60, 60, 50, 40,
		20, 40, 50, 50, 50, 50, 40, 20,
		 0, 20, 30, 45, 45, 30, 20,  0
	};
};

#endif