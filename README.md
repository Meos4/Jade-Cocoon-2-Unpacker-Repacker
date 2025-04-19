Feature
-------
Can unpack / repack CDDATA.000 files and is compatible with all known game versions.

How to Use
----------
Using the executable:

* Unpacker: Expect CDDATA.000 and CDDATA.LOC to be in the same directory as the executable and extract the files to a "data" directory.

* Repacker: Expect “data” directory containing game files to be in the same directory as the executable and repack CDDATA.000 and CDDATA.LOC to a “Repacked” directory.

With console arguments:

* Unpacker arguments: [0] [CDDATA.000 and CDDATA.LOC path] [Unpacked files path].

* Repacker arguments: [1] [Unpacked files path] [CDDATA.000 and CDDATA.LOC path].

Building
--------
Requirements:
* CMake
* C++20
