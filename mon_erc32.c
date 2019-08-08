/**************************************************************************
 * 
 * Filename:
 * 
 *   erc32_mon.c 
 *
 * Description:
 *
 *   This is the monitor for Generic TSC695F computer 
 *
 *   Yao Fei (feiyao@me.com) 
 *
 **************************************************************************/
#include <intrrpt.h>
#include <sys/kernel.h>
#include "mon_erc32.h"

/*------------------------------------------------------------------------*/
static unsigned  start_address = RAM_START;   /* user program entry point */
static int       eeprom_flag   = 0;           /* eeprom program flag,     */
                                              /* 0 not write, 1 write     */

volatile int edac_flag;                       /* EDAC error occured while read EEPROM */
unsigned int last_address;

// watchdog handler
static void default_watchdog_handler (int sig)
{
	unsigned *wdog_timer_register = (unsigned *) 0x01f80060;
	
	*wdog_timer_register = -1; 
}


static void edac_correct_handler(int sig)
{
	unsigned *reg = (unsigned *)0x01f800a0;
	unsigned addr;
	unsigned volatile v;

	*reg = 0; // Reset System Fault Status Register

	reg = (unsigned *)0x01f800a4;
	addr = (*reg) & 0xffffffc;  // word address

	// FIXME
	v = *(unsigned *)addr; 
	*(unsigned *)addr = v;
	reg = (int *)0x01f80050;
	*reg = 0x40;   //注意回写过程会引发第二次中断，退出时清除伊
	edac_flag++;
}

static void segv_handler(int sig)
{
	unsigned *reg = (unsigned *)0x01f800a0; 
	unsigned sysfsr = *reg;
	unsigned *addr;

	*reg = 0;  // 清异常

	reg = (unsigned *)0x01f800a4; 
	addr = (unsigned *)*reg;  //读出错误地址

#ifdef __DEBUG
	print_u32(sysfsr);
	print_u32((unsigned)addr);
#endif

	if ((sysfsr & 0x3c) == 0x3c) {   // EDAC
#ifdef __DEBUG
		put_string("EDAC Fatal Error");
#endif
		reg = (unsigned *)0x01f80004;
		*reg = 0;  // SWRST
	} 
#ifdef __DEBUG
	else {
	        put_string("System Error");
        }
#endif
}


// get byte
static int get_byte() 
{
	int h,l;

	do {
		h = get_char();
	} while ( -1 == h);  // 忽略串口超时


	do {
		l = get_char();
	} while ( -1 == l);

	return (((hex(h) << 4) + hex(l) ) & 0xff);
} 

//--------------------------------------------------------------------------------
// 把所有需要初始化的寄存器地址和值放在这里
#define NUM_INIT_REGS   10 
static struct INIT_VECT {
	int reg;
	int value;
}  init_vector[NUM_INIT_REGS] = {
	{0x01F80064,         0},  // 禁止看门狗
	{0x01F800A0,	     0},  // reset SYSFSR
	{0x01F80050,    0xfffe},  // INTCLR 清除所有中断
	{0,0}
};

// need fix
void init_board()
{
	int i;

	put_string("\nInit hardware regs\n");

	for (i=0;i<NUM_INIT_REGS;i++) {
		if (0 == init_vector[i].reg) 
			break;
		put_char('.');
		
		*(int *)(init_vector[i].reg) = init_vector[i].value;
	}
	put_string("OK\n");
}

// Init EDAC
#define EDAC_INIT_CHIP  64
extern int _erodata;           // end of read-only data seg. in art0.S
extern int _ERC32_MON_END;     // end of monitor self in ldscripts
extern int _ERC32_MON_START;   // begin of monitor self

