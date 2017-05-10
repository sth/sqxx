
## Scons

To compile the library with [scons][scons], simple run scons in the source directory:

    scons

To compile and run the tests

    scons test


## Meson

To compile the library with [meson][meson] and [ninja][ninja]:

    mkdir build
	 cd build
	 meson ..
	 ninja

Meson also supports other build backends.

  [scons]: http://scons.org/
  [meson]: http://mesonbuild.com/
  [ninja]: https://ninja-build.org/

