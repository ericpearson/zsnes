#ifndef C_PROCVID_H
#define C_PROCVID_H

#include "../types.h"

// Processes & displays video
void showvideo(void);

u4 SwapMouseButtons(u4 buttons);

void processmouse1(void);
void processmouse2(void);

#ifdef __MSDOS__
// Outputs val at buf
void outputhex(u1* buf, u1 val);
#endif

void outputhex16(u2* buf, u1 val);

#ifdef __MSDOS__
// Outputs char glyph at buf
void outputchar(u1* buf, u1 glyph);
#endif

void outputchar16b(u2* buf, u1 glyph);

#ifdef __MSDOS__
void outputchar5x5(u1* buf, u1 glyph);
#endif

void outputchar16b5x5(u2* buf, u1 glyph);

#endif
