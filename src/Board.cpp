#include "Board.h"

//Returns coordinates without SQUARE_SIZE factor
sf::Vector2f Board::toCoord(char a, char b) {
	int x = int(a) - 97;
	int y = 7 - int(b) + 49;
	return sf::Vector2f(x, y);
}
//Converts coord vector to mailbox coord
int Board::convertCoord(sf::Vector2f vec) {
	int pos = mailbox64[int(vec.y)][int(vec.x)];
	return pos;
}
//Converts mailbox coord to coord vector (without SQUARE_SIZE factor)
sf::Vector2i Board::convertCoord(int pos) {
	int coord_x = pos % 10 - 1;
	int coord_y = (pos - coord_x) / 10 - 2;
	return sf::Vector2i(int(coord_x), int(coord_y));
}
//Converts mailbox coord to 64 coord
int Board::to64Coord(int square) const {
	int unit = square % 10;
	int tenth = (square - unit) / 10;
	int rval = square - 21 - 2 * (tenth - 2);
	return rval;
}
//Converts mailbox coord to notation
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

	/*MOVE PATTERNS*/
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
		return checkAttackKnight(a, b, mailbox);
		break;
	case WB:
		return checkAttackBishop(a, b, mailbox);
		break;
	case WR:
		return checkAttackRook(a, b, mailbox);
		break;
	case WQ:
		return checkAttackQueen(a, b, mailbox);
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
	if (!legalMoveVec.empty()) {
		legalMoveVec.clear();
	}
	if (!captureVec.empty()) {
		captureVec.clear();
	}
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
					if (mailbox[m + n] * piece < 0) {
						captureVec.emplace_back(Move(m, m + n));
					}
				}
			}
		}
		else {
			for (int n = 21; n < 99; n++) {
				if (mailbox[n] == -9) {
					continue;
				}
				if (checkLegal(m, n)) {
					legalMoveVec.emplace_back(Move(m, n));
					if (mailbox[n] * piece < 0) {
						captureVec.emplace_back(Move(m, n));
					}
				}
			}
		}
	}
}

