rksdcardtool
===========

`rksdcardtool` is a minimal utility used to generate the ID block (IDB)
from a Rockchip loader image. The resulting `idb.bin` can be written to
an SD card or other storage for boot purposes.

Build
-----

1. Install a C++11 capable compiler and CMake.
2. From the project root run:

```bash
cmake .
make
```

Usage
-----

```
./rksdcardtool <loader.bin> <output_idb.bin>
```

The first argument is the Rockchip loader binary.  The second argument is
the filename for the generated IDB. Use `-` instead of a filename to write
the IDB to standard output.

