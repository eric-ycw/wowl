#include "Evaluation.h"

double Evaluation::getPhase(const Board& b) {
	int baseMaterial = (pieceValues[b.WN - 1] + pieceValues[b.WB - 1] + pieceValues[b.WR - 1]) * 4 + pieceValues[b.WQ - 1] * 2;
	int nonPMaterial = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece == 0 || piece == b.WP || piece == b.BP || piece == b.WK || piece == b.BK) { continue; }
		nonPMaterial += pieceValues[abs(piece) - 1];
	}
	return double(nonPMaterial) / baseMaterial;
}

int Evaluation::blockedPawns(const Board& b) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP && b.mailbox[i - 10] == b.BP 
			&& b.mailbox[i - 11] != b.BP && b.mailbox[i - 9] != b.BP
			&& b.mailbox[i - 1] != b.WP && b.mailbox[i + 1] != b.WP) {
			bpcount += 2;
		}
	}
	return bpcount;
}
int Evaluation::doubledAndIsolatedPawns(const Board& b, int color) {
	int filearray[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int dpcount = 0;
	int ipcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			filearray[i % 10 - 1]++;
		}
	}
	for (int i = 0; i < 8; ++i) {
		dpcount += (filearray[i] >= 2) ? filearray[i] - 1 : 0;
		if (i >= 1 && i <= 6) {
			if (filearray[i] == b.WP * color && filearray[i - 1] == 0 && filearray[i + 1] == 0) {
				ipcount++;
			}
		}
		else if (i == 0) {
			if (filearray[0] == b.WP * color && filearray[1] == 0) {
				ipcount++;
			}
		}
		else if (i == 7) {
			if (filearray[7] == b.WP * color && filearray[6] == 0) {
				ipcount++;
			}
		}
	}
	return dpcount * DOUBLED_P_PENALTY + ipcount * ISOLATED_P_PENALTY;
}
int Evaluation::connectedPawns(const Board&b, int color) {
	int supported = 0;
	int phalanx = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			supported += (b.mailbox[i + color * 10 + 1] == b.WP * color)
				+ (b.mailbox[i + color * 10 - 1] == b.WP * color);
			phalanx += (b.mailbox[i + 1] == b.WP * color)
				+ (b.mailbox[i - 1] == b.WP * color);
		}
	}
	return supported * SUPPORTED_P_BONUS + phalanx * PHALANX_P_BONUS;
}
int Evaluation::backwardPawns(const Board& b, int color) {
	int bpcount = 0;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] != b.WP * color) { continue; }
		if (b.mailbox[i - 10 * color] == b.WP * -color || !attackedByEnemyPawn(b, i - 10 * color, color)) { continue; }

		int j = i - 1;
		bool backward = true;
		while (b.mailbox[j] != b.NN) {
			if (b.mailbox[j] == b.WP * color) {
				backward = false;
				break;
			}
			j += color * 10;
		}
		if (backward) {
			j = i + 1;
			while (b.mailbox[j] != b.NN) {
				if (b.mailbox[j] == b.WP * color) {
					backward = false;
					break;
				}
				j += color * 10;
			}
		}
		if (backward) { bpcount++; }
	}
	return bpcount * BACKWARD_P_PENALTY;
}
int Evaluation::passedPawns(const Board& b, int color) {
	int pcount = 0;
	int rank, file;
	for (int i = 31; i < 89; ++i) {
		if (b.mailbox[i] == b.WP * color) {
			file = i % 10;
			rank = i / 10;
			if (isPassed(b, i, color)) {
				int reverse_dis = color == b.WHITE ? 9 - rank : rank - 2;
				//Reduce rank by one if blocked
				if (b.mailbox[i - color * 10] != 0) { reverse_dis -= 1; }
				if (reverse_dis < 5) {
					pcount += reverse_dis * 2 * PASSED_P_BONUS;
				}
				else if (reverse_dis == 5) {
					pcount += pow(reverse_dis, 2) * PASSED_P_BONUS;
				}
				else {
					pcount += pow(reverse_dis, 2.25) * PASSED_P_BONUS;
				}
			}
		}
	}
	return static_cast<int>(pcount * PASSED_P_BONUS);
}

