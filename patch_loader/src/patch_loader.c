#include <stdio.h>
#include <tamtypes.h>
#include <kernel.h>
#include <sifrpc.h>
#include <debug.h>
#include <loadfile.h>
#include <sio.h>
#include <dirent.h>
#include <malloc.h>
#include <time.h>
#include <unistd.h>
#include <fcntl.h>

extern char	patch_bin[]; //Generated by bin2s. 
extern u32	size_patch_bin;

//TODO: pre-reverse with bin2s if it can
#define REWEN(x) ((((x) >> 24) & 0xFF) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))

#define IOP_BASE_ADDR     0xBC000000
#define PATCH_INSTR_ADDR  0xBCA14000
#define PATCH_BASE_ADDR   0xBCA14004
#define PATCH_BRANCH_ADDR (IOP_BASE_ADDR + 0xa07434)

int main()
{
	SifInitRpc(0);
	
    SifLoadFileInit();

    SifIopReset("", 0);
    while (!SifIopSync()) {};

    printf("patch size: 0x%x\n", size_patch_bin);

    DI();               //disable interrupts on EE
    ee_kmode_enter();   //enter kernel mode 

    //load patch into memory
    for (int i = 0; i < size_patch_bin; i++) {
        *(vu8*)(PATCH_BASE_ADDR + i) = patch_bin[i];
    }

    //backup original instruction
    u32 original_instr = *(vu32*)(PATCH_BRANCH_ADDR);

    //copy original instruction to backup addr
    *(vu32*)(PATCH_INSTR_ADDR) = original_instr;

    //place branch instruction to patch    
    *(vu32*)(PATCH_BRANCH_ADDR) = 0x0640a148; //ba 0xa14004

    ee_kmode_exit();
	EI();

	FlushCache(0);
	FlushCache(2);

    //reset IOP to trigger branch / patch
    SifIopReset("", 0);
    while (!SifIopSync()) {};

    nanosleep((const struct timespec[]){{2, 0}}, NULL);

    printf("patch_loader complete\n");
    
    return 0;
}

