/***********************************************************************
 * send-echo-request - send ICMP(v6) echo request without waiting for answers
 * Copyright (C) {2014} Hans Ulrich Niedermann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **********************************************************************/

/*
 * Usage: send-echo-request [OPTIONS]... ADDR...
 *
 * Send ICMPv6 (or ICMP) echo requests to all ADDRs given with a
 * constant delay of 0.5s between each packet sent.
 *
 * If no ADDRs are given at all, send-echo-request aborts.
 *
 * Options:
 *   -n --dry-run  do not actually send any packets
 *      --loop     loop after sending packet to last addr in list
 *   -q --quiet    operate without generating output
 *   -v --verbose  generate more output (repeat for more output)
 *
 *      --help     print this usage message and exit
 *      --version  print program version information and exit
 *
 * Address examples:
 *   127.0.0.1     Numerical IPv4 address
 *   ::1           Numerical IPv6 address
 *
 * Verbosity levels:
 *   -qq           no output whatsoever until program crashes
 *   -q            report only packet send errors
 *   <default>     also report current loop iteration number
 *   -v            also report each address packet is sent to
 *   -vv           also report address details
 *
 * Exit status:
 *   0             problem free operation
 *   non-0         if there have been any errors sending packets or otherwise
 *   <none>        in --loop mode, send-echo-request does not exit.
 */

#define PROG "send-echo-request"

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip6.h>
#include <netinet/icmp6.h>

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>

#include "git-version.h"
#include "usage-msg.h"


/***********************************************************************
 * Global settings, can be changed by command line options
 ***********************************************************************/


typedef enum {
  VERB_MUTE = 0,
  VERB_QUIET = 1,
  VERB_NORMAL = 2,
  VERB_VERBOSE = 3,
  VERB_VERY = 4
} verbosity_T;


/* These can be set via cmdline options */
static bool dry_run = false;
static bool do_loop = false;
static verbosity_T verbosity = VERB_NORMAL;


/***********************************************************************
 * Output messages for --help and --version
 ***********************************************************************/


static
void print_usage()
{
  (void) write(STDOUT_FILENO, USAGE_MSG, strlen(USAGE_MSG));
}


#define VERSION_MSG ""				  \
  PROG " " GIT_VERSION_STR "\n"			  \
  "Copyright (C) 2014 Hans Ulrich Niedermann\n"	  \
  "License GPLv2+: GNU GPL version 2 or later\n"  \
  ""


static
void print_version()
{
  (void) write(STDOUT_FILENO, VERSION_MSG, strlen(VERSION_MSG));
}


/***********************************************************************
 * Output more or less verbose messages to user
 ***********************************************************************/


