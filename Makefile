# This is for the CELL target.  OS/X target uses the xcodeproj

LUA=lua/lapi.ppu.o lua/lauxlib.ppu.o lua/lbaselib.ppu.o lua/lcode.ppu.o lua/ldblib.ppu.o lua/ldebug.ppu.o lua/ldo.ppu.o lua/ldump.ppu.o lua/lfunc.ppu.o lua/lgc.ppu.o lua/linit.ppu.o lua/liolib.ppu.o lua/llex.ppu.o lua/lmathlib.ppu.o lua/lmem.ppu.o lua/loadlib.ppu.o lua/lobject.ppu.o lua/lopcodes.ppu.o lua/loslib.ppu.o lua/lparser.ppu.o lua/lstate.ppu.o lua/lstring.ppu.o lua/lstrlib.ppu.o lua/ltable.ppu.o lua/ltablib.ppu.o lua/ltm.ppu.o lua/lundump.ppu.o lua/lvm.ppu.o lua/lzio.ppu.o lua/print.ppu.o

PROTOCOL=protocol/protocol_pvt.ppu.o protocol/protocol_tree.ppu.o protocol/protocol_lua.ppu.o protocol/protocol_float.ppu.o protocol/protocol_string.ppu.o

NET=net/netClient.ppu.o net/netServer.ppu.o

PPU=FluidServer.ppu.o memory.ppu.o mp_mutex.ppu.o error/error_pvt.ppu.o field/field_pvt.ppu.o fluid/fluid_pvt.ppu.o fluid/fluid_advection_stam.ppu.o lagrange/lagrange_pvt.ppu.o $(LUA) $(NET)  $(PROTOCOL)

SPU=

INCLUDE= -I$(PWD) -I$(PWD)/field -I$(PWD)/fluid -I$(PWD)/lua -I$(PWD)/net -I$(PWD)/protocol -I$(PWD)/error

all:	ppu	spu
	@ppu32-gcc -maltivec $(PPU) $(SPU) -std=c99 -lm -lspe2 -o FluidServer
	@echo "Linking..."

run:	all pull
	@echo "Updating..."
	@echo "Launching..."
	./FluidServer

pull:
	git pull --upload-pack /usr/local/bin/git-upload-pack

push:
	git push --receive-pack /usr/local/bin/git-receive-pack

ppu: $(PPU)

spu: $(SPU)

%.spu.o: %.spu.exe
	@ppu32-embedspu $* $< $@
	@echo -n "SPU-Embed $*: "
	@ls -lhgGd $<

%.spu.exe: %.c
	@echo "SPU-Compile $<"
	@spu-gcc $< -oS -o $@

%.ppu.o: %.c
	@echo "PPU-Compile $<"
	@ppu32-gcc -maltivec -DCELL -DLINUX -Wno-multichar -c $< $(INCLUDE) -o $@

clean:
	rm $(PPU) $(SPU) FluidServer

