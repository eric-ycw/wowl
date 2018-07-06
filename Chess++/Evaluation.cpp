#include "stdafx.h"
#include "Evaluation.h"

/*GAME PHASE*/
void Evaluation::setGamePhase(Board b) {
	if (gamePhase == OPENING) {
		int score = 0;
		//Sparse backrank --> more developed pieces
		for (int i = 21; i < 29; i++) {
			if (b.mailbox[i] == 0) {
				score++;
			}
		}
		for (int i = 91; i < 99; i++) {
			if (b.mailbox[i] == 0) {
				score++;
			}
		}

		//Castled
		for (int i = 0; i < 2; i++) {
			if (b.castled[i]) {
				score += 2;
			}
		}

		//Move count
		for (int i = 0; i < b.moveVec.size(); i++) {
			score++;
		}

		if (score >= 24) {
			gamePhase = MIDGAME;
		}
	}
	else if (gamePhase == MIDGAME) {
		int count = 0;
		for (int i = 21; i < 99; i++) {
			if (b.mailbox[i] != 0 && abs(b.mailbox[i]) != WP && abs(b.mailbox[i]) != WK) {
				count++;
			}
		}
		//Less than seven major/minor pieces
		if (count < 8) {
			gamePhase = ENDGAME;
		}
	}
}

/*PAWN STRUCTURE*/
int Evaluation::openFiles(Board b) {
	bool filearray[8] = { true, true, true, true, true, true, true, true };
	int filecount = 0;
	for (int i = 21; i < 99; i++) {
		if (abs(b.mailbox[i]) == WP) {
			filearray[i % 10 - 1] = false;
		}
	}
	for (int i = 0; i < 8; i++) {
		if (filearray[i]) {
			filecount++;
		}
	}
	return filecount;
}
int Evaluation::semiOpenFiles(Board b) {
	int filearray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int filecount = 0;
	for (int i = 21; i < 99; i++) {
		if (abs(b.mailbox[i]) == WP) {
			filearray[i % 10 - 1]++;
		}
	}
	for (int i = 0; i < 8; i++) {
		if (filearray[i] == 1) {
			filecount++;
		}
	}
	return filecount;
}
int Evaluation::blockedPawns(Board b) {
	int bpcount = 0;
	for (int i = 21; i < 99; i++) {
		if (b.mailbox[i] == WP && b.mailbox[i - 10] == BP && b.mailbox[i - 11] >= 0 && b.mailbox[i - 9] >= 0) {
			bpcount++;
		}
		if (b.mailbox[i] == BP && b.mailbox[i + 10] == WP && b.mailbox[i + 11] <= 0 && b.mailbox[i + 9] <= 0) {
			bpcount++;
		}
	}
	return bpcount;
}
int Evaluation::doubledPawns(Board b, int color) {
	int filearray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int pcount = 0;
	for (int i = 21; i < 99; i++) {
		if (b.mailbox[i] == WP * color) {
			filearray[i % 10 - 1]++;
		}
	}
	for (int i = 0; i < 8; i++) {
		if (filearray[i] >= 2) {
			pcount++;
		}
	}
	return pcount * DOUBLED_P_PENALTY;
}
int Evaluation::isolatedPawns(Board b, int color) {
	int filearray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int pcount = 0;
	for (int i = 21; i < 99; i++) {
		if (b.mailbox[i] == WP * color) {
			filearray[i % 10 - 1]++;
		}
	}
	for (int i = 0; i < 8; i++) {
		if (i >= 1 && i <= 6) {
			if (filearray[i] == WP * color && filearray[i - 1] == 0 && filearray[i + 1] == 0) {
				pcount++;
			}
		}
		else if (i = 0) {
			if (filearray[0] == WP * color && filearray[1] == 0) {
				pcount++;
			}
		}
		else if (i = 7) {
			if (filearray[7] == WP * color && filearray[6] == 0) {
				pcount++;
			}
		}
	}
	return pcount * ISOLATED_P_PENALTY;
}

