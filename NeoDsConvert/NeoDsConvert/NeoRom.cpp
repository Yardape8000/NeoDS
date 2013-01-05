#include "default.h"
#include "NeoRom.h"
#include "NeoGame.h"
#include "NeoRomWrite.h"
#include "NeoRomFile.h"
#include "neogeo.h"
#include <vector>
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#define fatalerror printf
#define debugload printf

int NeoRom::smSystemBios = 1; //japan
NeoRom* NeoRom::smCurrentRom = 0;


/*-------------------------------------------------
    read_rom_data - read ROM data for a single
    entry
-------------------------------------------------*/

int NeoRom::readRomData(const rom_entry *romp, int regiontype, NeoRomFile& file)
{
	static UINT8 tempbuf[65536];

	int datashift = ROM_GETBITSHIFT(romp);
	int datamask = ((1 << ROM_GETBITWIDTH(romp)) - 1) << datashift;
	int numbytes = ROM_GETLENGTH(romp);
	int groupsize = ROM_GETGROUPSIZE(romp);
	int skip = ROM_GETSKIPCOUNT(romp);
	int reversed = ROM_ISREVERSED(romp);
	int numgroups = (numbytes + groupsize - 1) / groupsize;
	UINT8 *base = (UINT8*)mRomEntry[regiontype].pData + ROM_GETOFFSET(romp);
	int i;

	debugload("Loading ROM data: offs=%X len=%X mask=%02X group=%d skip=%d reverse=%d\n", ROM_GETOFFSET(romp), numbytes, datamask, groupsize, skip, reversed);

	/* make sure the length was an even multiple of the group size */
	if (numbytes % groupsize != 0)
		fatalerror("Error in RomModule definition: %s length not an even multiple of group size\n", ROM_GETNAME(romp));

	/* make sure we only fill within the region space */
	if (ROM_GETOFFSET(romp) + numgroups * groupsize + (numgroups - 1) * skip > mRomEntry[regiontype].size)
		fatalerror("Error in RomModule definition: %s out of memory region space\n", ROM_GETNAME(romp));

	/* make sure the length was valid */
	if (numbytes == 0)
		fatalerror("Error in RomModule definition: %s has an invalid length\n", ROM_GETNAME(romp));

	/* special case for simple loads */
	if (datamask == 0xff && (groupsize == 1 || !reversed) && skip == 0) {
		//return rom_fread(romdata, base, numbytes);
		return file.readSection(base, numbytes);
	}

	/* chunky reads for complex loads */
	skip += groupsize;
	while (numbytes)
	{
		int evengroupcount = (sizeof(tempbuf) / groupsize) * groupsize;
		int bytesleft = (numbytes > evengroupcount) ? evengroupcount : numbytes;
		UINT8 *bufptr = tempbuf;

		/* read as much as we can */
		//debugload("  Reading %X bytes into buffer\n", bytesleft);

		//if (rom_fread(romdata, romdata->tempbuf, bytesleft) != bytesleft)
		//	return 0;
		int readResult = file.readSection(tempbuf, bytesleft);
		if(readResult != bytesleft) {
			return 0;
		}

		numbytes -= bytesleft;

		//debugload("  Copying to %p\n", base);

		/* unmasked cases */
		if (datamask == 0xff)
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = *bufptr++;

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = *bufptr++;
					base += skip;
				}
		}

		/* masked cases */
		else
		{
			/* non-grouped data */
			if (groupsize == 1)
				for (i = 0; i < bytesleft; i++, base += skip)
					*base = (*base & ~datamask) | ((*bufptr++ << datashift) & datamask);

			/* grouped data -- non-reversed case */
			else if (!reversed)
				while (bytesleft)
				{
					for (i = 0; i < groupsize && bytesleft; i++, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}

			/* grouped data -- reversed case */
			else
				while (bytesleft)
				{
					for (i = groupsize - 1; i >= 0 && bytesleft; i--, bytesleft--)
						base[i] = (base[i] & ~datamask) | ((*bufptr++ << datashift) & datamask);
					base += skip;
				}
		}
	}

	debugload("  All done\n");
	return ROM_GETLENGTH(romp);
}

/*-------------------------------------------------
    fill_rom_data - fill a region of ROM space
-------------------------------------------------*/

