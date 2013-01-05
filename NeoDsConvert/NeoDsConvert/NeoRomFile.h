#ifndef _NEO_ROM_FILE_H
#define _NEO_ROM_FILE_H

class NeoRomFile {
public:
	NeoRomFile();
	bool open(NeoGame* pGame);
	bool openSection(const char* szSectionName);
	int readSection(void* buffer, int length);
	void closeSection();
	~NeoRomFile();
private:
	unzFile mRomFile;
	NeoRomFile* mParent;
	NeoRomFile* mActive;
	//unzFile mBiosFile;
	//unzFile mActiveFile;
};

#endif
