#include "stdafx.h"
#include "Wowl.h"

void Wowl::orderMoves(Board b, std::vector<sf::Vector2i>& lmV) {
	for (int i = 0; i < lmV.size(); i++) {
		//Captures
		if (b.mailbox[lmV[i].y] != 0) {
			lmV.insert(lmV.begin(), lmV[i]);
			lmV.pop_back();
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
	if (depth == SEARCH_DEPTH) {
		orderMoves(b, b.legalMoveVec);
	}

	//Check for end state
	if (size == 0) {
		return -color * WIN_SCORE;
	}

	//Terminate at end node
	if (depth == 0) {
		return color * WowlEval.totalEvaluation(b, WHITE);
	}

	for (int j = 0; j < size; j++) {
		b.move(b.legalMoveVec[j].x, b.legalMoveVec[j].y);
		score = -negaMax(b, depth - 1, -color, -beta, -alpha);
		b.undo();
		if (score > max) {
			max = score;
			if (depth == SEARCH_DEPTH) {
				bestMove.x = b.legalMoveVec[j].x;
				bestMove.y = b.legalMoveVec[j].y;
				std::cout << "move : " << b.legalMoveVec[j].x << " " << b.legalMoveVec[j].y << std::endl;
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