#include "default.h"
#include "NeoRom.h"

int neogeo_fixed_layer_bank_type = 0;

UINT8* memory_region(int region)
{
	return NeoRom::getCurrent()->getRegion(region);
}

int memory_region_length(int region)
{
	return NeoRom::getCurrent()->getRegionLength(region);
}

UINT8* malloc_or_die(int size)
{
	void* p = malloc(size);
	return (UINT8*)p;
}

void* auto_malloc(int size)
{
	void* p = malloc(size);
	return p;
}