int Evaluation::knightOutpost(const Board& b, int square, int color) {
	int support = (b.mailbox[square + 11 * color] == b.WP * color) + (b.mailbox[square + 9 * color] == b.WP * color);
	int val = 0;
	if (support > 0) {
		bool secure[2] = { true, true };
		for (int j = square - 1 - 10 * color; b.mailbox[j] != b.NN; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[0] = false; }
		}
		for (int j = square + 1 - 10 * color; b.mailbox[j] != b.NN; j -= 10 * color) {
			if (b.mailbox[j] == b.WP * -color) { secure[1] = false; }
		}
		if (secure[0] && secure[1]) {
			val += support * KNIGHT_OUTPOST_BONUS;
		}
	}
	return val;
}
int Evaluation::rookBehindPassed(const Board& b, int square, int color) {
	int rcount = 0;
	int sqr = square;
	while (sqr >= 31 && sqr <= 88) {
		//Supporting friendly passed pawn or blocking enemy passed pawn
		sqr += -color * 10;
		if (isPassed(b, sqr, color) || isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	while (sqr >= 31 && sqr <= 88) {
		//Behind enemy passed pawn
		sqr += color * 10;
		if (isPassed(b, sqr, -color)) {
			rcount += 1;
			break;
		}
	}
	return rcount * ROOK_BEHIND_PASSED_P_BONUS;
}
int Evaluation::trappedRook(const Board&b, int square, int color) {
	int blocked = 0;
	int castle_id = (color == b.WHITE) ? 0 : 1;
	if (b.mailbox[square + 1] != 0) {
		blocked++;
	}
	if (b.mailbox[square - 1] != 0) {
		blocked++;
	}
	if (b.mailbox[square + 10] != 0) {
		blocked++;
	}
	if (b.mailbox[square - 10] != 0) {
		blocked++;
	}
	if (blocked > 1) {
		return blocked * TRAPPED_ROOK_PENALTY;
	}
	else {
		return 0;
	}
}

int Evaluation::flipTableValue(int square) const {
	return (11 - square / 10) * 10 + square % 10;
}
int Evaluation::PST(const Board& b, int square, int color) {
	int val = 0;
	int coord = b.mailbox120[(color == b.WHITE) ? square : flipTableValue(square)];
	switch (abs(b.mailbox[square])) {
	case b.WP:
		val = pawnTable[coord] * phase;
		break;
	case b.WN:
		val = knightTable[coord];
		break;
	case b.WB:
		val = bishopTable[coord];
		break;
	case b.WR:
		val = rookTable[coord];
		break;
	case b.WQ:
		val = queenTable[coord];
		break;
	case b.WK:
		val = kingTable[coord] * phase + kingEndTable[coord] * (1 - phase);
		break;
	}
	return val;
}

int Evaluation::mobilityKnight(const Board&b, int square, int color) {
	int mobility = 0;
	for (const int i : b.pieceMoves[1]) {
		if (!i) { break; }
		int toVal = b.mailbox[square + i];
		if (toVal == b.NN || toVal * color > 0 || attackedByEnemyPawn(b, square + i, color)) {
			continue;
		}
		mobility++;
	}
	return mobility;
}
int Evaluation::mobilitySlider(const Board&b, int square, int color, int piece) {
	int mobility = 0;
	for (const int i : b.pieceMoves[piece - 1]) {
		if (!i) { break; }
		for (int j = 1; j < 8; ++j) {
			int toVal = b.mailbox[square + j * i];
			if (toVal == b.NN || toVal * color > 0 || attackedByEnemyPawn(b, square + i, color)) {
				break;
			}
			mobility++;
			if (toVal * color < 0) { break; }
		}
	}
	return mobility;
}

int Evaluation::spaceArea(const Board&b, int color) {
	int sval = 0;
	int pieces = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN) { continue; }
		if (piece * color > 0) { pieces++; }
		if (i >= 43 && i <= 76) {
			if (attackedByEnemyPawn(b, i, color)) { continue; }
			sval++;
			//Increase space value if square is behind own pawn by 1-3 squares
			for (int j = 0; j < 2; ++j) {
				if (b.mailbox[i - color * 10 * j] == color * b.WP) {
					sval++;
					break;
				}
			}
		}
	}
	return static_cast<int>(sval * pieces * 1.5);
}