void init_edac(BOOTMODE bootmode)
{
	int i, *p;

	put_string("Init EDAC");

	disable_edac();  // 关掉EDAC后，写操作依然会产生EDAC校验码
	if (COLD_BOOT == bootmode) {
		// 冷启动，向内存所有单元填写0，但是跳过ERC32_MON所在区域
		for ( p =(int *)RAM_START; p< (int *)&_ERC32_MON_START; p++) {
			*p = 0;
		}
		
		clear_watchdog();
		
		// ERC32_MON后面的区域
		for (p = (int *)&_ERC32_MON_END; p<(int *)(RAM_START+RAM_SIZE*1024); p++) {
			*p = 0;
		}

	} else {
		clear_watchdog();
		
		// 温和热启动，不能破坏内存，就自己拷贝自己一次。
		for (i=0; i<(RAM_SIZE/EDAC_INIT_CHIP); i++) {
			// 避开加载器本身 
			if (i != (RAM_SIZE-256-EDAC_INIT_CHIP)/EDAC_INIT_CHIP) {
				
				memcpy_w((int *)(RAM_START + i*EDAC_INIT_CHIP*1024), 
					 (int *)(RAM_START + i*EDAC_INIT_CHIP*1024), 
					 EDAC_INIT_CHIP*1024/sizeof(int));
				clear_watchdog();
				put_char('.');
			}
		}
	}

	// 加载器自身所在区域空白处的EDAC初始化
	memcpy_w(&_erodata, &_erodata, ((int)&_ERC32_MON_END - (int)&_erodata)/4);
	enable_edac();
		
	clear_watchdog();
	put_string("OK\n");
}
	
//判签名档
BOOTMODE check_sig() 
{
	//读签名
	int sig[4];

	BOOTMODE r = COLD_BOOT;

	// 处理一下数字签名区域的EDAC, 自己拷贝到自己
	disable_edac();
	memcpy_w(sig, (int *)SIG_ZONE, 4);
	memcpy_w((int *)SIG_ZONE, sig, 4);
	enable_edac();

#ifdef __DEBUG
	put_string("\nSig:");
	print_u32(sig[0]);
	put_char('\t');
	print_u32(sig[1]);
#endif

	if ((sig[0]    == SIG_WORD_1)
	    && (sig[1] == SIG_WORD_3)
	    && (sig[2] == (SIG_WORD_1 ^ SIG_WORD_3))) {
		put_string(" Warm");
		r = WARM_BOOT;
		
	} else if ((sig[0]    == SIG_WORD_2)
		   && (sig[1] == SIG_WORD_3)
		   && (sig[2] == (SIG_WORD_2 ^ SIG_WORD_3))) {
		
		put_string(" PROM");
		
		r = PROM_BOOT;  //直接调用PROM备份程序
	} else {
		// cold boot, help EDAC 
		put_string(" Cold");
	}

	put_char('\n');
	
	return r;  //冷启动
}

// 下载HEX代码文件 Download the Intel HEX file 
// HEX 文件数据域识别代码
enum {
	// 以下是Intel规定的
	DATA        = 0, 
	EOF         = 1, 
	SEGADDR     = 2, 
	STARTADDR   = 3, 
	EXLINEADDR  = 4,
	STRLINEADDR = 5,
	
	// 以下是我们自己扩展的
	EEPROM      = 0x88,      // 写EEPROM
	PROM        = 0x55,      // 写引导PROM
	FULL_EEPROM = 0x77,      // 写全部EEPROM
} HEXFILE;

