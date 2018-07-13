#include "stdafx.h"
#include "Wowl.h"

int Wowl::SEE(Board b, int square, int color) {
	int oldsqr;
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = b.mailbox[square];
	int targetval = target;

	if (piece > 0) {
		if (!(target == -9 || target == 0)) {
			oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
			b.move(oldsqr, square);
			if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
				b.undo();
			}
			else {
				switch (abs(target)) {
				case WP:
					targetval = P_BASE_VAL;
					break;
				case WN:
					targetval = N_BASE_VAL;
					break;
				case WB:
					targetval = B_BASE_VAL;
					break;
				case WR:
					targetval = R_BASE_VAL;
					break;
				case WQ:
					targetval = Q_BASE_VAL;
					break;
				case WK:
					targetval = K_BASE_VAL;
					break;
				}
				assert(targetval > 0);
				int netMaterial = targetval - SEE(b, square, -color);
				if (netMaterial > 0) {
					val = netMaterial;
				}
				else {
					val = 0;
				}
				b.undo();
			}
		}
	}
	return val;
}

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV, int depth, int initial, U64 poskey) {
	if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
		int val = hashTable.tt.at(poskey).hashBestMove;
		hashMove.x = (val - val % 100) / 100;
		hashMove.y = val % 100;
	}
	for (int i = 0; i < lmV.size(); i++) {
		//Move stored in hash table
		if (lmV[i] == hashMove) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		//Good captures
		if (SEE(b, lmV[i].y, b.getTurn()) > 0 && std::get<1>(b.getSmallestAttacker(lmV[i].y, b.getTurn())) == lmV[i].x) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		//Killer moves
		if (lmV[i].x == (killerMoves[0][depth] - killerMoves[0][depth] % 100) / 100 && lmV[i].y == killerMoves[0][depth] % 100) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
		if (lmV[i].x == (killerMoves[1][depth] - killerMoves[1][depth] % 100) / 100 && lmV[i].y == killerMoves[1][depth] % 100) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.erase(lmV.begin() + i + 1);
			continue;
		}
	}
}
void Wowl::resetKillerMoves() {
	for (int i = 0; i < SEARCH_DEPTH; i++) {
		killerMoves[0][i] = 0;
		killerMoves[1][i] = 0;
	}
}

void Wowl::storeMoveInfo(U64 k, int d, int maxScore, int a, int b) {
	hashTable.tt[k].hashDepth = d;
	hashTable.tt[k].hashScore = maxScore;
	hashTable.tt[k].hashAge = 0;
	if (maxScore <= a) {
		hashTable.tt[k].hashFlag = HASH_UPPER_BOUND;
	}
	else if (maxScore >= b) {
		hashTable.tt[k].hashFlag = HASH_LOWER_BOUND;
	}
	else {
		hashTable.tt[k].hashFlag = HASH_EXACT;
	}

}

