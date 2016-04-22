/**************************************************************************
 * 
 * Filename:
 * 
 *   erc32_io.c 
 *
 * Description:
 *
 *   This is the monitor for Generic TSC695F computer 
 *
 *   Yao Fei (feiyao@me.com) 
 *
 **************************************************************************/

#include <sys/kernel.h>
#include "mon_erc32.h"

// 选择使用哪个UART作为加载器使用
#ifdef __USE_UARTA

#define TXDATA (int *)0x01F800E0
#define TXRDY  4

#define RXDATA (int *)0x01F800E0
#define RXRDY  0x00000001
#define RXERR  0x00000070
#define RXCLR  0x00000080

#else   // USE Dual UART, UART B as mon_erc32

#define TXDATA (int *)0x01F800E4
#define TXRDY  0x40000
#define RXDATA (int *)0x01F800E4
#define RXRDY  0x00010000
#define RXERR  0x00700000
#define RXCLR  0x00800000	

#endif

//  ---------  STDIO  ------------------
// copy from XGC monitor 

/*
 * Write one character to the console. We use ERC32 serial interface A.
 */
void put_char (char c)
{
	volatile int *txdata = TXDATA;
	volatile int *txstat  = (int *) 0x01F800E8;
	
	if (c == '\n') {
		/* Wait until transmitter is ready for more data.  */
		while (!((*txstat) & TXRDY))
			;

		/* Send CR to transmitter.  */
		*txdata = '\r';
	}

	/* Wait until transmitter is ready for more data.  */
	while (!((*txstat) & TXRDY))
		;

	/* Send next character to transmitter.  */
	*txdata = c;
}

void put_string (char *s)
  /*
   * Write the string s to stdout.
   */
{
	while (*s) {
		put_char (*s++);
	}
}


int get_char ()
/*
 * Read one character from the serial interface. We use ERC32 board
 * serial interface A.
 */
{
	volatile unsigned int *mec    = (int *) 0x01F80000;
	volatile unsigned int *rxdata = RXDATA;
	volatile unsigned int *rxstat = (int *) 0x01F800E8;

#ifdef __USE_TIMEOUT
	int timeout = 0;
#endif

	/* Wait until receive buffer not empty.  */
	while (!((*rxstat) & RXRDY)
#ifdef __USE_TIMEOUT 
	       && (timeout < MAX_TIMEOUT) 
#endif
		) {
		if (*rxstat & RXERR) {
			/* Clear the error. Note: this is a workaround for
			   MEC bug 4.2 */
			*rxstat = RXCLR;
			mec[0] = mec[0];
		}
#ifdef __USE_TIMEOUT
		timeout++;
#endif
	}

	/* Read one character.  */

#ifdef __USE_TIMEOUT	
	if (timeout < MAX_TIMEOUT) {
		timeout = (*rxdata) & 0xff;
	} else {
		timeout = -1;
	}

	return timeout;
#else
	return (*rxdata) & 0xff;
#endif
}

int hex (unsigned char ch)
/*
 * Convert a hex character, '0' to '9', 'a' to 'f', or 'A' to 'F' to an
 * integer in the range 0 .. 15.  Return -1 if the given character is
 * not a hex character.
 */
{
	if (ch >= 'a' && ch <= 'f')
		return ch - 'a' + 10;
	if (ch >= '0' && ch <= '9')
		return ch - '0';
	if (ch >= 'A' && ch <= 'F')
		return ch - 'A' + 10;
	return -1;
}

void inline clear_watchdog()
{
	/* FIXME */
	/* Add your code here */
}


// memcpy in word by word, 32bit
// dst : dest
// src : sorce
// len : length in 32bit words
int memcpy_w(int *dst, int *src, int len)
{
	while(len-- > 0 ) {
		*dst++ = *src++;
	}

	return 0;
}


// 输出一个32位无符号十六进制数
static const char hexchars[] = "0123456789ABCDEF";

void print_u32(unsigned v)
{
	int i,a;

	for (i=0;i<8;i++) {
		a = v & 0xf0000000;
		v <<= 4;
		put_char(hexchars[(a>>28) & 0xf]);
	}
	put_char('\t');
} 
