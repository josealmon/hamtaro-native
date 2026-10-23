/* Isolated mGBA floating-bus loads; no ROM, BIOS or CPU execution required.
   This optional oracle is never linked into or shipped with the application. */
#include <mgba/flags.h>
#include <mgba/core/log.h>
#include <mgba/internal/arm/arm.h>
#include <mgba/internal/gba/gba.h>
#include <mgba/internal/gba/memory.h>
#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static void quiet(struct mLogger *logger,int category,enum mLogLevel level,const char *format,va_list args)
{(void)logger;(void)category;(void)level;(void)format;(void)args;}
int main(int argc,char **argv)
{
    bool scene_copy=argc==2&&!strcmp(argv[1],"--scene-copy");
    if(argc!=1&&!scene_copy)return 2;
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    struct mLogger logger={.log=quiet};mLogSetDefaultLogger(&logger);
    struct GBA gba={0};struct ARMCore cpu={0};
    static uint32_t ram[0x10000];
    for(unsigned i=0;i<sizeof(ram);i++)((uint8_t *)ram)[i]=(uint8_t)(i*37+13);
    gba.memory.wram=ram;
    /* Protected BIOS reads after the reference's BIOS service return. */
    gba.memory.biosPrefetch=0xE3A02004u;
    cpu.master=&gba.d;gba.cpu=&cpu;cpu.executionMode=MODE_THUMB;cpu.gprs[ARM_PC]=0x08001004;
    uint32_t command[3];
    while(fread(command,sizeof(command),1,stdin)==1){
        cpu.prefetch[0]=command[1];cpu.prefetch[1]=command[2];
        if(scene_copy){
            if(!((command[0]<=0x01FFFF80u)||
                 (command[0]>=0x02000000u&&command[0]<=0x0203FF80u)||
                 (command[0]>=0x10000000u&&command[0]<=0xFFFFFF80u)))return 2;
            cpu.gprs[ARM_PC]=0x08006690;gba.memory.activeRegion=8;
            uint32_t words[32];
            for(unsigned i=0;i<32;i++){
                GBALoadMultiple(&cpu,command[0]+i*4,1,LSM_IA,NULL);words[i]=cpu.gprs[0];
            }
            if(fwrite(words,sizeof(words),1,stdout)!=1||fflush(stdout))return 3;
            continue;
        }
        uint32_t io=command[0]&0x00FFFFFEu;
        bool floating_io=(command[0]>>24)==4&&io>=0x400&&io!=0x800&&io!=0x802&&io!=0xFFF780;
        if(command[0]<0x10000000u&&!floating_io)return 2;
        uint32_t result=GBALoad16(&cpu,command[0],NULL);
        if(fwrite(&result,sizeof(result),1,stdout)!=1||fflush(stdout))return 3;
    }
    return ferror(stdin)?4:0;
}
