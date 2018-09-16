#include "UCI.h"

#define MAX_INPUT = 2048;

void parsePosition(Board& b, Wowl& AI, std::string line) {
	// Remove "position " from input
	line.erase(0, 9);

	if (!line.compare(0, 8, "startpos")) {
		b.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n");
	}
	else {
		if (line.find("fen") != std::string::npos) {
			line.erase(0, 4);
			b.parseFEN(line);
		}
		else {
			// Default to startpos
			b.parseFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1\n");
		}
	}

	std::vector<std::string> moveStrVec;
	moveStrVec.reserve(50);
	size_t pos;
	std::string movestr;

	if ((pos = line.find("moves")) != std::string::npos) {
		line.erase(0, pos + 6);

		// Failsafe if last character is not newline
		if (line.back() != '\n') {
			line.insert(line.end(), '\n');
		}
		line.insert(line.end() - 1, ' ');  // Insert space after the last move so it can be parsed

		while ((pos = line.find(' ')) != std::string::npos) {
			movestr = line.substr(0, pos);
			assert(movestr.length() == 4 || movestr.length() == 5);  // Underpromotions default to queen
			moveStrVec.emplace_back(movestr);
			line.erase(0, pos + 1);
		}
	}

	// Play out moves
	if (!moveStrVec.empty()) {
		for (int i = 0; i < moveStrVec.size(); ++i) {
			movestr = moveStrVec[i];
			std::cout << movestr << std::endl;
			int move_coords[2] = { b.toCoord(movestr[0], movestr[1]), b.toCoord(movestr[2], movestr[3]) };
			b.move(move_coords[0], move_coords[1], false);
			AI.hashPosVec.emplace_back(AI.tt.generatePosKey(b));
		}
	}

	b.outputBoard();
	Evaluation e;
	b.lazyScore[0] = e.lazyEvaluation(b, b.WHITE);
	b.lazyScore[1] = e.lazyEvaluation(b, b.BLACK);
	std::cout << e.totalEvaluation(b, b.WHITE, b.lazyScore) << std::endl;
	b.setEnPassantSquare();
}
void parseGo(Board& b, Evaluation& e, Wowl& AI, std::string line) {
	int depth = -1, movetime = -1, time = -1;
	int movestogo = 40;
	int increment = 0;
	int timeleft = 0;

	size_t pos;
	if ((pos = line.find("winc")) != std::string::npos && b.turn == b.WHITE) {
		increment = std::stoi(line.substr(pos + 5));
	}
	if ((pos = line.find("binc")) != std::string::npos && b.turn == b.BLACK) {
		increment = std::stoi(line.substr(pos + 5));
	}
	if ((pos = line.find("wtime")) != std::string::npos && b.turn == b.WHITE) {
		time = std::stoi(line.substr(pos + 6));
	}
	if ((pos = line.find("btime")) != std::string::npos && b.turn == b.BLACK) {
		time = std::stoi(line.substr(pos + 6));
	}
	if ((pos = line.find("depth")) != std::string::npos) {
		depth = std::stoi(line.substr(pos + 6));
	}
	if ((pos = line.find("movestogo")) != std::string::npos) {
		movestogo = std::stoi(line.substr(pos + 10));
	}
	if ((pos = line.find("movetime")) != std::string::npos) {
		movetime = std::stoi(line.substr(pos + 9));
	}

	if (movetime != -1) {
		time = movetime;
		movestogo = 1;
	}

	if (time != -1) {
		timeleft = time;
		time /= movestogo;
		time -= 100;
	}

	if (depth == -1) {
		depth = AI.maxSearchDepth;
	}

	double total = time + increment * 0.9;
	if (timeleft != -1 && total > timeleft - 100) {
		total = std::min(timeleft * 0.75, total * 0.75);
	}
	AI.ID(b, e, depth, b.getTurn(), total);
}

int main() {
	Board b;
	Evaluation e;
	Wowl AI;

	std::cout << "id name Wowl\n";
	std::cout << "id author Plan B\n";
	std::cout << "uciok\n";

	std::string input;
	while (1) {
		std::getline(std::cin, input);
		if (input == "\n" || input == "") { continue; }

		if (!input.compare(0, 7, "isready")) {
			std::cout << "readyok\n";
			continue;
		}
		else if (!input.compare(0, 8, "position")) {
			parsePosition(b, AI, input);
		}
		else if (!input.compare(0, 10, "ucinewgame")) {
			parsePosition(b, AI, "position startpos\n");
		}
		else if (!input.compare(0, 2, "go")) {
			parseGo(b, e, AI, input);
		}
		else if (!input.compare(0, 3, "uci")) {
			std::cout << "id name Wowl\n";
			std::cout << "id author Plan B\n";
			std::cout << "uciok\n";
		}
		else if (!input.compare(0, 5, "perft")) {
			size_t pos = input.find("perft");
			int depth = std::stoi(input.substr(pos + 6));
			AI.perft(b, e, depth, depth);
		}
		else if (!input.compare(0, 4, "quit")) {
			break;
		}
	}
}