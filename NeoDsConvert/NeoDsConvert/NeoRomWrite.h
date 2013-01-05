#ifndef _NEO_ROM_WRITE_H
#define _NEO_ROM_WRITE_H

#define NEO_ROM_MAGIC 0x7e0116e0
#define NEO_ROM_VERSION 0

#include <vector>
#include <stdio.h>

class NeoRomWriter {
public:
	bool open(const char* szFileName, const char* szGameName, UINT32 audio2Offset);
	void close();
	void writeSection(void* pData, int length);
	void writeDualSection(void* pData0, int length0, void* pData1, int length1);
private:
	struct SectionHeader {
		void* pData[2];
		int length[2];
	};
	
	FILE* mFile;
	int mOffset;
	std::vector<SectionHeader> mSectionList;
};

#endif