bool Board::checkAttack(int a, int b, const int arr[]) const {
	int oldSquareVal = arr[a];
	int newSquareVal = arr[b];

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
			return checkAttackPawn(a, b, arr, WHITE);
		}
		else {
			return checkAttackPawn(a, b, arr, BLACK);
		}
		break;

	case WN:
		return checkAttackKnight(a, b, arr);
		break;

	case WB:
		return checkAttackBishop(a, b, arr);
		break;

	case WR:
		return checkAttackRook(a, b, arr);
		break;

	case WQ:
		return checkAttackQueen(a, b, arr);
		break;

	case WK:
		return checkAttackKing(a, b, arr);
		break;

	}
}
bool Board::checkAttackPawn(int a, int b, const int arr[], int color) const {
	if ((a - color * 11 == b || a - color * 9 == b) && arr[b] * arr[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackKnight(int a, int b, const int arr[]) const {
	if ((a - 20 - 1 == b || a - 20 + 1 == b || a - 10 - 2 == b || a - 10 + 2 == b || a + 20 - 1 == b || a + 20 + 1 == b || a + 10 - 2 == b || a + 10 + 2 == b) && arr[b] * arr[a] <= 0) {
		return true;
	}
	return false;
}
bool Board::checkAttackBishop(int a, int b, const int arr[]) const {
	if ((a - b) % 9 != 0 && (a - b) % 11 != 0) { return false; }
	int checkpos = 0;
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			switch (type) {
			case 0: checkpos = a - 11 * dis;
				break;
			case 1: checkpos = a - 9 * dis;
				break;
			case 2: checkpos = a + 11 * dis;
				break;
			case 3: checkpos = a + 9 * dis;
				break;
			}
			if (arr[checkpos] * arr[a] != 0) {
				if (checkpos == b) {
					return true;
				}
				goto BISHOPNEXT;
			}
			if (checkpos == b) {
				return true;
			}
		}
	BISHOPNEXT:;
	}
	return false;
}
bool Board::checkAttackRook(int a, int b, const int arr[]) const {
	if ((a - b) % 10 != 0 && (a - b < - 7 || a - b > 7)) { return false; }
	int checkpos = 0;
	for (int type = 0; type < 4; type++) {
		for (int dis = 1; dis < 8; dis++) {
			switch (type) {
			case 0: checkpos = a - 10 * dis;
				break;
			case 1: checkpos = a + 10 * dis;
				break;
			case 2: checkpos = a - dis;
				break;
			case 3: checkpos = a + dis;
				break;
			}
			if (arr[checkpos] * arr[a] != 0) {
				if (checkpos == b) {
					return true;
				}
				goto ROOKNEXT;
			}
			if (checkpos == b) {
				return true;
			}
		}
	ROOKNEXT:;
	}
	return false;
}
bool Board::checkAttackQueen(int a, int b, const int arr[]) const {
	if (checkAttackBishop(a, b, arr)) {
		return true;
	}
	if (checkAttackRook(a, b, arr)) {
		return true;
	}
	return false;
}
bool Board::checkAttackKing(int a, int b, const int arr[]) const {
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
			if (checkAttackPawn(i, square, mailbox, mailbox[i])) {
				std::get<0>(attackerArray[0])++;
				std::get<1>(attackerArray[0]) = i;
			}
			break;
		case WN:
			if (checkAttackKnight(i, square, mailbox)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WB:
			if (checkAttackBishop(i, square, mailbox)) {
				std::get<0>(attackerArray[1])++;
				std::get<1>(attackerArray[1]) = i;
			}
			break;
		case WR:
			if (checkAttackRook(i, square, mailbox)) {
				std::get<0>(attackerArray[2])++;
				std::get<1>(attackerArray[2]) = i;
			}
			break;
		case WQ:
			if (checkAttackQueen(i, square, mailbox)) {
				std::get<0>(attackerArray[3])++;
				std::get<1>(attackerArray[3]) = i;
			}
			break;
		case WK:
			if (checkAttackKing(i, square, mailbox)) {
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

void Board::setKingSquare(int arr[]) {
	for (int k = 98; k > 20; k--) {
		if (arr[k] == WHITE * 6) {
			kingSquareWhite = k;
		}
		if (arr[k] == BLACK * 6) {
			kingSquareBlack = k;
		}
	}
}
bool Board::inCheck(int color, int arr[]) {
	setKingSquare(arr);	
	for (int n = 21; n < 99; n++) {
		if (n % 10 == 0 || n % 10 == 9 || arr[n] * color > 0) {
			continue;
		}
		if (color == WHITE) {
			if (checkAttack(n, kingSquareWhite, arr)) {
				return true;
			}
		}
		else {
			if (checkAttack(n, kingSquareBlack, arr)) {
				return true;
			}
		}
	}
	return false;
}
bool Board::checkMoveCheck(int a, int b) {
	move(a, b);
	if (inCheck(turn * -1, mailbox)) {
		undo();
		return true;
	}
	undo();
	return false;
}

//Check castling rights
std::tuple<bool, bool, bool, bool> Board::checkCastling() {
	std::tuple<bool, bool, bool, bool> castling(true, true, true, true);
	//King is under check
	if (inCheck(WHITE, mailbox)) {
		std::get<0>(castling) = false;
		std::get<1>(castling) = false;
	}
	if (inCheck(BLACK, mailbox)) {
		std::get<2>(castling) = false;
		std::get<3>(castling) = false;
	}
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
	//Squares are attacked
	for (int n = 21; n < 99; n++) {
		if (mailbox[n] != -9 || mailbox[n] == 0) {
			if (mailbox[n] < 0) {
				if (checkAttack(n, 96, mailbox) || checkAttack(n, 97, mailbox)) {
					std::get<0>(castling) = false;
				}
				if (checkAttack(n, 92, mailbox) || checkAttack(n, 93, mailbox) || checkAttack(n, 94, mailbox)) {
					std::get<1>(castling) = false;
				}
			}
			else {
				if (checkAttack(n, 26, mailbox) || checkAttack(n, 27, mailbox)) {
					std::get<2>(castling) = false;
				}
				if (checkAttack(n, 22, mailbox) || checkAttack(n, 23, mailbox) || checkAttack(n, 24, mailbox)) {
					std::get<3>(castling) = false;
				}
			}
		}
	}
	return castling;
}

//Defines special behavior for en passant, castling and promotion
void Board::specialMoves(int oldpos, int newpos, int last, int arr[]) {
	//White en passant
	if ((newpos - oldpos == -11 || newpos - oldpos == -9) && arr[newpos] == WP && !moveVec.empty()) {
		if (moveVec[last - 1].to == newpos + 10 && arr[newpos + 10] == BP && moveVec[last - 1].from == newpos - 10) {
			arr[newpos + 10] = 0;
		}
	}
	//Black en passant
	else if ((newpos - oldpos == 11 || newpos - oldpos == 9) && arr[newpos] == BP && !moveVec.empty()) {
		if (moveVec[last - 1].to == newpos - 10 && arr[newpos - 10] == WP && moveVec[last - 1].from == newpos + 10) {
			arr[newpos - 10] = 0;
		}
	}
	//Short castling
	else if (oldpos - newpos == -2 && abs(arr[newpos]) == WK && abs(arr[newpos + 1]) == WR) {
		arr[newpos + 1] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos + 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos + 1] = BR;
			castled[1] = true;
		}
	}
	//Long castling
	else if (oldpos - newpos == 2 && abs(arr[newpos]) == WK && abs(arr[newpos - 2]) == WR) {
		arr[newpos - 2] = 0;
		//White
		if (arr[newpos] > 0) {
			arr[oldpos - 1] = WR;
			castled[0] = true;
		}
		//Black
		else {
			arr[oldpos - 1] = BR;
			castled[1] = true;
		}
	}
	//White promotion (to queen)
	if (arr[newpos] == WP && newpos >= 21 && newpos <= 28) {
		arr[newpos] = WQ;
	}
	else if (arr[newpos] == BP && newpos >= 91 && newpos <= 98) {
		arr[newpos] = BQ;
	}
}
void Board::move(int a, int b) {
	mailbox[b] = mailbox[a];
	mailbox[a] = 0;
	//Update move vec
	moveVec.emplace_back(Move(a, b));

	//Special moves
	specialMoves(a, b, moveVec.size() - 1, mailbox);

	//Check for turn
	if (moveVec.size() % 2 == 0) {
		turn = WHITE;
	}
	else {
		turn = BLACK;
	}
}
void Board::undo() {
	if (!moveVec.empty()) {
		moveVec.pop_back();
	}
	setPosition();
}
void Board::nullMove() {
	turn *= -1;
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

//Set board position (from position vector)
void Board::setPosition(std::vector<std::string> pV) {
	//Reset board
	resetBoard();
	for (int i = 0; i < pV.size(); ++i) {
		int oldpos = convertCoord(toCoord(pV[i][0], pV[i][1]));
		int newpos = convertCoord(toCoord(pV[i][2], pV[i][3]));
		move(oldpos, newpos);

		if (i > 0) {
			specialMoves(moveVec[i].from, moveVec[i].to, i, mailbox);
		}
	}
	turn = pV.size() % 2 == 0 ? WHITE : BLACK;
}
//Set board position (from internal move vector)
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
			specialMoves(oldc, newc, i, mailbox);
		}
	}
	turn = (size % 2 == 0) ? WHITE : BLACK;
}

int Board::getTurn() const { return turn; }
int Board::getSquarePiece(int a) const { return mailbox[a]; }