bool NeoRom::fillRomData(const rom_entry *romp, int regiontype)
{
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT8 *base = (UINT8*)mRomEntry[regiontype].pData + ROM_GETOFFSET(romp);

	/* make sure we fill within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > mRomEntry[regiontype].size) {
		fatalerror("Error in RomModule definition: FILL out of memory region space\n");
		return false;
	}

	/* make sure the length was valid */
	if (numbytes == 0) {
		fatalerror("Error in RomModule definition: FILL has an invalid length\n");
		return false;
	}

	/* fill the data (filling value is stored in place of the hashdata) */
	memset(base, (FPTR)ROM_GETHASHDATA(romp) & 0xff, numbytes);

	return true;
}

/*-------------------------------------------------
    copy_rom_data - copy a region of ROM space
-------------------------------------------------*/

bool NeoRom::copyRomData(const rom_entry *romp, int regiontype)
{
	UINT8 *base = (UINT8*)mRomEntry[regiontype].pData + ROM_GETOFFSET(romp);
	int srcregion = ROM_GETFLAGS(romp) >> 24;
	UINT32 numbytes = ROM_GETLENGTH(romp);
	UINT32 srcoffs = (FPTR)ROM_GETHASHDATA(romp);  /* srcoffset in place of hashdata */
	UINT8 *srcbase;

	/* make sure we copy within the region space */
	if (ROM_GETOFFSET(romp) + numbytes > mRomEntry[regiontype].size) {
		fatalerror("Error in RomModule definition: COPY out of target memory region space\n");
		return false;
	}

	/* make sure the length was valid */
	if (numbytes == 0) {
		fatalerror("Error in RomModule definition: COPY has an invalid length\n");
		return false;
	}

	/* make sure the source was valid */
	srcbase = getRegion(srcregion);
	if (!srcbase) {
		fatalerror("Error in RomModule definition: COPY from an invalid region\n");
		return false;
	}

	/* make sure we find within the region space */
	if (srcoffs + numbytes > getRegionLength(srcregion)) {
		fatalerror("Error in RomModule definition: COPY out of source memory region space\n");
		return false;
	}

	/* fill the data */
	memcpy(base, srcbase + srcoffs, numbytes);

	return true;
}

bool NeoRom::processRomEntry(const rom_entry* romp, int regiontype, NeoRomFile& file)
{
	UINT32 lastflags = 0;

	/* loop until we hit the end of this region */
	while (!ROMENTRY_ISREGIONEND(romp))
	{
		/* if this is a continue entry, it's invalid */
		if (ROMENTRY_ISCONTINUE(romp)) {
			fatalerror("Error in RomModule definition: ROM_CONTINUE not preceded by ROM_LOAD\n");
			return false;
		}

		/* if this is an ignore entry, it's invalid */
		if (ROMENTRY_ISIGNORE(romp)) {
			fatalerror("Error in RomModule definition: ROM_IGNORE not preceded by ROM_LOAD\n");
			return false;
		}

		/* if this is a reload entry, it's invalid */
		if (ROMENTRY_ISRELOAD(romp)) {
			fatalerror("Error in RomModule definition: ROM_RELOAD not preceded by ROM_LOAD\n");
			return false;
		}

		/* handle fills */
		if (ROMENTRY_ISFILL(romp)) {
			if(!fillRomData(romp++, regiontype)) {
				return false;
			}
			//fill_rom_data(romdata, romp++);
		}
		/* handle copies */
		else if (ROMENTRY_ISCOPY(romp)) {
			//copy_rom_data(romdata, romp++);
			if(!copyRomData(romp++, regiontype)) {
				return false;
			}
		}
		/* handle files */
		else if (ROMENTRY_ISFILE(romp))
		{
			int bios_flags = ROM_GETBIOSFLAGS(romp);
			if (!bios_flags || (bios_flags == smSystemBios)) /* alternate bios sets */
			{
				const rom_entry *baserom = romp;
				int explength = 0;
				bool fileWasOpened = false;

				/* open the file */
				debugload("Opening ROM file: %s\n", ROM_GETNAME(romp));
				
				//if (!open_rom_file(romdata, romp))
				//	handle_missing_file(romdata, romp);
				/*if(unzLocateFile(file, romp->_name, 0) != UNZ_OK) {
					fatalerror("Missing file");
					continue;
				}
				if(unzOpenCurrentFile(file) != UNZ_OK) {
					fatalerror("Bad file");
					continue;
				}*/
				if(!file.openSection(romp->_name)) {
					fatalerror("Missing file");
					return false;
				}

				fileWasOpened = true;

				/* loop until we run out of reloads */
				do
				{
					/* loop until we run out of continues/ignores */
					do
					{
						rom_entry modified_romp = *romp++;
						int readresult;

						/* handle flag inheritance */
						if (!ROM_INHERITSFLAGS(&modified_romp))
							lastflags = modified_romp._flags;
						else
							modified_romp._flags = (modified_romp._flags & ~ROM_INHERITEDFLAGS) | lastflags;

						explength += ROM_GETLENGTH(&modified_romp);

						/* attempt to read using the modified entry */
						if (!ROMENTRY_ISIGNORE(&modified_romp)) {
							//TODO: read data
							readresult = readRomData(&modified_romp, regiontype, file);
							//readresult = read_rom_data(romdata, &modified_romp);
						}
					}
					while (ROMENTRY_ISCONTINUE(romp) || ROMENTRY_ISIGNORE(romp));

					/* if this was the first use of this file, verify the length and CRC */
					if (baserom)
					{
						debugload("Verifying length (%X) and checksums\n", explength);
						//verify_length_and_hash(romdata, ROM_GETNAME(baserom), explength, ROM_GETHASHDATA(baserom));
						debugload("Verify finished\n");
					}

					/* reseek to the start and clear the baserom so we don't reverify */
					//if (romdata->file)
					//	mame_fseek(romdata->file, 0, SEEK_SET);
					baserom = NULL;
					explength = 0;
				}
				while (ROMENTRY_ISRELOAD(romp));

				/* close the file */
				if (fileWasOpened)
				{
					debugload("Closing ROM file\n");
					file.closeSection();
					//mame_fclose(romdata->file);
					//romdata->file = NULL;
				}
			}
			else
			{
				romp++; /* skip over file */
			}
		}
		else
		{
			romp++;	/* something else; skip */
		}
	}
	return true;
}

