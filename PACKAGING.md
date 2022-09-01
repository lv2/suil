<!-- Copyright 2011-2022 David Robillard <d@drobilla.net> -->
<!-- SPDX-License-Identifier: ISC -->

Packaging Suil
==============

This library is designed to allow parallel installation of different major
versions.  To facilitate this, the shared library name, include directory, and
pkg-config file are suffixed with the major version number of the library.

Dependencies check for the pkg-config package `suil-0` and will build against a
compatible version 0, regardless any other installed versions.

Packages should follow the same conventions as above, that is, include the
major version (and only the major version) in the name of the package so that
it can be installed in parallel with future major versions.

Dependencies
------------

The purpose of Suil is to abstract plugin UI toolkits away from host code.  To
achieve this, Suil dynamically loads modules for the toolkits in use.  The main
Suil library does NOT depend on any toolkit libraries, and its package
shouldn't either (otherwise, for example, every LV2 host in the distribution
would depend directly on Gtk and Qt).  Individual modules (like
`libsuil_gtk2_in_qt5.so`) should be packaged separately and themselves depend
on the involved toolkits.  These packages should also be versioned as described
above to support parallel installation.

Please do not make the main Suil package strongly depend on any toolkit
package, this defeats the purpose of Suil and will irritate users who want to
avoid a particular toolkit dependency for whatever reason.  "Weak" or
"recommended" dependencies are fine, the important thing is that users are able
to avoid particular toolkits if they choose.
