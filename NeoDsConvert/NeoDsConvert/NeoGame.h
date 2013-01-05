#ifndef _NEO_GAME_H
#define _NEO_GAME_H

#include "romload.h"

class NeoGame {
public:
	NeoGame(const char* szName, const game_driver* pDriver);
	const game_driver* getDriver() { return mDriver; }
	const char* getName() { return mName; }
	static bool load(const char* szName);
	static bool loadAll();
	static NeoGame* find(const char* szName);
private:
	const char* mName;
	const game_driver* mDriver;
	NeoGame* mNext;

	static NeoGame* smListHead;
};

#endif