/*PIECE VALUES*/
int Evaluation::baseMaterial(Board b, int color) {
	int mval = 0;
	for (int i = 21; i < 99; i++) {
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			case WP:
				mval += P_BASE_VAL;
				break;
			case WN:
				mval += N_BASE_VAL;
				break;
			case WB:
				mval += B_BASE_VAL;
				break;
			case WR:
				mval += R_BASE_VAL;
				break;
			case WQ:
				mval += Q_BASE_VAL;
				break;
			}
		}
	}
	return mval;
}
int Evaluation::comboMaterial(Board b, int color) {
	int mval = 0;
	int ncount = 0;
	int bcount = 0;
	int rcount = 0;
	for (int i = 21; i < 99; i++) {
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			case WN:
				ncount += 1;
				break;
			case WB:
				bcount += 1;
				break;
			case WR:
				rcount += 1;
				break;
			}
		}
	}
	//Knight pair penalty
	if (ncount >= 2) {
		mval -= 10;
	}
	//Bishop pair bonus
	if (bcount >= 2) {
		mval += 50;
	}
	//Rook pair penalty
	if (rcount >= 2) {
		mval -= 10;
	}
	return mval;
}
int Evaluation::structureMaterial(Board b, int color) {
	int mval = 0;
	for (int i = 21; i < 99; i++) {
		if (color * b.mailbox[i] > 0) {
			switch (abs(b.mailbox[i])) {
			//Knights are better in closed positions
			case WN:
				mval += blockedPawns(b) * 5;
				break;
			//Bishops are worse in closed positions
			case WB:
				mval -= blockedPawns(b) * 5;
				break;
			//Rooks are better in positions with open files
			case WR:
				mval += openFiles(b) * 20 + semiOpenFiles(b) * 10;
				break;
			}
		}
	}
	return mval;
}

/*POSITION*/
int Evaluation::flipTableValue(int square) {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = (11 - tenth) * 10 + unit;
	return rval;
}
int Evaluation::piecePosition(Board b, int color) {
	int pval = 0;
	for (int i = 21; i < 99; i++) {
		if (color * b.mailbox[i] > 0) {
			switch (b.mailbox[i]) {
			case WP:
				pval += pawnTable[b.to64Coord(i)];
				break;
			case WN:
				pval += knightTable[b.to64Coord(i)];
				break;
			case WB:
				pval += bishopTable[b.to64Coord(i)];
				break;
			case WR:
				pval += rookTable[b.to64Coord(i)];
				break;
			case WQ:
				pval += queenTable[b.to64Coord(i)];
				break;
			case WK:
				setGamePhase(b);
				if (gamePhase <= MIDGAME) {
					pval += kingNormalTable[b.to64Coord(i)];
				}
				else if (gamePhase == ENDGAME) {
					pval += kingEndTable[b.to64Coord(i)];
				}
				break;
			case BP:
				pval += pawnTable[b.to64Coord(flipTableValue(i))];
				break;
			case BN:
				pval += knightTable[b.to64Coord(flipTableValue(i))];
				break;
			case BB:
				pval += bishopTable[b.to64Coord(flipTableValue(i))];
				break;
			case BR:
				pval += rookTable[b.to64Coord(flipTableValue(i))];
				break;
			case BQ:
				pval += queenTable[b.to64Coord(flipTableValue(i))];
				break;
			case BK:
				setGamePhase(b);
				if (gamePhase <= MIDGAME) {
					pval += kingNormalTable[b.to64Coord(flipTableValue(i))];
				}
				else if (gamePhase == ENDGAME) {
					pval += kingEndTable[b.to64Coord(flipTableValue(i))];
				}
				break;
			}
		}
	}
	return pval;
}
int Evaluation::mobility(Board b, int color) {
	/*if (b.getTurn() == color) {
		b.getLegalMoves();
		return b.legalMoveVec.size();
	}
	else {
		b.turn *= -1;
		b.getLegalMoves();
		b.turn *= -1;
		return b.legalMoveVec.size();
	}*/
	return 0;
}

/*CENTER*/
int Evaluation::centerControl(Board b, int color) {
	return 1;
}

int Evaluation::totalEvaluation(Board b, int color) {
	//Reevaluates game phase
	setGamePhase(b);
	int material = baseMaterial(b, WHITE) + comboMaterial(b, WHITE) + structureMaterial(b, WHITE) - baseMaterial(b, BLACK) - comboMaterial(b, BLACK) - structureMaterial(b, BLACK);
	int pawns = doubledPawns(b, WHITE) + isolatedPawns(b, WHITE) - doubledPawns(b, BLACK) - isolatedPawns(b, BLACK);
	int position = piecePosition(b, WHITE) + mobility(b, WHITE) - piecePosition(b, BLACK) - mobility(b, BLACK);
	int total = material + pawns + position;
	return total;
}