int Wowl::qSearch(Board b, int alpha, int beta, int color) {

	int score;
	Evaluation qEval;

	int stand_pat = qEval.totalEvaluation(b, color);

	if (stand_pat >= beta) {
		return beta;
	}
	if (stand_pat > alpha) {
		alpha = stand_pat;
	}

	b.getCaptureMoves();
	int size = b.captureVec.size();
	for (int j = 0; j < size; j++) {
		if (SEE(b, b.captureVec[j].y, color) > 0) {
			qSearchNodes++;
			b.move(b.captureVec[j].x, b.captureVec[j].y);
			score = -qSearch(b, -beta, -alpha, -color);
			b.undo();
			if (score >= beta) {
				return beta;
			}
			if (score > alpha) {
				alpha = score;
			}
		}	
	}

	return alpha;
}
int Wowl::negaSearch(Board b, int depth, int initial, int color, int alpha, int beta) {

	Evaluation WowlEval;
	U64 key = hashTable.generatePosKey(b);

	//Look up transposition table
	if (hashTable.tt.find(key) != hashTable.tt.end()) {
		if (hashTable.tt.at(key).hashDepth >= depth) {
			if (hashTable.tt.at(key).hashFlag == HASH_EXACT) {
				return hashTable.tt.at(key).hashScore;
			}
			else if (hashTable.tt.at(key).hashFlag == HASH_UPPER_BOUND) {
				if (beta > hashTable.tt.at(key).hashScore) {
					beta = hashTable.tt.at(key).hashScore;
				}
			}
			else if (hashTable.tt.at(key).hashFlag == HASH_LOWER_BOUND) {
				if (alpha < hashTable.tt.at(key).hashScore) {
					alpha = hashTable.tt.at(key).hashScore;
				}
			}
			if (alpha >= beta) {
				return hashTable.tt.at(key).hashScore;
			}
		}
	}

	if (depth == SEARCH_DEPTH) {
		for (auto entry : hashTable.tt) {
			entry.second.hashAge++;
			if (entry.second.hashAge >= TT_CLEAR_AGE) {
				hashTable.tt.erase(entry.first);
			}
		}
	}

	if (depth == 0) {
		negaNodes++;
		return qSearch(b, alpha, beta, color);
	}

	int score;
	int max = -WIN_SCORE;

	b.getLegalMoves();
	int size = b.legalMoveVec.size();
	if (size == 0) {
		if (b.checkKing(color, b.mailbox)) {
			return -WIN_SCORE;
		}
		else {
			return 0;
		}
	}

	orderMoves(b, b.legalMoveVec, depth, initial, key);

	for (int j = 0; j < size; j++) {
		b.move(b.legalMoveVec[j].x, b.legalMoveVec[j].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		else {
			negaNodes++;
			//NegaScout
			if (j == 0) {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
			}
			else {
				score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha);
				if (alpha < score && score < beta) {
					int scoutScore = -negaSearch(b, depth - 1, initial, -color, -beta, -score);
					if (scoutScore > score) {
						score = scoutScore;
					}
				}
			}
		}
		b.undo();
		if (score > max) {
			max = score;
			//Store best move in hash table (replace if depth is greater)
			if (hashTable.tt.find(key) == hashTable.tt.end()) {
				hashTable.tt[key].hashBestMove = b.legalMoveVec[j].x * 100 + b.legalMoveVec[j].y;
			}
			else {
				if (hashTable.tt.at(key).hashDepth <= depth) {
					hashTable.tt[key].hashBestMove = b.legalMoveVec[j].x * 100 + b.legalMoveVec[j].y;
				}
			}
			//Update best move
			if (depth == SEARCH_DEPTH) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
			}
			if (depth == SEARCH_DEPTH) {
				std::cout << "best move : " << bestMove.x << " " << bestMove.y << " at depth " << depth << " by " << color << std::endl;
				std::cout << "best score : " << max << std::endl << std::endl;
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			//Store killer move
			killerMoves[1][depth] = killerMoves[0][depth];
			killerMoves[0][depth] = b.legalMoveVec[j].x * 100 + b.legalMoveVec[j].y;
			break;
		}
	}

	//Store in hash table
	if (hashTable.tt.find(key) == hashTable.tt.end()) {
		storeMoveInfo(key, depth, max, alpha, beta);
	}
	else {
		if (hashTable.tt.at(key).hashDepth <= depth) {
			storeMoveInfo(key, depth, max, alpha, beta);
		}
	}
	return max;
}
void Wowl::DLS(Board b, int depth, int color) {

	negaNodes = 0;
	qSearchNodes = 0;
	b.setEnPassantSquare();
	resetKillerMoves();

	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	bool research = false;

	for (int idepth = 1; idepth < SEARCH_DEPTH; idepth++) {

		estimate = negaSearch(b, idepth, idepth, color, id_alpha, id_beta);

		std::cout << id_alpha << " " << id_beta << " at depth " << idepth << std::endl;
		std::cout << estimate << " at depth " << idepth << std::endl;

		if ((estimate <= id_alpha) || (estimate >= id_beta)) {
			if (estimate == WIN_SCORE || estimate == -WIN_SCORE) {
				break;
			}
			if (research) {
				id_alpha = -WIN_SCORE;
				id_beta = WIN_SCORE;
			}
			else {
				id_alpha = estimate - ASPIRATION_WINDOW * 2;
				id_beta = estimate + ASPIRATION_WINDOW * 2;
			}
			idepth--;
			research = true;
			continue;
		}

		id_alpha = estimate - ASPIRATION_WINDOW;
		id_beta = estimate + ASPIRATION_WINDOW;
		research = false;
	}
	std::cout << id_alpha << " " << id_beta << std::endl << std::endl;
	std::cout << estimate << " at final depth" << std::endl;

	negaSearch(b, depth, depth, color, id_alpha, id_beta);

	std::cout << "Nodes explored in negaSearch : " << negaNodes << std::endl;
	std::cout << "Nodes explored in qSearch : " << qSearchNodes << std::endl;
}

long Wowl::perft(Board b, int depth) {

	int nodes = 0;
	if (depth == 0) { return 1; }

	b.getLegalMoves();
	int size = b.legalMoveVec.size();

	for (int i = 0; i < size; i++) {
		b.move(b.legalMoveVec[i].x, b.legalMoveVec[i].y);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
			continue;
		}
		nodes += perft(b, depth - 1);
		b.undo();
		if (depth == SEARCH_DEPTH) {
			std::cout << nodes << std::endl;
		}
	}

	return nodes;
}