int Evaluation::kingShelter(Board& b, int color) {
	int rval = 0;
	int pval = 0;
	int kingsqr = (color == b.WHITE) ? b.kingSquare[0] : b.kingSquare[1];

	//Open files
	rval = isOpenFile(b, kingsqr) + isOpenFile(b, kingsqr + 1) * 0.75 + isOpenFile(b, kingsqr - 1) * 0.75;
	//Pawn shield in front of king
	if (b.mailbox[kingsqr - 10 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 11 * color] != b.WP * color) {
		pval++;
	}
	if (b.mailbox[kingsqr - 9 * color] != b.WP * color) {
		pval++;
	}

	return static_cast<int>((rval * K_OPEN_FILE_PENALTY + pval * K_P_SHIELD_PENALTY) * phase);
}
int Evaluation::kingDangerProximity(Board& b, int color) {
	int kingsqr = (color == b.WHITE) ? b.kingSquare[1] : b.kingSquare[0];
	int kingRing[25] = {
		0, 1, -1, 10, -10, 11, 9, -11, 9,
		2, -2, 20, -20, 22, 18, -22, 18,
		12, 8, -12, -8, 21, -21, 19, -19
	};
	int attack = 0;
	int attackers = 0;

	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece * color <= b.WP) { continue; }
		switch (piece * color) {
		case b.WN:
			for (int j = 0; j < 25; ++j) {
				int square = kingsqr + kingRing[j];
				if (square < 21 || square > 98 || b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackKnight(i, square)) {
					attack += KNIGHT_KING_ATTACKER;
					attackers++;
					break;
				}
			}
			break;
		case b.WB:
			for (int j = 0; j < 25; ++j) {
				int square = kingsqr + kingRing[j];
				if (square < 21 || square > 98 || b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackSlider(i, square, b.WB)) {
					attack += BISHOP_KING_ATTACKER;
					attackers++;
					break;
				}
			}
			break;
		case b.WR:
			for (int j = 0; j < 25; ++j) {
				int square = kingsqr + kingRing[j];
				if (square < 21 || square > 98 || b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackSlider(i, square, b.WR)) {
					attack += ROOK_KING_ATTACKER;
					attackers++;
					break;
				}
			}
			break;
		case b.WQ:
			for (int j = 0; j < 25; ++j) {
				int square = kingsqr + kingRing[j];
				if (square < 21 || square > 98 || b.mailbox[square] == b.NN) { continue; }
				if (b.checkAttackSlider(i, square, b.WQ)) {
					attack += QUEEN_KING_ATTACKER;
					attackers++;
					break;
				}
			}
			break;
		}
	}
	return attack * (attackers > 1);
}

int Evaluation::pawnPushThreat(const Board& b, int square, int color) {
	if (b.mailbox[square - 19 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 9 * color, color)) {
		return PAWN_PUSH_THREAT_PENALTY;
	}
	if (b.mailbox[square - 21 * color] == b.WP * -color && attackedByEnemyPawn(b, square - 11 * color, color)) {
		return PAWN_PUSH_THREAT_PENALTY;
	}
	return 0;
}
int Evaluation::pawnAttackThreat(const Board& b, int square, int color) {
	if (!attackedByEnemyPawn(b, square, color)) { return 0; }
	if (attackedByEnemyPawn(b, square - 11 * color, color) || 
		attackedByEnemyPawn(b, square - 9 * color, color)) {
		return PAWN_ATTACK_THREAT_PENALTY;
	}
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		if (piece == b.NN || piece * color >= b.BP) { continue; }
		switch (piece * color) {
		case b.BN:
			if (b.checkAttackKnight(i, square)) { return PAWN_ATTACK_THREAT_PENALTY; }
			break;
		case b.BB:
			if (b.checkAttackSlider(i, square, b.WB)) { return PAWN_ATTACK_THREAT_PENALTY; }
			break;
		case b.BR:
			if (b.checkAttackSlider(i, square, b.WR)) { return PAWN_ATTACK_THREAT_PENALTY; }
			break;
		case b.BQ:
			if (b.checkAttackSlider(i, square, b.WQ)) { return PAWN_ATTACK_THREAT_PENALTY; }
			break;
		case b.BK:
			if (b.checkAttackKing(i, square)) { return PAWN_ATTACK_THREAT_PENALTY; }
			break;
		}
	}
	return 0;
}

