cmake_minimum_required(VERSION 3.21)

project(
	quake1
	LANGUAGES    C ASM
	VERSION      1.0.0
	DESCRIPTION  "Quake 1 PSX port"
	HOMEPAGE_URL ""
)

set(COMMON_SRC
	cl_demo.c
	cl_input.c
	cl_main.c
	cl_parse.c
	cl_tent.c
	chase.c
	cmd.c
	common.c
	console.c
	crc.c
	cvar.c
	host.c
	host_cmd.c
	keys.c
	menu.c
	mathlib.c
	# net_dgrm.c
	net_none.c
	net_loop.c
	net_main.c
	net_vcr.c
	# net_udp.c
	# net_bsd.c
	pr_cmds.c
	pr_edict.c
	pr_exec.c
	r_part.c
	sbar.c
	sv_main.c
	sv_phys.c
	sv_move.c
	sv_user.c
	zone.c
	view.c
	wad.c
	world.c
	# cd_linux.c
	cd_null.c
	sys_psx.c
	snd_null.c
	# snd_dma.c
	snd_mem.c
	snd_mix.c
	in_psx.c
	psxlib/psx_io.c
	# psxlib/psx_math.c
	psxlib/lib.c
	psxlib/crc16.c
)

set(SW_SRC
	vid_psx.c
	draw.c
	model.c
	r_efrag.c
	r_light.c
	r_main.c
	r_misc.c
	d_surf.c
	screen.c
	nonintel.c
	d_edge.c
	d_fill.c
	d_init.c
	d_modech.c
	d_part.c
	d_polyse.c
	d_scan.c
	d_sky.c
	d_sprite.c
	d_vars.c
	d_zpoint.c
	r_aclip.c
	r_alias.c
	r_bsp.c
	r_draw.c
	r_edge.c
	r_sky.c
	r_sprite.c
	r_surf.c
	r_vars.c
	# snd_linux.c
)

add_definitions(
	-DPSX
	-Os
)

include_directories(BEFORE SYSTEM
	psxlib/
)

file(GLOB MATH_SRC psxlib/ext/*.c)

set(QUAKE_SRC ${COMMON_SRC} ${SW_SRC} ${MATH_SRC})
psn00bsdk_add_executable(quake GPREL ${QUAKE_SRC})
psn00bsdk_add_cd_image(
	quake_iso quake psx_cd/iso.xml
	DEPENDS quake psx_cd/system.cnf psx_cd/iso.xml
)
# psn00bsdk_target_incbin(quake PRIVATE balls psx_cd/ID0/config.cfg)

install(FILES ${PROJECT_BINARY_DIR}/quake.exe TYPE BIN)
