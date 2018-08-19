#include "Wowl.h"

void Wowl::initMVVLVA(const Board& b, const Evaluation& e) {
	//Same value for knight and bishop
	for (int attacker = b.WP; attacker <= b.WK; attacker++) {
		for (int victim = b.WP; victim <= b.WK; victim++) {
			MVVLVAScores[attacker - 1][victim - 1] = e.pieceValues[victim - 1] * 100 - attacker;
		}
	}
}
int Wowl::SEE(Board& b, const Evaluation& e, int square, int color) const {
	int oldsqr;
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = abs(b.mailbox[square]);
	int targetval = target;

	if (piece > 0) {
		if (target != 9 && target != 0) {
			oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
			int mailbox_copy[120];
			int castling_copy[4];
			memcpy(mailbox_copy, b.mailbox, sizeof(b.mailbox));
			memcpy(castling_copy, b.castling, sizeof(b.castling));
			b.move(oldsqr, square);
			if (!b.inCheck(color)) {
				targetval = e.pieceValues[target - 1];
				if (target == b.WN) {
					targetval = e.pieceValues[b.WB - 1];  //We use the same value for bishop and knight
				}
				val = targetval - SEE(b, e, square, -color);
			}
			b.undo(mailbox_copy, castling_copy);
		}
	}
	return val;
}
bool Wowl::checkThreefold(const U64 key) const {
	int poscount = 0;
	for (const auto& i : tempHashPosVec) {
		if (key == i) { poscount++; }
	}
	return poscount >= 3;
}

int Wowl::pstScore(const Board& b, Evaluation& e, const Move& m, int color) {
	int old_coord = (color == b.WHITE) ? m.from : e.flipTableValue(m.from);
	int new_coord = (color == b.WHITE) ? m.to : e.flipTableValue(m.to);
	int pst_score = 0;
	switch (abs(b.mailbox[m.from])) {
	case b.WP:
		pst_score = e.pawnTable[b.to64Coord(new_coord)] - e.pawnTable[b.to64Coord(old_coord)];
		break;
	case b.WN:
		pst_score = e.knightTable[b.to64Coord(new_coord)] - e.knightTable[b.to64Coord(old_coord)];
		break;
	case b.WB:
		pst_score = e.bishopTable[b.to64Coord(new_coord)] - e.bishopTable[b.to64Coord(old_coord)];
		break;
	case b.WR:
		pst_score = e.rookTable[b.to64Coord(new_coord)] - e.rookTable[b.to64Coord(old_coord)];
		break;
	case b.WQ:
		pst_score = e.queenTable[b.to64Coord(new_coord)] - e.queenTable[b.to64Coord(old_coord)];
		break;
	case b.WK:
		pst_score = static_cast<int>((e.kingTable[b.to64Coord(new_coord)] - e.kingTable[b.to64Coord(old_coord)]) * e.phase +
									 (e.kingEndTable[b.to64Coord(new_coord)] - e.kingEndTable[b.to64Coord(old_coord)]) * (1 - e.phase));
		break;
	}
	return pst_score;
}
std::vector<int> Wowl::scoreMoves(Board& b, Evaluation& e, const std::vector<Move>& lmV, const int color, const int depth, const U64 poskey, const bool is_root) {
	
	std::vector<int> scoreVec(lmV.size(), 0);
	
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashTable.tt.at(poskey).hashBestMove;
	}
	else {
		hashMove = NO_MOVE;
	}

	bool isCapture, isPassed, isPromotion;
	e.phase = e.getPhase(b);

	for (int i = 0; i < lmV.size(); ++i) {
		isCapture = b.mailbox[lmV[i].to] != 0;
		isPassed = e.isPassed(b, lmV[i].from, color);
		isPromotion = b.mailbox[lmV[i].from] == b.WP * color && (lmV[i].to / 10 == 2 || lmV[i].to / 10 == 9);

		//Hash table move
		if (lmV[i] == hashMove) {
			scoreVec[i] = 99999;
			continue;
		}
		//Moves from previous iterations
		if (is_root) {
			for (int j = 0; j < depth; ++j) {
				if (lmV[i] == prevIDMoves[j]) {
					scoreVec[i] = 90000 + j;
					continue;
				}
			}
		}
		//Promotion
		if (isPromotion) {
			scoreVec[i] = 50000;
			continue;
		}
		//Captures
		if (isCapture) {
			int see_score = SEE(b, e, lmV[i].to, color);
			if (see_score < 0) {
				//Bad captures
				scoreVec[i] = see_score * 100;
			}
			else {
				//Winning or equal captures
				scoreVec[i] = see_score + 30000;
			}
			continue;
		}
		//Killer moves
		if (lmV[i] == killerMoves[0][depth]) {
			scoreVec[i] = 20000;
			continue;
		}
		if (lmV[i] == killerMoves[1][depth]) {
			scoreVec[i] = 17500;
			continue;
		}
		if (depth - 2 > 0) {
			if (lmV[i] == killerMoves[0][depth - 2]) {
				scoreVec[i] = 15000;
				continue;
			}
			if (lmV[i] == killerMoves[1][depth - 2]) {
				scoreVec[i] = 12500;
				continue;
			}
		}
		//Passed pawn move
		if (isPassed) {
			scoreVec[i] = 10000;
			continue;
		}
		//History heuristic
		scoreVec[i] += historyMoves[(color!= b.WHITE)][abs(b.mailbox[lmV[i].from]) - 1][lmV[i].to];
		scoreVec[i] += pstScore(b, e, lmV[i], color);
	}
	return scoreVec;
}
void Wowl::pickNextMove(std::vector<Move>& moveVec, std::vector<int>& scoreVec, int startpos) {
	assert(moveVec.size() == scoreVec.size());
	int best = -WIN_SCORE;
	int bestMoveIndex = 0;
	if (startpos == scoreVec.size() - 1) { return; }
	for (int i = startpos; i < scoreVec.size(); ++i) {
		if (scoreVec[i] > best) {
			best = scoreVec[i];
			bestMoveIndex = i;
		}
	}
	std::swap(moveVec[startpos], moveVec[bestMoveIndex]);
	std::swap(scoreVec[startpos], scoreVec[bestMoveIndex]);
}
void Wowl::orderCaptures(Board& b, std::vector<Move>& cV) {	int best = -WIN_SCORE;
	int score;
	for (int i = 0; i < cV.size(); ++i) {
		score = MVVLVAScores[abs(b.mailbox[cV[i].from]) - 1][abs(b.mailbox[cV[i].to]) - 1];
		if (score > best) {
			best = score;
			std::swap(cV[0], cV[i]);
		}
	}
}
void Wowl::resetMoveHeuristics() {
	for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
		killerMoves[0][i] = NO_MOVE;
		killerMoves[1][i] = NO_MOVE;
	}
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 6; ++j) {
			for (int k = 0; k < 120; k++) {
				historyMoves[i][j][k] = 0;
			}
		}
	}
}

