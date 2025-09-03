# shv-libs4c: a memory-lightweight implementation of SHV

A library that implements the SHV packing and unpacking in Chainpack with an emphasis
on low memory footprint and scalability across various platforms, preferably embedded
systems.
Currently, the library supports these platforms out of the box:
- Linux,
- NuttX.

## Build
### Linux
`CMake` support to be added. Run this command:
```
make CONFIG_SHV_LIBS4C_PLATFORM="linux"
```
to get three archive files (`libshvchainpack.a`, `libshvtree.a`, `libulut.a`) with object files under `./_compiled/lib`.
You can then link them against your application. The headers are exported to `./_compiled/include`.
In your app, you also need to link `zlib` (`-lz`), as this library uses the `crc32` function.

### NuttX
The library should be <s>is</s> integrated in the NuttX mainline. To enable `shv-libs4c` support, check
```
CONFIG_NETUTILS_LIBSHVC=y
```
in `make menuconfig`. You may wish to turn on SHV examples:
- `CONFIG_EXAMPLES_SHV_TEST` - showcases the tree creation and TCP/IP communication with the broker,
- `CONFIG_EXAMPLES_SHV_NXBOOT_UPDATER` - showcases the file node capabilities, the file node is bound
to NXBoot's update partition and allows for firmware updates.

## Porting to other platforms

The only requirement needed is an environment where you can allocate using `malloc`/`calloc`
as when constructing the tree and all the relevant structures. Also, you need
an environment where you can launch threads (or at least, somehow emulate them) because
the supposes a separate communication thread is created.

See `libs4c/libshvtree/shv_clayer_posix.c`, how it's done for Linux and Nuttx.
