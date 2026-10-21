/* Copyright (c) 2013-2015 Jeffrey Pfau
 * SPDX-License-Identifier: MPL-2.0
 * Adapted from mGBA 0.10.5 src/gba/bios.c's arithmetic HLE helpers.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. A copy is in licenses/mGBA-MPL-2.0.txt.
 * No emulated instructions or BIOS image are used. */
#include "gba/bios.h"

int16_t hn_bios_arctan(int32_t value)
{
    static const int32_t coefficients[]={0x390,0x91C,0xFB6,0x16AA,0x2081,0x3651,0xA2F9};
    int32_t square=(int32_t)((uint32_t)value*(uint32_t)value)>>14;
    int32_t a=(int32_t)(0u-(uint32_t)square),b=0xA9;
    for(unsigned i=0;i<7;i++)b=(int32_t)((uint32_t)((int32_t)((uint32_t)b*(uint32_t)a)>>14)+(uint32_t)coefficients[i]);
    return (int16_t)((int32_t)((uint32_t)value*(uint32_t)b)>>16);
}
uint16_t hn_bios_arctan2(int32_t x,int32_t y)
{
    if(!y)return x<0?0x8000:0;if(!x)return y<0?0xC000:0x4000;
    int64_t ax=x<0?-(int64_t)x:x,ay=y<0?-(int64_t)y:y;
    bool horizontal=y>=0?ax>=ay:(x<=0?ax>ay:ax>=ay);
    int32_t numerator=(int32_t)((uint32_t)(horizontal?y:x)<<14);
    int32_t angle=hn_bios_arctan(hn_runtime_divide(numerator,horizontal?x:y));
    return (uint16_t)(horizontal?angle+(x<0?0x8000:y<0?0x10000:0):(y<0?0xC000:0x4000)-angle);
}