int Wowl::probeHashTable(const U64 key, int depth, int initial, int alpha, int beta) {
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt.at(key).hashDepth >= depth) {
			if (hashTable.tt.at(key).hashFlag == hashTable.HASH_EXACT) {
				if (depth == initial) { bestMove = hashTable.tt.at(key).hashBestMove; }
				return hashTable.tt.at(key).hashScore;
			}
			else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_ALPHA) {
				if (hashTable.tt.at(key).hashScore <= alpha) {
					return alpha;
				}
			}
			else if (hashTable.tt.at(key).hashFlag == hashTable.HASH_BETA) {
				if (hashTable.tt.at(key).hashScore >= beta) {
					return beta;
				}
			} 
		}
	}
	return VAL_UNKWOWN;
}
void Wowl::recordHash(const U64 key, int depth, int score, int flag) {
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt[key].hashDepth <= depth) {
			hashTable.tt[key].hashDepth = depth;
			hashTable.tt[key].hashScore = score;
			hashTable.tt[key].hashFlag = flag;
		}
	}
	else {
		hashTable.tt[key].hashDepth = depth;
		hashTable.tt[key].hashScore = score;
		hashTable.tt[key].hashFlag = flag;
		hashTable.tt[key].hashAge = 0;
	}
}
void Wowl::ageHash() {
	for (auto it = hashTable.tt.begin(); it != hashTable.tt.end();) {
		if (it->second.hashAge < TT_CLEAR_AGE) {
			(*it).second.hashAge++;
			it++;
		}
		else {
			it = hashTable.tt.erase(it);
		}
	}
}

