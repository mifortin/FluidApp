# This is for the CELL target.  OS/X target uses the xcodeproj

PPU=FluidServer.ppu.o

SPU=

INCLUDE= -I. -Ifield -Ifluid -Ilua -Inet -Iprotocol -Ierror

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
	@ppu32-gcc -maltivec -c $< -o $(INCLUDE) $@

clean:
	rm *.ppu.o *.spu.o FluidServer