const rom_entry *rom_next_region(const rom_entry *romp)
{
	romp++;
	while (!ROMENTRY_ISREGIONEND(romp))
		romp++;
	return ROMENTRY_ISEND(romp) ? NULL : romp;
}

void NeoRom::optimizeTileData(int region)
{
	int length = getRegionLength(region);
	UINT8* data = getRegion(region);
	if(length == 0 || data == 0) return;

	UINT8 tileSrc[32];

	for(int i = 0; i < length; i += 32) {
		//Array.Copy(this.data, i, tileSrc, 0, 32);
		memcpy(tileSrc, data + i, 32);
		for(int y = 0; y < 8; y++) {
			data[i + 4 * y + 0] = tileSrc[0x10 + y];
			data[i + 4 * y + 1] = tileSrc[0x18 + y];
			data[i + 4 * y + 2] = tileSrc[0x00 + y];
			data[i + 4 * y + 3] = tileSrc[0x08 + y];
		}
	}
}

void NeoRom::optimizeSpriteData()
{
	/* convert the sprite graphics data into a format that
       allows faster blitting */
	int i;
	UINT8 *src;
	UINT8 *dest;
	UINT32 bit;
	UINT8 data;

	/* get mask based on the length rounded up to the nearest
       power of 2 */
	/*sprite_gfx_address_mask = 0xffffffff;

	for (bit = 0x80000000; bit != 0; bit >>= 1)
	{
		if (((memory_region_length(NEOGEO_REGION_SPRITES) * 2) - 1) & bit)
			break;

		sprite_gfx_address_mask >>= 1;
	}*/

	//sprite_gfx = auto_malloc(sprite_gfx_address_mask + 1);
	//memset(sprite_gfx, 0, sprite_gfx_address_mask + 1);
	
	int length = getRegionLength(NEOGEO_REGION_SPRITES);
	UINT8* newSprites = (UINT8*)malloc(length);
	dest = newSprites;
	src = getRegion(NEOGEO_REGION_SPRITES);

	for (i = 0; i < length; i += 0x80, src += 0x80)
	{
		int y;

		for (y = 0; y < 0x10; y++)
		{
			int dstData = 0;
			int x;

			for (x = 0; x < 8; x++)
			{
				UINT8 d = (((src[0x43 | (y << 2)] >> x) & 0x01) << 3) |
						    (((src[0x41 | (y << 2)] >> x) & 0x01) << 2) |
							(((src[0x42 | (y << 2)] >> x) & 0x01) << 1) |
							(((src[0x40 | (y << 2)] >> x) & 0x01) << 0);
				dstData |= d << (x * 4);
			}

			*dest++ = (dstData & 0xff);
			*dest++ = ((dstData >> 8) & 0xff);
			*dest++ = ((dstData >> 16) & 0xff);
			*dest++ = ((dstData >> 24) & 0xff);

			dstData = 0;
			for (x = 0; x < 8; x++)
			{
				UINT8 d = (((src[0x03 | (y << 2)] >> x) & 0x01) << 3) |
						    (((src[0x01 | (y << 2)] >> x) & 0x01) << 2) |
							(((src[0x02 | (y << 2)] >> x) & 0x01) << 1) |
							(((src[0x00 | (y << 2)] >> x) & 0x01) << 0);
				dstData |= d << (x * 4);
			}

			*dest++ = (dstData & 0xff);
			*dest++ = ((dstData >> 8) & 0xff);
			*dest++ = ((dstData >> 16) & 0xff);
			*dest++ = ((dstData >> 24) & 0xff);
		}
	}
	
	free(mRomEntry[NEOGEO_REGION_SPRITES].pData);
	mRomEntry[NEOGEO_REGION_SPRITES].pData = newSprites;
}