#define normalf(fmt, ...)					\
  messagef_i(VERB_NORMAL,  "* " fmt "\n", ##__VA_ARGS__)


#define verbosef(fmt, ...)					\
  messagef_i(VERB_VERBOSE, "  * " fmt "\n", ##__VA_ARGS__)


#define veryverbosef(fmt, ...)					\
  messagef_i(VERB_VERY,    "    * " fmt "\n", ##__VA_ARGS__)


static
void messagef_i(const verbosity_T msg_verb, const char *format, ...)
  __attribute__(( unused ))
  __attribute__(( format(printf, 2, 3) ));

static
void messagef_i(const verbosity_T msg_verb, const char *format, ...)
{
  va_list ap;
  if (verbosity >= msg_verb) {
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    fflush(stdout);
    va_end(ap);
  }
}


#define quietfe(MSG) quietfe_i(strlen(PROG ": " MSG ": "), \
			       PROG ": " MSG ": ")

static
void quietfe_i(const size_t msglen, const char *msg)
{
  if (verbosity >= VERB_QUIET) {
    const int local_errno = errno; /* read global var errno */
    const char *errmsg = strerror(local_errno);
    (void) write(STDERR_FILENO, msg, msglen);
    (void) write(STDERR_FILENO, errmsg, strlen(errmsg));
    (void) write(STDERR_FILENO, "\n", 1);
  }
}


/***********************************************************************
 * Output error message and exit program
 ***********************************************************************/


#define error_exit(MSG) error_exit_i(strlen(PROG ": " MSG "\n"), \
				     PROG ": " MSG "\n")

static
void error_exit_i(const size_t msglen, const char *msg)
  __attribute__((noreturn));

/* this should even work when realloc or malloc have failed */
static
void error_exit_i(const size_t msglen, const char *msg)
{
  (void) write(STDERR_FILENO, msg, msglen);
  exit(EXIT_FAILURE);
}


#define error_exitf(fmt, ...)					\
  do {								\
    fprintf(stderr, PROG ": " fmt "\n", ##__VA_ARGS__);		\
    exit(EXIT_FAILURE);						\
  } while (0)


/***********************************************************************
 * Prepare echo request packets and send them
 ***********************************************************************/


static
uint16_t icmp_checksum(const uint16_t *data, const size_t byte_sz)
{
  if (0 != (byte_sz & 1)) {
    error_exitf("icmp_checksum: number of bytes %zu must be even",
		byte_sz);
  }

  uint32_t accu = 0;
  for (size_t i=0; i < (byte_sz >> 1); ++i) {
    accu = accu + data[i];
  }

  /*  Fold 32-bit sum to 16 bits */
  while (accu >> 16) {
    accu = (accu & 0xffff) + (accu >> 16);
  }

  const uint16_t checksum = ~accu;
  return checksum;
}


#define IDENTIFIER 0x2342


static
int send_ping4(struct sockaddr_in *dest_addr,
	       const uint16_t sequenceno, const char *dest_str)
{
  verbosef("send_ping4 %s", dest_str);
  veryverbosef("%-13s %d", "sin_family", dest_addr->sin_family);
  veryverbosef("%-13s %u.%u.%u.%u", "sin_addr",
	       (ntohl(dest_addr->sin_addr.s_addr) & 0xff000000) >> 24,
	       (ntohl(dest_addr->sin_addr.s_addr) & 0x00ff0000) >> 16,
	       (ntohl(dest_addr->sin_addr.s_addr) & 0x0000ff00) >>  8,
	       (ntohl(dest_addr->sin_addr.s_addr) & 0x000000ff) >>  0
	       );
  veryverbosef("%-13s %d", "sin_port", ntohs(dest_addr->sin_port));

  if (dry_run) {
    return 0;
  }

  const int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
  if (sock < 0) {
    quietfe("socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)");
    return 1;
  }

  /* compose ICMP packet */
  struct icmphdr hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.type             = ICMP_ECHO;         /* ICMP echo request */
  hdr.code             = 0;
  hdr.checksum         = 0;  /* required for actual checksum calculation */
  hdr.un.echo.id       = htons(IDENTIFIER); /* identifier */
  hdr.un.echo.sequence = htons(sequenceno); /* sequence no */

  /* Fill out the ICMP checksum */
  hdr.checksum         = icmp_checksum((uint16_t *)&hdr, sizeof(hdr));

  /* send the echo request packet */
  const ssize_t sent_bytes =
    sendto(sock,
	   &hdr, sizeof(hdr),
	   0 /* no MSG_* flags */,
	   (struct sockaddr *)dest_addr, sizeof(*dest_addr));
  if (sent_bytes != sizeof(hdr)) {
    error_exit("sendto");
  }

  close(sock);

  return 0;
}


static
int send_ping6(struct sockaddr_in6 *dest_addr,
	       const uint16_t sequenceno, const char *dest_str)
{
  verbosef("send_ping6 %s", dest_str);
  veryverbosef("%-13s %d", "sin6_family", dest_addr->sin6_family);
  veryverbosef("%-13s %d", "sin6_port", ntohs(dest_addr->sin6_port));
  veryverbosef("%-13s 0x%08x", "sin6_flowinfo", dest_addr->sin6_flowinfo);
  veryverbosef("%-13s "
	       "%02x%02x:%02x%02x:%02x%02x:%02x%02x:"
	       "%02x%02x:%02x%02x:%02x%02x:%02x%02x",
	       "sin6_addr",
	       dest_addr->sin6_addr.s6_addr[0x0],
	       dest_addr->sin6_addr.s6_addr[0x1],
	       dest_addr->sin6_addr.s6_addr[0x2],
	       dest_addr->sin6_addr.s6_addr[0x3],
	       dest_addr->sin6_addr.s6_addr[0x4],
	       dest_addr->sin6_addr.s6_addr[0x5],
	       dest_addr->sin6_addr.s6_addr[0x6],
	       dest_addr->sin6_addr.s6_addr[0x7],
	       dest_addr->sin6_addr.s6_addr[0x8],
	       dest_addr->sin6_addr.s6_addr[0x9],
	       dest_addr->sin6_addr.s6_addr[0xa],
	       dest_addr->sin6_addr.s6_addr[0xb],
	       dest_addr->sin6_addr.s6_addr[0xc],
	       dest_addr->sin6_addr.s6_addr[0xd],
	       dest_addr->sin6_addr.s6_addr[0xe],
	       dest_addr->sin6_addr.s6_addr[0xf]
	       );
  veryverbosef("%-13s 0x%08x", "sin6_scope_id", dest_addr->sin6_scope_id);

  if (dry_run) {
    return 0;
  }

  const int sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
  if (sock < 0) {
    quietfe("socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6)");
    return 1;
  }

  /* compose ICMPv6 packet */
  struct icmp6_hdr hdr;
  memset(&hdr, 0, sizeof(hdr));
  hdr.icmp6_type                      = ICMP6_ECHO_REQUEST;
  hdr.icmp6_code                      = 0;
  hdr.icmp6_dataun.icmp6_un_data16[0] = htons(IDENTIFIER); /* identifier */
  hdr.icmp6_dataun.icmp6_un_data16[1] = htons(sequenceno); /* sequence no */

  /* Have the system fill out hdr.icmp6_cksum for us */
  int cksum_ofs = offsetof(struct icmp6_hdr, icmp6_cksum);
  const socklen_t cksum_ofs_sz = sizeof(cksum_ofs);
  const int sso_ret = setsockopt(sock, SOL_RAW, IPV6_CHECKSUM,
				 &cksum_ofs, cksum_ofs_sz);
  if (sso_ret < 0) {
    quietfe("setsockopt(SOL_RAW, IPV6_CHECKSUM)");
    return 1;
  }

  /* send the echo request packet */
  const ssize_t sent_bytes =
    sendto(sock,
	   &hdr, sizeof(hdr),
	   0 /* no MSG_* flags */,
	   (struct sockaddr *)dest_addr, sizeof(*dest_addr));
  if (sent_bytes < 0) {
    quietfe("sendto");
    return 1;
  } else if (((size_t)sent_bytes) < sizeof(hdr)) {
    error_exitf("sendto only sent %zu bytes of %zu",
		sent_bytes, sizeof(hdr));
  }

  close(sock);

  return 0;
}


/***********************************************************************
 * main program
 ***********************************************************************/


typedef struct {
  const char *addr_str;
  struct sockaddr_storage sas;
  struct timespec delay;
} task_T;


int main(int argc, char *argv[])
{
  int     task_cnt = 0;
  task_T *tasks    = NULL;


  /*** parse command line ***/

#define enlarge_array(addrstr)				     \
  tasks = realloc(tasks, sizeof(tasks[0]) * (task_cnt + 1)); \
  if (!tasks) {						     \
    quietfe("realloc");					     \
    exit(EXIT_FAILURE);					     \
  }							     \
  memset(&tasks[task_cnt], 0, sizeof(tasks[task_cnt]));	     \
  tasks[task_cnt].addr_str = addrstr;


  for (int i=1; i<argc; ++i) {
    const char *arg = argv[i];

    union {
      struct sockaddr_in  sin;
      struct sockaddr_in6 sin6;
    } u;
    memset(&u, 0, sizeof(u));

    /* These blocks should either
     *   exit          to exit the program
     *   return        to exit the program
     *   <do nothing>  to clean up the adding of a task
     *   continue      having not added a task
     */
    if (0 == strcmp("--help", arg)) {
      print_usage();
      return 0;
    } else if (0 == strcmp("--version", arg)) {
      print_version();
      return 0;
    } else if ((0 == strcmp("--dry-run", arg)) || (0 == strcmp("-n", arg))) {
      dry_run = true;
      continue;
    } else if (0 == strcmp("--loop", arg)) {
      do_loop = true;
      continue;
    } else if (0 == strcmp("-qq", arg)) {
      switch (verbosity) {
      case VERB_NORMAL:
	verbosity = VERB_MUTE;
	break;
      default:
	error_exit("Illegal use of --quiet/-q");
      }
      continue;
    } else if ((0 == strcmp("--quiet", arg)) || (0 == strcmp("-q", arg))) {
      switch (verbosity) {
      case VERB_QUIET:
	verbosity = VERB_MUTE;
	break;
      case VERB_NORMAL:
	verbosity = VERB_QUIET;
	break;
      default:
	error_exit("Illegal use of --quiet/-q");
      }
      continue;
    } else if (0 == strcmp("-vv", arg)) {
      switch (verbosity) {
      case VERB_NORMAL:
	verbosity = VERB_VERY;
	break;
      default:
	error_exit("Illegal use of --verbose/-v");
      }
      continue;
    } else if ((0 == strcmp("--verbose", arg)) || (0 == strcmp("-v", arg))) {
      switch (verbosity) {
      case VERB_NORMAL:
	verbosity = VERB_VERBOSE;
	break;
      case VERB_VERBOSE:
	verbosity = VERB_VERY;
	break;
      default:
	error_exit("Illegal use of --verbose/-v");
      }
      continue;
    } else if (1 == inet_pton(AF_INET6, arg, &u.sin6.sin6_addr)) {
      enlarge_array(arg);
      u.sin6.sin6_family = AF_INET6;
      memcpy(&tasks[task_cnt].sas, &u.sin6, sizeof(u.sin6));
    } else if (1 == inet_pton(AF_INET, arg, &u.sin.sin_addr)) {
      enlarge_array(arg);
      u.sin.sin_family = AF_INET;
      memcpy(&tasks[task_cnt].sas, &u.sin, sizeof(u.sin));
    } else {
      error_exitf("Cannot parse address: %s", arg);
    }

    tasks[task_cnt].delay.tv_sec  = 0;
    tasks[task_cnt].delay.tv_nsec = 500000000;
    ++task_cnt;
  }

#undef enlarge_array

  if (task_cnt == 0) {
    error_exit("No adress(es) given");
  }


  /*** main loop ***/

  /* endless main loop unless !do_loop */
  for (uint16_t sequenceno=1; ; ++sequenceno) {
    normalf("sequenceno %u", sequenceno);

    int ping_errors = 0;
    for (int i=0; i<task_cnt; ++i) {
      switch (tasks[i].sas.ss_family) {
      case AF_UNSPEC:
	return 0;
      case AF_INET:
	if (send_ping4((struct sockaddr_in *) &tasks[i].sas,
		       sequenceno, tasks[i].addr_str)) {
	  ++ping_errors;
	}
	break;
      case AF_INET6:
	if (send_ping6((struct sockaddr_in6 *) &tasks[i].sas,
		       sequenceno, tasks[i].addr_str)) {
	  ++ping_errors;
	}
	break;
      default:
	error_exitf("Invalid ss_family %u", tasks[i].sas.ss_family);
      }

      struct timespec rem_ts;
      nanosleep(&tasks[i].delay, &rem_ts);
      /* ignore rem_ts value */
    }
    if (ping_errors) {
      normalf("ping_errors %u/%u", ping_errors, task_cnt);
    }

    if (!do_loop) {
      if (ping_errors) {
	return 1;
      }
      break;
    }
  }

  return 0;
}
