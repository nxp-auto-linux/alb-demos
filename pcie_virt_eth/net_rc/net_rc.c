/*
 * Copyright 2017-2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * RootComplex code (LS2080A RDB/LS2084A BBMINI/S32V234 EVB)
 */

//---------------------------------------------------------------------------
// Included headers
//---------------------------------------------------------------------------

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <poll.h>
#include <net/if.h>
#include <linux/if_tun.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/time.h>
#include <termios.h>
#include <linux/serial.h>
#include <ctype.h>
#include "netComm.h"

#include "../../pcie_common/include/pcie_handshake.h"

// To check S32V234 EP config space (including BAR addresses):
//
// root@ls2080abluebox:~# cat /sys/bus/pci/devices/0000\:01\:00.0/resource
// 0x0000001446000000 0x00000014460fffff 0x0000000000040200
// 0x0000000000000000 0x0000000000000000 0x0000000000000000
// 0x0000001446100000 0x00000014461fffff 0x0000000000040200
// 0x0000001446211000 0x00000014462110ff 0x0000000000040200
// 0x0000001446210000 0x0000001446210fff 0x0000000000040200
// 0x0000001446200000 0x000000144620ffff 0x0000000000040200
// 0x0000000000000000 0x0000000000000000 0x0000000000000000 

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

//#define ENABLE_DUMP
//#define LOG_VERBOSE

#ifdef LOG_VERBOSE
#define LOG printf
#else
#define LOG(...)
#endif

/// Poll mask for input
const int POLL_INPUT  = (POLLIN | POLLPRI);
/// Poll mask for error
const int POLL_ERROR  = (POLLERR | POLLHUP | POLLNVAL);

/// Number of files in poll array
#define POSIX_IF_CNT 1
/// TAP file index for poll
#define POSIX_IF_TAP 0

#define CMD1_PATTERN	0x42
#define CMD3_PATTERN	0xC8
#define CMD6_PATTERN	CMD1_PATTERN

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

#ifdef ENABLE_DUMP
/// dump format selection
typedef enum {
  dumpHexOnly,
  dumpHexAscii
} tDumpFormat;
#endif

struct test_write_args {
	uint32_t count;
	void *dst;
	void *src;
	ssize_t size;
};

#if 1
/**
 * @brief memcpyW: memcpy which copies 4 byte at a time
 * @param dst  - target buffer start
 * @param src  - source buffer start
 * @param len  - number of words to copy
 * @return number of words copied
 **/
int memcpyW(unsigned int *dst, unsigned int *src, int len)
{
  int wcnt = 0;
  while (len--) {
    *dst++ = *src++;
    wcnt++;
  }
  return wcnt;
}
#endif

/**
 * @brief Print llc-net usage information
 *
 */
static void PrintUsage(void)
{
  fprintf(stderr, "Usage: net_rc [OPTION...]\n"
          "\n"
          "  -h                 Print this help output\n"
          "  -i <ifacename>     Name of interface to use (default tun1)\n"
          "  -a <ddr_addr>      Local DDR address for shared mem buffer, from device tree, hex (mandatory)\n"
          "  -e <ep_bar2_addr>  S32V EP BAR2 address, displayed during enumeration, hex (mandatory)\n\n");
  fprintf(stderr, "E.g. for BBMini (LS2084A):\nnet_rc -a 0x8080100000 -e 0x3840100000\n\n");
}

/**
 * @brief cread: read routine that checks for errors and exits
 *         if an error is returned.
 * @param fd   - tap interface file descriptor
 * @param buf  - start of buffer pointer
 * @param n    - maximum space in buffer
 * @return number of bytes read
 **/
int cread(int fd, uint8_t *buf, int n){

  int nread;

  if((nread=read(fd, buf, n)) < 0){
#ifdef ENABLE_DUMP
    perror("Reading data");
#endif
  }
  return nread;
}

/**
 * @brief cwrite: write routine that checks for errors and exits
 *         if an error is returned.
 * @param fd  - tap interface file descriptor
 * @param buf - start of buffer pointer
 * @param n   - number of bytes to be written
 * @return number of bytes written
 **/
int cwrite(int fd, uint8_t *buf, int n){

  int nwrite;

  if ((nwrite=write(fd, buf, n)) < 0) {
#ifdef ENABLE_DUMP
    perror("Writing data");
#endif
  }
  return nwrite;
}

