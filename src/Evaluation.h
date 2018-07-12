#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

//Piece values
#define P_BASE_VAL 100
#define N_BASE_VAL 355
#define B_BASE_VAL 365
#define R_BASE_VAL 520
#define Q_BASE_VAL 980
#define K_BASE_VAL 40000

//Pawn structure values
#define DOUBLED_P_PENALTY -5
#define ISOLATED_P_PENALTY -8
#define PROTECTED_P_BONUS 2
#define PASSED_P_BONUS 20

//Position
#define R_OPEN_FILE_BONUS 40
#define K_OPEN_FILE_PENALTY -30
#define K_P_SHIELD_PENALTY -20
#define K_CASTLED_BONUS 60
#define SPACE_BONUS 5

//Center
#define P_CENTER_BONUS 3
#define P_EXTENDED_CENTER_BONUS 10
#define PIECE_EXTENDED_CENTER_BONUS 8

//Tempo
#define SIDE_TO_MOVE_BONUS 10
#define KING_MOVE_PENALTY -10
#define TEMPO_PENALTY -20

//Game phase
#define OPENING 1
#define MIDGAME 2
#define ENDGAME 3

class Evaluation {

public:

	//Constructor
	Evaluation() : gamePhase(OPENING) {}

	/*GAME PHASE*/
	void setGamePhase(const Board&);

	/*PAWN STRUCTURE*/
	int blockedPawns(const Board&);
	int doubledPawns(const Board&, int);
	int isolatedPawns(const Board&, int);
	int protectedPawns(const Board&, int);
	int passedPawns(const Board&, int);

	/*PIECE VALUES*/
	int baseMaterial(const Board&, int);
	int comboMaterial(const Board&, int);
	int structureMaterial(const Board&, int);

	/*POSITION*/
	int flipTableValue(int);
	int piecePosition(Board&, int);
	int space(const Board&, int);
	int kingSafety(Board&, int);

	/*CENTER*/
	int pawnCenterControl(const Board&, int);
	int pieceExtendedCenterControl(Board&, int);

	/*GETTERS*/
	int isOpenFile(const Board&, int);
	int getGamePhase();

	int totalEvaluation(Board&, int);

private:

	int gamePhase;

	/*PIECE SQUARE TABLES*/
	const int pawnTable[64]
	{
		99, 99, 99, 99, 99, 99, 99, 99,
		55, 55, 55, 55, 55, 55, 55, 55,
		40, 40, 40, 40, 40, 40, 40, 40,
		10, 10, 15, 30, 30, 15, 10, 10,
		 0,  0, 15, 20, 20,  0,  0,  0,
		 0,  0, -5,  0,  0,-15,  0,  0,
		 5, 10,-10,-15,-15, 20, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	const int knightTable[64]
	{
		-70,-50,-30,-30,-30,-30,-50,-70,
		-50,-20,  0,  0,  0,  0,-20,-50,
		-50,  0, 10, 12, 12, 10,  0,-50,
		-50,  0, 10, 12, 12, 10,  0,-50,
		-50,  0, 10, 12, 12, 10,  0,-50,
		-50,  5, 10,  8,  8, 10,  5,-50,
		-50,-20,  0,  5,  5,  0,-20,-50,
		-70,-15,-10,-10,-10,-10,-20,-70,
	};
	const int bishopTable[64]
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-20,  0,  0,  0,  0,  0,  0,-20,
		-20,  0,  5, 10, 10,  5,  0,-20,
		-20, 15, 10, 15, 15, 10, 15,-20,
		-20,  0, 15, 15, 15, 15,  0,-20,
		-20, 10, 10, 10, 10, 10, 10,-20,
		-20, 15,  0,  5,  5,  0, 15,-20,
		-20,-15,-15,-15,-15,-15,-15,-20,
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
		 0,  0,  5, 10, 10,  0,  0,  0
	};
	const int queenOpeningTable[64]
	{
		-40,-30,-30,-10,-10,-30,-30,-40,
		-40,-30,-30,-30,-30,-30,-30,-40,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-30,-30,-30,-30,-30,-30,
		-30,-30,-50,-30,-30,-50,-30,-30,
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