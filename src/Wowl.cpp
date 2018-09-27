#include "Wowl.h"

bool Wowl::checkThreefold(const U64 key) const {
	int poscount = 1;
	for (const auto& i : tempHashPosVec) {
		if (key == i) { poscount++; }
	}
	return poscount >= 3;
}

void Wowl::initMVVLVA(const Board& b, const Evaluation& e) {
	// Same value for knight and bishop
	for (int attacker = b.WP; attacker <= b.WK; attacker++) {
		for (int victim = b.WP; victim <= b.WK; victim++) {
			MVVLVAScores[attacker - 1][victim - 1] = e.pieceValues[victim - 1] * 100 - attacker;
		}
	}
}
int Wowl::SEE(Board& b, const Evaluation& e, int square, int color) const {
	int val = 0;
	int target = abs(b.mailbox[square]);
	int targetval = target;

	if (target != 9 && target != 0) {
		auto smallestAttacker = b.getSmallestAttacker(square, color);
		int piece = std::get<0>(smallestAttacker);
		if (piece <= 0) { return 0; }

		int attackerSquare = std::get<1>(smallestAttacker);
		int mailboxCopy[120], castlingCopy[4], kingCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		int epCopy = b.epSquare;

		b.move(attackerSquare, square, true);
		if (!b.inCheck(color)) {
			targetval = e.pieceValues[target - 1];
			if (target == b.WN) {
				targetval = e.pieceValues[b.WB - 1];  // We use the same value for bishop and knight
			}
			val = targetval - SEE(b, e, square, -color);
		}
		b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore, epCopy, true);
	}
	return val;
}

