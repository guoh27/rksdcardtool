rksdcardtool
===========

`rksdcardtool` is a minimal utility used to generate the ID block (IDB)
from a Rockchip loader image. The resulting `idbloader.bin` can be written to
an SD card or other storage for boot purposes.

Build
-----

1. Install a C++11 capable compiler and CMake.
2. From the project root run:

```bash
cmake -Bbuild -S.
cmake --build build
```

Usage
-----

```bash
./rksdcardtool <loader.bin> <idbloader.bin>
```

The first argument is the Rockchip loader binary.  The second argument is
the filename for the generated IDB. Use `-` instead of a filename to write
the IDB to standard output.
