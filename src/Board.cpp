#include "Board.h"

int Board::toCoord(char a, char b) {
	int x = int(a) - 97;
	int y = 7 - int(b) + 49;
	int pos = mailbox64[y][x];
	return pos;
}
//Converts mailbox coord to 64 coord
int Board::to64Coord(int square) const {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = square - 21 - 2 * (tenth - 2);
	return rval;
}
std::string Board::toNotation(int square) const {
	std::string s = "  ";
	int coord = to64Coord(square);
	int x = coord % 8;
	int y = floor(coord / 8);
	s[0] = char(x + 97);
	s[1] = char(7 - y + 49);
	return s;
}

void Board::reserveVectors() {
	moveVec.reserve(150);
	legalMoveVec.reserve(50);
	captureVec.reserve(30);
}

//Check if move is pseudolegal
bool Board::checkLegal(int a, int b) {
	if (a < 0 || b < 0 || a > 119 || b > 119) { return false; }

	int oldSquareVal = mailbox[a];
	int newSquareVal = mailbox[b];

	//Turn + own piece
	if (oldSquareVal * turn <= 0 || newSquareVal * turn > 0) {
		return false;
	}

	//Out of bounds
	if (newSquareVal == -9) {
		return false;
	}

	//Destination cannot be king
	if (abs(newSquareVal) == 6) {
		return false;
	}

	int sval;
	if (oldSquareVal < 0) {
		sval = oldSquareVal * -1;
	}
	else {
		sval = oldSquareVal;
	}
	switch (sval) {
	case WP:
		return checkLegalPawn(a, b, oldSquareVal);
		break;
	case WN:
		return checkAttackKnight(a, b);
		break;
	case WB:
		return checkAttackBishop(a, b);
		break;
	case WR:
		return checkAttackRook(a, b);
		break;
	case WQ:
		return checkAttackQueen(a, b);
		break;
	case WK:
		return checkLegalKing(a, b, turn);
		break;
	}
	return true;
}
bool Board::checkLegalPawn(int a, int b, int color) const {
	int target = mailbox[b];

	//Move forward one square
	if (a - color * 10 == b) {
		if (target == 0) {
			return true;
		}
	}
	//Diagonal capture
	if (a - color * 10 - 1 == b || a - color * 10 + 1 == b) {
		if (target * color < 0) {
			return true;
		}
	}

	if (color == WHITE) {
		//Pawns cannot move backwards
		if (b > a) {
			return false;
		}
		//Check for second-rank pawns
		if (a <= 88 && a >= 81) {
			if (a - 20 == b) {
				if (target == 0 && mailbox[b + 10] == 0) {
					return true;
				}
			}
		}
		//En passant
		//Check if on fifth rank
		if (a <= 58 && a >= 51) {
			//Black pawn adjacency
			if (mailbox[a - 1] == -1 && target == 0 && b == a - 10 - 1) {
				//Check last move
				if (moveVec.back().from == a - 20 - 1 && moveVec.back().to == a - 1) {
					return true;
				}
			}
			if (mailbox[a + 1] == -1 && target == 0 && b == a - 10 + 1) {
				if (moveVec.back().from == a - 20 + 1 && moveVec.back().to == a + 1) {
					return true;
				}
			}
		}
	}
	else if (color == BLACK) {
		if (b < a) {
			return false;
		}
		if (a <= 38 && a >= 31) {
			if (a + 20 == b) {
				if (target == 0 && mailbox[b - 10] == 0) {
					return true;
				}
			}
		}
		if (a <= 68 && a >= 61) {
			if (mailbox[a - 1] == 1 && target == 0 && b == a + 10 - 1) {
				if (moveVec.back().from == a + 20 - 1 && moveVec.back().to == a - 1) {
					return true;
				}
			}
			if (mailbox[a + 1] == 1 && target == 0 && b == a + 10 + 1) {
				if (moveVec.back().from == a + 20 + 1 && moveVec.back().to == a + 1) {
					return true;
				}
			}
		}
	}

	return false;
}
bool Board::checkLegalKing(int a, int b, int color) {
	if (a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b) {
		return true;
	}
	if (a + 2 == b || a - 2 == b) {
		castlingRights = checkCastling();
	}
	if (color == WHITE) {
		if ((a + 2 == b && std::get<0>(castlingRights)) || (a - 2 == b && std::get<1>(castlingRights))) {
			return true;
		}
	}
	else if (color == BLACK) {
		if ((a + 2 == b && std::get<2>(castlingRights)) || (a - 2 == b && std::get<3>(castlingRights))) {
			return true;
		}
	}
	return false;
}
//Get all pseudolegal moves
void Board::getLegalMoves() {
	legalMoveVec.clear();
	for (int m = 21; m < 99; m++) {
		int piece = mailbox[m];
		int abs_piece = piece * turn;
		int tmp;
		if (piece == -9 || piece == 0 || abs_piece < 0) { continue; }
		if (abs_piece == WP || abs_piece == WN || abs_piece == WK) {
			if (abs_piece == WP) {
				tmp = 0;
			}
			else if (abs_piece == WN) {
				tmp = 1;
			}
			else if (abs_piece == WK) {
				tmp = 2;
			}
			for (int n : pieceMoves[tmp]) {
				if (tmp == 0) { n *= turn; }
				if (n == 0 || mailbox[m + n] == -9) {
					continue;
				}
				if (checkLegal(m, m + n)) {
					legalMoveVec.emplace_back(Move(m, m + n));
				}
			}
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == -9) {
					continue;
				}
				if (abs_piece == WB) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0) { continue; }
				}
				else if (abs_piece == WR) {
					if ((m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) { continue; }
				}
				else if (abs_piece == WQ) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0 && (m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) {
						continue; 
					}
				}
				if (checkLegal(m, n)) {
					legalMoveVec.emplace_back(Move(m, n));
				}
			}
		}
	}
}
void Board::getCaptures() {
	captureVec.clear();
	for (int m = 21; m < 99; m++) {
		int piece = mailbox[m];
		int abs_piece = piece * turn;
		int tmp;
		if (piece == -9 || piece == 0 || abs_piece < 0) { continue; }
		if (abs_piece == WP || abs_piece == WN || abs_piece == WK) {
			if (abs_piece == WP) {
				tmp = 0;
			}
			else if (abs_piece == WN) {
				tmp = 1;
			}
			else if (abs_piece == WK) {
				tmp = 2;
			}
			for (int n : pieceMoves[tmp]) {
				if (tmp == 0) { n *= turn; }
				if (n == 0 || mailbox[m + n] == -9 || mailbox[m + n] * piece >= 0) {
					continue;
				}
				if (abs_piece == WP && (n == -10 || n == -20)) { continue; }  //Pawn forward moves can't be captures
				if (checkLegal(m, m + n)) {
					captureVec.emplace_back(Move(m, m + n));
				}
			}
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == -9 || mailbox[n] * piece >= 0) {
					continue;
				}
				if (abs_piece == WB) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0) { continue; }
				}
				else if (abs_piece == WR) {
					if ((m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) { continue; }
				}
				else if (abs_piece == WQ) {
					if ((m - n) % 9 != 0 && (m - n) % 11 != 0 && (m - n) % 10 != 0 && (m - n < -7 || m - n > 7)) {
						continue;
					}
				}
				if (checkLegal(m, n)) {
					captureVec.emplace_back(Move(m, n));
				}
			}
		}
	}
}

