#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

class Evaluation {

public:

	enum pieceValue {
		P_BASE_VAL = 100, N_BASE_VAL = 365, B_BASE_VAL = 380,
		R_BASE_VAL = 560, Q_BASE_VAL = 1225, K_BASE_VAL = 40000
	};

	double phase;

	double getPhase(const Board&);
	int blockedPawns(const Board&);
	int doubledAndIsolatedPawns(const Board&, int);
	int connectedPawns(const Board&, int);
	int backwardPawns(const Board&, int);
	int passedPawns(const Board&, int);

	int baseMaterial(const Board&, int);
	int structureMaterial(const Board&, int);
	int bishopPair(const Board&, int);
	int rookBehindPassed(const Board&, int);
	int trappedRook(const Board&, int);

	int flipTableValue(int) const;
	int PST(const Board&, int);
	int space(const Board&, int);
	int kingSafety(Board&, int);

	int pawnCenterControl(const Board&, int);
	int pieceCenterControl(const Board&, int);

	int isOpenFile(const Board&, int);
	int isPassed(const Board&, int, int);

	int totalEvaluation(Board&, int);
	void outputEvalInfo(Board&, int);

private:

	enum pawnStructValue {
		DOUBLED_P_PENALTY = -15, ISOLATED_P_PENALTY = -10,
		SUPPORTED_P_BONUS = 6, PHALANX_P_BONUS = 4,
		BACKWARD_P_PENALTY = -12,
		PASSED_P_BONUS = 3
	};

	enum piecesBonusValue {
		BISHOP_PAIR_BONUS = 50,
		ROOK_BEHIND_PASSED_P_BONUS = 30,
		TRAPPED_ROOK_PENALTY = -16
	};

	enum positionValue {
		OPEN_CLOSED_POS_PIECE_VALUE = 3,
		R_OPEN_FILE_BONUS = 20,
		K_OPEN_FILE_PENALTY = -20, K_P_SHIELD_PENALTY = -15, K_CASTLED_BONUS = 50,
		SPACE_BONUS = 8
	};

	enum centerValue { 
		P_CENTER_BONUS = 4,
		P_EXTENDED_CENTER_BONUS = 8,
		PIECE_CENTER_BONUS = 6
	};

	enum { SIDE_TO_MOVE_BONUS = 10 };

	const int pawnTable[64]
	{
		99, 99, 99, 99, 99, 99, 99, 99,
		40, 50, 50, 50, 50, 50, 50, 40,
		25, 30, 35, 45, 45, 35, 30, 25,
		10, 10, 15, 25, 25, 15, 10, 10,
		 0,  0, 12, 20, 20, 12,  0,  0,
		 0,  0, 10, 12, 12, 10,  0,  0,
		-5,  5,  5,  0,  0,  5,  5, -5,
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
		-15, 15, 10,  0,  0, 10, 15,-15,
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
		 -5,  0,  0,  0,  0,  0,  0, -5,
		  0,  5,  5,  5,  5,  5,  5,  0,
		  0,  5,  8, 10, 10,  8,  5,  0,
		  0,  5,  8, 10, 10,  8,  5,  0,
		  0,  5,  8,  8,  8,  8,  5,  0,
		 -5,  5,  8,  8,  8,  8,  5, -5,
		-20,-10,-10,  0,  0,-10,-10,-20
	};
	const int kingNormalTable[64]
	{
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-80,-80,-80,-80,-80,-80,-80,-80,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-30,-70,-70,-50,-50,-70,-70,-30,
		  0,  0,-30,-50,-50,-30,  0,  0,
		  0,  0,  0,  0,  0,  0,  0,  0
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