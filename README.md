A groovy tta library
=======================

Overview
--------

libtta is a free software library to send/receive data with a
device which respects the TTA protocol. This library can use a
serial port or an Ethernet connection.

The functions included in the library have been derived from the
Modicon TTA Protocol Reference Guide which can be obtained from
Schneider at [www.schneiderautomation.com](http://www.schneiderautomation.com).

The license of libtta is LGPL v2.1 or later and the licence of programs in
tests directory is GPL v3.

The documentation is available under the Creative Commons Attribution-ShareAlike
License 3.0 (Unported) (<http://creativecommons.org/licenses/by-sa/3.0/>).

The official website is [www.libtta.org](http://www.libtta.org).

The library is written in C and designed to run on Linux, Mac OS X, FreeBSD and
QNX and Windows.

Installation
------------

To install, just run the usual dance, `./configure && make install`. Run
`./autogen.sh` first to generate the `configure` script.

If you want to compile with Microsoft Visual Studio, you need to install
<http://code.google.com/p/msinttypes/> to fill the absence of stdint.h.

To compile under Windows, install [MinGW](http://www.mingw.org/) and MSYS then
select the common packages (gcc, automake, libtool, etc).

To compile under OS X with [homebrew](http://mxcl.github.com/homebrew/), you will need
to install the following dependencies first: `brew install autoconf automake libtool`.

Testing
-------

Some tests are provided in *tests* directory, you can freely edit the source
code to fit your needs (it's Free Software :).

See *tests/README* for a description of each program.

Report a Bug
------------

To report a bug, you can:

* fill a bug report on the issue tracker <http://github.com/stephane/libtta/issues>
* or send an email to stephane.raimbault@gmail.com
