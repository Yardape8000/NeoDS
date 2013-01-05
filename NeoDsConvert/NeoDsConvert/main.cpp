#include <stdio.h>
#include "default.h"
#include "unzip.h"
#include "NeoGame.h"
#include "NeoRom.h"

int main(int argc, const char* argv[])
{
	const char* szGame = 0;

	for(int i = 1; i < argc; i++) {
		const char* szArg = argv[i];
		if(strncmp(szArg, "-bios", 5) == 0) {
			int bios = atoi(szArg + 5);
			NeoRom::setBios(bios);
		} else {
			szGame = szArg;
		}
	}

	bool ok = false;

	if(szGame) {
		ok = NeoGame::load(szGame);
	} else {
		ok = NeoGame::loadAll();
	}
	if(ok) {
		return 0;
	}
	return -1;
}