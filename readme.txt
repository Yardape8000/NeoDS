NeoDS v0.2.0 by Ben Ingram
Website: http://groups.google.com/group/neods
Forums: http://boards.pocketheaven.com/viewforum.php?f=43

If you have questions, READ THIS DOCUMENT FIRST!!!  Then check the website, and the forums at www.gbadev.org and www.pocketheaven.com.  If you still have questions, email me at ingramb AT gmail DOT com.  I will be happy to help, but possibly slightly annoyed if your question is answered in this document.

INTRODUCTION

This is a NeoGeo AES/MVS emulator for the Nintendo DS.  It can run all types of NeoGeo roms with some limitations.

Currently emulated:

* M68000 cpu (cyclone)
* Z80 cpu (DrZ80)
* All forms of NeoGeo protection/encryption
* Graphics
* ADPCM audio
* PSG audio

Not emulated:

* FM audio
* Raster effects
* Multiplayer
* Some timings are not that accurate

FM audio has already been done in jEnesisDS, so it is possible in theory.  It will be hard to squeeze it into NeoDS, as both the ARM7 and ARM9 are pretty busy.  But I will try.  Most of the graphics in NeoDS are drawn with textured quads.  Emulating raster effects using this method would require more polygons per frame than the DS is capable of (as far as I can tell).  It may be possible to emulate raster effects using the DS 2D hardware, but there are lots of reasons why this would be difficult.

USING THE EMULATOR

Required:
* Nintendo DS (lite)
* DLDI compatible flash card
Optional:
* Slot2 flashcard with RAM (Opera expansion, SuperCard lite, etc.)

First, you need to patch NeoDS.nds for your DLDI flash card (NOTE - not all flashcards require patching).  See here: http://chishm.drunkencoders.com/DLDI/index.html.  Please don't email me for help about this, there are plenty of places online that explain how this works.

Next you will need to convert some NeoGeo roms (mslug.zip for example).  You also need a bios rom (neogeo.zip).  NeoDS uses the same rom sets as MAME, so make sure your games work in MAME before proceeding.  Put all the roms you want to convert along with the bios together in a folder.  Copy NeoDSConvert.exe into the same folder.  Run NeoDSConvert, and it will convert all the NeoGeo roms in that folder.  The converted roms will have the *.neo extension.

Copy the DLDI patched NeoDS.nds, and all the *.neo roms into the root of your flashcard.  Run NeoDS.nds.  The main menu should load, showing you a list of all the roms on your card.  Use the arrow keys to select, and press start to choose.  The rom should load, and you should be playing.  You can load a rom without audio which will improve frame rate, but you won't get any sound (obviously).  Once a game is loaded without audio, the only way to get audio back is to reload the game.  Some games will freeze with audio disabled, so be warned!

If you have a supported flashcard with external ram in the GBA slot, NeoDS will use the extra ram to cache more data.  This will improve the speed of some games, but is never required.  The main gui screen will display the text SLOT2 if this feature is active.

Default Controls
[Arrow keys] - Arrow keys
[A, B, X, Y] - A, B, C, D
[L] - Pause
[R] - Select (acts as coin input using newer versions of universal bios)
[Start] - Start
[Select] - Coin

The NeoDS gui is controlled with the stylus.
* Video - Video can be normal or scaled.  Normal is a cropped screen.  Scaled shows the full screen, but scaled down to fit.

* CPU Clock- The NeoGeo cpu can be underclocked.  This can actually make some games run faster!  It is easier for NeoDS to emulate a slower cpu, and some NeoGeo games don't use the full cpu power anyway.  Experiment and see.

* Screen Off - The lower screen can be turned off.  Touch anywhere to turn it back on.

* Load rom - Load a new game

* Save - Write the current configuration and sram to the flashcard

* Input - Remap the input

The input configuraiton screen is a grid.  DS buttons are down the right side of the screen.  NeoGeo buttons are along the top of the screen.  Each row of boxes coorisponds to one DS button.  Putting a check in a box maps a NeoGeo button to a DS button.  You can map multiple NeoGeo buttons to a single DS button this way.  This can be useful for fighting games, or for other things.

Please report any crashes to me.  The more info you can give me, the better.  Thanks!

ADVANCED

You can run NeoDSConvert from the command line.  In this case, it takes up to 2 parameters.
-bios0 use the Europe MVS (Ver. 2)
-bios1 use the Europe MVS (Ver. 1)
-bios2 use the US MVS (Ver. 2?)
-bios3 use the US MVS (Ver. 1)
-bios4 use the Asia MVS (Ver. 3)
-bios5 use the Japan MVS (Ver. 3)
-bios6 use the Japan MVS (Ver. 2)
-bios7 use the Japan MVS (Ver. 1)
-bios8 use the Universe Bios
-bios9 use the Debug MVS
-bios10 use the Asia AES
-bios11 use the Japan AES
If there is another parameter, it will be interpreted as the name of the game, as given to MAME.  In this case, only this rom will be converted.

