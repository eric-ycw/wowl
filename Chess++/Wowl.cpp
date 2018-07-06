#include "stdafx.h"
#include "Wowl.h"

void Wowl::orderMoves(Board b) {
	for (int i = 0; i < b.legalMoveVec.size(); i++) {
		//Check pieces first
		if (b.mailbox[b.legalMoveVec[i].y] != 0 && b.mailbox[b.legalMoveVec[i].y] != 1) {
			b.legalMoveVec.insert(b.legalMoveVec.begin(), b.legalMoveVec[i]);
			b.legalMoveVec.erase(b.legalMoveVec.begin() + i);
		}
		//If move is a capture, put it at front
		if (b.mailbox[b.legalMoveVec[i].y] != 0) {
			b.legalMoveVec.insert(b.legalMoveVec.begin(), b.legalMoveVec[i]);
			b.legalMoveVec.erase(b.legalMoveVec.begin() + i);
		}
	}
}

int Wowl::negaMax(Board b, int depth, int color, int alpha, int beta) {

	int score;
	Evaluation WowlEval;
	int max = -WIN_SCORE;
	int a = alpha;

	//Get legal moves and order them
	b.getLegalMoves();
	int size = b.legalMoveVec.size();
	orderMoves(b);

	//Check for end state
	if (size == 0) {
		return -color * WIN_SCORE;
	}

	//Terminate at end node
	if (depth == 0) {
		return color * WowlEval.totalEvaluation(b, WHITE);
	}

	for (int j = 0; j < size; j++) {
		b.move(b.legalMoveVec.at(j).x, b.legalMoveVec.at(j).y);
		score = -negaMax(b, depth - 1, -color, -beta, -alpha);
		b.undo();
		if (score > max) {
			max = score;
			if (depth == SEARCH_DEPTH) {
				bestMove.x = b.legalMoveVec.at(j).x;
				bestMove.y = b.legalMoveVec.at(j).y;
				std::cout << "move : " << b.legalMoveVec.at(j).x << " " << b.legalMoveVec.at(j).y << std::endl;
				std::cout << "score : " << score << std::endl << std::endl;
			}
		}
		if (score > alpha) {
			alpha = score;
		}
		if (alpha >= beta) {
			b.undo();
			break;
		}
	}

	return max;
}