bool Board::checkAttack(int a, int b) const {
	if (a < 0 || b < 0 || a > 119 || b > 119) { return false; }

	int oldSquareVal = mailbox[a];
	int newSquareVal = mailbox[b];

	int sval;
	if (oldSquareVal < 0) {
		sval = oldSquareVal * -1;
	}
	else {
		sval = oldSquareVal;
	}
	switch (sval) {
	case WP:
		if (oldSquareVal == 1) {
			return checkAttackPawn(a, b, WHITE);
		}
		else {
			return checkAttackPawn(a, b, BLACK);
		}
		break;

	case WN:
		return checkAttackKnight(a, b);
		break;

	case WB:
		return checkAttackBishop(a, b);
		break;

	case WR:
		return checkAttackRook(a, b);
		break;

	case WQ:
		return checkAttackQueen(a, b);
		break;

	case WK:
		return checkAttackKing(a, b);
		break;

	}
}
bool Board::checkAttackPawn(int a, int b, int color) const {
	if ((a - color * 11 == b || a - color * 9 == b) && mailbox[b] * mailbox[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackKnight(int a, int b) const {
	if ((a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) && mailbox[b] * mailbox[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackBishop(int a, int b) const {
	if ((a - b) % 9 != 0 && (a - b) % 11 != 0) { return false; }
	int checkpos = 0;
	int move_dirs[4] = { 11, 9, -11, -9 };
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			checkpos = a + move_dirs[type] * dis;
			if (mailbox[checkpos] * mailbox[a] != 0) {
				if (checkpos == b) {
					return true;
				}
				break;
			}
			if (checkpos == b) {
				return true;
			}
		}
	}
	return false;
}
bool Board::checkAttackRook(int a, int b) const {
	if ((a - b) % 10 != 0 && (a - b < - 7 || a - b > 7)) { return false; }
	int checkpos = 0;
	int move_dirs[4] = { 1, 10, -1, -10 };
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			checkpos = a + move_dirs[type] * dis;
			if (mailbox[checkpos] * mailbox[a] != 0) {
				if (checkpos == b) { return true; }
				break;
			}
			if (checkpos == b) { return true; }
		}
	}
	return false;
}
bool Board::checkAttackQueen(int a, int b) const {
	if (checkAttackBishop(a, b) || checkAttackRook(a, b)) {
		return true;
	}
	return false;
}
bool Board::checkAttackKing(int a, int b) const {
	if (a + 10 == b || a - 10 == b || a + 10 + 1 == b || a - 10 + 1 == b || a + 10 - 1 == b || a - 10 - 1 == b || a + 1 == b || a - 1 == b) {
		return true;
	}
	return false;
}
std::tuple<int, int> Board::getSmallestAttacker(int square, int color) {

	std::tuple<int, int> attackerArray[5];
	for (int i = 0; i < 5; ++i) {
		std::get<0>(attackerArray[i]) = 0;
		std::get<1>(attackerArray[i]) = 0;
	}

	int sqval;
	if (mailbox[square] * color >= 0) {
		assert(std::get<0>(attackerArray[0]) == 0);
		assert(std::get<1>(attackerArray[0]) == 0);
		return attackerArray[0];
	}

	for (int i = 21; i < 99; ++i) {
		sqval = mailbox[i];
		if (sqval == -9 || sqval * color <= 0) {
			continue;
		}
		if (sqval < 0) {
			sqval *= -1;
		}
		switch (sqval) {
		case WP:
			if (checkAttackPawn(i, square, mailbox[i])) {
				std::get<0>(attackerArray[0])++;
				std::get<1>(attackerArray[0]) = i;
			}
			break;
		case WN:
			if (checkAttackKnight(i, square)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WB:
			if (checkAttackBishop(i, square)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WR:
			if (checkAttackRook(i, square)) {
				std::get<0>(attackerArray[2])++;
				std::get<1>(attackerArray[2]) = i;
			}
			break;
		case WQ:
			if (checkAttackQueen(i, square)) {
				std::get<0>(attackerArray[3])++;
				std::get<1>(attackerArray[3]) = i;
			}
			break;
		case WK:
			if (checkAttackKing(i, square)) {
				std::get<0>(attackerArray[4])++;
			}
			break;
		}
	}

	for (int i = 0; i < 5; ++i) {
		if (std::get<0>(attackerArray[i]) > 0) {
			std::get<0>(attackerArray[i]) = i + 1;
			return attackerArray[i];
		}
	}
	assert(std::get<0>(attackerArray[0]) == 0);
	assert(std::get<1>(attackerArray[0]) == 0);
	return attackerArray[0];
}

void Board::setKingSquare() {
	for (int k = 21; k < 99; ++k) {
		if (mailbox[k] == WHITE * 6) {
			kingSquareWhite = k;
		}
		if (mailbox[k] == BLACK * 6) {
			kingSquareBlack = k;
		}
	}
}
bool Board::inCheck(int color) {
	setKingSquare();	
	int kingsqr = (color == WHITE) ? kingSquareWhite : kingSquareBlack;
	for (int n = 21; n < 99; n++) {
		if (n % 10 == 0 || n % 10 == 9 || mailbox[n] * color > 0) {
			continue;
		}
		if (checkAttack(n, kingsqr)) {
			return true;
		}
	}
	return false;
}
bool Board::checkMoveCheck(int a, int b) {
	move(a, b);
	if (inCheck(turn * -1)) {
		undo();
		return true;
	}
	undo();
	return false;
}

//Check castling rights
std::tuple<bool, bool, bool, bool> Board::checkCastling() {
	std::tuple<bool, bool, bool, bool> castling(true, true, true, true);
	//Blocked by pieces
	if (mailbox[96] != 0 || mailbox[97] != 0) {
		std::get<0>(castling) = false;
	}
	if (mailbox[92] != 0 || mailbox[93] != 0 || mailbox[94] != 0) {
		std::get<1>(castling) = false;
	}
	if (mailbox[26] != 0 || mailbox[27] != 0) {
		std::get<2>(castling) = false;
	}
	if (mailbox[22] != 0 || mailbox[23] != 0 || mailbox[24] != 0) {
		std::get<3>(castling) = false;
	}
	for (int c = 0; c < moveVec.size(); ++c) {
		int oldpos = moveVec[c].from;
		//White king has moved
		if (oldpos == 95) {
			std::get<0>(castling) = false;
			std::get<1>(castling) = false;
		}
		//Black king has moved
		if (oldpos == 25) {
			std::get<2>(castling) = false;
			std::get<3>(castling) = false;
		}
		//White h-file rook has moved
		if (oldpos == 98) {
			std::get<0>(castling) = false;
		}
		//White a-file rook has moved
		if (oldpos == 91) {
			std::get<1>(castling) = false;
		}
		//Black h-file rook has moved
		if (oldpos == 28) {
			std::get<2>(castling) = false;
		}
		//Black a-file rook has moved
		if (oldpos == 21) {
			std::get<3>(castling) = false;
		}
	}
	//King is under check
	if ((std::get<0>(castling) || std::get<1>(castling)) && inCheck(WHITE)) {
		std::get<0>(castling) = false;
		std::get<1>(castling) = false;
	}
	if ((std::get<2>(castling) || std::get<3>(castling)) && inCheck(BLACK)) {
		std::get<2>(castling) = false;
		std::get<3>(castling) = false;
	}
	//Squares are attacked
	for (int n = 21; n < 99; n++) {
		if (mailbox[n] != -9 || mailbox[n] == 0) {
			if (mailbox[n] < 0) {
				if (std::get<0>(castling) && (checkAttack(n, 96) || checkAttack(n, 97))) {
					std::get<0>(castling) = false;
				}
				if (std::get<1>(castling) && (checkAttack(n, 93) || checkAttack(n, 94))) {
					std::get<1>(castling) = false;
				}
			}
			else {
				if (std::get<2>(castling) && (checkAttack(n, 26) || checkAttack(n, 27))) {
					std::get<2>(castling) = false;
				}
				if (std::get<3>(castling) && (checkAttack(n, 23) || checkAttack(n, 24))) {
					std::get<3>(castling) = false;
				}
			}
		}
	}
	return castling;
}

//Defines special behavior for en passant, castling and promotion
void Board::specialMoves(int oldpos, int newpos, int last) {
	//White en passant
	if ((newpos - oldpos == -11 || newpos - oldpos == -9) && mailbox[newpos] == WP && !moveVec.empty()) {
		if (moveVec[last - 1].to == newpos + 10 && mailbox[newpos + 10] == BP && moveVec[last - 1].from == newpos - 10) {
			mailbox[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && mailbox[newpos] == BP && !moveVec.empty()) {
		if (moveVec[last - 1].to == newpos - 10 && mailbox[newpos - 10] == WP && moveVec[last - 1].from == newpos + 10) {
			mailbox[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(mailbox[newpos]) == WK && abs(mailbox[newpos + 1]) == WR) {
		mailbox[newpos + 1] = 0;
		//White
		if (mailbox[newpos] > 0) {
			mailbox[oldpos + 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			mailbox[oldpos + 1] = BR;
			castled[1] = true;
		}
	}
	//Long castling
	else if (oldpos - newpos == 2 && abs(mailbox[newpos]) == WK && abs(mailbox[newpos - 2]) == WR) {
		mailbox[newpos - 2] = 0;
		//White
		if (mailbox[newpos] > 0) {
			mailbox[oldpos - 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			mailbox[oldpos - 1] = BR;
			castled[1] = true;
		}
	}
	//White promotion (to queen)
	if (mailbox[newpos] == WP && newpos >= 21 && newpos <= 28) {
		mailbox[newpos] = WQ;
	}
	else if (mailbox[newpos] == BP && newpos >= 91 && newpos <= 98) {
		mailbox[newpos] = BQ;
	}
}
void Board::move(int a, int b) {
	assert(a >= 0 && a < 120);
	assert(b >= 0 && b < 120);
	std::copy(std::begin(mailbox), std::end(mailbox), std::begin(prev_mailbox));
	prev_castled[0] = castled[0];
	prev_castled[1] = castled[1];
	mailbox[b] = mailbox[a];
	mailbox[a] = 0;
	moveVec.emplace_back(Move(a, b));
	specialMoves(a, b, moveVec.size() - 1);

	turn = (moveVec.size() % 2 == 0) ? WHITE : BLACK;
}
void Board::undo() {
	std::copy(std::begin(prev_mailbox), std::end(prev_mailbox), std::begin(mailbox));
	castled[0] = prev_castled[0];
	castled[1] = prev_castled[1];

	if (!moveVec.empty()) {
		moveVec.pop_back();
	}
	turn = (moveVec.size() % 2 == 0) ? WHITE : BLACK;
}
void Board::nullMove() {
	std::copy(std::begin(mailbox), std::end(mailbox), std::begin(prev_mailbox));
	prev_castled[0] = castled[0];
	prev_castled[1] = castled[1];

	moveVec.emplace_back(Move(0, 0));
	turn = (moveVec.size() % 2 == 0) ? WHITE : BLACK;
}
void Board::setEnPassantSquare() {
	if (moveVec.empty()) {
		epSquare = -1;
		return;
	}

	int oldsq = moveVec.back().from;
	int newsq = moveVec.back().to;
	int color = mailbox[newsq];

	if (mailbox[newsq] != WP || mailbox[newsq] != BP) {
		epSquare = -1;
		return;
	}
	if (abs(oldsq - newsq) != 20) {
		epSquare = -1;
		return;
	}
	if (mailbox[newsq + 1] != WP * -color && mailbox[newsq - 1] != WP * -color) {
		epSquare = -1;
		return;
	}
	if (mailbox[newsq + color * 10] != 0) {
		epSquare = -1;
		return;
	}
	epSquare = newsq + color * 10;
}

void Board::outputBoard() const {
	std::cout << std::endl;
	for (int i = 0; i < 120; ++i) {
		switch (mailbox[i]) {
		case 0:
			std::cout << "* ";
			break;
		case WP:
			std::cout << "P ";
			break;
		case WN:
			std::cout << "N ";
			break;
		case WB:
			std::cout << "B ";
			break;
		case WR:
			std::cout << "R ";
			break;
		case WQ:
			std::cout << "Q ";
			break;
		case WK:
			std::cout << "K ";
			break;
		case BP:
			std::cout << "p ";
			break;
		case BN:
			std::cout << "n ";
			break;
		case BB:
			std::cout << "b ";
			break;
		case BR:
			std::cout << "r ";
			break;
		case BQ:
			std::cout << "q ";
			break;
		case BK:
			std::cout << "k ";
			break;
		}
		if ((i + 1) % 10 == 0) {
			std::cout << std::endl;
		}
	}
}
void Board::resetBoard() {
	for (int i = 21; i < 99; ++i) {
		mailbox[i] = start[i];
	}
	castled[0] = false;
	castled[1] = false;
	moveVec.clear();
}

//Set board position from move vector
void Board::setPosition() {
	for (int i = 21; i < 99; ++i) {
		mailbox[i] = start[i];
	}
	//Reset castled array
	castled[0] = false;
	castled[1] = false;
	int oldc, newc;
	int size = moveVec.size();

	for (int i = 0; i < size; ++i) {
		oldc = moveVec[i].from;
		newc = moveVec[i].to;
		mailbox[newc] = mailbox[oldc];
		mailbox[oldc] = 0;
		if (i > 0) {
			specialMoves(oldc, newc, i);
		}
	}
	turn = (size % 2 == 0) ? WHITE : BLACK;
}

int Board::getTurn() const { return turn; }
int Board::getSquarePiece(int a) const { return mailbox[a]; }