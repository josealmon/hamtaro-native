/* ROM-free arithmetic oracle. Build against the caller's mGBA 0.10.5 checkout.
   Including its translation unit exposes static math helpers without running a
   CPU or supplying a BIOS. This executable is never linked into the app. */
#include <gba/bios.c>
#include <stdio.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
int main(void)
{
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    uint32_t command[3];
    while(fread(command,sizeof(command),1,stdin)==1){
        int32_t a=(int32_t)command[2],b=0;uint32_t cycles=0,result[4]={command[1],command[2],0,0};
        if(command[0]==9){result[0]=(uint32_t)(int32_t)_ArcTan((int32_t)command[1],&a,&b,&cycles);result[1]=(uint32_t)a;result[3]=(uint32_t)b;}
        else if(command[0]==10){result[0]=(uint16_t)_ArcTan2((int32_t)command[1],(int32_t)command[2],&a,&cycles);result[1]=(uint32_t)a;result[3]=0x170;}
        else return 2;
        if(fwrite(result,sizeof(result),1,stdout)!=1||fflush(stdout))return 3;
    }
    return ferror(stdin)?4:0;
}
