#include "Default.h"
#include "guiConsole.h"
#include "NeoVideo.h"
#include "NeoConfig.h"

#define MAKE_CC(a, b, c, d) (((a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define CONFIGFILE_MAGIC 0xc07f16f1

typedef struct _TConfigFileHeader {
	u32 magic;
	u32 chunkCount;
} TConfigFileHeader;

typedef struct _TChunkHeader {
	u32 type;
	u32 size;
} TChunkHeader;

typedef struct _TConfigChunkV1 {
	u32 videoSize;
	u32 cpuClockDivide;
	u8 keyGrid[8];
} TConfigChunkV1;
#define CONFIGCHUNK_V1_TYPE MAKE_CC('C','F','G','1')

#define CONFIGCHUNK_SRAM_TYPE MAKE_CC('S','R','A','M')

#define CONFIGCHUNK_CARD_TYPE MAKE_CC('C','A','R','D')

//config file looks like this:
//TConfigFileHeader header
//TChunkHeader chunk0
//u8 data[chunk0.size]
//TChunkHeader chunk1
//u8 data[chunk1.size]
//...
//TChunkHeader chunkN
//u8 data[chunkN.size]

static char g_szConfigFile[256];

static void neoLoadDefaultConfig()
{
	//set default config
	g_neo->keyGrid[0] = NEOINPUT_A;
	g_neo->keyGrid[1] = NEOINPUT_B;
	g_neo->keyGrid[2] = NEOINPUT_C;
	g_neo->keyGrid[3] = NEOINPUT_D;
	g_neo->keyGrid[4] = NEOINPUT_PAUSE; //pause
	g_neo->keyGrid[5] = NEOINPUT_SELECT;
	g_neo->keyGrid[6] = NEOINPUT_START;
	g_neo->keyGrid[7] = NEOINPUT_COIN;
	neoSystemSetClockDivide(2);
	neoVideoSetSize(NEOVIDEO_NORMAL);
	memset(g_neo->pSram, 0, 64*KB);
	memset(g_neo->pCard, 0, 2*KB);
}

bool neoLoadConfig(const char* szFileName)
{
	u32 i;

	//load default values (will be over-written below if everything loads properly)
	neoLoadDefaultConfig();

	strncpy(g_szConfigFile, szFileName, sizeof(g_szConfigFile));
	char* pExt = strrchr(g_szConfigFile, '.');
	if(pExt) *pExt = 0;
	strcat(g_szConfigFile, ".cfg");
	int fd = systemOpen(g_szConfigFile, false);
	if(fd < 0) {
		systemWriteLine("Failed to load config: %s", g_szConfigFile);
		return false;
	}

	TConfigFileHeader fileHeader;
	systemRead(fd, &fileHeader, sizeof(TConfigFileHeader));
	if(fileHeader.magic != CONFIGFILE_MAGIC) {
		systemWriteLine("Invalid config");
		systemClose(fd);
		return false;
	}

	for(i = 0; i < fileHeader.chunkCount; i++) {
		TChunkHeader chunkHeader;
		TConfigChunkV1 v1Header;

		systemRead(fd, &chunkHeader, sizeof(TChunkHeader));
		switch(chunkHeader.type) {
		case CONFIGCHUNK_V1_TYPE:
			if(chunkHeader.size != sizeof(TConfigChunkV1)) {
				systemWriteLine("Invalid config");
				systemClose(fd);
				return false;
			}
			systemRead(fd, &v1Header, sizeof(TConfigChunkV1));
			memcpy(g_neo->keyGrid, v1Header.keyGrid, sizeof(g_neo->keyGrid));
			neoSystemSetClockDivide(v1Header.cpuClockDivide);
			neoVideoSetSize((TNeoVideoSize)v1Header.videoSize);
			break;
		case CONFIGCHUNK_SRAM_TYPE:
			if(chunkHeader.size != 64*KB) {
				systemWriteLine("Invalid config");
				systemClose(fd);
				return false;
			}
			systemRead(fd, g_neo->pSram, 64*KB);
			break;
		case CONFIGCHUNK_CARD_TYPE:
			if(chunkHeader.size != 2*KB) {
				systemWriteLine("Invalid config");
				systemClose(fd);
				return false;
			}
			systemRead(fd, g_neo->pCard, 2*KB);
			break;
		default:
			//skip past unknown chunk
			systemSeek(fd, chunkHeader.size, true);
			break;
		}
	}

	systemClose(fd);
	
	systemWriteLine("Loaded config file: %s", g_szConfigFile);
	return true;
}

bool neoSaveConfig()
{
	guiConsoleLog("");
	guiConsoleLog("Saving config...");
	guiConsoleLog("*** DON'T POWER OFF ***");
	guiConsoleDump();

	int fd = systemOpen(g_szConfigFile, true);
	systemWriteLine("opened: %d", fd);
	guiConsoleDump();

	if(fd < 0) {
		return false;
	}
	//write the file header
	TConfigFileHeader fileHeader;
	fileHeader.magic = CONFIGFILE_MAGIC;
	fileHeader.chunkCount = 3;
	systemWrite(fd, &fileHeader, sizeof(TConfigFileHeader));

	systemWriteLine("wrote header");
	guiConsoleDump();

	//now write the chunks
	//v1 config
	TChunkHeader v1Type;
	v1Type.type = CONFIGCHUNK_V1_TYPE;
	v1Type.size = sizeof(TConfigChunkV1);
	TConfigChunkV1 v1Header;
	memcpy(v1Header.keyGrid, g_neo->keyGrid, sizeof(v1Header.keyGrid));
	v1Header.cpuClockDivide = g_neo->cpuClockDivide;
	v1Header.videoSize = neoVideoGetSize();
	systemWrite(fd, &v1Type, sizeof(v1Type));
	systemWrite(fd, &v1Header, sizeof(v1Header));

	systemWriteLine("wrote v1");
	guiConsoleDump();

	//sram
	TChunkHeader sramType;
	sramType.type = CONFIGCHUNK_SRAM_TYPE;
	sramType.size = 64*KB;
	systemWrite(fd, &sramType, sizeof(sramType));
	systemWriteLine("wrote sram header");
	guiConsoleDump();
	systemWrite(fd, g_neo->pSram, 64*KB);

	systemWriteLine("wrote sram");
	guiConsoleDump();

	//memory card
	TChunkHeader cardType;
	cardType.type = CONFIGCHUNK_CARD_TYPE;
	cardType.size = 2*KB;
	systemWrite(fd, &cardType, sizeof(cardType));
	systemWrite(fd, g_neo->pCard, 2*KB);

	systemWriteLine("wrote card");
	guiConsoleDump();

	systemClose(fd);

	guiConsoleLog("Save complete!");
	guiConsoleDump();
	return true;
}