bool testSprite(UINT8* src)
{
	for(int i = 0; i < 128; i++) {
		if(src[i]) return true;
	}
	return false;
}

bool testTile(UINT8* src)
{
	for(int i = 0; i < 32; i++) {
		if(src[i]) return true;
	}
	return false;
}

UINT8* NeoRom::getSpriteUsage()
{
	int length = getRegionLength(NEOGEO_REGION_SPRITES);
	UINT8* src = getRegion(NEOGEO_REGION_SPRITES);
	int spriteCount = length / 128;
	UINT8* ret = (UINT8*)malloc(spriteCount / 8);
	memset(ret, 0, spriteCount / 8);

	for(int i = 0; i < spriteCount; i++) {
		if(testSprite(&src[i * 128])) {
			ret[i / 8] |= (1 << (i & 7));
		}
	}
	return ret;
}

UINT8* NeoRom::getTileUsage(int region)
{
	int length = getRegionLength(region);
	UINT8* src = getRegion(region);
	int tileCount = length / 32;
	UINT8* ret = (UINT8*)malloc(tileCount / 8);
	memset(ret, 0, tileCount / 8);

	for(int i = 0; i < tileCount; i++) {
		if(testTile(&src[i * 32])) {
			ret[i / 8] |= (1 << (i & 7));
		}
	}
	return ret;
}

UINT8* NeoRom::expandTiles(int region)
{
	int length = getRegionLength(region);
	UINT8* src = getRegion(region);
	int tileCount = length / 32;
	UINT8* ret = (UINT8*)malloc(tileCount * 64);
	memset(ret, 0, tileCount / 8);

	for(int i = 0; i < tileCount; i++) {
		for(int j = 0; j < 32; j++) {
			int byte = src[i * 32 + j];
			ret[i * 64 + j * 2] = (byte & 0x0f);
			ret[i * 64 + j * 2 + 1] = (byte >> 4);
		}
	}
	return ret;
}

void NeoRom::byteSwap(int region)
{
	int length = getRegionLength(region);
	UINT16* src = (UINT16*)getRegion(region);

	for(int i = 0; i < length / 2; i++) {
		UINT16 data = *src;
		*src++ = ((data >> 8) & 0x00ff) | ((data << 8) & 0xff00);
	}
}

void NeoRom::regionPostProcess(int regnum, const rom_entry *regiondata)
{
	int type = ROMREGION_GETTYPE(regiondata);
	int datawidth = ROMREGION_GETWIDTH(regiondata) / 8;
	int littleendian = ROMREGION_ISLITTLEENDIAN(regiondata);
	UINT8 *pRegion = getRegion(regnum);
	int length = getRegionLength(regnum);
	UINT8 *base;
	int i, j;

	debugload("+ datawidth=%d little=%d\n", datawidth, littleendian);

	/* if this is a CPU region, override with the CPU width and endianness */
	if(type == NEOGEO_REGION_MAIN_CPU_CARTRIDGE)
	{
		littleendian = 0; //m68k
		datawidth = 2;
	}
	else if(type == NEOGEO_REGION_AUDIO_CPU_CARTRIDGE)
	{
		littleendian = 1; //z80
		datawidth = 1;
	}

	/* if the region is inverted, do that now */
	if (ROMREGION_ISINVERTED(regiondata))
	{
		debugload("+ Inverting region\n");
		for (i = 0, base = pRegion; i < length; i++)
			*base++ ^= 0xff;
	}

	/* swap the endianness if we need to */
//#ifdef LSB_FIRST
	if (datawidth > 1 && !littleendian)
//#else
//	if (datawidth > 1 && littleendian)
//#endif
	{
		debugload("+ Byte swapping region\n");
		for (i = 0, base = pRegion; i < length; i += datawidth)
		{
			UINT8 temp[8];
			memcpy(temp, base, datawidth);
			for (j = datawidth - 1; j >= 0; j--)
				*base++ = temp[j];
		}
	}
}

