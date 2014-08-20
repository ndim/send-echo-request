send-echo-request
=================

send-echo-request sends an ICMP (IPv4) or ICMPv6 echo request to a
list of addresses without waiting for echo reply answer packets. There
is a constant delay of about 0.5s between sending packets.

As send-echo-request does not wait for any echo reply packets,
processing of the received reply packets must be done in another
system, e.g. in iptables as described with my [someone else is using the internet LED](http://n-dimensional.de/blog/2014/07/04/led-configuration-with-openwrt/).

send-echo-request has been designed and developed on and for use on
GNU/Linux (Fedora) and uClibc/Linux (OpenWRT). On other systems, YMMV.

Running `send-echo-request` should be easy enough:

    # ./send-echo-request --help
    # ./send-echo-request -vv --loop ::1 127.0.0.1
    # ./send-echo-request -q  --loop 192.168.1.23 192.168.1.42

send-echo-request is licensed under GPLv2+, i.e. GNU GPL version 2 or
(at your option) any later version. Read the LICENSE file for details.


Building and installing, generic edition
----------------------------------------

This requires GNU `make` and `gcc` and possible a few more things.

Building `send-echo-request` should be easy:

    $ make

If you want to run `send-echo-request` as non-root user, you need to
give it the appropriate priviledges:

    # setcap "cap_net_raw=ep" send-echo-request

The `cap_net_admin=ep` capability often used for `iputils`' `ping` and
`ping6` appears not to be required for `send-echo-request`.

For installation using GNU `make`, you can define `DESTDIR` `bindir`,
and then run

    make install
	make uninstall


Building and installing, OpenWRT hack edition
---------------------------------------------

After
[setting up an OpenWRT buildroot](http://wiki.openwrt.org/doc/howto/build)
and building at least the toolchain part of it, you can build
`send-echo-request´ like

    make clean
    make STAGING_DIR="\$(HOME)/src/openwrt-build/12.09/staging_dir" CC="\$(STAGING_DIR)/toolchain-i386_gcc-4.6-linaro_uClibc-0.9.33.2/bin/i486-openwrt-linux-uclibc-gcc"
	strip send-echo-request

The stripping part reduces the file size from ~20K to ~8K. (Yes, the
strip utility should be from the OpenWRT buildroot toolchain.)

After copying `send-echo-request` over to the OpenWRT host, it should
be available for use.

	scp send-echo-request openwrt_host:/bin/

(Yes, this is a hack. No IPKG software package. Using
non-crosscompile-strip tool. However, it works for me. YMMV.)


References
----------

  * [RFC 792 - Internet Control Message Protocol](http://tools.ietf.org/html/rfc792)
  * [RFC 1071 - Computing the Internet Checksum](http://tools.ietf.org/html/rfc1071)
  * [RFC 4443 - Internet Control Message Protocol (ICMPv6) for the Internet Protocol Version 6 (IPv6) Specification](http://tools.ietf.org/html/rfc4443)
  * Man pages (in addition to those of the functions we run): ip(7), ipv6(7)
  * Some local header files for the detailed struct and constant definitions
