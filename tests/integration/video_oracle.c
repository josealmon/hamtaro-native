/* Optional static-snapshot PPU oracle; never linked into the application.
   Built separately against a caller-provided mGBA 0.10.5 checkout/library. */
#include <mgba/flags.h>
#include <mgba/internal/gba/renderers/video-software.h>
#ifdef COLOR_5_6_5
#error The optional oracle requires RGB555 rather than RGB565.
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
static uint8_t io[1024],palette[1024],vram[98304],oam[1024];
static uint16_t palette_channel[512],vram_channel[49152];
static color_t output[240*160];
static uint32_t rgba[240*160];
static uint8_t line_registers[160][12];
static const unsigned line_addresses[]={0x18,0x1C,0x40,0x42,0x50,0x52};
static uint16_t word(const uint8_t *p){return (uint16_t)(p[0]|p[1]<<8);}
int main(int argc,char **argv)
{
    int scanlines=argc==2&&!strcmp(argv[1],"--scanlines");
    if(argc!=1&&!scanlines)return 1;
#ifdef _WIN32
    _setmode(_fileno(stdin),_O_BINARY);_setmode(_fileno(stdout),_O_BINARY);
#endif
    if(sizeof(color_t)!=2){fprintf(stderr,"An RGB555 mGBA build is required.\n");return 2;}
    struct GBAVideoSoftwareRenderer *software=calloc(1,sizeof(*software));
    if(!software)return 3;
    for(;;){
        size_t bytes=fread(io,1,sizeof(io),stdin);
        if(!bytes&&feof(stdin))break;
        if(bytes!=sizeof(io)||fread(palette,1,sizeof(palette),stdin)!=sizeof(palette)||
           fread(vram,1,sizeof(vram),stdin)!=sizeof(vram)||fread(oam,1,sizeof(oam),stdin)!=sizeof(oam))return 4;
        if(scanlines&&fread(line_registers,1,sizeof(line_registers),stdin)!=sizeof(line_registers))return 4;
        for(unsigned i=0;i<240*160;i++)rgba[i]=0xFF000000u;
        /* mGBA 0.10.5's RGB555 fade helper rounds shifted G/B differently
           from R. Channels are independent, so render each through R to keep
           five-bit arithmetic consistent without changing the reference code. */
        for(unsigned channel=0;channel<3;channel++){
            for(unsigned i=0;i<512;i++)palette_channel[i]=(word(palette+i*2)>>(channel*5))&31;
            memcpy(vram_channel,vram,sizeof(vram));
            unsigned display=word(io),mode=display&7;
            if(mode==3||mode==5){
                unsigned base=mode==5&&(display&16)?0xA000/2:0,count=mode==3?240*160:160*128;
                for(unsigned i=0;i<count;i++)vram_channel[base+i]=(word(vram+(base+i)*2)>>(channel*5))&31;
            }
            memset(software,0,sizeof(*software));GBAVideoSoftwareRendererCreate(software);
            software->d.palette=palette_channel;software->d.vram=vram_channel;
            software->d.oam=(union GBAOAM *)oam;software->outputBuffer=output;software->outputBufferStride=240;
            struct GBAVideoRenderer *renderer=&software->d;renderer->init(renderer);
            for(unsigned address=0;address<=0x54;address+=2){
                if(address==4||address==6||address==0x4E)continue;
                renderer->writeVideoRegister(renderer,address,word(io+address));
            }
            /* A snapshot describes settled registers, not enable pipelines
               or mid-scanline writes. Warm up one frame before comparing. */
            for(unsigned frame=0;frame<2;frame++){
                for(int y=0;y<160;y++){
                    if(scanlines)for(unsigned i=0;i<sizeof(line_addresses)/sizeof(*line_addresses);i++)
                        renderer->writeVideoRegister(renderer,line_addresses[i],word(line_registers[y]+i*2));
                    renderer->drawScanline(renderer,y);
                }
                renderer->finishFrame(renderer);
            }
            for(unsigned i=0;i<240*160;i++){
                unsigned value=output[i]&31;rgba[i]|=((value<<3)|(value>>2))<<(channel*8);
            }
            renderer->deinit(renderer);
        }
        if(fwrite(rgba,sizeof(*rgba),240*160,stdout)!=240*160||fflush(stdout))return 5;
    }
    free(software);return 0;
}
