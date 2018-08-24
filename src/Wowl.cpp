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
			int mailboxCopy[120], castlingCopy[4], kingCopy[2];
			memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
			memcpy(castlingCopy, b.castling, sizeof(b.castling));
			memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
			b.move(oldsqr, square);
			if (!b.inCheck(color)) {
				targetval = e.pieceValues[target - 1];
				if (target == b.WN) {
					targetval = e.pieceValues[b.WB - 1];  //We use the same value for bishop and knight
				}
				val = targetval - SEE(b, e, square, -color);
			}
			b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore);
		}
	}
	return val;
}
bool Wowl::checkThreefold(const U64 key) const {
	int poscount = 1;
	for (const auto& i : tempHashPosVec) {
		if (key == i) { poscount++; }
	}
	return poscount >= 3;
}

int Wowl::pstScore(const Board& b, Evaluation& e, const Move& m, int color) {
	int old_coord = b.mailbox120[(color == b.WHITE) ? m.from : e.flipTableValue(m.from)];
	int new_coord = b.mailbox120[(color == b.WHITE) ? m.to : e.flipTableValue(m.to)];
	int pst_score = 0;
	switch (abs(b.mailbox[m.from])) {
	case b.WP:
		pst_score = e.pawnTable[new_coord] - e.pawnTable[old_coord];
		break;
	case b.WN:
		pst_score = e.knightTable[new_coord] - e.knightTable[old_coord];
		break;
	case b.WB:
		pst_score = e.bishopTable[new_coord] - e.bishopTable[old_coord];
		break;
	case b.WR:
		pst_score = e.rookTable[new_coord] - e.rookTable[old_coord];
		break;
	case b.WQ:
		pst_score = e.queenTable[new_coord] - e.queenTable[old_coord];
		break;
	case b.WK:
		pst_score = static_cast<int>((e.kingTable[new_coord] - e.kingTable[old_coord]) * e.phase +
									 (e.kingEndTable[new_coord] - e.kingEndTable[old_coord]) * (1 - e.phase));
		break;
	}
	return pst_score;
}
std::vector<int> Wowl::scoreMoves(Board& b, Evaluation& e, const std::vector<Move>& moves, const int color, const int depth, const U64 poskey, const bool is_root) {
	
	int size = moves.size();
	std::vector<int> scoreVec(size, 0);
	Move hashMove = NO_MOVE;
	
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		hashMove = hashTable.tt.at(poskey).hashBestMove;
	}

	bool isCapture, isPassed, isPromotion;
	e.phase = e.getPhase(b);

	for (int i = 0; i < size; ++i) {
		Move m = moves[i];
		isCapture = b.mailbox[m.to] != 0;
		isPassed = e.isPassed(b, m.from, color);
		isPromotion = b.mailbox[m.from] == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9);

		//Hash table move
		if (m == hashMove) {
			scoreVec[i] = 99999;
			continue;
		}
		//Moves from previous iterations
		if (is_root) {
			bool found = false;
			for (int j = depth - 1; j >= 0; --j) {
				if (m == IDMoves[j]) {
					scoreVec[i] = 90000 + j;
					found = true;
					break;
				}
			}
			if (found) { continue; }
		}
		//Promotion
		if (isPromotion) {
			scoreVec[i] = 50000;
			continue;
		}
		//Captures
		if (isCapture) {
			int see_score = SEE(b, e, m.to, color);
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
		if (m == killerMoves[0][depth]) {
			scoreVec[i] = 20000;
			continue;
		}
		if (m == killerMoves[1][depth]) {
			scoreVec[i] = 17500;
			continue;
		}
		if (depth - 2 > 0) {
			if (m == killerMoves[0][depth - 2]) {
				scoreVec[i] = 15000;
				continue;
			}
			if (m == killerMoves[1][depth - 2]) {
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
		scoreVec[i] += historyMoves[(color!= b.WHITE)][abs(b.mailbox[m.from]) - 1][m.to];
		//Piece-square tables
		scoreVec[i] += pstScore(b, e, m, color);
	}
	return scoreVec;
}
void Wowl::pickNextMove(std::vector<Move>& moveVec, std::vector<int>& scoreVec, int startpos) {
	int size = scoreVec.size();
	assert(moveVec.size() == size);
	int best = -WIN_SCORE;
	int bestMoveIndex = 0;
	if (startpos == size - 1) { return; }
	for (int i = startpos; i < size; ++i) {
		int score = scoreVec[i];
		if (score > best) {
			best = score;
			bestMoveIndex = i;
		}
	}
	if (bestMoveIndex == 0) { return; }
	std::swap(moveVec[startpos], moveVec[bestMoveIndex]);
	std::swap(scoreVec[startpos], scoreVec[bestMoveIndex]);
}
void Wowl::orderCaptures(Board& b, std::vector<Move>& caps) {
	int best = -WIN_SCORE;
	int size = caps.size();
	for (int i = 0; i < size; ++i) {
		int score = MVVLVAScores[abs(b.mailbox[caps[i].from]) - 1][abs(b.mailbox[caps[i].to]) - 1];
		if (score > best) {
			best = score;
			std::swap(caps[0], caps[i]);
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
	auto it = hashTable.tt.find(key);
	if (it != hashTable.tt.end()) {
		if (it->second.hashDepth >= depth) {
			if (it->second.hashFlag == hashTable.HASH_EXACT) {
				if (depth == initial) { bestMove = it->second.hashBestMove; }
				return it->second.hashScore;
			}
			else if (it->second.hashFlag == hashTable.HASH_ALPHA) {
				if (it->second.hashScore <= alpha) {
					return alpha;
				}
			}
			else if (it->second.hashFlag == hashTable.HASH_BETA) {
				if (it->second.hashScore >= beta) {
					return beta;
				}
			} 
		}
	}
	return VAL_UNKWOWN;
}
void Wowl::recordHash(const U64 key, int depth, int score, int flag) {
	auto it = hashTable.tt.find(key);
	if (it != hashTable.tt.end()) {
		if (it->second.hashDepth <= depth) {
			it->second.hashDepth = depth;
			it->second.hashScore = score;
			it->second.hashFlag = flag;
		}
	}
	else {
		hashTable.tt[key] = HashVal(depth, score, flag, 0);
	}
}
void Wowl::ageHash() {
	for (auto it = hashTable.tt.begin(); it != hashTable.tt.end();) {
		if (it->second.hashAge < TT_CLEAR_AGE) {
			it->second.hashAge++;
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
int Wowl::qSearch(Board& b, Evaluation& e, int alpha, int beta, int color) {

	int stand_pat = e.totalEvaluation(b, color, b.lazyScore);
	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}
	
	auto captures = b.getCaptures();
	if (captures.empty()) { return stand_pat; }

	orderCaptures(b, captures);

	int size = captures.size();
	for (int i = 0; i < size; ++i) {
		Move c = captures[i];
		bool isPromotion = b.mailbox[c.from] == b.WP * color && (c.to / 10 == 2 || c.to / 10 == 9);

		//Negative SEE
		if (SEE(b, e, c.to, color) < 0) {
			continue;
		}
		qSearchNodes++;
		int mailboxCopy[120], castlingCopy[4], kingCopy[2], lazyCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		memcpy(lazyCopy, b.lazyScore, sizeof(b.lazyScore));

		int lazyIndex = !(color == b.WHITE);
		b.lazyScore[!lazyIndex] -= e.pieceValues[abs(b.mailbox[c.to]) - 1];
		b.lazyScore[!lazyIndex] -= e.PST(b, c.to, -color);
		if (isPromotion) { b.lazyScore[lazyIndex] += e.pieceValues[4] - e.pieceValues[0]; }
		int preMovePST = e.PST(b, c.from, color);
		b.move(c.from, c.to);
		b.lazyScore[lazyIndex] += e.PST(b, c.to, color) - preMovePST;

		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy);
			continue;
		}
		int score = -qSearch(b, e, -beta, -alpha, -color);
		b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy);
		if (score >= beta) {
			return beta;
		}
		if (score > alpha) {
			alpha = score;
		}
	}

	return alpha;
}
int Wowl::negaSearch(Board& b, int depth, int initial, int color, int alpha, int beta, bool can_null, clock_t startTime, double stopTime) {

	if (timeOver(startTime, stopTime)) return alpha;

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);
	int tempHashFlag = hashTable.HASH_ALPHA;
	bool isInCheck = b.inCheck(color);
	bool nearCheckmate = (alpha <= -WIN_SCORE);

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
	double phase = WowlEval.getPhase(b);

	//Null move pruning
	int R = NULL_MOVE_REDUCTION + depth / 6;
	if (depth > R + 1 && can_null && !isInCheck && phase > 0.25) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - R, initial, -color, -beta, -beta + 1, false, startTime, stopTime);
		b.undoNullMove();
		if (score >= beta) {
			return score;
		}
	}

	//Futility pruning
	bool f_prune = false;
	int f_prune_count = 0;
	int futilityScore = -WIN_SCORE - MAX_SEARCH_DEPTH;
	if (depth <= 2 && !isInCheck && !nearCheckmate) {
		//We increase the futility margin as material decreases
		futilityScore = WowlEval.totalEvaluation(b, color, b.lazyScore) + futilityMargin[depth] + static_cast<int>((1 - phase) * 50);
		f_prune = true;
	};


	auto legalMoves = b.getLegalMoves();

	bool haveMove = false;
	bool raised_alpha = false;

	std::vector<int> scoreVec = scoreMoves(b, WowlEval, legalMoves, color, depth, key, (depth == initial));

	int size = legalMoves.size();
	for (int i = 0; i < size; i++) {

		pickNextMove(legalMoves, scoreVec, i);

		Move m = legalMoves[i];
		int piece = b.mailbox[m.from];
		int target = b.mailbox[m.to];

		bool isCapture = target != 0;  //Does not consider en passant as capture
		bool isEnPassant = piece == b.WP * color && !isCapture && (abs(m.from - m.to) == 11 || abs(m.from - m.to) == 9);
		bool isPassed = piece == b.WP * color && WowlEval.isPassed(b, m.from, color);
		bool isPromotion = piece == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9);
		bool isKiller = killerMoves[0][depth] == m || killerMoves[1][depth] == m;
		bool isCastling = piece == b.WK * color && abs(m.from - m.to) == 2;

		bool isDangerous = isPassed || isPromotion || isKiller || isInCheck;


		if (f_prune && !isDangerous && !isEnPassant && i > 0) {
			if (futilityScore + WowlEval.pieceValues[abs(target) - 1] <= alpha) { 
				f_prune_count++;
				continue; 
			}
		}

		//Copy board state and info before making move
		int mailboxCopy[120], castlingCopy[4], kingCopy[2], lazyCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		memcpy(lazyCopy, b.lazyScore, sizeof(b.lazyScore));

		int lazyIndex = !(color == b.WHITE);
		if (isCapture) { 
			b.lazyScore[!lazyIndex] -= WowlEval.pieceValues[abs(target) - 1]; 
			b.lazyScore[!lazyIndex] -= WowlEval.PST(b, m.to, -color);
		} else if (isEnPassant) {
			b.lazyScore[!lazyIndex] -= WowlEval.pieceValues[0];
			b.lazyScore[!lazyIndex] -= WowlEval.PST(b, m.to + color * 10, -color);
		}
		if (isCastling) {
			b.lazyScore[lazyIndex] += 3;  //Hard-coded PST benefit from moving the rook during castling
		}
		//Currently treats queening as the only possible promotion
		if (isPromotion) { b.lazyScore[lazyIndex] += WowlEval.pieceValues[4] - WowlEval.pieceValues[0]; }
		int preMovePST = WowlEval.PST(b, m.from, color);  //We calculate the PST value before moving in case the piece changes (i.e. promotion)
		b.move(m.from, m.to);
		b.lazyScore[lazyIndex] += WowlEval.PST(b, m.to, color) - preMovePST;

		if (b.inCheck(b.getTurn() * -1)) {
			f_prune_count++;
			b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy);
			continue;
		}
		else {
			negaNodes++;
			tempHashPosVec.emplace_back(key);
			haveMove = true;

			//Late move reduction
			if (i >= LMR_STARTING_MOVE && depth >= LMR_STARTING_DEPTH && !isDangerous && !isCapture && !isEnPassant && !b.inCheck(b.getTurn())) {
				int R = 1;
				if (depth >= LMR_STARTING_DEPTH + 1) {
					R += i / (LMR_STARTING_MOVE * 4);
					if (R > 2) { R = 2; }
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

		b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy);
		tempHashPosVec.pop_back();
		if (timeOver(startTime, stopTime) && depth != initial) return alpha;

		if (score >= beta) {
			if (can_null) {
				recordHash(key, depth, beta, hashTable.HASH_BETA);
				if (!isCapture && !isPromotion && isEnPassant) {
					//Killer moves
					if (m != killerMoves[0][depth]) {
						killerMoves[1][depth] = killerMoves[0][depth];
						killerMoves[0][depth] = m;
					}
					//History heuristic
					historyMoves[(color != b.WHITE)][abs(target) - 1][m.to] += depth * depth;
				}
			}
			return score;
		}
		if (score > alpha) {
			if (depth == initial && can_null) {
				bestMove = m;
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
		if (f_prune && f_prune_count == size) {
			return alpha;
		}
		return (isInCheck) ? -WIN_SCORE - initial + depth : DRAW_SCORE;
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
		PVLine[i] = NO_MOVE;
		IDMoves[i] = NO_MOVE;
	}

	int idAlpha = -WIN_SCORE;
	int idBeta = WIN_SCORE;
	int delta = ASPIRATION_WINDOW;
	int failed = 0;

	int totalNodes = 0;
	int highestDepth = 0;

	auto startTime = clock();
	std::cout.precision(3);

	for (int idepth = 1; idepth <= max_depth; ++idepth) {

		negaNodes = 0, qSearchNodes = 0;

		if (double(clock() - startTime) / (CLOCKS_PER_SEC / 1000) >= moveTime) { break; }

		tempHashPosVec = hashPosVec;

		bestScore = negaSearch(b, idepth, idepth, color, idAlpha, idBeta, true, startTime, moveTime);
		totalNodes += negaNodes + qSearchNodes;

		//UCI output
		std::cout << "info score cp " << bestScore << " depth " << idepth << " nodes " << negaNodes + qSearchNodes;
		std::cout << " time " << (clock() - startTime) / (CLOCKS_PER_SEC / 1000);
		std::cout << " pv ";
		getPVLine(b, hashTable.generatePosKey(b));
		for (int i = 0; i < idepth; ++i) {
			if (PVLine[i] != NO_MOVE) {
				bool isPromotion = abs(b.mailbox[PVLine[i].from] == b.WP) && (PVLine[i].to / 10 == 2 || PVLine[i].to / 10 == 9);
				std::cout << b.toNotation(PVLine[i].from) << b.toNotation(PVLine[i].to);
				if (isPromotion) {
					std::cout << "q";
				}
				std::cout << " ";
			}
		}
		std::cout << "\n";

		if ((bestScore <= idAlpha) || (bestScore >= idBeta)) {
			idBeta += delta;
			idAlpha -= delta;
			delta += delta / 2;
			idepth--;
			continue;
		}

		delta = ASPIRATION_WINDOW;
		if (idepth >= 5) {
			idAlpha = bestScore - delta;
			idBeta = bestScore + delta;
		}
		else {
			idAlpha = -WIN_SCORE - MAX_SEARCH_DEPTH;
			idBeta = WIN_SCORE + MAX_SEARCH_DEPTH;
		}
		failed = 0;

		IDMoves[idepth - 1] = bestMove;
		highestDepth++;
	}

	//We only use best moves from full searches
	if (highestDepth < 0) { highestDepth = 0; }
	bestMove = IDMoves[highestDepth - 1];

	//PV failsafe
	if (bestMove == NO_MOVE) { bestMove = PVLine[0]; }

	if (bestMove != NO_MOVE) {
		//Add q suffix to move output if promotion
		bool isPromotion = abs(b.mailbox[bestMove.from] == b.WP) && (bestMove.to / 10 == 2 || bestMove.to / 10 == 9);
		std::cout << "bestmove " << b.toNotation(bestMove.from) << b.toNotation(bestMove.to);
		if (isPromotion) {
			std::cout << "q";
		}
	}
	else {
		std::cout << "bestmove 0000";
	}
	std::cout << "\n";

	ageHash();
}

void Wowl::getPVLine(Board b, U64 key) {
	Move move = NO_MOVE;
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		move = hashTable.tt.at(key).hashBestMove;
	}
	int count = 0;
	while (move != NO_MOVE) {
		if (count >= MAX_SEARCH_DEPTH) { break; }
		PVLine[count] = move;
		count++;
		b.move(move.from, move.to);
		U64 poskey = hashTable.generatePosKey(b);
		move = NO_MOVE;
		if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
			move = hashTable.tt.at(poskey).hashBestMove;
		}
	}
}
long Wowl::perft(Board& b, Evaluation& e, int depth) {

	int nodes = 0;
	if (depth == 0) {
		return 1; 
	}

	auto legalMoves = b.getLegalMoves();

	bool haveMove = false;

	for (const auto& i : legalMoves) {
		int mailboxCopy[120], castlingCopy[4], kingCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		b.move(i.from, i.to);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore);
			continue;
		}
		haveMove = true;
		nodes += perft(b, e, depth - 1);
		std::cout << b.toNotation(i.from) << b.toNotation(i.to) << std::endl;
		b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore);
	}
	if (!haveMove) {
		return 1;
	}
	return nodes;
}