bool Wowl::timeOver(clock_t startTime, double moveTime) {
	if (double(clock() - startTime) / (CLOCKS_PER_SEC / 1000) >= moveTime) {
		return true;
	}
	else {
		return false;
	}
}
int Wowl::qSearch(Board b, Evaluation& e, int alpha, int beta, int color) {

	int score, stand_pat;
	Evaluation qEval;
	U64 key = hashTable.generatePosKey(b);

	int hashScore = probeHashTable(key, 0, -1, alpha, beta);
	if (hashScore != VAL_UNKWOWN) {
		stand_pat = hashScore;
	}
	else {
		stand_pat = qEval.totalEvaluation(b, color);
	}

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}
	
	b.getCaptures();
	orderCaptures(b, b.captureVec);

	for (int i = 0; i < b.captureVec.size(); ++i) {
		//Delta pruning
		int piece = b.mailbox[b.captureVec[i].to] * color;
		if (stand_pat + e.pieceValues[piece - 1] + DELTA_MARGIN < alpha && e.phase > 0.25) { 
			continue; 
		}
		//Negative SEE
		if (SEE(b, e, b.captureVec[i].to, color) < 0) {
			continue;
		}
		qSearchNodes++;
		int mailbox_copy[120];
		int castling_copy[4];
		memcpy(mailbox_copy, b.mailbox, sizeof(b.mailbox));
		memcpy(castling_copy, b.castling, sizeof(b.castling));
		b.move(b.captureVec[i].from, b.captureVec[i].to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailbox_copy, castling_copy);
			continue;
		}
		score = -qSearch(b, e, -beta, -alpha, -color);
		b.undo(mailbox_copy, castling_copy);
		if (score >= beta) {
			if (i >= 0 && i < 4) {
				qfhf[i]++;
			}
			qfh++;
			return beta;
		}
		if (score > alpha) {
			alpha = score;
		}
	}

	return alpha;
}
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta, bool can_null, clock_t startTime, double stopTime) {

	if (timeOver(startTime, stopTime)) return alpha;

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);
	int tempHashFlag = hashTable.HASH_ALPHA;
	bool isInCheck = b.inCheck(color);

	if (checkThreefold(key)) { return DRAW_SCORE; }

	int hashScore = probeHashTable(key, depth, initial, alpha, beta);
	if (hashScore != VAL_UNKWOWN) {
		return hashScore;
	}

	if (depth <= 0) {
		if (!isInCheck) {
			negaNodes++;
			int qScore = qSearch(b, WowlEval, alpha, beta, color);
			recordHash(key, depth, qScore, hashTable.HASH_EXACT);
			return qScore;
		}
		else if (depth + 1 < initial) {
			depth++;
		}
	}

	int score;

	//Null move pruning
	int R = NULL_MOVE_REDUCTION + depth / 6;
	if (depth - R - 1 > 0 && can_null && !isInCheck && WowlEval.getPhase(b) > 0.25) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - R, initial, -color, -beta, -beta + 1, false, startTime, stopTime);
		b.undoNullMove();
		if (score >= beta) {
			return score;
		}
	}

	//Futility pruning
	bool f_prune = 0;
	int f_prune_count = 0;
	int futilityScore = WIN_SCORE;
	if (depth <= 4 && !isInCheck && WowlEval.getPhase(b) > 0.25) {
		futilityScore = WowlEval.totalEvaluation(b, color) + futilityMargin[depth];
		if (futilityScore <= alpha && futilityScore < WIN_SCORE && futilityScore > -WIN_SCORE) {
			f_prune = 1;
		}
	};


	b.getLegalMoves();

	//Single reply extension
	if (b.legalMoveVec.size() == 1 && depth + 1 < initial) {
		depth++;
	}

	bool haveMove = false;
	bool raised_alpha = false;
	bool isCapture, isPassed, isPromotion, isKiller;

	std::vector<int> scoreVec = scoreMoves(b, WowlEval, b.legalMoveVec, color, depth, key, (depth == initial));

	for (int i = 0; i < b.legalMoveVec.size(); i++) {

		pickNextMove(b.legalMoveVec, scoreVec, i);

		Move m = b.legalMoveVec[i];
		isCapture = b.mailbox[m.to] != 0;
		isPassed = WowlEval.isPassed(b, m.from, color);
		isPromotion = b.mailbox[m.from] == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9);
		isKiller = killerMoves[0][depth] == m || killerMoves[1][depth] == m;

		if (f_prune == 1 && !isPassed && !isPromotion && !isKiller && futilityScore + WowlEval.pieceValues[abs(b.mailbox[m.to]) - 1] <= alpha) {
			assert(futilityScore != WIN_SCORE);
			f_prune_count++;
			continue; 
		}

		int mailbox_copy[120];
		int castling_copy[4];
		memcpy(mailbox_copy, b.mailbox, sizeof(b.mailbox));
		memcpy(castling_copy, b.castling, sizeof(b.castling));
		b.move(m.from, m.to);
		if (b.inCheck(b.getTurn() * -1)) {
			f_prune_count++;
			b.undo(mailbox_copy, castling_copy);
			continue;
		}
		else {
			negaNodes++;
			tempHashPosVec.emplace_back(key);
			haveMove = true;

			//Late move reduction
			if (i >= LMR_STARTING_MOVE && depth >= LMR_STARTING_DEPTH
				&& !b.inCheck(b.getTurn()) && !isCapture && !isPassed && !isPromotion && !isKiller) {
				int R = 1;
				if (depth >= LMR_STARTING_DEPTH + 1) {
					R += i / (LMR_STARTING_MOVE * 2);
					if (depth - R - 1 <= 0) {
						R = depth - 2;
					}
				}
				score = -negaSearch(b, depth - R - 1, initial, -color, -alpha - 1, -alpha, true, startTime, stopTime);
			}
			else {
				score = alpha + 1;
			}
			if (score > alpha) {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha, true, startTime, stopTime);
			}
		}
		b.undo(mailbox_copy, castling_copy);
		tempHashPosVec.pop_back();
		if (timeOver(startTime, stopTime)) return alpha;

		if (score >= beta) {
			if (i >= 0 && i < 4) {
				fhf[i]++;
			}
			fh++;
			if (can_null) {
				recordHash(key, depth, beta, hashTable.HASH_BETA);
				if (!isCapture && !isPromotion) {
					//Killer moves
					if (m != killerMoves[0][depth]) {
						killerMoves[1][depth] = killerMoves[0][depth];
						killerMoves[0][depth] = m;
					}
					//History heuristic
					historyMoves[(color != b.WHITE)][abs(b.mailbox[m.from]) - 1][m.to] += depth * depth;
				}
			}
			return score;
		}
		if (score > alpha) {
			if (depth == initial && can_null) {
				bestMove.from = m.from;
				bestMove.to = m.to;
			}
			raised_alpha = true;
			alpha = score;
			if (can_null) {
				hashTable.tt[key].hashBestMove = m;
				tempHashFlag = hashTable.HASH_EXACT;
			}
		}
	}

	if (!haveMove) {
		if (depth == initial) {
			bestMove = NO_MOVE;
		}
		//No moves searched due to futility pruning
		if (f_prune && f_prune_count == b.legalMoveVec.size()) {
			return alpha;
		}
		if (isInCheck) {
			return -WIN_SCORE;  //Checkmate
		}
		else {
			return DRAW_SCORE;  //Stalemate
		}
	}

	if (can_null) {
		recordHash(key, depth, alpha, tempHashFlag);
	}
	return alpha;
}
void Wowl::ID(Board& b, const Evaluation& e, int max_depth, int color, double moveTime) {

	initMVVLVA(b, e);
	resetMoveHeuristics();
	bestMove = NO_MOVE;

	for (int i = 0; i < MAX_SEARCH_DEPTH; ++i) {
		PVLine[i], prevIDMoves[i] = NO_MOVE, NO_MOVE;
	}

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	int delta = ASPIRATION_WINDOW;
	bool first_search = true;

	int prevNodes = 0;
	int totalNodes = 0;

	fh = 0, qfh = 0;
	for (int i = 0; i < 4; ++i) { 
		fhf[i] = 0;
		qfhf[i] = 0;
	}

	auto startTime = clock();
	std::cout.precision(3);

	for (int idepth = 1; idepth <= max_depth; ++idepth) {

		negaNodes = 0, qSearchNodes = 0;
		fh = 0, qfh = 0;
		for (int i = 0; i < 4; ++i) {
			fhf[i] = 0;
			qfhf[i] = 0;
		}

		if (double(clock() - startTime) / (CLOCKS_PER_SEC / 1000) >= moveTime) { break; }

		tempHashPosVec = hashPosVec;

		bestScore = negaSearch(b, idepth, idepth, color, id_alpha, id_beta, true, startTime, moveTime);
		totalNodes += negaNodes + qSearchNodes;


		//Break when mate is found
		if (bestScore == WIN_SCORE || bestScore == -WIN_SCORE) {
			prevIDMoves[idepth - 1] = bestMove;
			break;
		}

		//UCI output
		std::cout << "info score cp " << bestScore << " depth " << idepth << " nodes " << negaNodes + qSearchNodes;
		std::cout << " time " << (clock() - startTime) / (CLOCKS_PER_SEC / 1000);
		std::cout << " pv ";
		getPVLine(b, hashTable.generatePosKey(b));
		for (int i = 0; i < idepth; ++i) {
			if (PVLine[i] != NO_MOVE) {
				std::cout << b.toNotation(PVLine[i].from) << b.toNotation(PVLine[i].to) << " ";
			}
			else {
				std::cout << "0000 ";
			}
		}
		std::cout << "\n";

		if ((bestScore <= id_alpha) || (bestScore >= id_beta)) {
			id_beta += delta;
			id_alpha -= delta;
			delta += delta / 2;
			idepth--;
			first_search = false;
			continue;
		}

		delta = ASPIRATION_WINDOW;
		if (idepth >= 5) {
			id_alpha = bestScore - delta;
			id_beta = bestScore + delta;
		}
		else {
			id_alpha = -WIN_SCORE;
			id_beta = WIN_SCORE;
		}
		first_search = true;

		prevIDMoves[idepth - 1] = bestMove;
		prevNodes = negaNodes;
	}

	//Add q suffix to move output if promotion
	bool isPromotion = abs(b.mailbox[bestMove.from] == b.WP) && (bestMove.to / 10 == 2 || bestMove.to / 10 == 9);
	std::cout << "bestmove " << b.toNotation(bestMove.from) << b.toNotation(bestMove.to);
	if (isPromotion) {
		std::cout << "q";
	}
	std::cout << "\n";

	ageHash();
}

