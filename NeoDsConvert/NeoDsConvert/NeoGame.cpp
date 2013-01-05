#include "default.h"
#include "NeoRom.h"
#include "NeoRomFile.h"
#include "NeoGame.h"

NeoGame* NeoGame::smListHead = 0;

NeoGame::NeoGame(const char* szName, const game_driver* pDriver)
{
	mName = szName;
	mDriver = pDriver;
	mNext = smListHead;
	smListHead = this;
}

NeoGame* NeoGame::find(const char* szName)
{
	NeoGame* pGame = smListHead;
	while(pGame) {
		if(strcmp(pGame->mName, szName) == 0) {
			return pGame;
		}
		pGame = pGame->mNext;
	}
	return 0;
}

bool NeoGame::loadAll()
{
	bool loaded = false;
	NeoGame* pGame = smListHead;
	while(pGame) {
		NeoRomFile file;
		printf("Looking for %s...\n", pGame->getName());
		if(file.open(pGame)) {
			NeoRom rom;
			if(rom.load(pGame, file)) {
				loaded = true;
			}
		}

		pGame = pGame->mNext;
	}
	return loaded;
}

bool NeoGame::load(const char* szName)
{
	NeoGame* pGame = smListHead;
	while(pGame) {
		if(strcmp(pGame->mName, szName) == 0) {
			//char szFileName[1024] = "";
			//strcat(szFileName, szName);
			//strcat(szFileName, ".zip");
			NeoRomFile file;
			if(!file.open(pGame)) return false;
			
			NeoRom rom;
			return rom.load(pGame, file);
		}
		pGame = pGame->mNext;
	}
	return false;
}
