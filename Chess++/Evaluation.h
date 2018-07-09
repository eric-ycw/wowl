#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

//Piece values
#define P_BASE_VAL 100
#define N_BASE_VAL 355
#define B_BASE_VAL 370
#define R_BASE_VAL 520
#define Q_BASE_VAL 980
#define K_BASE_VAL 40000

//Pawn structure values
#define DOUBLED_P_PENALTY -15
#define ISOLATED_P_PENALTY -15
#define PROTECTED_P_BONUS 5
#define PASSED_P_BONUS 40

//Position
#define R_OPEN_FILE_BONUS 30;
#define K_OPEN_FILE_PENALTY -30;
#define K_P_SHIELD_PENALTY -30;
#define K_CASTLED_BONUS 80;
#define SPACE_BONUS 5;

//Center
#define P_CENTER_BONUS 5
#define P_EXTENDED_CENTER_BONUS 10
#define PIECE_EXTENDED_CENTER_BONUS 8

//Tempo
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
	int openFiles(const Board&);
	int semiOpenFiles(const Board&);
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
	int pawnExtendedCenterControl(const Board&, int);
	int pieceExtendedCenterControl(Board&, int);

	/*GETTERS*/
	int isOpenFile(const Board&, int);

	int totalEvaluation(Board&, int);

private:

	int gamePhase;

	/*PIECE SQUARE TABLES*/
	const int pawnTable[64]
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		80, 80, 80, 80, 80, 80, 80, 80,
		20, 20, 30, 50, 50, 30, 20, 20,
		 5,  5, 20, 25, 25, 15,  5,  5,
		 5,  5, 20, 25, 25,  5,  5,  5,
		 5,  0,  5,  0,  0,  0,  0,  5,
		 5, 10,-10,-15,-15, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	const int knightTable[64]
	{
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-40,  0,  5,  5,  5,  5,  0,-40,
		-40,  5,  5, 10, 10,  5,  5,-40,
		-40,  0, 10, 10, 10, 10,  0,-40,
		-40,  5, 10,  5,  5, 10,  5,-40,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-20,-10,-10,-10,-10,-20,-50,
	};
	const int bishopTable[64]
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-20,  0,  0,  0,  0,  0,  0,-20,
		-20,  0,  5, 10, 10,  5,  0,-20,
		-20, 15, 10, 10, 10, 10, 15,-20,
		-20,  0, 15, 15, 15, 15,  0,-20,
		-20, 10, 10, 10, 10, 10, 10,-20,
		-20, 20,  0,  5,  5,  0, 20,-20,
		-20,-15,-15,-15,-15,-15,-15,-20,
	};
	const int rookTable[64]
	{
		 0,  0,  0,  0,  0,  0,  0,  0,
		 5, 10, 10, 10, 10, 10, 10,  5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		-5,  0,  0,  0,  0,  0,  0, -5,
		 0,  0,  0,  5,  5,  0,  0,  0
	};
	const int queenOpeningTable[64]
	{
		-40,-20,-20,-10,-10,-20,-20,-40,
		-20,  0,  0,  0,  0,  0,  0,-20,
		-20,-20,-20,-20,-20,-20,-20,-20,
		-20,-20,-20,-20,-20,-20,-20,-20,
		-20,-20,-20,-20,-20,-20,-20,-20,
		-20,-20,-15,-10,-15,-20,-20,-20,
		-20,-15,-10,-10,-10,-15,-15,-20,
		-40,-15,-20, 10,-20,-15,-15,-40
	};
	const int queenNormalTable[64]
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  0,  0,  0,  0,  0, 10,
		 -5,  0,  0,  0,  0,  0,  0, 10,
		-10,  5,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  0,  0,  0,  0,-10,
		-20,-10,-10,  0,  0,-10,-10,-20
	};
	const int kingNormalTable[64]
	{
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-99,-99,-99,-99,-99,-99,-99,-99,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-70,-70,-70,-70,-70,-70,-70,-70,
		-20,-70,-70,-50,-50,-70,-70,-20,
		  0,  0,-40,-40,-40,-40,  0,  0,
		  0,  0,  0,-10,  0,  0,  0,  0
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