void Wowl::getPVLine(Board b, U64 key) {
	Move move;
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		move = hashTable.tt.at(key).hashBestMove;
	}
	else {
		move = NO_MOVE;
	}
	int count = 0;
	while (move != NO_MOVE) {
		if (count > MAX_SEARCH_DEPTH) { break; }
		PVLine[count] = move;
		count++;
		b.move(move.from, move.to);
		U64 poskey = hashTable.generatePosKey(b);
		if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
			move = hashTable.tt.at(poskey).hashBestMove;
		}
		else {
			move = NO_MOVE;
		}
	}
}
void Wowl::outputSearchInfo(Board b, const int depth, const int prevNodes) {
	if (negaNodes > 0) {
		std::cout << "NegaSearch cutoff percentage : ";
		for (int num = 0; num < 4; ++num) {
			std::cout << fhf[num] / fh * 100 << " ";
		}
		std::cout << std::endl;
		std::cout << "qSearch cutoff percentage : ";
		for (int num = 0; num < 4; ++num) {
			std::cout << qfhf[num] / qfh * 100 << " ";
		}
	}
	std::cout << std::endl << std::endl << std::endl;
}
long Wowl::perft(Board b, int depth) {

	int nodes = 0;
	U64 key = hashTable.generatePosKey(b);
	if (depth == 0) { return 1; }

	b.getLegalMoves();

	bool haveMove = false;

	for (const auto& i : b.legalMoveVec) {
		int mailbox_copy[120];
		int castling_copy[4];
		memcpy(mailbox_copy, b.mailbox, sizeof(b.mailbox));
		memcpy(castling_copy, b.castling, sizeof(b.castling));
		b.move(i.from, i.to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailbox_copy, castling_copy);
			continue;
		}
		haveMove = true;
		nodes += perft(b, depth - 1);
		b.undo(mailbox_copy, castling_copy);
	}
	if (!haveMove) {
		return 1;
	}
	return nodes;
}