#include "default.h"
#include "NeoRom.h"
#include "NeoGame.h"
#include "NeoRomFile.h"
#include "neogeo.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

NeoRomFile::NeoRomFile()
{
	mRomFile = NULL;
	mActive = 0;
	mParent = 0;
}

bool NeoRomFile::open(NeoGame* pGame)
{
	char szFileName[1024] = "";
	strcat(szFileName, pGame->getName());
	strcat(szFileName, ".zip");

	mRomFile = unzOpen(szFileName);
	if(mRomFile == 0) return false;
	
	mParent = 0;
	mActive = 0;
	if(pGame->getDriver()->parent[0] != '0') {
		NeoGame* pParentGame = NeoGame::find(pGame->getDriver()->parent);
		if(pParentGame) {
			mParent = new NeoRomFile();
			if(!mParent->open(pParentGame)) {
				printf("Failed to open parent: %s\n", pGame->getDriver()->parent);
				return false;
			}
		} else {
			printf("Can't find parent: %s\n", pGame->getDriver()->parent);
			return false;
		}
	}
	
	return true;
}

bool NeoRomFile::openSection(const char* szSectionName)
{
	NeoRomFile* pFile = this;

	while(pFile) {
		if(unzLocateFile(pFile->mRomFile, szSectionName, 0) == UNZ_OK) {
			if(unzOpenCurrentFile(pFile->mRomFile) != UNZ_OK) {
				return false;
			}
			mActive = pFile;
			return true;
		}
		pFile = pFile->mParent;
	}
	return false;
	/*if(unzLocateFile(mBiosFile, szSectionName, 0) == UNZ_OK) {
		if(unzOpenCurrentFile(mBiosFile) != UNZ_OK) {
			return false;
		}
		mActiveFile = mBiosFile;
		return true;
	}
	if(unzLocateFile(mRomFile, szSectionName, 0) == UNZ_OK) {
		if(unzOpenCurrentFile(mRomFile) != UNZ_OK) {
			return false;
		}
		mActiveFile = mRomFile;
		return true;
	}
	return false;*/
}

int NeoRomFile::readSection(void* buffer, int length)
{
	return unzReadCurrentFile(mActive->mRomFile, buffer, length);
}

void NeoRomFile::closeSection()
{
	if(mActive) {
		unzCloseCurrentFile(mActive->mRomFile);
		mActive = NULL;
	}
}

NeoRomFile::~NeoRomFile()
{
	closeSection();
	if(mRomFile) {
		unzClose(mRomFile);
	}
	if(mParent) {
		delete mParent;
	}
}