int Evaluation::piecesEval(const Board&b, int color) {
	int val = 0;
	int bcount = 0;
	int blocked = blockedPawns(b);
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		int abs_piece = color * piece;
		if (piece == b.NN || abs_piece <= b.WP) { continue; }
		switch (abs_piece) {
		case b.WN:
			val += blocked * CLOSED_POSITION_BONUS;
			val += MINOR_BEHIND_PAWN_BONUS * (b.mailbox[i - 10 * color] == b.WP * color);
			val += pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color);
			val += knightOutpost(b, i, color);
			val += knightMobilityTable[mobilityKnight(b, i, color)];
			break;
		case b.WB:
			bcount++;
			val -= blocked * CLOSED_POSITION_BONUS;
			val += MINOR_BEHIND_PAWN_BONUS * (b.mailbox[i - 10 * color] == b.WP * color);
			val += pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color);
			val += bishopMobilityTable[mobilitySlider(b, i, color, b.WB)];
			break;
		case b.WR:
			val -= blocked * CLOSED_POSITION_BONUS;
			val += isOpenFile(b, i) * R_OPEN_FILE_BONUS;
			val += rookBehindPassed(b, i, color) + trappedRook(b, i, color);
			val += pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color);
			val += rookMobilityTable[mobilitySlider(b, i, color, b.WR)];
			break;
		case b.WQ:
			val += pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color);
			val += queenMobilityTable[mobilitySlider(b, i, color, b.WQ)];
			break;
		case b.WK:
			val += pawnPushThreat(b, i, color) + pawnAttackThreat(b, i, color);
		}
	}
	if (bcount >= 2) {
		val += BISHOP_PAIR_BONUS;
	}
	return val;
}

bool Evaluation::attackedByEnemyPawn(const Board& b, int square, int color) {
	return (b.mailbox[square - 11 * color] == b.WP * -color) || (b.mailbox[square - 9 * color] == b.WP * -color);
}
int Evaluation::isOpenFile(const Board& b, int square) {
	int file = square % 10;
	int pcount = 0;
	if (b.mailbox[square] != b.NN) {
		for (int i = 2; i <= 9; ++i) {
			pcount = 0;
			if (abs(b.mailbox[i * 10 + file] == b.WP)) {
				pcount++;
			}
		}
	}
	if (pcount > 1) {
		return 0;
	}
	else if (pcount == 1) {
		return 1;
	}
	else if (pcount == 0) {
		return 2;
	}
}
int Evaluation::isPassed(const Board&b, int square, int color) {
	if (b.mailbox[square] == b.WP * color) {
		bool passed = true;
		int file = square % 10;
		int rank = (square - file) / 10;
		int sqr = square;
		while (sqr >= 31 && sqr <= 88) {
			sqr += -color * 10;
			if (b.mailbox[sqr] == b.WP * -color || b.mailbox[sqr - 1] == b.WP * -color || b.mailbox[sqr + 1] == b.WP * -color) {
				passed = false;
			}
		}
		if (passed) {
			return true;
		}
	}
	return false;
}

int Evaluation::totalEvaluation(Board& b, int color, int lazyScore[]) {
	phase = getPhase(b);
	int lazyIndex = !(color == b.WHITE);
	int lazy = lazyScore[lazyIndex] - lazyScore[!lazyIndex];

	int pieces = piecesEval(b, color) - piecesEval(b, -color);
	int pawns = doubledAndIsolatedPawns(b, color) + connectedPawns(b, color) + backwardPawns(b, color) + passedPawns(b, color)
		- doubledAndIsolatedPawns(b, -color) - connectedPawns(b, -color) - backwardPawns(b, -color) - passedPawns(b, -color);
	int space = spaceArea(b, color) - spaceArea(b, -color);
	int king = kingShelter(b, color) - kingShelter(b, -color) + kingDangerProximity(b, color) - kingDangerProximity(b, -color);
	return lazy + pieces + pawns + space + king;
}
int Evaluation::lazyEvaluation(const Board& b, int color) {
	phase = getPhase(b);
	int val = 0;
	for (int i = 21; i < 99; ++i) {
		int piece = b.mailbox[i];
		int abs_piece = color * piece;
		if (piece == b.NN || abs_piece <= 0) { continue; }
		val += pieceValues[abs(piece) - 1] + PST(b, i, color);
	}
	return val;
}