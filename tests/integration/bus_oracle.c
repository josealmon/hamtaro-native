/* Isolated mGBA floating-bus loads; no ROM, BIOS or CPU execution required.
   This optional oracle is never linked into or shipped with the application. */
#include <mgba/flags.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/memory.h>
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static void quiet(struct mLogger *logger,int category,enum mLogLevel level,const char *format,va_list args)
{(void)logger;(void)category;(void)level;(void)format;(void)args;}
int main(void)
{
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    struct mLogger logger={.log=quiet};mLogSetDefaultLogger(&logger);
    struct GBA gba={0};struct ARMCore cpu={0};
    cpu.master=&gba.d;gba.cpu=&cpu;cpu.executionMode=MODE_THUMB;cpu.gprs[ARM_PC]=0x08001004;
    uint32_t command[3];
    while(fread(command,sizeof(command),1,stdin)==1){
        if(command[0]<0x10000000u)return 2;
        cpu.prefetch[0]=command[1];cpu.prefetch[1]=command[2];
        uint32_t result=GBALoad16(&cpu,command[0],NULL);
        if(fwrite(&result,sizeof(result),1,stdout)!=1||fflush(stdout))return 3;
    }
    return ferror(stdin)?4:0;
}
