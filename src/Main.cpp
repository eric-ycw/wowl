#include "Wowl.h"
int main() 
{
	Board b;
	Evaluation e;
	Wowl AI;

	int AIturn = 0;
	std::string AIturnstr;
	std::cout << "Choose which color to play as (w/b/noai)" << std::endl;

	while (1) {
		std::getline(std::cin, AIturnstr);
		std::cout << std::endl;
		if (AIturnstr == "w") {
			AIturn = b.BLACK;
			break;
		}
		else if (AIturnstr == "b") {
			AIturn = b.WHITE;
			break;
		}
		else if (AIturnstr == "noai") {
			AIturn = 0;
			break;
		}
		else {
			std::cout << "Invalid input" << std::endl;
		}
	}

	b.outputBoard();

	while (1) {
		if (b.getTurn() != AIturn) {
			std::string movestr;
			while (1) {
				std::getline(std::cin, movestr);
				if (movestr.length() != 4) {
					std::cout << "Invalid input" << std::endl;
					continue;
				}
				int move_coords[2] = { b.toCoord(movestr[0], movestr[1]), b.toCoord(movestr[2], movestr[3]) };
				if (!b.checkMoveCheck(move_coords[0], move_coords[1]) && b.checkLegal(move_coords[0], move_coords[1])) {
					b.setEnPassantSquare();
					b.move(move_coords[0], move_coords[1]);
					AI.hashPosVec.emplace_back(AI.hashTable.generatePosKey(b));
					b.outputBoard();
					break;
				}
				else {
					std::cout << "Illegal move" << std::endl;
				}
			}
		}
		else if (b.getTurn() == AIturn) {
			AI.ID(b, e, AI.MAX_SEARCH_DEPTH, AIturn, 10);
			if (AI.finalBestMove.from != AI.NO_MOVE && AI.finalBestMove.to != AI.NO_MOVE) {
				b.move(AI.finalBestMove.from, AI.finalBestMove.to);
				AI.hashPosVec.emplace_back(AI.hashTable.generatePosKey(b));
				std::cout << "Wowl played : " << b.toNotation(AI.finalBestMove.from) << b.toNotation(AI.finalBestMove.to) << std::endl;
				b.outputBoard();
				std::cout << e.totalMobility(b, b.WHITE) << " " << e.totalMobility(b, b.BLACK) << std::endl << std::endl;
			}
		}
	}
}