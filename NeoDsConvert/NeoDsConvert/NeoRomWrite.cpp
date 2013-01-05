#include "default.h"
#include "NeoRom.h"
#include "NeoRomWrite.h"

static const int smHeaderSize = 512;

static UINT32 getSramProtection(const char* szGameName)
{
	if(!strcmp(szGameName,"fatfury3") ||
	!strcmp(szGameName,"samsho3") ||
	!strcmp(szGameName,"samsho3a") ||
	!strcmp(szGameName,"samsho4") ||

	!strcmp(szGameName,"samsho5") ||
	!strcmp(szGameName,"samsho5h") ||
	!strcmp(szGameName,"samsho5b") ||

	!strcmp(szGameName,"samsh5sp") ||
	!strcmp(szGameName,"samsh5sh") ||
	!strcmp(szGameName,"samsh5sn") ||

	!strcmp(szGameName,"aof3") ||
	!strcmp(szGameName,"rbff1") ||
	!strcmp(szGameName,"rbffspec") ||
	!strcmp(szGameName,"kof95") ||
	!strcmp(szGameName,"kof96") ||
	!strcmp(szGameName,"kof96h") ||
	!strcmp(szGameName,"kof97") ||
	!strcmp(szGameName,"kof97a") ||
	!strcmp(szGameName,"kof97pls") ||
	!strcmp(szGameName,"kof98") ||
	!strcmp(szGameName,"kof98k") ||
	!strcmp(szGameName,"kof98n") ||
	!strcmp(szGameName,"kof99") ||
	!strcmp(szGameName,"kof99a") ||
	!strcmp(szGameName,"kof99e") ||
	!strcmp(szGameName,"kof99n") ||
	!strcmp(szGameName,"kof99p") ||
	!strcmp(szGameName,"kof2000") ||
	!strcmp(szGameName,"kof2000n") ||
	!strcmp(szGameName,"kizuna") ||
	!strcmp(szGameName,"lastblad") ||
	!strcmp(szGameName,"lastblda") ||
	!strcmp(szGameName,"lastbld2") ||
	!strcmp(szGameName,"rbff2") ||
	!strcmp(szGameName,"rbff2a") ||
	!strcmp(szGameName,"mslug2") ||
	!strcmp(szGameName,"mslug3") ||
	!strcmp(szGameName,"garou") ||
	!strcmp(szGameName,"garouo") ||
	!strcmp(szGameName,"garoup"))
    	return 0x100;

	if(!strcmp(szGameName, "pulstar"))
		return 0x35a;

	return -1;
}

bool NeoRomWriter::open(const char* szFileName, const char* szGameName, UINT32 audio2Offset)
{
	int sectionNumber = 9;
	int i;

	mFile = fopen(szFileName, "wb");
	if(!mFile) {
		return false;
	}
	UINT32 magic = NEO_ROM_MAGIC;
	UINT32 version = NEO_ROM_VERSION;
	UINT32 prot = NeoRom::getCurrent()->getProtection();
	UINT32 fixedBank = neogeo_fixed_layer_bank_type;
	UINT32 sramProt = getSramProtection(szGameName);

	fwrite(&magic, 4, 1, mFile);
	fwrite(&version, 4, 1, mFile);
	fwrite(&prot, 4, 1, mFile);
	fwrite(&sramProt, 4, 1, mFile);
	fwrite(&fixedBank, 4, 1, mFile);
	fwrite(&audio2Offset, 4, 1, mFile);
	fwrite(&sectionNumber, 4, 1, mFile);

	for(i = 0; i < 16; i++) {
		if(!szGameName[i]) break;
		fwrite(&szGameName[i], 1, 1, mFile);
	}
	for(; i < 16; i++) {
		int d = 0;
		fwrite(&d, 1, 1, mFile);
	}

	mOffset = smHeaderSize;
	return true;
}

void NeoRomWriter::close()
{
	//pad header out to 512 bytes
	int pos = ftell(mFile);
	int padSize = smHeaderSize - pos;
	UINT8 padding[smHeaderSize] = {0};
	fwrite(padding, padSize, 1, mFile);

	for(int i = 0; i < mSectionList.size(); i++) {
		for(int s = 0; s < 2; s++) {
			void* pData = mSectionList[i].pData[s];
			int length = mSectionList[i].length[s];

			if(pData != 0 && length > 0) {
				fwrite(pData, length, 1, mFile);
			}
		}
	}
	fclose(mFile);
	mFile = NULL;
}


void NeoRomWriter::writeDualSection(void* pData0, int length0, void* pData1, int length1)
{
	int totalLength = length0 + length1;

	fwrite(&mOffset, 4, 1, mFile);
	fwrite(&totalLength, 4, 1, mFile);

	SectionHeader section;
	section.pData[0] = pData0;
	section.length[0] = length0;
	section.pData[1] = pData1;
	section.length[1] = length1;
	mSectionList.push_back(section);

	mOffset += totalLength;
}

void NeoRomWriter::writeSection(void* pData, int length)
{
	writeDualSection(pData, length, 0, 0);
}