std::vector<int> Wowl::scoreMoves(Board& b, Evaluation& e, const std::vector<Move>& moves, const int color, const int depth, const U64 poskey, const bool isRoot) {
	int size = moves.size();
	std::vector<int> scoreVec(size, 0);

	e.phase = e.getPhase(b);

	for (int i = 0; i < size; ++i) {

		Move m = moves[i];
		int piece = b.mailbox[m.from];
		int target = b.mailbox[m.to];

		bool isCapture = target != 0;
		bool isPromotion = piece == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9);

		// Hash table move
		if (m == ttMove) {
			scoreVec[i] = 99999;
			continue;
		}
		// Moves from previous iterations
		if (isRoot) {
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
		// Captures
		if (isCapture) {
			int seeScore = SEE(b, e, m.to, color);
			scoreVec[i] = seeScore + (seeScore >= 0) * 30000;
			if (seeScore == 0) { scoreVec[i] += abs(target); }
			continue;
		}
		// Promotions
		if (isPromotion) {
			scoreVec[i] = 30000;
			continue;
		}
		// Killer moves
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
		// History heuristic
		int historyScore = historyMoves[(color != b.WHITE)][abs(piece) - 1][m.to];
		if (historyScore >= historyMax) { reduceHistory(); }
		scoreVec[i] = historyScore;
	}
	return scoreVec;
}
void Wowl::pickNextMove(std::vector<Move>& moveVec, std::vector<int>& scoreVec, int startpos) {
	int size = scoreVec.size();
	assert(moveVec.size() == size);
	int best = -mateScore;
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
	int best = -mateScore;
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
	for (int i = 0; i < maxSearchDepth; ++i) {
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
void Wowl::reduceHistory() {
	for (int i = 0; i < 2; ++i) {
		for (int j = 0; j < 6; ++j) {
			for (int k = 0; k < 120; k++) {
				historyMoves[i][j][k] /= 2;
			}
		}
	}
}

int Wowl::probeTT(const U64 key, int depth, int ply, int alpha, int beta) {
	auto it = tt.hashTable.find(key);
	if (it != tt.hashTable.end()) {
		if (it->second.depth >= depth) {
			if (it->second.flag == tt.HASH_EXACT) {
				if (ply == 0) { bestMove = it->second.hashBestMove; }
				return it->second.score;
			}
			else if (it->second.flag == tt.HASH_ALPHA) {
				if (it->second.score <= alpha) {
					return alpha;
				}
			}
			else if (it->second.flag == tt.HASH_BETA) {
				if (it->second.score >= beta) {
					return beta;
				}
			} 
		}
	}
	return NO_ENTRY;
}
void Wowl::storeTT(const U64 key, int depth, int score, int flag) {
	auto it = tt.hashTable.find(key);
	if (it != tt.hashTable.end()) {
		if (it->second.depth <= depth) {
			it->second.depth = depth;
			it->second.score = score;
			it->second.flag = flag;
		}
	}
	else {
		tt.hashTable.emplace(std::piecewise_construct, std::forward_as_tuple(key), std::forward_as_tuple(depth, score, flag, 0));
	}
}
void Wowl::ageTT() {
	for (auto it = tt.hashTable.begin(); it != tt.hashTable.end();) {
		if (it->second.age < ttAgeLimit) {
			it->second.age++;
			it++;
		}
		else {
			it = tt.hashTable.erase(it);
		}
	}
}

bool Wowl::timeOver(clock_t startTime, double moveTime) {
	return (double(clock() - startTime) / (CLOCKS_PER_SEC / 1000) >= moveTime);
}
int Wowl::qSearch(Board& b, Evaluation& e, int alpha, int beta, int color) {

	int standPat = e.totalEvaluation(b, color, b.lazyScore);
	if (standPat >= beta) {
		return beta;
	}
	if (standPat > alpha) {
		alpha = standPat;
	}

	auto captures = b.getCaptures();
	if (captures.empty()) { return standPat; }
	int size = captures.size();

	orderCaptures(b, captures);


	for (int i = 0; i < size; ++i) {
		Move c = captures[i];

		bool isPromotion = b.mailbox[c.from] == b.WP * color && (c.to / 10 == 2 || c.to / 10 == 9);

		// Delta pruning
		if (!isPromotion && standPat + deltaMargin + e.pieceValues[abs(b.mailbox[c.to]) - 1] < alpha) {
			continue;
		}

		// Negative SEE pruning
		if (!isPromotion && SEE(b, e, c.to, color) < 0) {
			continue;
		}

		int mailboxCopy[120], castlingCopy[4], kingCopy[2], lazyCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		memcpy(lazyCopy, b.lazyScore, sizeof(b.lazyScore));
		int epCopy = b.epSquare;

		int lazyIndex = !(color == b.WHITE);
		b.lazyScore[!lazyIndex] -= e.pieceValues[abs(b.mailbox[c.to]) - 1];
		b.lazyScore[!lazyIndex] -= e.PST(b, c.to, -color);
		if (isPromotion) { b.lazyScore[lazyIndex] += e.pieceValues[4] - e.pieceValues[0]; }
		int preMovePST = e.PST(b, c.from, color);
		b.move(c.from, c.to, false);
		b.lazyScore[lazyIndex] += e.PST(b, c.to, color) - preMovePST;

		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy, epCopy, false);
			continue;
		}

		int score = -qSearch(b, e, -beta, -alpha, -color);

		qSearchNodes++;
		b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy, epCopy, false);

		if (score >= beta) {
			return beta;
		}
		if (score > alpha) {
			alpha = score;
		}
	}
	return alpha;
}
int Wowl::negaSearch(Board& b, int depth, int ply, int color, int alpha, int beta, bool can_null, clock_t startTime, double stopTime) {

	if (timeOver(startTime, stopTime)) return alpha;

	Evaluation WowlEval;
	U64 key = tt.generatePosKey(b);
	int flag = tt.HASH_ALPHA;
	bool isRoot = (ply == 0);

	if (!isRoot && checkThreefold(key)) { return 0; }
	
	int ttScore = probeTT(key, depth, ply, alpha, beta);
	if (ttScore != NO_ENTRY) {
		return ttScore;
	}

	bool isInCheck = b.inCheck(color);
	bool nearCheckmate = (alpha <= -mateScore + maxSearchDepth);

	if (depth <= 0) {
		if (isInCheck) {
			depth = 1;
		}
		else {
			depth = 0;
			int qScore = qSearch(b, WowlEval, alpha, beta, color);
			storeTT(key, depth, qScore, tt.HASH_EXACT);
			return qScore;
		}
	}

	int eval;
	auto it = tt.hashTable.find(key);
	if (it != tt.hashTable.end()) {
		if (it->second.eval != mateScore) {
			eval = it->second.eval;
		}
		else {
			eval = WowlEval.totalEvaluation(b, color, b.lazyScore);
			tt.hashTable[key].eval = eval;
		}
	}
	else {
		eval = WowlEval.totalEvaluation(b, color, b.lazyScore);
		tt.hashTable[key].eval = eval;
	}

	// Reverse futility pruning
	if (depth <= 6 && !isInCheck && eval - reverseFutilityMargin * depth > beta && !isRoot) {
		return eval - reverseFutilityMargin * depth;
	}

	int score;
	double phase = WowlEval.getPhase(b);

	// Null move pruning
	int nullR = 2 + depth / 6;
	if (nullR > 4) { nullR = 4; }
	if (depth > 2 && can_null && !isInCheck && phase > 0.25) {
		b.nullMove();
		score = -negaSearch(b, depth - 1 - nullR, ply + 1, -color, -beta, -beta + 1, false, startTime, stopTime);
		b.undoNullMove();
		if (score >= beta) {
			return score;
		}
	}

	// Futility pruning
	bool fPrune = false;
	int futilityScore = -mateScore;
	if (depth <= 6 && !isInCheck && !nearCheckmate) {
		futilityScore = eval + futilityMargin * depth;
		fPrune = (futilityScore <= alpha);
	};


	auto legalMoves = b.getMoves();
	int size = legalMoves.size();
	int movesSearched = 0;

	ttMove = NO_MOVE;
	if (tt.hashTable.find(key) != tt.hashTable.end()) {
		ttMove = tt.hashTable.at(key).hashBestMove;
	}
	std::vector<int> scoreVec = scoreMoves(b, WowlEval, legalMoves, color, depth, key, isRoot);

	for (int i = 0; i < size; ++i) {

		pickNextMove(legalMoves, scoreVec, i);

		Move m = legalMoves[i];
		int piece = b.mailbox[m.from];
		int target = b.mailbox[m.to];

		bool isCapture = target != 0;  // Does not consider en passant as capture
		bool isEnPassant = piece == b.WP * color && !isCapture && (abs(m.from - m.to) == 11 || abs(m.from - m.to) == 9);
		bool isPassed = piece == b.WP * color && WowlEval.isPassed(b, m.from, color);
		bool isPromotion = piece == b.WP * color && (m.to / 10 == 2 || m.to / 10 == 9);
		bool isHash = m == ttMove;
		bool isKiller = killerMoves[0][depth] == m || killerMoves[1][depth] == m;
		bool isCastling = piece == b.WK * color && abs(m.from - m.to) == 2;
		bool wouldCheck = b.wouldCheck(m, color);

		bool isDangerous = isPassed || isPromotion || isInCheck || wouldCheck;

		// if (isRoot) { std::cout << b.toNotation(m.from) << b.toNotation(m.to) << " " << isEnPassant << " " << scoreVec[i] << std::endl; }

		if (fPrune && !isDangerous && !isKiller && !isHash && !isRoot && movesSearched > 0) {
			if (!isCapture && !isEnPassant) { continue; }
			if (isCapture && futilityScore + WowlEval.pieceValues[abs(target) - 1] <= alpha) {
				continue;
			}
			if (isEnPassant && futilityScore + WowlEval.pieceValues[0] <= alpha) {
				continue;
			}
		}

		// Copy board state and info before making move
		int mailboxCopy[120], castlingCopy[4], kingCopy[2], lazyCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		memcpy(lazyCopy, b.lazyScore, sizeof(b.lazyScore));
		int epCopy = b.epSquare;

		int lazyIndex = !(color == b.WHITE);
		if (isCapture) { 
			b.lazyScore[!lazyIndex] -= WowlEval.pieceValues[abs(target) - 1]; 
			b.lazyScore[!lazyIndex] -= WowlEval.PST(b, m.to, -color);
		} else if (isEnPassant) {
			b.lazyScore[!lazyIndex] -= WowlEval.pieceValues[0];
			b.lazyScore[!lazyIndex] -= WowlEval.PST(b, m.to + color * 10, -color);
		}
		if (isCastling) {
			b.lazyScore[lazyIndex] += 3;  // Hard-coded PST benefit from moving the rook during castling
		}
		// Currently treats queening as the only possible promotion
		if (isPromotion) { b.lazyScore[lazyIndex] += WowlEval.pieceValues[4] - WowlEval.pieceValues[0]; }
		int preMovePST = WowlEval.PST(b, m.from, color);  // We calculate the PST value before moving in case the piece changes (i.e. promotion)
		b.move(m.from, m.to, false);
		b.lazyScore[lazyIndex] += WowlEval.PST(b, m.to, color) - preMovePST;

		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy, epCopy, false);
			continue;
		}

		negaNodes++;
		tempHashPosVec.emplace_back(key);

		score = alpha + 1;

		// Late move reduction	
		if (movesSearched > 3 && depth > 2 && !isDangerous && !isCapture && !isEnPassant && !isKiller && !isHash) {
			int lateR = 1 + (depth > 3) * movesSearched / 14;
			if (lateR > depth - 2) { lateR = depth - 2; }
			score = -negaSearch(b, depth - lateR - 1, ply + 1, -color, -alpha - 1, -alpha, true, startTime, stopTime);
		}

		if (score > alpha) {
			score = -negaSearch(b, depth - 1, ply + 1, -color, -beta, -alpha, true, startTime, stopTime);
		}

		b.undo(mailboxCopy, castlingCopy, kingCopy, lazyCopy, epCopy, false);
		movesSearched++;
		tempHashPosVec.pop_back();

		if (timeOver(startTime, stopTime) && !isRoot) return alpha;

		if (score >= beta) {
			if (depth > 2) {
				if (movesSearched == 1) { failHighFirst++; }
				failHigh++;
			}
			storeTT(key, depth, beta, tt.HASH_BETA);
			if (!isCapture && !isEnPassant && !isPromotion) {
				// Killer moves
				if (m != killerMoves[0][depth]) {
					killerMoves[1][depth] = killerMoves[0][depth];
					killerMoves[0][depth] = m;
				}
				// History heuristic
				historyMoves[(color != b.WHITE)][abs(piece) - 1][m.to] += depth * depth;
			}
			return score;
		}
		if (score > alpha) {
			alpha = score;
			if (isRoot) { bestMove = m; }
			tt.hashTable[key].hashBestMove = m;
			flag = tt.HASH_EXACT;
		}
	}

	if (movesSearched == 0) {
		if (isRoot) { bestMove = NO_MOVE;}
		return (isInCheck) ? -mateScore + ply : 0;
	}

	storeTT(key, depth, alpha, flag);
	return alpha;
}
void Wowl::ID(Board& b, const Evaluation& e, int max_depth, int color, double moveTime) {

	initMVVLVA(b, e);
	resetMoveHeuristics();
	bestMove = NO_MOVE;

	for (int i = 0; i < maxSearchDepth; ++i) {
		PVLine[i] = NO_MOVE;
		IDMoves[i] = NO_MOVE;
	}

	int idAlpha = -mateScore;
	int idBeta = mateScore;
	int window = 50;

	int totalNodes = 0;
	int highestDepth = 0;

	auto startTime = clock();
	std::cout.precision(3);

	for (int idepth = 1; idepth <= max_depth; ++idepth) {

		negaNodes = 0, qSearchNodes = 0, failHigh = 0, failHighFirst = 0, pvCounter = 0;

		if (double(clock() - startTime) / (CLOCKS_PER_SEC / 1000) >= moveTime) { break; }

		tempHashPosVec = hashPosVec;

		bestScore = negaSearch(b, idepth, 0, color, idAlpha, idBeta, true, startTime, moveTime);
		totalNodes += negaNodes + qSearchNodes;

		// UCI output
		std::cout << "info score cp " << int(bestScore / 1.2) << " depth " << idepth << " nodes " << negaNodes + qSearchNodes;
		std::cout << " time " << (clock() - startTime) / (CLOCKS_PER_SEC / 1000);
		std::cout << " pv ";
		getPVLine(b, tt.generatePosKey(b));
		for (int i = 0; i < idepth; ++i) {
			if (PVLine[i] != NO_MOVE) {
				bool isPromotion = abs(b.mailbox[PVLine[i].from]) == b.WP && (PVLine[i].to / 10 == 2 || PVLine[i].to / 10 == 9);
				std::cout << b.toNotation(PVLine[i].from) << b.toNotation(PVLine[i].to);
				if (isPromotion) {
					std::cout << "q";
				}
				std::cout << " ";
			}
		}
		std::cout << "\n";
		std::cout << "move ordering " << double(failHighFirst) / failHigh * 100 << std::endl;

		if ((bestScore <= idAlpha) || (bestScore >= idBeta)) {
			idBeta += window;
			idAlpha -= window;
			window += window / 2;
			idepth--;
			continue;
		}

		window = 50;
		if (idepth >= 5) {
			idAlpha = bestScore - window;
			idBeta = bestScore + window;
		}
		else {
			idAlpha = -mateScore;
			idBeta = mateScore;
		}

		IDMoves[idepth - 1] = bestMove;
		highestDepth++;

		if (bestScore >= mateScore) {
			break;
		}
	}

	// We only use best moves from full searches
	if (highestDepth < 0) { highestDepth = 0; }
	bestMove = IDMoves[highestDepth - 1];

	// PV failsafe
	if (bestMove == NO_MOVE) { bestMove = PVLine[0]; }

	if (bestMove != NO_MOVE) {
		// Add q suffix to move output if promotion
		bool isPromotion = abs(b.mailbox[bestMove.from]) == b.WP && (bestMove.to / 10 == 2 || bestMove.to / 10 == 9);
		std::cout << "bestmove " << b.toNotation(bestMove.from) << b.toNotation(bestMove.to);
		if (isPromotion) {
			std::cout << "q";
		}
		std::cout << " ";
	}
	else {
		std::cout << "bestmove 0000 ";
	}
	std::cout << "\n";

	ageTT();
}