void downldhex()
{
        unsigned data, addr;   // don't use unsigned *!!!
	unsigned i, sum;
	int      clas, len;
	unsigned base_addr = RAM_START;
	int      eof = 0;

	while (0 == eof) {
		//Intel HEX file 的每一行都是从字符:开始的
		do {
			clear_watchdog();
		} while (get_char() != ':'); 
		clear_watchdog();

		len = get_byte(); //有效数据的字节数，不包括和校验
		sum = len;

		addr = get_byte();
		sum += addr;
		addr *= 256; 
		i = get_byte();
		sum += i;
		addr += i;

		clas = get_byte();
		sum += clas;

		switch (clas) {
		case DATA: // Data 

			addr += base_addr; 
			while(len-- > 0 ) {
				data = get_byte();
				sum += data;
				*(unsigned char *)addr = (unsigned char)data;
				addr++;
			}

			// 记录加载到的最大地址
			if (last_address < addr) 
				last_address = addr;

			sum += get_byte();  //和校验
			sum &= 0xff;
			if (sum) {
				put_string("Recv: Checksum error, aborted...\n");
				eeprom_flag = 0; //出错就不再作可能的写EEPROM操作
			} else {
				put_char('.');
			}

#ifdef __UPDATE_PROM
			if ((PROM == eeprom_flag) && (addr > (RAM_START + 32768))) {
				put_string("PROM image too large");
				eeprom_flag = 0; //出错就不再作可能的写EEPROM操作
			}
#endif // __UPDATE_PROM
			break;

		case EOF: // EOF flag
			if (get_byte() ==  0xff)  {
				put_string("Receive success.\n");
				if ( 1 == eeprom_flag ) {
					write_eeprom(last_address);
				}
#ifdef __UPDATE_PROM			    
				else if (PROM == eeprom_flag) {
					write_prom(eeprom_flag);
				}
#endif // __UPDATE_PROM
				eof = -1; 
			}
			break;


		case EXLINEADDR:
			i = get_byte();
			sum += i;
			data = ( i << 24 );
			i = get_byte();
			sum += i;
			data +=  (i<<16);

			base_addr = data;

			sum += get_byte();
			sum &= 0xff;

			if (sum) {
				put_string("Checksum error , aborted ...\n\n");
				eeprom_flag = 0; //出错就不再作可能的写EEPROM操作
			}

			break;
			
			
		case STRLINEADDR:
			data = 0;
			for (len=0; len<4; len++) {
				i    = get_byte();
				sum  += i;
				data <<= 8;
				data  += i;
			}
			
			start_address = data;  // seg addr shift 4 bits
	
			sum += get_byte(); 
			sum &= 0xff;
			
			if (sum) {
				put_string("Checksum error , aborted ...\n\n");
				eeprom_flag = 0; //出错就不再作可能的写EEPROM操作
			}

			break;
#ifdef __UPDATE_PROM
		case PROM:
			eeprom_flag = PROM;
			put_string("\nWrite to PROM.\n !!! Very dangerous, caution !!!");
			break;

#endif //__UPDATE_PROM

		case FULL_EEPROM:
			last_address = RAM_START + EEPROM_ZONE_LEN*1024 -1;
			put_string("flush whole EEPROM\n");
			break;

		case EEPROM:
			put_string("Write to EEPROM \n");
			eeprom_flag = 1; 
			break;

		default:
			put_string("Error, aborted...\n");
			eeprom_flag = 0; //出错就不再作可能的写EEPROM操作
			break;
		} // switch
	}
}


int main ()
{
	int i, n=-1;
	void (*Application)();  // 用户应用程序
	

	// raise to supervisor mode 
	__xgc_set_level (128);

	/*
	 * Print the subversion identifier on the console. 
	 */
	put_string("\n\n\n------------------------------------\n\r");
	put_string("Universal monitor for Generic TSC695F computer (c)2012,2020 SISE\n");
	put_string("Build on ");put_string(__DATE__);put_char(' ');put_string(__TIME__);
#ifdef __DEBUG
	put_string("  *** DEBUG verbose version ***\n");
#else
	put_char('\n');
#endif

	handler (INTWATCHDOG_TIMEOUT,         default_watchdog_handler);
	handler (INTCORRECTABLE_MEMORY_ERROR, edac_correct_handler);
	handler (INTSEGV,                     segv_handler);

	init_board();
	boot_status = check_sig();
	init_edac(boot_status);

#ifdef __UPDATE_PROM
	put_string("Input :00000088 to write EEPROM Zone.\n");
	put_string("Input :00000077 to flush whole 2Mbytes EEPROM Zone.\n");

	put_string("Input :00000055 to write PROM Zone.\n");
#endif

	switch (boot_status) {
	case COLD_BOOT:
		
		// wait user to press any key
		put_string("Press <ENTER> to upload HEX code.\n");

		for (i=0; i<10; i++) {   //about 2s
			n = get_char();  //规定的时间内，串口有数据，则get_char()返回，否则返回-1
			if (n != -1) 
				break;
		}

		if (-1 == n) {            // TIMEOUT
			put_string("Load from EEPROM\n");
			if (0 != load_eeprom()) {
				start_address = (unsigned)load_prom();   // load default from PROM
			}
			
		} else {
			// 串口
			//从串口下载数据到扩展内存，同时把数据搬到EEPROM的主份代码区，
			//并计算校验和存到EEPROM相应位置，补丁代码区清零
			put_string("\nWait HEX.\n");
			downldhex();
		}
		break;

	case WARM_BOOT:
		//  热启动和温启动根本不等待串口	
		put_string("Load from EEPROM\n");
		if (0 != load_eeprom()) {
			start_address = (unsigned)load_prom();   // load default from PROM
		}
		break;

	case PROM_BOOT:
	default:
		start_address = (unsigned)load_prom();
		break;

	} // switch 

	clear_watchdog();
	init_board();   // 重新初始化一次板上寄存器，给用户程序一个干净环境

	Application  = (void *)start_address;
	Application();   // jump to user code 
}
