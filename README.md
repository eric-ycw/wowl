# Wowl
Wowl is a basic chess engine written in C++.<br />
A chess interface is included along with the engine.<br />
Requires SFML to compile.

## Board representation
* 10x12 mailbox

## Search
* Iterative deepening with aspiration window
* Transposition table
* Quiescence search
* MVV-LVA
* SEE
* Killer moves
* History heuristic
* Null move pruning
* PVS
* Late move reduction

## Evaluation
* Material evaluation with piece-square tables
* King safety
* Space and center control
* Pawn structure
* Passed pawns