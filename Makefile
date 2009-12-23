# This is for the CELL target.  OS/X target uses the xcodeproj

LIB_HALF=half.ppu.o

LIB_ERROR=error/error_pvt.ppu.o error/error_lst.ppu.o

LIB_FIELD=field/field_pvt.ppu.o field/field_server.ppu.o field/field_client.ppu.o

LIB_FLUID=fluid/fluid_cpu.ppu.o fluid/fluid_advection_fwd.ppu.o fluid/fluid_advection_fwd2.ppu.o fluid/fluid_advection_stam.ppu.o fluid/fluid_advection_stam2.ppu.o fluid/fluid_advection_repos.ppu.o fluid/fluid_repos.ppu.o fluid/fluid_viscosity.ppu.o fluid/fluid_pressure.ppu.o fluid/fluid_vorticity.ppu.o fluid/fluid_dampen.ppu.o fluid/fluid_visual.ppu.o fluid/fluid_input.ppu.o fluid/fluid_pvt.ppu.o

LIB_SYS=memory.ppu.o

LIB_MP=mp_mutex.ppu.o mp_queue.ppu.o mp_x.ppu.o mp_taskWorld.ppu.o mp_coherence.ppu.o

LIB_NET=net/netInStream.ppu.o net/netOutStream.ppu.o net/netClient.ppu.o net/netServer.ppu.o

APP_SERVER=FluidServer.ppu.o

PPU= $(LIB_HALF) $(LIB_ERROR) $(LIB_FIELD) $(LIB_FLUID) $(LIB_SYS) $(LIB_MP) $(LIB_NET) $(APP_SERVER)

SPU=

INCLUDE= -I$(PWD) -I$(PWD)/field -I$(PWD)/fluid -I$(PWD)/net -I$(PWD)/error

all:	ppu	spu
	@ppu32-gcc -maltivec $(PPU) $(SPU) -std=c99 -lm -lspe2 -o FluidServer
	@echo "Linking..."

run:	all pull
	@echo "Updating..."
	@echo "Launching..."
	./FluidServer

pull:
	git pull

push:
	git push

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

