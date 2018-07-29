#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

class Evaluation {

public:

	Evaluation() : gamePhase(OPENING) {}

	enum pieceValue {
		P_BASE_VAL = 100, N_BASE_VAL = 355, B_BASE_VAL = 365,
		R_BASE_VAL = 520, Q_BASE_VAL = 980, K_BASE_VAL = 40000
	};

	enum Phase { OPENING = 1, MIDGAME = 2, ENDGAME = 3 };

	/*GAME PHASE*/
	void setGamePhase(const Board&);

	/*PAWN STRUCTURE*/
	int blockedPawns(const Board&);
	int doubledAndIsolatedPawns(const Board&, int);
	int connectedPawns(const Board&, int);
	int passedPawns(const Board&, int);

	/*PIECE VALUES*/
	int baseMaterial(const Board&, int);
	int structureMaterial(const Board&, int);
	int bishopPair(const Board&, int);

	/*POSITION*/
	int flipTableValue(int) const;
	int piecePosition(const Board&, int);
	int space(const Board&, int);
	int kingSafety(Board&, int);

	/*CENTER*/
	int pawnCenterControl(const Board&, int);
	int pieceCenterControl(const Board&, int);

	/*GETTERS*/
	int isOpenFile(const Board&, int);
	int getGamePhase();

	int totalEvaluation(Board&, int);

private:

	enum pawnStructValue {
		DOUBLED_P_PENALTY = -20, ISOLATED_P_PENALTY = -20,
		SUPPORTED_P_BONUS = 6, PHALANX_P_BONUS = 4,
		PASSED_P_BONUS = 4
	};

	enum positionValue {
		OPEN_CLOSED_POS_PIECE_VALUE = 3,
		R_OPEN_FILE_BONUS = 25,
		K_OPEN_FILE_PENALTY = -20, K_P_SHIELD_PENALTY = -20, K_CASTLED_BONUS = 60,
		SPACE_BONUS = 4
	};

	enum centerValue { 
		P_CENTER_BONUS = 4,
		P_EXTENDED_CENTER_BONUS = 8,
		PIECE_CENTER_BONUS = 8
	};

	enum { SIDE_TO_MOVE_BONUS = 10 };

	int gamePhase;

	/*PIECE SQUARE TABLES*/
	const int pawnTable[64]
	{
		99, 99, 99, 99, 99, 99, 99, 99,
		99, 99, 99, 99, 99, 99, 99, 99,
		50, 50, 50, 50, 50, 50, 50, 50,
		10, 10, 15, 30, 30, 15, 10, 10,
		 0,  0, 15, 20, 20,  0,  0,  0,
		 0,  0,  0,  0,  0,-10,  0,  0,
		 5, 10,-10,-15,-15, 20, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	const int knightTable[64]
	{
		-50,-50,-30,-30,-30,-30,-50,-50,
		-50,-15,  0,  0,  0,  0,-15,-50,
		-50,  0, 10, 12, 12, 10,  0,-50,
		-50,  0, 10, 15, 15, 10,  0,-50,
		-50,  0, 10, 15, 15, 10,  0,-50,
		-50,  5, 12,  8,  8, 12,  5,-50,
		-50,-15,  0,  5,  5,  0,-15,-50,
		-50,-20,-10,-10,-10,-10,-25,-50,
	};
	const int bishopTable[64]
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-20,  0,  0,  0,  0,  0,  0,-20,
		-20,  0,  5, 10, 10,  5,  0,-20,
		-20, 15, 10, 15, 15, 10, 15,-20,
		-20,  0, 15, 15, 15, 15,  0,-20,
		-20, 10,  8,  8,  8,  8, 10,-20,
		-20, 15,  0,  5,  5,  0, 15,-20,
		-20,-15,-20,-15,-15,-20,-15,-20,
	};
	const int rookTable[64]
	{
		 0,  5,  5,  5,  5,  5,  5,  0,
		 5, 20, 20, 20, 20, 20, 20,  5,
		-5,  5,  5,  5,  5,  5,  5, -5,
		-5,  0,  5,  5,  5,  5,  0, -5,
		-5,  0,  5,  5,  5,  5,  0, -5,
		-5,  0,  5,  5,  5,  5,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5, -5,  5, 10, 10, -5, -5, -5
	};
	const int queenOpeningTable[64]
	{
		-40,-30,-30,-10,-10,-30,-30,-40,
		-40,-30,-30,-30,-30,-30,-30,-40,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-40,-30,-30,-40,-30,-30,
		-40,-40,-40,-40,-40,-40,-40,-40,
		-40,-40,-30, 15,-30,-40,-40,-40
	};
	const int queenNormalTable[64]
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		-10,  0,  5,  5,  5,  5,  0, 10,
		 -5,  0,  0,  0,  0,  0,  0, 10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10,  0,  0,-10,-10,-20
	};
	const int kingNormalTable[64]
	{
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-30,-70,-70,-50,-50,-70,-70,-30,
		  0,  0,-50,-50,-50,-50,  0,  0,
		  0,  0,  0,  0,  0,  0,  0,  0
	};
	const int kingEndTable[64]
	{
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  5,  5,  5,  5,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50
	};
};

#endif