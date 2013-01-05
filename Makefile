#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM)
endif

include $(DEVKITARM)/ds_rules

export TARGET		:=	$(shell basename $(CURDIR))
export TOPDIR		:=	$(CURDIR)


#---------------------------------------------------------------------------------
# path to tools - this can be deleted if you set the path in windows
#---------------------------------------------------------------------------------
export PATH		:=	$(DEVKITARM)/bin:$(PATH)

.PHONY: $(TARGET).arm7 $(TARGET).arm9

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all: $(TARGET).ds.gba

$(TARGET).ds.gba	: $(TARGET).nds
	#dsbuild $(TARGET).nds
	#padbin 512 $(TARGET).ds.gba
	#cat $(TARGET).ds.gba neo.img  > $(TARGET)_fs.ds.gba
	#dlditool fcsr.dldi $(TARGET)_fs.ds.gba

#---------------------------------------------------------------------------------
$(TARGET).nds	:	$(TARGET).arm7 $(TARGET).arm9
	ndstool	-c $(TARGET).nds -7 $(TARGET).arm7 -9 $(TARGET).arm9
	cp $(TARGET).nds $(TARGET).x9sd.nds
	cp $(TARGET).nds $(TARGET).sclt.nds
	cp $(TARGET).nds $(TARGET).mmcf.nds
	cp $(TARGET).nds $(TARGET).mpcf.nds
	cp $(TARGET).nds $(TARGET).scp.nds
	dlditool x9sd.dldi $(TARGET).x9sd.nds
	dlditool sclt.dldi $(TARGET).sclt.nds
	dlditool mmcf.dldi $(TARGET).mmcf.nds
	dlditool mpcf.dldi $(TARGET).mpcf.nds
	dlditool scp.dldi $(TARGET).scp.nds
	

#---------------------------------------------------------------------------------
$(TARGET).arm7	: arm7/$(TARGET).elf
$(TARGET).arm9	: arm9/$(TARGET).elf

#---------------------------------------------------------------------------------
arm7/$(TARGET).elf:
	$(MAKE) -C arm7
	
#---------------------------------------------------------------------------------
arm9/$(TARGET).elf:
	$(MAKE) -C arm9

#---------------------------------------------------------------------------------
clean:
	$(MAKE) -C arm9 clean
	$(MAKE) -C arm7 clean
	rm -f $(TARGET).ds.gba $(TARGET).nds $(TARGET).arm7 $(TARGET).arm9