/**
 * @brief tun_alloc: allocates or reconnects to a tun/tap device.
 *        The caller must reserve enough space in *dev.
 * @param dev   - name of the device to be created
 * @param flags - should include IFF_TAP
 * @returns int file pointer of opened device
 */
int tun_alloc(char *dev, int flags) {

  struct ifreq ifr;
  int fd, err;
  char *clonedev = "/dev/net/tun";

  if( (fd = open(clonedev , O_RDWR)) < 0 ) {
    perror("Opening /dev/net/tun");
    return fd;
  }

  memset(&ifr, 0, sizeof(ifr));

  ifr.ifr_flags = flags;

  if (*dev) {
    strncpy(ifr.ifr_name, dev, IFNAMSIZ);
  }

  if( (err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0 ) {
    perror("ioctl(TUNSETIFF)");
    close(fd);
    return err;
  }

  strcpy(dev, ifr.ifr_name);

  return fd;
}

#ifdef ENABLE_DUMP

/**
 * @brief dump data to output at debug level D_INFO
 * @param data  points to start of buffer which is to be dumped to output
 * @param len   number of bytes to be dumped
 * @param header dump header
 * @param dForm  dump format [dumpHexOnly|dumpHexAscii]
 *
 */
void dump_data(uint8_t *data, size_t len, char *header, tDumpFormat dForm)
{
  size_t index = 0;

  if (len > 0) {
    printf("%s", header);
    while (index < len) {
      size_t  byte;
      char    buffer[16];
      char    line[81] = {0};

      /* Address */
      sprintf(buffer, "data[0x%.4x]: ", (unsigned int) index);
      strcat(line, buffer);

      /* Bytes in HEX */
      byte = 0;
      while (byte < 16) {
  	if ((index + byte) < len) {
  	  	sprintf(buffer, " %.2x", (data[index + byte] & 0xFF));
  	  	strcat(line, buffer);
  	} else
  	  	strcat(line, "   ");
  	byte++;
      }

      if (dForm == dumpHexAscii) {
  	/* Separator */
  	strcat(line, "  ");

  	/* Bytes in ASCII */
  	byte = 0;
  	while ((byte < 16) && ((index + byte) < len)) {
  	  if ((data[index + byte] >= ' ') &&
  	  	  (data[index + byte] <= '~')) {
  	  	  sprintf(buffer, "%c", (data[index + byte] & 0xFF));
  	  	  strcat(line, buffer);
  	  } else
  	  	  strcat(line, ".");
  	  byte++;
  	}
      }
      /* Line ending */
      printf("%s\n", line);
      line[0] = 0;
      /* Next line */
      index += 16;
    }
  }
}
#endif

/**
 * @brief receive payload message, by checking receive counter, FLAGS and a MAGIC_HEADER
 * @param buf      - unsigned int pointer to start of payload buffer
 * @param mapDDR   - unsigned int pointer to start of receive memory
 * @return         - number of bytes payload to be received
 *                   -1 if no date is received
 */
static int receive_msg(unsigned int *buf, unsigned int *mapDDR)
{
    static unsigned int RecCount = 0;
    unsigned int pCount, tmp, len;
  
    len = -1;

    tmp    = mapDDR[MESSBUF_SIZE/4];  // previous acknowledge
    pCount = tmp & 0xFFFF;
    if (tmp & ACK_FLAG) {
      // sync counter
      RecCount = (pCount + 1) & 0xFFFF;
    }
    // check if new packet arrived
    if (mapDDR[(MESSBUF_SIZE - 4)/4] == (RecCount + DONE_FLAG)) {
      // we have data :)
      if (memcmp((unsigned int *)&(mapDDR[1]), MAGIC_HEADER, 4) == 0) {
        int alignLen;
        // the packet type we expect
        len = mapDDR[2];                  // get length of payload
        alignLen = (len + 8) & 0xFFF8;
        memcpy(buf, (unsigned int *)&(mapDDR[4]), alignLen);  // copy payload
      }
      // acknowledge packet
      mapDDR[MESSBUF_SIZE/4] = RecCount + ACK_FLAG;
    } else { 
      LOG("Message %x (%x)\n",  mapDDR[(MESSBUF_SIZE - 4)/4], RecCount);
      LOG("mapDDR %lx, messAddr %lx\n",
                (long unsigned int) mapDDR,
                (long unsigned int) &mapDDR[(MESSBUF_SIZE - 4)/4]);

      usleep(100);
    }
    
    return len;
}

/**
 * @brief send payload message, by adding send counter, FLAGS and a MAGIC_HEADER
 * @param buf     - uint8_t pointer to start of payload buffer
 * @param len     - number of bytes payload to be send
 * @param mapDDR  - unsigned int pointer to start of send memory
 */
static void send_msg(unsigned int *buf, int len, unsigned int *mapDDR)
{
  static unsigned int SendCount = 0;
  unsigned int __attribute__ ((aligned (8)))lbuf[MESSBUF_SIZE/4];
  int alignLen;
  
  if (len > 0) {
    if (len > BUFSIZE) {
      // should never happen !!!
      fprintf(stderr, "send_msg: len > %d\n", BUFSIZE);
      len = BUFSIZE; // just clip
    }
    *lbuf = (unsigned int) START_FLAG;  // write start flag 0x20000
    memcpy(&(lbuf[1]), MAGIC_HEADER, 4);
    memcpy(&(lbuf[2]), &len, 4);
    memcpy(&(lbuf[4]), buf, len); // move payload to safe location in buffer
    alignLen = (len + 8 + 16) & 0xFFF8;
    memcpy(mapDDR, lbuf, alignLen); // MESSBUF_SIZE); 
    *mapDDR = SendCount + DONE_FLAG;    
    
    // wait until date is read by S32V
    while (mapDDR[MESSBUF_SIZE/4] != (SendCount + ACK_FLAG)) {
        LOG("Got %x (%x)\n", mapDDR[MESSBUF_SIZE/4], SendCount);
	usleep(100);
    }
    SendCount = (SendCount + 1) & 0xFFFF;   // prevent flag overwrite 
  }
}

extern unsigned long int ep_bar2_addr;
extern unsigned long int rc_local_ddr_addr;

/**
 * @brief network through PCIe shared memory
 * @param Argc   command line argument count
 * @param ppArgv command line parameter list
 *
 */
int main (int Argc, char **ppArgv)
{
  int           fd1, tapFd;
  int           flags = IFF_TAP;
  int           goon, rlen;
  pid_t         pidf;
  unsigned int  *mapDDR;
  unsigned int  *mapPCIe;
  unsigned int  *src_buff;
  unsigned int  *dest_buff;
  unsigned int  mapsize = MAP_DDR_SIZE;/* Use a default 1M value if no arg */
  int           C;
  struct pollfd FDs[POSIX_IF_CNT] = { {-1, } };
  char		if_name[IFNAMSIZ] = "tun1";
  unsigned int  __attribute__ ((aligned (8))) buffer[BUFSIZE/4];

  if (pcie_parse_command_arguments(Argc, ppArgv))
    exit(1);
		
  // parse net_rc specific command line options using getopt() for POSIX compatibility
  while ((C = getopt(Argc, ppArgv, "+h?i:" COMMON_COMMAND_ARGUMENTS)) != -1)
  {
    switch (C)
    {
      case 'h':
        PrintUsage();
        exit(0);
        break;

      case 'i':
        strncpy(if_name, optarg, IFNAMSIZ-1);
        break;

      case '?':
        if (isprint(optopt))
          fprintf(stderr, "Unknown option '-%c'\n", optopt);
        else
          fprintf(stderr, "Unknown option character `\\x%x'\n", optopt);
        exit(1);

      default:
        break;
    }
  }
  if (*if_name == '\0') {
	  perror("Must specify interface name!");
	  PrintUsage();
  }
  /* initialize tun interface */
  if ( (tapFd = tun_alloc(if_name, flags | IFF_NO_PI)) < 0 ) {
	  fprintf(stderr, "Error connecting to tun interface %s!\n", if_name);
	  exit(1);
  }
  FDs[POSIX_IF_TAP].fd     = tapFd;
  FDs[POSIX_IF_TAP].events = POLL_INPUT;

  printf("Successfully connected to network interface %s\n", if_name );
  
  src_buff = (unsigned int *)malloc(mapsize);
  if (!src_buff) {
  	  printf(" Cannot allocate mem for source buffer\n");
  }

  dest_buff = (unsigned int *)malloc(mapsize);
  if (!dest_buff) {
  	  printf(" Cannot allocate heap for dest buffer\n");
  }

  fd1 = open("/dev/mem", O_RDWR);
  if (fd1 < 0) {
  	  perror("Errors opening /dev/mem file");
  	  goto err;
  } else {
  	  printf(" /dev/mem file opened successfully\n");
  }

  /* MAP PCIe area */
  mapPCIe = (unsigned int *)mmap(NULL, mapsize,
  		  PROT_READ | PROT_WRITE,
  		  MAP_SHARED, fd1, ep_bar2_addr);
  printf(" EP_BAR2_ADDR = %lx, mapPCIe = %lx\n",
  	 ep_bar2_addr, (long unsigned int) mapPCIe);
  if (!mapPCIe) {
  	  perror("/dev/mem PCIe area mapping FAILED");
  	  goto err;
  } else {
  	  printf(" /dev/mem PCIe area mapping  OK\n");
  }	  

  /* MAP DDR free 1M area. This was reserved at boot time */
  mapDDR = (unsigned int *)mmap(NULL, MAP_DDR_SIZE,
  		  PROT_READ | PROT_WRITE,
  		  MAP_SHARED, fd1, rc_local_ddr_addr);
  printf(" RC_DDR_ADDR = %lx, mapDDR = %lx\n",
  	  rc_local_ddr_addr, (long unsigned int) mapDDR);
  if (!mapDDR) {
  	  perror("/dev/mem DDR area mapping FAILED");
  	  goto err;
  } else {
  	  printf(" /dev/mem DDR area mapping OK\n");
  }

  /* Connect to EP and send RC_DDR_ADDR */
  printf("Connecting to EP\n");
  if (pcie_notify_ep((struct s32v_handshake *)mapPCIe) < 0) {
      perror("Unable to send RC_DDR_ADDR to EP");
      goto err;
  }

  memset(src_buff, 0, mapsize);
  memcpy(mapDDR, src_buff, mapsize);
  sleep(1);

  pidf = fork();
  if (pidf == 0) {
    // child process
    goon = 1;
    //Cnt  = 0;
    while (goon) {
      rlen = receive_msg(buffer, &mapDDR[REC_BASE/4]);
      if (rlen > 0) {
      // data received from S32V
      int nwrite;

#ifdef ENABLE_DUMP
      dump_data((uint8_t *)buffer, rlen, "From PCIe EP to TAP interface\n", dumpHexOnly);
#endif

      nwrite = cwrite(tapFd, (uint8_t *)buffer, rlen);
#ifdef ENABLE_DUMP
      if (nwrite != rlen) {
        fprintf(stderr, "Not all data written to TAP (only %d of %d)\nTAP interface not ready\n",
                nwrite, rlen);
        }
      printf("Done\n");
#endif
      }
    }
  } else if (pidf > 0) {
    // parent process
    goon = 1;
    //Cnt  = 0;
    while (goon) {
      // Wait for an event on the MKx file descriptor
      // poll() returns >0 if descriptor is readable, 0 if timeout, -1 if error
      int Data = poll(FDs, POSIX_IF_CNT, 200);
      if (Data < 0) {	  // Error
    	fprintf(stderr, "Poll error %d '%s'\n", errno, strerror(errno));
    	goon = 0;
      } 
      if (Data > 0) {
    	if (FDs[POSIX_IF_TAP].revents & POLL_INPUT) {
    	  int nread;

    	  nread = cread(tapFd, (uint8_t *)buffer, BUFSIZE);
#ifdef ENABLE_DUMP
    	  dump_data((uint8_t *)buffer, nread, "From TAP interface to EP\n", dumpHexOnly);
#endif
    	  // sent to S32v
    	  send_msg(buffer, nread, mapDDR); // data copied in transmitt buffer
    	}
      }
    }
  } else {
     fprintf(stderr, "Fork failed, stopping program\n");
  }
  
err :
	close(fd1);
	close(tapFd);
	printf("\n Gonna exit now\n");
	exit(0);
}
