#include <time.h>

#include <sys/kernel.h>
#include "mon_erc32.h"

BOOTMODE boot_status;
extern int volatile  edac_flag;

/*
 * write EEPROM 
 */
unsigned eeprom_temp[512];

// 写一个"扇区" 128字
// data   : input data ( max 128 words)
// sector : SECTOR number in EEPROM . 128words per sector
//

// If want to use EDAC, uncomment following
// #define __USE_EDAC_IN_EEPROM


// EEPROM Wait timeout 
// 10MHz: 0x1000 
// 16MHz: 0x1a00?
#define MAX_EEPROM_WAIT  0x2000



// 禁止读操作的EDAC检查
void inline disable_edac()
{
	int *MCNFR  = (int *)0x01f80010;

	*MCNFR &= (~(1<<14));
}

// 容许读操作的EDAC
void inline enable_edac()
{
	int *MCNFR = (int *)0x01f80010;

	*MCNFR |= (1<<14);
}

int write_sector(unsigned *data, unsigned sec)
{
	/* FIXME */
	/* Add your code here */
	return 0;
}

void write_eeprom(unsigned last_address)
{
	/* FIXME */
	/* Add your code here */
	return;
}

/*
 * 从EEPROM加载代码
 */ 

/*
 * Load from EEPROM
 *
 *
       for each page in zone {
           read and sum zone A (to 0x200_0000 )
           read and sum zone B 
           read and sum zone C
            
           if (SA == SB) {
	       if (SA != SC) {
	          write C
               }
           } else if (SA == SC) {
                 write B
           } else if (SB == SC) {
               read B to target;
               write A
           } else 
                flag 3to2 error;
        } // for each 

        // now zone A is in target, even A!=B!=C
        if ( 3to2 error) && (prom has code)  {
              return -1; // load prom
	} else {
	      do_patch();

              return 0;
	}
*/

// 检查三取二
int check32(unsigned int a, unsigned int b, unsigned int c)
{
	int r = 0;
	
	if (a == b) {
		if (b != c) {
			r = 4;  // C error
		}
		// now a==b==c
	} else if ( a == c) {
		r = 2; // b error
	} else if ( b == c) {
		r = 1; // a error
	} else {
		r = -1;
	}
	
	return r;
}

int load_eeprom()
{
	int i,j;
	unsigned int sa, sb, sc;
	unsigned int *ea = (unsigned int *)EEPROM_BASE;
	unsigned int *p  = (unsigned int *)RAM_START;
	int flag32 = 0;
	int local_flag;
	clear_watchdog();

	disable_edac();

	for (i=0; i<EEPROM_ZONE_LEN*2; i++, p+=128, ea+=128) {
		
		edac_flag = 0;

		put_char('A'+ (i%16));
		if (15 == (i%16)) { 
			put_char('\n');
			clear_watchdog();
		}
		
		for (j=0; j<128; j++) {
			sa = ea[j];
			sb = ea[j + EEPROM_ZONE_LEN*1024/4];
			sc = ea[j + EEPROM_ZONE_LEN*2*1024/4]; // C
			
			switch (local_flag = check32(sa,sb,sc)) {
			case 0:
				p[j] = sa;
				break;
				
			case 1:
				p[j] = sb;
				break;
				
			case 2:
				p[j] = sa;
				break;
				
			case 4:
				p[j] = sa;
				break;
				
			default:
				p[j] = sa;
				local_flag = -1;
			}

		} // for j

		if (-1 == local_flag) {
			flag32 = local_flag;
			continue;
		} else {
			if (local_flag & 1) {
				put_string("\n B->A \n");
				write_sector(p, i);  // Write A
			}
			
			if (local_flag & 2) {
				put_string("\n A->B \n");
				write_sector(p, i+EEPROM_ZONE_LEN*2);  // Write B
			}
			
			if (local_flag & 4) {
				put_string("\n A->C \n");
				write_sector(p, i+EEPROM_ZONE_LEN*2*2); // Write C
			} 
		}
	} // for each

	p = (unsigned int *)PROM_PROG_START; 
	if ((flag32 != 0) && (0x10800000 == (*p & 0xFFFF0000))) {
		i = -1;       // 三取二错误，三份互不相同，而且PROM里有合法程序
	} else {
		i = 0;
	}

	enable_edac();
	return i;
}


int load_prom()
{
	clear_watchdog();
	
	// PROM is 8bits width
	memcpy((void *)BACKUP_PROG_START, (void *)PROM_PROG_START, PROM_LENGTH);
	return BACKUP_PROG_START;
}

#ifdef __UPDATE_PROM
// Write PROM here
int write_prom_sector(uchar *data, int sec)
{
	uchar volatile *prom_base = (uchar *)0;
	int i;
	uchar  old, new; 

	i = (sec*64) &0xf8000;

	// Open SDP 
	prom_base[i+0x5555] = 0xaa;
	prom_base[i+0x2aaa] = 0x55;
	prom_base[i+0x5555] = 0xa0;

	// ATMEL 28HC256 的页面只有64字节长
	for (i=0;i<64;i++) {
		prom_base[sec*64 + i] = data[i];
	}

	old = prom_base[sec*64+63];
	for (i=0; i<MAX_EEPROM_WAIT; i++) {
		//clear_watchdog();
		new = prom_base[sec*64+63];
		if ((old == new) && (new == data[63]))
			break;
		else
			old = new; 
	}

	// just for delay some  time 
	put_char('a' + (sec % 16));
	if (15 == (sec % 16)) {
		put_char('\n');
	}

	for (i=0;i<64;i++) {
		if (data[i] != prom_base[sec*64+i]) 
			break;
	}

	return (i==64);
}

/*
 * Update the bootloader image in PROM, very interesting 
 */
int write_prom(int flag)
{
	int *MCNFR = (int *)0x01f80010;
	int i,j;
	int r=0,p,st,len;

	p  = RAM_START; 

	if (0x55 == flag) {
		st = 0 ;

		len = 32768;

	} else if (0x81 == flag) {
		st = PROM_PROG_START/64;

		len = PROM_LENGTH;
	} else {
		return 0;
	}
		
	*MCNFR |= (1<<16);  // enable PROM write
	
	for (i=0; i<(len/64); i++, p+=64) {
		for (j=0;j<8;j++) { 
			if (write_prom_sector((uchar *)p, (st+i))) 
				break;
		}
		r+=(8==j);
	}

	*MCNFR &= ~(1<<16); // disable PROM write

	if (r) {
		put_string("Write PROM failed.\n");
	} else {
		put_string("Write PROM OK.");
	}

	return r;
}

#endif // __UPDATE_PROM
