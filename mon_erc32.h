/**************************************************************************
 * 
 * Filename:
 * 
 *   erc32_mon.h 
 *
 * Description:
 *
 *   This is the monitor for Generic TSC695F computer 
 *
 *   Yao Fei (feiyao@me.com) 
 *
 **************************************************************************/

/*-----------------------  配置区域 ----------------------------------*/
/*  If use EEPROM        */
//#define __USE_EEPROM  1

// Check UART timeout
#define __USE_TIMEOUT 1

//Update PROM 容许更改PROM区域，仅适合ATMEL EEPROM代用的情况
//#define __UPDATE_PROM


/* ---------------------- 计算机硬件配置区域 -----------------------------*/
/*
 *         ------ 内存布局 ------
 * 0                 PROM mon_erc32
 * 32K - 160K        PROM for backup
 * 
 * 0x200_0000        User Application text 
 * 0x208_0000        data segment
 * 0x217_0000        erc32_mon 运行区域 (64K）
 * 0x218_0000        留给用户程序
 * 0x21C_0000        Reserved 
 * 0x21F_FFF0        数字签名区域
 *
 * 0x400_0000        EEPROM
*/

#define RAM_START          0x02000000
#define RAM_SIZE           2048 
#define PATCH_ZONE         (RAM_START + 256*7*1024)

// EEPROM size in MByte s
// EEPROM的起始地址            
// 通用设计，EEPROM在扩展内存区
#define EEPROM_BASE        0x04000000
#define EEPROM_SIZE        2048 

#define EEPROM_ZONE_LEN    (EEPROM_SIZE/4) 
#define EEPATCH_ZONE       (EEPROM_BASE + EEPROM_ZONE_LEN*7*1024/2)   /* 1.75M - 2M */

#define PROM_PROG_START    (32*1024)
#define PROM_LENGTH        (5*32*1024)

// 备份程序运行位置，跳过512K，便于修改主程序
#define BACKUP_PROG_START   (RAM_START + EEPROM_ZONE_LEN*1024)

//数字签名
#define SIG_ZONE           (RAM_START + RAM_SIZE*1024 - 16)

#define SIG_WORD_1         0x12345678   
#define SIG_WORD_2         0x9abcdef0
#define SIG_WORD_3         0xeb90146f

/**********************************************************************/

typedef unsigned char uchar;
typedef unsigned int uint;

typedef enum { 
	COLD_BOOT        = 0xffffffff,  /* 冷启动                     */
	WARM_BOOT        = 0xAAAA5555,  /* 温启动                     */
	PROM_BOOT        = 0xA5A5A5A5   /* 直接从PROM引导             */
} BOOTMODE;

extern  BOOTMODE  boot_status;                    // cold, warm or hot

// 串口等待超时计数
#define MAX_TIMEOUT 0x20000

// --------------  erc32_io ---------------------

void put_char (char c);
void put_string (char *s);
void put_debug_char (unsigned char c);
int  get_char ();
int  hex (unsigned char ch);
void print_u32(unsigned v);

// -------------- EEPROM ------------------------

// 写一个"扇区" 128字
int write_sector(unsigned *data, unsigned sec);

// 把加载到内存的代码写入EEPROM
void write_eeprom(unsigned last_address);

// 从EEPROM加载
int load_eeprom();

// 从PROM加载
int load_prom();

// EDAC control
void inline enable_edac();
void inline disable_edac();
void inline clear_watchdog();

// memcpy in word by word, 32bit
// dst : dest
// src : sorce
// len : length in 32bit words
// return: always 0
int memcpy_w(int *dst, int *src, int len);

/* Update PROM */
int write_prom(int flag);