NeoRom::~NeoRom()
{
	smCurrentRom = 0;
	for(int i = 0; i < REGION_MAX; i++) {
		if(mRomEntry[i].pData) {
			free(mRomEntry[i].pData);
		}
	}
}

NeoRom::NeoRom()
{
	int i;

	smCurrentRom = this;

	for(i = 0; i < REGION_MAX; i++) {
		mRomEntry[i].pData = 0;
		mRomEntry[i].size = 0;
	}
}

bool NeoRom::load(NeoGame* pGame, NeoRomFile& file)
{
	const rom_entry *regionlist[REGION_MAX];
	const rom_entry* romp = pGame->getDriver()->rom;
	const rom_entry *region;
	int regnum;
	int i;

	neogeo_fixed_layer_bank_type = 0;

	/* reset the region list */
	memset((void *)regionlist, 0, sizeof(regionlist));

	for(region = romp, regnum = 0; region; region = rom_next_region(region), regnum++) {
		int regiontype = ROMREGION_GETTYPE(region);
		int length = ROMREGION_GETLENGTH(region);

		if(mRomEntry[regiontype].pData == 0) {
			mRomEntry[regiontype].pData = malloc(length);
			mRomEntry[regiontype].size = length;

			if (ROMREGION_ISERASE(region)) {
				memset(mRomEntry[regiontype].pData, ROMREGION_GETERASEVAL(region), length);
			/* or if it's sufficiently small (<= 4MB) */
			} else if(length <= 0x400000) {
				memset(mRomEntry[regiontype].pData, 0, length);
			}
		}

		if (ROMREGION_ISROMDATA(region)) {
			if(!processRomEntry(region + 1, regiontype, file)) {
				return false;
			}
		}

		/* add this region to the list */
		if (regiontype < REGION_MAX)
			regionlist[regiontype] = region;
	}

	/* post-process the regions */
	for (regnum = 0; regnum < REGION_MAX; regnum++)
	{
		if (regionlist[regnum])
		{
			debugload("Post-processing region %02X\n", regnum);
			//romdata.regionlength = memory_region_length(regnum);
			//romdata.regionbase = memory_region(regnum);
			regionPostProcess(regnum, regionlist[regnum]);
		}
	}
	
	//byteSwap(NEOGEO_REGION_MAIN_CPU_CARTRIDGE);

	pGame->getDriver()->driver_init(0);

	optimizeSpriteData();
	optimizeTileData(NEOGEO_REGION_FIXED_LAYER_BIOS);
	optimizeTileData(NEOGEO_REGION_FIXED_LAYER_CARTRIDGE);

	//byteSwap(NEOGEO_REGION_MAIN_CPU_CARTRIDGE);
	
	//byteSwap(NEOGEO_REGION_MAIN_CPU_CARTRIDGE);
	//byteSwap(NEOGEO_REGION_MAIN_CPU_BIOS);
	//TDriverInitFunc initFunc = NeoInitConstructor::getFunc(rom->mName);
	//if(initFunc) {
	//	initFunc();
	//}
	

	write(pGame);

	return true;
}

void NeoRom::writeRegion(NeoRomWriter& writer, int region)
{
	writer.writeSection(mRomEntry[region].pData, mRomEntry[region].size);
}

void NeoRom::writeDualRegion(NeoRomWriter& writer, int region0, int region1)
{
	writer.writeDualSection(
		mRomEntry[region0].pData, mRomEntry[region0].size,
		mRomEntry[region1].pData, mRomEntry[region1].size);
}

