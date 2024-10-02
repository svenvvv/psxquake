# PSXQuake

A port of the game Quake to the Playstation 1.

Currently the software renderer is functional, albeit with horrible performance (<1 fps without emulator overlock).

The hardware renderer can draw menus (at 60 fps!), but the world rendering is a bit broken.

No sound, saving, etc.

## Building

### Prerequisites

1. Set up PSn00bSDK following [their installation guide](https://github.com/Lameguy64/PSn00bSDK/blob/master/doc/installation.md);
2. Clone this repo.

### Compiling PSXQuake

1. Copy the `ID0.pak` to `${PROJECT_SOURCE_DIR}/psx_cd/ID1/PAK0.PAK`;
2. Generate the build files:
```sh
PSN00BSDK_LIBS=/path/to/your/PSn00bSDK/lib/libpsn00b cmake --preset=default .
```
3. Build the project (assuming Ninja generator):
```sh
ninja -C build
```

The resulting PSXQuake CD image (`quake.{bin,que}`) will be output in the `build` directory.

Switching out the renderer can be done by changing the `source` of the exe in the file `psx_cd/iso.xml`:
```xml
<!-- For hardware-rendered PSXQuake -->
<file name="QUAKE.EXE"	type="data" source="glquake.exe" />
<!-- For software-rendered PSXQuake -->
<file name="QUAKE.EXE"	type="data" source="swquake.exe" />
```

## Running

Currently PSXQuake can only run on dev consoles (8 MiB of RAM), so you will have to enable it in the emulator settings.

It's pretty heavy on the CD drive, so enabling CD drive read speedups in your emulator is recommended.

PSXQuake has been tested with the Duckstation emulator, never tried on real hardware.


