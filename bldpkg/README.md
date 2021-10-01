This directory conains scripts for building deb and rpm package.
* Type `make -C .. deb` under this directory to build a debian package on Ubuntu platforms
* Type `make -C .. rpm` under this directory to build an rpm package on Redhat platforms
* Limitation is that deb package can be built on Ubuntu only, and for other debian system, the script cannot work yet since some more files are needed by Debian OS.