void NeoRom::write(NeoGame* pGame)
{
	char szFileName[1024];
	char szRomName[16] = {0};

	strcpy(szFileName, pGame->getName());
	strcat(szFileName, ".neo");

	int audio2Offset = 0;
	if(getRegionLength(NEOGEO_REGION_AUDIO_DATA_2) > 0) {
		audio2Offset = getRegionLength(NEOGEO_REGION_AUDIO_DATA_1);
	}

	NeoRomWriter writer;
	writer.open(szFileName, pGame->getName(), audio2Offset);

	int spriteCount = getRegionLength(NEOGEO_REGION_SPRITES) / 128;
	int biosTileCount = getRegionLength(NEOGEO_REGION_FIXED_LAYER_BIOS) / 32;
	int cartTileCount = getRegionLength(NEOGEO_REGION_FIXED_LAYER_CARTRIDGE) / 32;

	UINT8* spriteUsage = getSpriteUsage();
	UINT8* biosTileUsage = getTileUsage(NEOGEO_REGION_FIXED_LAYER_BIOS);
	UINT8* cartTileUsage = getTileUsage(NEOGEO_REGION_FIXED_LAYER_CARTRIDGE);
	UINT8* biosTileExpand = expandTiles(NEOGEO_REGION_FIXED_LAYER_BIOS);
	UINT8* cartTileExpand = expandTiles(NEOGEO_REGION_FIXED_LAYER_CARTRIDGE);

	writeRegion(writer, NEOGEO_REGION_MAIN_CPU_CARTRIDGE);
	writeRegion(writer, NEOGEO_REGION_MAIN_CPU_BIOS);
	writeRegion(writer, NEOGEO_REGION_AUDIO_CPU_CARTRIDGE);
	writeRegion(writer, NEOGEO_REGION_AUDIO_CPU_BIOS);
	//writeDualRegion(writer,
	//	NEOGEO_REGION_AUDIO_CPU_BIOS, NEOGEO_REGION_AUDIO_CPU_CARTRIDGE);
	/*writer.writeDualSection(
		getRegion(NEOGEO_REGION_AUDIO_CPU_BIOS),
		getRegionLength(NEOGEO_REGION_AUDIO_CPU_BIOS),
		getRegion(NEOGEO_REGION_AUDIO_CPU_CARTRIDGE) + 0x10000,
		getRegionLength(NEOGEO_REGION_AUDIO_CPU_CARTRIDGE) - 0x10000);*/
	writeDualRegion(writer, NEOGEO_REGION_AUDIO_DATA_1, NEOGEO_REGION_AUDIO_DATA_2);
	writeRegion(writer, NEOGEO_REGION_SPRITES);
	//writeDualRegion(writer,
	//	NEOGEO_REGION_FIXED_LAYER_BIOS, NEOGEO_REGION_FIXED_LAYER_CARTRIDGE);
	writer.writeDualSection(
		biosTileExpand, biosTileCount * 64,
		cartTileExpand, cartTileCount * 64);

	writer.writeSection(spriteUsage, spriteCount / 8);
	writer.writeDualSection(
		biosTileUsage, biosTileCount / 8,
		cartTileUsage, cartTileCount / 8);

	writer.close();

	free(spriteUsage);
	free(biosTileUsage);
	free(cartTileUsage);
	free(biosTileExpand);
	free(cartTileExpand);



	/*FILE* file = fopen(szFileName, "wb");
	if(!file) return;

	UINT32 magic = NEO_ROM_MAGIC;
	UINT32 version = NEO_ROM_VERSION;
	strncpy(szRomName, rom->mName, 16);
	
	fwrite(&magic, 4, 1, file);
	fwrite(&version, 4, 1, file);
	fwrite(szRomName, 1, 16, file);

	writeRegionHeader(file, NEOGEO_REGION_MAIN_CPU_CARTRIDGE);
	writeRegionHeader(file, NEOGEO_REGION_MAIN_CPU_BIOS);
	writeRegionHeader(file, NEOGEO_REGION_AUDIO_CPU_CARTRIDGE);
	writeRegionHeader(file, NEOGEO_REGION_AUDIO_CPU_BIOS);
	writeRegionHeader(file, NEOGEO_REGION_AUDIO_DATA_1);
	writeRegionHeader(file, NEOGEO_REGION_AUDIO_DATA_2);
	writeRegionHeader(file, NEOGEO_REGION_SPRITES);
	writeRegionHeader(file, NEOGEO_REGION_FIXED_LAYER_CARTRIDGE);
	writeRegionHeader(file, NEOGEO_REGION_FIXED_LAYER_BIOS);*/

}

