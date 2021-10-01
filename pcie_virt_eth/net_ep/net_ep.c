/*
 * Copyright 2017, 2021 NXP
 *
 * SPDX-License-Identifier: GPL-2.0+
 * 
 * EndPoint code (S32V234 PCIE/BBMINI)
 */

/*---------------------------------------------------------------------------
 * Included headers
 *---------------------------------------------------------------------------*/

#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <signal.h>
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
#include "pcie_ops.h"
#include "pcie_ep_addr.h"
#include "pcie_handshake.h"

/*------------------------------------------------------------------------------
 * Macros & Constants
 *------------------------------------------------------------------------------*/
// #define ENABLE_DMA
//#define ENABLE_DUMP
//#define LOG_VERBOSE

/* Poll mask for input */
const int POLL_INPUT  = (POLLIN | POLLPRI);
/* Poll mask for error */
const int POLL_ERROR  = (POLLERR | POLLHUP | POLLNVAL);

/* Number of files in poll array */
#define POSIX_IF_CNT 1
/* TAP file index for poll */
#define POSIX_IF_TAP 0

#define CMD1_PATTERN	0x12
#define CMD3_PATTERN	0x67
#define CMD8_PATTERN	CMD1_PATTERN

#ifdef LOG_VERBOSE
#define LOG printf
#else
#define LOG(...)
#endif

#define EP_DBGFS_FILE		"/sys/kernel/debug/ep_dbgfs/ep_file"

/*------------------------------------------------------------------------------
 * Type definitions
 *------------------------------------------------------------------------------*/

#ifdef ENABLE_DUMP
/* dump format selection */
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

/**
 * @brief Print llc-net usage information
 */
static void PrintUsage(void)
{
  fprintf(stderr, "Usage: net_ep [OPTION...]\n"
          "\n"
          "  -h                 Print this help output\n"
          "  -i <ifacename>     Name of interface to use (default tun1)\n"
          "  -a <ddr_addr>      Local DDR address for shared mem buffer, from device tree, hex (mandatory)\n"
          "\n");

  fprintf(stderr, "E.g. for BBMini (S32V234):\nnet_ep -a 0xC1100000\n\n");
}

#ifdef ENABLE_DMA
volatile sig_atomic_t dma_flag = 0;
volatile sig_atomic_t cntSignalHandler = 0;
struct sigaction action;

/**
 * @brief signal_handler for DMA transfer end
 * @param signum - received signal, only handles SIGUSR1
 */
void signal_handler(int signum)
{
  if (signum == SIGUSR1) {
    printf ("\n DMA transfer completed");
    dma_flag = 1;
    cntSignalHandler++;
  }
  return;
}

void dmaCpy(unsigned int *dst, unsigned int *src, int len, int fd1)
{
  /* Struct for DMA ioctl */
  struct dma_data_elem dma_single = {0, 0, 0, 0, 0, 0};
  
  dma_single.flags  = 0;
  dma_flag          = 0;
  dma_single.size   = len;
  dma_single.sar    = (unsigned long int) src;
  dma_single.dar    = (unsigned long int) dst;
  dma_single.ch_num = 0;
  dma_single.flags  = (DMA_FLAG_WRITE_ELEM | DMA_FLAG_EN_DONE_INT | DMA_FLAG_LIE);

  ioctl(fd1, SEND_SINGLE_DMA, &dma_single);
  while (!dma_flag) { ; }
}
#endif

/**
 * @brief cread: read routine that checks for errors and exits
 *         if an error is returned.
 * @param fd   - tap interface file descriptor
 * @param buf  - start of buffer pointer
 * @param n    - maximum space in buffer
 * @return number of bytes read
 */
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
 */
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
 */
