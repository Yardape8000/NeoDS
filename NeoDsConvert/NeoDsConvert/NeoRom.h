#ifndef _NEOROM_H
#define _NEOROM_H

#include "romload.h"
#include "unzip.h"

class NeoRom;
class NeoGame;
class NeoRomWriter;
class NeoRomFile;

#define DRIVER_INIT_CALL(name) ((void)0)
#define DRIVER_INIT(name)		void driver_init_##name(void* machine)

enum NeoRomProtection {
	NEOPROT_NONE,
	NEOPROT_PVC,
	NEOPROT_KOF2000,
	NEOPROT_MSLUG3,
	NEOPROT_GAROUO,
	NEOPROT_GAROU,
	NEOPROT_KOF99,
	NEOPROT_KOF98,
	NEOPROT_FATFURY2,
};

struct NeoRomEntry {
	void* pData;
	int size;
};

class NeoRom {
public:
	UINT8* getRegion(int index) { return (UINT8*)mRomEntry[index].pData; }
	int getRegionLength(int index) { return mRomEntry[index].size; }
	static NeoRom* getCurrent() { return smCurrentRom; }
	static void setBios(int bios) { smSystemBios = bios + 1; }
	
	NeoRom();
	~NeoRom();
	bool load(NeoGame* rom, NeoRomFile& file);
	void setProtection(NeoRomProtection prot) { mProtection = prot; }
	NeoRomProtection getProtection() { return mProtection; }
private:
	bool processRomEntry(const rom_entry* romp, int regiontype, NeoRomFile& file);
	bool fillRomData(const rom_entry *romp, int regiontype);
	bool copyRomData(const rom_entry *romp, int regiontype);
	void regionPostProcess(int regnum, const rom_entry *regiondata);
	int readRomData(const rom_entry *romp, int regiontype, NeoRomFile& file);
	void optimizeSpriteData();
	void optimizeTileData(int region);
	void byteSwap(int region);
	UINT8* getSpriteUsage();
	UINT8* getTileUsage(int region);
	UINT8* expandTiles(int region);
	
	void writeRegion(NeoRomWriter& writer, int region);
	void writeDualRegion(NeoRomWriter& writer, int region0, int region1);
	void write(NeoGame* pGame);

	NeoRomEntry mRomEntry[REGION_MAX];
	NeoRomProtection mProtection;
	static int smSystemBios;
	static NeoRom* smCurrentRom;
};

#endif
