#ifndef __F_GUI_ICON_H__
#define __F_GUI_ICON_H__

#define ICON_MAGIC 0x4e4712

struct IconDir
{
    int nIconMagic;
    int nNumImages;
};

struct IconHeader
{
    int nBitsPerPixel;
    int nWidth;
    int nHeight;
  
};

#endif // __F_GUI_ICON_H__
