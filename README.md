send-echo-request
=================

`send-echo-request` sends one ICMP (IPv4) or ICMPv6 echo request each
to a list of addresses without waiting for the echo reply packets. A
packet is sent every 0.5 seconds. Optionally, sending the packets is
repeated in an infinite loop.

As `send-echo-request` does not wait for or evaluate any echo reply
packets, processing the received reply packets must be done in another
place, e.g. in `iptables` like my
[someone else is using the internet LED (YUP LED)](http://n-dimensional.de/blog/2014/07/04/led-configuration-with-openwrt/)
setup does.

`send-echo-request` has been designed and developed on and for use on
GNU/Linux (Fedora) and uClibc/Linux (OpenWRT). On other systems, YMMV.

Running `send-echo-request` should be easy enough:

    # send-echo-request --help
    # send-echo-request -vv --loop ::1 127.0.0.1
    # send-echo-request -q  --loop 192.168.1.23 192.168.1.42

`send-echo-request` is licensed under GPLv2+, i.e. GNU GPL version 2 or
(at your option) any later version. Read the LICENSE file for details.


Building and installing, generic Linux edition
----------------------------------------------

This requires GNU `make` and `gcc` and possible a few more things I
forgot to list here.

Building `send-echo-request` should be easy:

    $ make

This builds both `host/send-echo-request.exe` and
`host/send-echo-request.stripped` versions of the executable to be run
on the build host. You can choose to `ln -s` one of them to
`send-echo-request` for convenience:

    $ ln -s host/send-echo-request.exe send-echo-request

If you want to run `send-echo-request` as a non-root user, the root
user must give the executable the required set of capabilities:

    # setcap "cap_net_raw=ep" host/send-echo-request.exe

The complete `cap_net_raw=ep cap_net_admin=ep` capability set often
used for `iputils`' `ping` and `ping6` executables appears not to be
required for the more limited feature set of `send-echo-request`.

For installation using GNU `make`, you can optionally define `DESTDIR`
and `bindir`, and then run

    make install
	make uninstall


Building and installing, embedded/OpenWRT hack edition
------------------------------------------------------

After
[setting up an OpenWRT buildroot](http://wiki.openwrt.org/doc/howto/build)
and building at least the toolchain part of it, you can build a
cross-compile version of `send-echo-request` like so:

    $ make STAGING_DIR="\$(HOME)/src/openwrt-build/12.09/staging_dir" crossprefix="\$(STAGING_DIR)/toolchain-i386_gcc-4.6-linaro_uClibc-0.9.33.2/bin/i486-openwrt-linux-uclibc-"

This builds both `cross/send-echo-request.exe` and
`cross/send-echo-request.stripped` versions of the executable to be
run on the embedded OpenWRT system.

Stripping the symbols reduces the file size from ~20K to ~8K, so the
`cross/send-echo-request.stripped` executable is probably the better
candidate for installing to the OpenWRT system:

    -rwxrwxr-x. 1 USER USER 21838 Aug 23 21:57 send-echo-request.exe
    -rwxrwxr-x. 1 USER USER  8432 Aug 23 21:57 send-echo-request.stripped

After copying the `cross/send-echo-request.stripped` executable over
to the OpenWRT host, it should be available for use:

	$ scp cross/send-echo-request.stripped openwrt_host:/bin/send-echo-request

(Yes, this a hack: No IPKG software package. No proper build process
for the OpenWRT buildchain and `send-echo-request`. However, it works
for me. YMMV.)


References
----------

  * [YUP LED With send-echo-request](http://n-dimensional.de/blog/2014/08/20/yup-led-with-send-echo-request/)
  * [Someone Else Is Using the Internet LED (YUP LED)](http://n-dimensional.de/blog/2014/07/04/led-configuration-with-openwrt/)
  * [RFC 792 - Internet Control Message Protocol](http://tools.ietf.org/html/rfc792)
  * [RFC 1071 - Computing the Internet Checksum](http://tools.ietf.org/html/rfc1071)
  * [RFC 4443 - Internet Control Message Protocol (ICMPv6) for the Internet Protocol Version 6 (IPv6) Specification](http://tools.ietf.org/html/rfc4443)
  * Man pages (in addition to those of the functions we run): ip(7), ipv6(7), ...
  * Some local header files for the detailed struct and constant definitions