void Wowl::getPVLine(Board b, U64 key) {
	Move move = NO_MOVE;
	if (tt.hashTable.find(key) != tt.hashTable.end()) {
		move = tt.hashTable.at(key).hashBestMove;
	}
	int count = 0;
	while (move != NO_MOVE) {
		if (count >= maxSearchDepth) { break; }
		PVLine[count] = move;
		count++;
		b.move(move.from, move.to, true);
		U64 poskey = tt.generatePosKey(b);
		move = NO_MOVE;
		if (tt.hashTable.find(poskey) != tt.hashTable.end()) {
			move = tt.hashTable.at(poskey).hashBestMove;
		}
	}
}
long Wowl::perft(Board& b, Evaluation& e, int depth, int ply) {

	int totalNodes = 0;
	bool isRoot = (ply == 0);

	if (depth == 0) { return 1; }

	auto legalMoves = b.getMoves();

	bool haveMove = false;

	for (const auto& i : legalMoves) {
		int mailboxCopy[120], castlingCopy[4], kingCopy[2];
		memcpy(mailboxCopy, b.mailbox, sizeof(b.mailbox));
		memcpy(castlingCopy, b.castling, sizeof(b.castling));
		memcpy(kingCopy, b.kingSquare, sizeof(b.kingSquare));
		int epCopy = b.epSquare;

		b.move(i.from, i.to, false);
		if (b.inCheck(b.getTurn() * -1)) {
			b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore, epCopy, false);
			continue;
		}
		haveMove = true;
		int nodes = perft(b, e, depth - 1, ply + 1);
		totalNodes += nodes;
		if (isRoot) { std::cout << b.toNotation(i.from) << b.toNotation(i.to) << " " << nodes << std::endl; }
		b.undo(mailboxCopy, castlingCopy, kingCopy, b.lazyScore, epCopy, false);
	}
	if (isRoot) { std::cout << "total " << totalNodes << std::endl; }
	if (!haveMove) { return 0; }
	return totalNodes;
}