void dump_data(uint8_t *data, size_t len, char *header, tDumpFormat dForm)
{
    size_t index = 0;

    if (len > 0) {
        printf("%s", header);
        while (index < len) {
            size_t byte;
            char buffer[16];
            char line[81] = { 0 };

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
                    if ((data[index + byte] >= ' ')
                            && (data[index + byte] <= '~')) {
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
 * @param buf      - uint8_t pointer to start of payload buffer
 * @param src_buff - unsigned int pointer to start of transfer buffer
 * @param mapPCIe  - unsigned int pointer to start of receive memory
 * @param fd1      - int file pointer to PCIe driver
 * @return         - number of bytes payload to be received
 *                    -1 if no date is received
 */
static int receive_msg_ls2(uint8_t *buf, unsigned int *src_buff, unsigned int *mapPCIe, int fd1)
{
  static unsigned int RecCount = 0;
  unsigned int pCount, tmp, len;
  
  len = -1;
  memcpy((unsigned int *)&src_buff[MESSBUF_SIZE/4],
         (unsigned int *)&mapPCIe[MESSBUF_SIZE/4], 4);
  tmp	 = src_buff[MESSBUF_SIZE/4];
  pCount = tmp & 0xFFFF;
  if (tmp & ACK_FLAG) {
    /* sync counter */
    RecCount = (pCount + 1) & 0xFFFF;
  }
  /* check if new packet arrived */
  memcpy((unsigned int *)(src_buff),
         (unsigned int *)(mapPCIe), 4);
  
  if (*src_buff == (RecCount + DONE_FLAG)) {
    /* we have data :) */
#ifndef ENABLE_DMA
    memcpy((unsigned int *)(src_buff), (unsigned int *)(mapPCIe), MESSBUF_SIZE);
#else
    dmaCpy((unsigned int *)(src_buff), (unsigned int *)(mapPCIe), MESSBUF_SIZE, fd1);
#endif     
    if (memcmp(&(src_buff[1]), MAGIC_HEADER, 4) == 0) {
      /* the packet type we expect */
      len = src_buff[2];		  /* get length of payload */
      memcpy(buf, &(src_buff[4]), len);  /* copy payload */
    }
    /* acknowledge packet and transferr to LS2 */
    src_buff[MESSBUF_SIZE/4] = RecCount + ACK_FLAG;
    memcpy((unsigned int *)&mapPCIe[MESSBUF_SIZE/4],
           (unsigned int *)&src_buff[MESSBUF_SIZE/4], 4);
    
  } else {
     LOG("Message %x (%x)\n",  *src_buff, RecCount);
     usleep(100); 
  }
  return len;
}

/**
 * @brief send payload message, by adding send counter, FLAGS and a MAGIC_HEADER
 * @param buf       - uint8_t pointer to start of payload buffer
 * @param len       - number of bytes payload to be send
 * @param dest_buff - unsigned_int pointer to start of transfer buffer
 * @param mapPCIe   - unsigned int pointer to start of send memory
 * @param fd1       - int file pointer to PCIe driver
 */
static void send_msg_ls2(uint8_t *buf, int len, unsigned int *dest_buff, unsigned int *mapPCIe, int fd1)
{
  static unsigned int SendCount = 0;
  int gotit;
  
  if (len > 0) {
    if (len > BUFSIZE) {
      /* should never happen !!! */
      fprintf(stderr, "send_msg: len > %d\n", BUFSIZE);
      len = BUFSIZE; /* just clip */
    }
    /* make sure a previous count ACK is available */
    dest_buff[MESSBUF_SIZE/4] = ((SendCount - 1) & 0xFFFF) + ACK_FLAG; 
    memcpy((unsigned int *)&mapPCIe[MESSBUF_SIZE/4],
    	   (unsigned int *)&dest_buff[MESSBUF_SIZE/4], 4);
    *dest_buff = (unsigned int) START_FLAG;  /* write start flag 0x20000 */
    memcpy(&(dest_buff[1]), MAGIC_HEADER, 4);
    memcpy(&(dest_buff[2]), &len, 4);
    memcpy(&(dest_buff[4]), buf, len); /* move payload to safe location in buffer */
    dest_buff[(MESSBUF_SIZE - 4)/4] = SendCount + DONE_FLAG;
    /* transfer data to RC */
#ifndef ENABLE_DMA
    memcpy((unsigned int *)mapPCIe, (unsigned int *) dest_buff, MESSBUF_SIZE + 8);
#else
    dmaCpy((unsigned int *)mapPCIe, (unsigned int *) dest_buff, MESSBUF_FULL, fd1);
#endif
    LOG("Send to %lx\n", (long unsigned int) mapPCIe);
    /* wait until date is read by RC */
    gotit = 0;
    while (!gotit) {
       memcpy((unsigned int *)&dest_buff[MESSBUF_SIZE/4],
              (unsigned int *)&mapPCIe[MESSBUF_SIZE/4], 4);
       if (dest_buff[MESSBUF_SIZE/4] == (SendCount + ACK_FLAG)) {
    	 gotit = 1;
       } else {
         LOG("Got %x (%x)\n", dest_buff[MESSBUF_SIZE/4], SendCount);
    	 usleep(100);
       }
    }
    SendCount = (SendCount + 1) & 0xFFFF;   /* prevent flag overwrite */
  }
}

/**
 * @brief network through PCIe shared memory
 * @param Argc   command line argument count
 * @param ppArgv command line parameter list
 */
int main (int Argc, char **ppArgv)
{
  int           fd1 = 0, fd2 = 0, tapFd;
  int           flags = IFF_TAP;
  int           ret = 0;
  unsigned int  *mapDDR  = NULL;
  unsigned int  *mapPCIe = NULL;
  unsigned int  *src_buff;
  unsigned int  *dest_buff;
  unsigned long int rc_ddr_addr = UNDEFINED_DATA;
  int           goon, rlen;
  pid_t         pidf;
  int           C;
  struct pollfd FDs[POSIX_IF_CNT] = { {-1, } };
  char		if_name[IFNAMSIZ] = "tun1";
  uint8_t       buffer[BUFSIZE];
  unsigned long int ep_pcie_base_address = 0;
  unsigned long int ep_local_ddr_addr = 0;
  unsigned int bar_number = 0;

  if (pcie_parse_ep_command_arguments(Argc, ppArgv,
      &ep_pcie_base_address, &ep_local_ddr_addr, &bar_number, NULL))
    exit(1);

  /* parse command line options using getopt() for POSIX compatibility */
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
  
#ifdef ENABLE_DMA
  memset(&action, 0, sizeof (action));	/* clean variable */
  action.sa_handler = signal_handler;	/* specify signal handler */
  action.sa_flags = SA_NODEFER;		/* do not block SIGUSR1 within sig_handler_int */
  sigaction(SIGUSR1, &action, NULL);	/* attach action with SIGIO */
#endif
  src_buff  = (unsigned int *)malloc(MAP_DDR_SIZE);
  dest_buff = (unsigned int *)malloc(MAP_DDR_SIZE);

  fd1 = open(EP_DBGFS_FILE, O_RDWR);
  if (fd1 < 0) {
  	  perror("Error while opening debug file");
  	  goto err;
  } else {
  	  printf("Ep debug file opened successfully\n");
  }

  fd2 = open("/dev/mem", O_RDWR);
  if (fd2 < 0) {
  	  perror("Error while opening /dev/mem file");
  	  goto err;
  } else {
  	  printf("Mem opened successfully\n");
  }

  /* MAP DDR free 1M area. This was reserved at boot time */
  mapDDR = mmap(NULL, MAP_DDR_SIZE,
  		  PROT_READ | PROT_WRITE,
  		  MAP_SHARED, fd2, ep_local_ddr_addr);
  printf(" EP local DDR address = %lx, mapDDR = %lx\n",
  	  ep_local_ddr_addr, (long unsigned int) mapDDR);
  if (!mapDDR) {
  	  perror("/dev/mem DDR area mapping FAILED");
  	  goto err;
  } else {
  	  printf("/dev/mem DDR area mapping OK\n");
  }

  /* Map PCIe area */
  mapPCIe = mmap(NULL, MAP_DDR_SIZE,
  		  PROT_READ | PROT_WRITE,
  		  MAP_SHARED, fd2, ep_pcie_base_address);
  printf(" PCIe base address = %lx, mapPCIe = %lx\n",
  	 ep_pcie_base_address, (long unsigned int) mapPCIe);
  if (!mapPCIe) {
  	  perror("/dev/mem PCIe area mapping FAILED");
  	  goto err;
  } else {
  	  printf("/dev/mem PCIe area mapping OK\n");
  }

  /* Setup inbound window for receiving data into local shared buffer */
  ret = pcie_init_inbound(ep_local_ddr_addr, bar_number, fd1);
  if (ret < 0) {
      perror("Error while setting inbound region");
      goto err;
  } else {
      printf("Inbound region setup successfully\n");
  }

  printf("Connecting to RC...\n");
  rc_ddr_addr = pcie_wait_for_rc((struct s32v_handshake *)mapDDR);
  printf(" RC DDR address = %lx\n", rc_ddr_addr);

  /* Setup outbound window for accessing RC mem */
  ret = pcie_init_outbound(ep_pcie_base_address,
        rc_ddr_addr, MESSBUF_LONG, fd1);
  if (ret < 0) {
  	  perror("Error while setting outbound region");
  	  goto err;
  } else {
  	  printf("Outbound region setup successfully\n");
  }

  memset(src_buff, 8, MESSBUF_LONG);
  memcpy((unsigned int *) mapPCIe, (unsigned int *) src_buff, MESSBUF_LONG);
  printf("RecBase %lx\n", (long unsigned int) &mapPCIe[REC_BASE/4]);
  
  pidf = fork();
  if (pidf == 0) {
    /* child process */
    goon = 1;

    while (goon) {
      rlen = receive_msg_ls2(buffer, dest_buff, mapPCIe, fd1);
      if (rlen > 0) {
        /* data received from RC */
        int nwrite;

#ifdef ENABLE_DUMP
        dump_data(buffer, rlen, "From PCIe RC to TAP interface\n", dumpHexOnly);
#endif

        nwrite = cwrite(tapFd, buffer, rlen);
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
    /* parent process */
    goon = 1;

    while (goon) {
      /* Wait for an event on the MKx file descriptor
       * poll() returns >0 if descriptor is readable, 0 if timeout, -1 if error */
      int Data = poll(FDs, POSIX_IF_CNT, 200);
      if (Data < 0) {	  /* Error */
    	fprintf(stderr, "Poll error %d '%s'\n", errno, strerror(errno));
    	goon = 0;
      } 
      if (Data > 0) {
    	if (FDs[POSIX_IF_TAP].revents & POLL_INPUT) {
    	  int nread;

    	  nread = cread(tapFd, buffer, BUFSIZE);
#ifdef ENABLE_DUMP
    	  dump_data(buffer, nread, "From TAP interface to PCIe RC\n", dumpHexOnly);
#endif
    	  /* send to RC
    	   * data copied in transmit buffer */
    	  send_msg_ls2(buffer, nread, src_buff, &mapPCIe[REC_BASE/4], fd1);
#ifdef ENABLE_DUMP
    	  printf("Done\n");
#endif
    	}
      }
    }
  } else {
     fprintf(stderr, "Fork failed, stopping program\n");
  }
err :
  if (fd1) close(fd1);
  if (fd2) close(fd2);
  exit(0);
}
