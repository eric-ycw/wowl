#ifndef EVALUATION_INCLUDED
#define EVALUATION_INCLUDED

#include "Board.h"

//Base piece values
#define P_BASE_VAL 100
#define N_BASE_VAL 320
#define B_BASE_VAL 330
#define R_BASE_VAL 510
#define Q_BASE_VAL 900
#define K_BASE_VAL 20000

//Pawn structure values
#define DOUBLED_P_PENALTY -50
#define ISOLATED_P_PENALTY -40
#define PROTECTED_P_BONUS 15

//Position
#define R_OPEN_FILE_BONUS 30;
#define HANGING_PIECE_PENALTY -20;

//Center
#define P_CENTER_BONUS 15
#define P_EXTENDED_CENTER_BONUS 5

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

	/*PIECE VALUES*/
	int baseMaterial(const Board&, int);
	int comboMaterial(const Board&, int);
	int structureMaterial(const Board&, int);

	/*POSITION*/
	int flipTableValue(int);
	int piecePosition(Board, int);
	int hangingPieces(const Board&, int);
	int mobility(const Board&, int);

	/*CENTER*/
	int pawnCenterControl(const Board&, int);
	int pawnExtendedCenterControl(const Board&, int);

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
		 5,  5, 20, 35, 35, 15,  5,  5,
		 0,  0, 15, 25, 25,  0,  0,  0,
		 5,  0,  5,  0,  0, -5,  0,  5,
		 5, 10, -5,-40,-40, 10, 10,  5,
		 0,  0,  0,  0,  0,  0,  0,  0
	};
	const int knightTable[64]
	{
		-50,-40,-30,-30,-30,-30,-40,-50,
		-40,-20,  0,  0,  0,  0,-20,-40,
		-30,  0,  5,  5,  5,  5,  0,-30,
		-30,  5,  5,  5,  5,  5,  5,-30,
		-30,  0,  5,  5,  5,  5,  0,-30,
		-30,  5, 10,  5,  5, 10,  5,-30,
		-40,-20,  0,  5,  5,  0,-20,-40,
		-50,-20,-10,-10,-10,-10,-20,-50,
	};
	const int bishopTable[64]
	{
		-20,-10,-10,-10,-10,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		-10, 10,  5, 10, 10,  5, 10,-10,
		-10,  0, 10, 10, 10, 10,  0,-10,
		-10, 10, 10, 10, 10, 10, 10,-10,
		-10, 15,  0,  5,  5,  0, 15,-10,
		-20,-10,-10,-10,-10,-10,-10,-20,
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
	const int queenTable[64]
	{
		-20,-10,-10, -5, -5,-10,-10,-20,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-10,  0,  5,  5,  5,  5,  0,-10,
		 -5,  0,  5,  5,  5,  5,  0, -5,
		  0,  0,  5,  5,  5,  0,  0, -5,
		-10,  0,  0,  5,  5,  0,  0,-10,
		-10,  0,  0,  0,  0,  0,  0,-10,
		-20,-10,-10,  0,  0,-10,-10,-20
	};
	const int kingNormalTable[64]
	{
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-30,-40,-40,-50,-50,-40,-40,-30,
		-20,-30,-30,-40,-40,-30,-30,-20,
		-10,-20,-20,-20,-20,-20,-20,-10,
		 20, 20,  0,  0,  0,  0, 20, 20,
		 20, 30,  0,  0,  0,  0, 30, 20
	};
	const int kingEndTable[64]
	{
		-50,-40,-30,-20,-20,-30,-40,-50,
		-30,-20,-10,  0,  0,-10,-20,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 30, 40, 40, 30,-10,-30,
		-30,-10, 20, 30, 30, 20,-10,-30,
		-30,-30,  0,  0,  0,  0,-30,-30,
		-50,-30,-30,-30,-30,-30,-30,-50
	};
};
#endif