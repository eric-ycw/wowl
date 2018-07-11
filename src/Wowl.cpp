#include "stdafx.h"
#include "Wowl.h"

int Wowl::SEE(Board& b, int square, int color) {
	int val = 0;
	int piece = std::get<0>(b.getSmallestAttacker(square, color));
	int target = b.mailbox[square];
	int oldsqr;
	if (abs(piece)> 0) {
		oldsqr = std::get<1>(b.getSmallestAttacker(square, color));
		b.move(oldsqr, square);
		if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
			b.undo();
		}
		else {
			int netMaterial = abs(target) - SEE(b, square, -color);
			if (netMaterial > 0) {
				val = netMaterial;
			}
			else {
				val = 0;
			}
			b.undo();
		}

	}
	return val;
}

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV, int depth, int initial, U64 poskey) {
	if (depth != SEARCH_DEPTH) {
		if (hashTable.tt.find(poskey) != hashTable.tt.end()) {
			int val = hashTable.tt.at(poskey).hashBestMove;
			hashMove.x = (val - val % 100) / 100;
			hashMove.y = val % 100;
		}
	}
	for (int i = 0; i < lmV.size(); i++) {
		if (depth == SEARCH_DEPTH) {
			//Move from iterative deepening
			if (lmV[i] == priorityMove) {
				lmV.insert(lmV.begin(), lmV[i]);
				lmV.erase(lmV.begin() + i + 1);
			}
		}
		else {
			//Move stored in hash table
			if (lmV[i] == hashMove) {
				lmV.insert(lmV.begin(), lmV[i]);
				lmV.erase(lmV.begin() + i + 1);
			}
		}
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
			b.move(b.captureVec[j].x, b.captureVec[j].y);
			if (b.checkKing(b.getTurn() * -1, b.mailbox)) {
				b.undo();
				continue;
			}
			else {
				score = -qSearch(b, -beta, -alpha, -color);
				b.undo();
			}
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
			else if (hashTable.tt.at(key).hashFlag == HASH_ALPHA) {
				if (beta > hashTable.tt.at(key).hashScore) {
					beta = hashTable.tt.at(key).hashScore;
				}
			}
			else if (hashTable.tt.at(key).hashFlag == HASH_BETA) {
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
			//NegaScout
			if (j == 0) {
				score = -negaSearch(b, depth - 1, initial, -color, -beta, -alpha);
			}
			else {
				score = -negaSearch(b, depth - 1, initial, -color, -alpha - 1, -alpha);
				if (alpha < score && score < beta && depth > 1) {
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
			//Store best move in hash table
			hashTable.tt[key].hashBestMove = b.legalMoveVec[j].x * 100 + b.legalMoveVec[j].y;
			//Update best move
			if (depth == initial) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
			}
			if (depth == SEARCH_DEPTH) {
				std::cout << "best move : " << bestMove.x << " " << bestMove.y << " at depth " << depth << " by " << color << std::endl;
				std::cout << "best score : " << max << std::endl;
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			break;
		}
	}

	//Store in hash table
	hashTable.tt[key].hashDepth = depth;
	hashTable.tt[key].hashScore = max;
	hashTable.tt[key].hashAge = 0;
	if (max <= alpha) {
		hashTable.tt[key].hashFlag = HASH_ALPHA;
	}
	else if (max >= beta) {
		hashTable.tt[key].hashFlag = HASH_BETA;
	}
	else {
		hashTable.tt[key].hashFlag = HASH_EXACT;
	}

	return max;
}
void Wowl::ID(Board b, int depth, int color) {
	int id_alpha = -WIN_SCORE;
	int id_beta = WIN_SCORE;
	b.setEnPassantSquare();
	for (int idepth = 1; idepth < SEARCH_DEPTH; idepth++) {
		//Save move for use in move ordering
		priorityMove.x = -1;
		priorityMove.y = -1;
		estimate = negaSearch(b, idepth, idepth, color, id_alpha, id_beta);
		priorityMove = bestMove;

		std::cout << estimate << " at depth " << idepth << std::endl << std::endl;
		std::cout << "ID best move is " << priorityMove.x << " " << priorityMove.y << " at depth " << idepth << std::endl;

		if ((estimate <= id_alpha) || (estimate >= id_beta)) {
			if (estimate == WIN_SCORE || estimate == -WIN_SCORE) {
				break;
			}
			id_alpha = -WIN_SCORE;
			id_beta = WIN_SCORE;
			idepth--;
			continue;
		}
		id_alpha = estimate - ASPIRATION_WINDOW;
		id_beta = estimate + ASPIRATION_WINDOW;
	}
	int nodes = 0;
	negaSearch(b, depth, depth, color, id_alpha, id_beta);
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