For example, pretend you want to convert Metal Slug 1, using the japan-s2 bios.  Create a new directory (C:\roms).  Copy mslug.zip and NeoDsConvert.exe into C:\roms.  To open a command line, click start, select "run...", type "cmd", and press enter.  Type "cd c:\roms".  Press enter.  Then type "NeoDSConvert -bios6 mslug".  Press enter.

If you want to use the universal bios, name the file uni-bios.rom, and add it to neogeo.zip.  This enables lots of nice features like switching between home and arcade mode, and cheats.  This is the recommended way to use NeoDS.

TODO
* Save states
* Faster M68k cpu core
* Improved stability
* FM audio (?)
* Raster effects (?)
* Wifi multiplayer (?)
* Clean up NeoDSConvert source if I feel like it

TECHNICAL NOTES

NeoGeo games can be close to 100MB in size, while the ds only has 4MB of ram.  Graphics, sound, and program code all need to be streamed into ram constantly while the emulator is running.  NeoDs uses a modified libfat which has a lookup table to vastly speed up seek times.  In the future, slot2 ds ram expansion packs could provide some speed up.  But they only have 32MB, so games will still have to stream.

The emulator uses a slightly modified version of Cyclone for the m68k core.  The memory handlers are all done in assembly code and integrated into the core.  The jump table is also compressed using a series of sub jump tables.  This uses an extra arm instruction to decode each opcode, but reduces the .nds file size by ~200k, and improves cache utilization.  This seems to give a slight speed increase.  My feeling is that cpu emulation is heavily memory bound.  I think a smaller m68k core that fit mostly into the TCM could be much faster than cyclone, even if it needed more instructions to execute each opcode.

The NeoGeo sprite graphics are all done with textured quads using the ds 3d hardware.  The NeoGeo tile layer is done with a ds tile background.  Doing raster effects with quads is possible in theory, but changing quads per-scanline would need way more quads per frame than the ds is capable of rendering.  2d sprites could be used instead of quads, but this has several problems.  The NeoGeo can render many more sprites than the ds, so it would take a complex hblank handler to swap them on a per-scanline basis.  Also, the ds can only address 1024 sprites at once.  This corresponds to 256k worth of vram, which is half what I can get using quads.

The NeoGeo adpcm audio is streamed from the card and decoded in software because the NeoGeo's audio format doesn't quite match the ds hardware format.  The NeoGeo PSG is mapped directly to the ds PSG hardware.  FM audio would probably have to be done on the arm7.  But the arm7 has very little memory, and can't afford any slowdown (or else the audio will be choppy).  A faster and smaller Z80 core might be needed first.

LICENCE

The gui code is all public domain (all files that begin with gui).  The NeoDSConvert code is all public domain, except where the MAME/zlib licenses apply.  The emulator code is free to use for non-commercial purposes.  Contact me if these terms don't work for you.

I would appreciate credit/thanks in all cases if you use parts of NeoDs, but it's not required.  Also, I would discourage the release of modified versions.  If you have useful changes, submit them to me, and I'll put them in the official version, with full credit going to you.  If you must release your own version, I would encourage you to release the source.  But if you really want to release your own closed source version, I won't stop you (just don't try to sell it).

CREDITS
* FinalDave, notaz for Cyclone
* Reesy for DrZ80
* Wintermute for devkitPro toolchain
* chishm for libfat
* MAME for parts of NeoDSConvert
* Minizip used by NeoDSConvert
* www.pocketheaven.com for hosting a NeoDS forum
* FluBBa for Z80 contribution
* GnGeo, FinalBurnAlpha, MAME, MVSPSP for source code reference
* Charles MacDonald for NeoGeo technical documentation
* Alexander Stante for NeoGeo technical documentation
* Brandon Long for a nice small sprintf implementation
* Everyone who answers questions on the gbadev.org forums
* Let me know if you think you should be here!

HISTORY

Project started Summer 2007

v0.1.0 4/29/2008
initial release

v0.1.1 5/6/2008
* Added level2 sprite cache in slot2 ram (if installed)
* Increased rom page size (fix grenades Metal Slug 1)
* Fix interrupt acknowledge (fix Metal Slug 2)
* Fix tile layer palette update (Last Blade 2 gui)
* Update converter to use latest MAME data (fix issues with King of Fighters 98, etc)
* Thanks to FluBBa for a DAA Z80 instruction that doesnt need a stupid lookup table (saves ram)
* Fixed -bios options in NeoDSConvert

v0.1.1a 5/6/2008
* Forgot to include updated NeoDSConvert is last version

v0.2.0 6/19/2008
* Key configuration
* Pause key
* Put DS to sleep when lid is closed
* Save SRAM and NeoDS configuration to flashcard
* Fix crash when switching roms on certain flash cards
* Fix palettte bank swap bug on tile layer (metal slug 1 hud)
* Improve accuracy of vcounter, and vposition interrupts (fix freeze in Samurai Showdown 3, fix graphics in Blazing Star level 2, probly others)
* Fix issues when using dldi and slot2 ram from same flashcard (like supercard lite)
* Fix graphical corruption that occurs after a while when using slot2 ram