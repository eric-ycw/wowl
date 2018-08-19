#ifndef UCI_INCLUDED
#define UCI_INCLUDED

#include "Wowl.h"

void parsePosition(Board&, Wowl&, std::string);
void parseGo(Board&, Evaluation&, Wowl&, std::string);
void UCILoop();

#endif