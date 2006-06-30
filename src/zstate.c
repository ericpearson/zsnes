/*
Copyright (C) 1997-2006 ZSNES Team ( zsKnight, _Demo_, pagefault, Nach )

http://www.zsnes.com
http://sourceforge.net/projects/zsnes

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
version 2 as published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/



#ifdef __UNIXSDL__
#include "gblhdr.h"
#define DIR_SLASH "/"
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <time.h>
#ifdef __WIN32__
#include <io.h>
#else
#include <unistd.h>
#endif
#define DIR_SLASH "\\"
#endif
#include "numconv.h"
#include "gblvars.h"
#include "asm_call.h"
#include "zpath.h"

#ifdef __MSDOS__
#define clim() __asm__ __volatile__ ("cli");
#define stim() __asm__ __volatile__ ("sti");
#else
#define clim()
#define stim()
#endif

void SA1UpdateDPageC(), unpackfunct(), repackfunct();
void PrepareOffset(), ResetOffset(), initpitch(), UpdateBanksSDD1();
void procexecloop(), outofmemory();

extern char *ZSaveName;
void setextension(char *str);

static void copy_snes_data(unsigned char **buffer, void (*copy_func)(unsigned char **, void *, size_t))
{
  //65816 status, etc.
  copy_func(buffer, &curcyc, PH65816regsize);
  //SPC Timers
  copy_func(buffer, &cycpbl, 2*4);
  //SNES PPU Register status
  copy_func(buffer, &sndrot, 3019);
}

static void copy_spc_data(unsigned char **buffer, void (*copy_func)(unsigned char **, void *, size_t))
{
  //SPC stuff, DSP stuff
  copy_func(buffer, SPCRAM, PHspcsave);
  copy_func(buffer, &BRRBuffer, PHdspsave);
  copy_func(buffer, &DSPMem, sizeof(DSPMem));
}

static void copy_extra_data(unsigned char **buffer, void (*copy_func)(unsigned char **, void *, size_t))
{
  copy_func(buffer, &soundcycleft, 4);
  copy_func(buffer, &curexecstate, 4);
  copy_func(buffer, &nmiprevaddrl, 4);
  copy_func(buffer, &nmiprevaddrh, 4);
  copy_func(buffer, &nmirept, 4);
  copy_func(buffer, &nmiprevline, 4);
  copy_func(buffer, &nmistatus, 4);
  copy_func(buffer, &joycontren, 4);
  copy_func(buffer, &NextLineCache, 1);
  copy_func(buffer, &spc700read, 10*4);
  copy_func(buffer, &timer2upd, 4);
  copy_func(buffer, &xa, 14*4);
  copy_func(buffer, &spcnumread, 1);
  copy_func(buffer, &opcd, 6*4);
  copy_func(buffer, &HIRQCycNext, 4);
  copy_func(buffer, &HIRQNextExe, 1);
  copy_func(buffer, &oamaddr, 14*4);
  copy_func(buffer, &prevoamptr, 1);
}

static size_t load_save_size;

enum copy_state_method { csm_save_zst_new,
                         csm_load_zst_new,
                         csm_load_zst_old,
                         csm_save_rewind,
                         csm_load_rewind };

static void copy_state_data(unsigned char *buffer, void (*copy_func)(unsigned char **, void *, size_t), enum copy_state_method method)
{
  copy_snes_data(&buffer, copy_func);

  //WRAM (128k), VRAM (64k)
  copy_func(&buffer, wramdata, 8192*16);
  copy_func(&buffer, vram, 4096*16);

  if (spcon)
  {
    copy_spc_data(&buffer, copy_func);
    /*
    if (buffer) //Rewind stuff
    {
      copy_func(&buffer, &echoon0, PHdspsave2);
    }
    */
  }

  if (C4Enable)
  {
    copy_func(&buffer, C4Ram, 2048*4);
  }

  if (SFXEnable)
  {
    copy_func(&buffer, sfxramdata, 8192*16);
    copy_func(&buffer, &SfxR0, PHnum2writesfxreg);
  }

  if (SA1Enable)
  {
    copy_func(&buffer, &SA1Mode, PHnum2writesa1reg);
    copy_func(&buffer, SA1RAMArea, 8192*16);
    if (method != csm_load_zst_old)
    {
      copy_func(&buffer, &SA1Status, 3);
      copy_func(&buffer, &SA1xpc, 1*4);
      copy_func(&buffer, &sa1dmaptr, 2*4);
    }
  }

  if (DSP1Type && (method != csm_load_zst_old))
  {
    copy_func(&buffer, &DSP1COp, 70+128);
    copy_func(&buffer, &Op00Multiplicand, 3*4+128);
    copy_func(&buffer, &Op10Coefficient, 4*4+128);
    copy_func(&buffer, &Op04Angle, 4*4+128);
    copy_func(&buffer, &Op08X, 5*4+128);
    copy_func(&buffer, &Op18X, 5*4+128);
    copy_func(&buffer, &Op28X, 4*4+128);
    copy_func(&buffer, &Op0CA, 5*4+128);
    copy_func(&buffer, &Op02FX, 11*4+3*4+28*8+128);
    copy_func(&buffer, &Op0AVS, 5*4+14*8+128);
    copy_func(&buffer, &Op06X, 6*4+10*8+4+128);
    copy_func(&buffer, &Op01m, 4*4+128);
    copy_func(&buffer, &Op0DX, 6*4+128);
    copy_func(&buffer, &Op03F, 6*4+128);
    copy_func(&buffer, &Op14Zr, 9*4+128);
    copy_func(&buffer, &Op0EH, 4*4+128);
  }

  if (SETAEnable)
  {
    copy_func(&buffer, setaramdata, 256*16);

    //Todo: copy the SetaCmdEnable?  For completeness we should do it
    //but currently we ignore it anyway.
  }

  if (SPC7110Enable)
  {
    copy_func(&buffer, romdata+0x510000, 65536);
    copy_func(&buffer, &SPCMultA, PHnum2writespc7110reg);
  }

  if (method != csm_load_zst_old)
  {
    copy_extra_data(&buffer, copy_func);

    //We don't load SRAM from new states if box isn't checked
    if ((method != csm_load_zst_new) || SRAMState)
    {
      copy_func(&buffer, sram, ramsize);
    }

    if ((method == csm_save_rewind) || (method == csm_load_rewind))
    {
      copy_func(&buffer, &tempesi, 4);
      copy_func(&buffer, &tempedi, 4);
      copy_func(&buffer, &tempedx, 4);
      copy_func(&buffer, &tempebp, 4);
    }
  }
}

static void memcpyinc(unsigned char **dest, void *src, size_t len)
{
  memcpy(*dest, src, len);
  *dest += len;
}

static void memcpyrinc(unsigned char **src, void *dest, size_t len)
{
  memcpy(dest, *src, len);
  *src += len;
}

extern unsigned int RewindTimer, DblRewTimer;
extern unsigned char RewindStates, EMUPause, PauseRewind, EmuSpeed;

unsigned char *StateBackup = 0;
unsigned char AllocatedRewindStates, LatestRewindPos, EarliestRewindPos;
bool RewindPosPassed;

size_t rewind_state_size, cur_zst_size, old_zst_size;

extern unsigned char RewindFrames, romispal, MovieProcessing;
void zmv_rewind_save(size_t, bool);
void zmv_rewind_load(size_t, bool);

void ClearCacheCheck()
{
  memset(vidmemch2, 1, sizeof(vidmemch2));
  memset(vidmemch4, 1, sizeof(vidmemch4));
  memset(vidmemch8, 1, sizeof(vidmemch8));
}

//Code to handle special frames for pausing, and desync checking
unsigned char *SpecialPauseBackup = 0, PauseFrameMode = 0;
/*
Pause frame modes

0 - no pause frame stored
1 - pause frame ready to be stored
2 - pause frame stored
3 - pause frame ready for reload
*/

void *doMemAlloc(size_t);

void BackupPauseFrame()
{
  copy_state_data(SpecialPauseBackup, memcpyinc, csm_save_rewind);
  PauseFrameMode = 2;
}

void RestorePauseFrame()
{
  copy_state_data(SpecialPauseBackup, memcpyrinc, csm_load_rewind);
  //ClearCacheCheck();
  PauseFrameMode = 0;
}

#define ActualRewindFrames (unsigned int)(RewindFrames * (romispal ? 10 : 12))

void BackupCVFrame()
{
  unsigned char *RewindBufferPos = StateBackup + LatestRewindPos*rewind_state_size;

  if (MovieProcessing == 1) { zmv_rewind_save(LatestRewindPos, true); }
  else if (MovieProcessing == 2) { zmv_rewind_save(LatestRewindPos, false); }
  copy_state_data(RewindBufferPos, memcpyinc, csm_save_rewind);

  if (RewindPosPassed)
  {
    EarliestRewindPos = (EarliestRewindPos+1)%AllocatedRewindStates;
    RewindPosPassed = false;
  }
//  printf("Backing up in #%u, earliest: #%u, allocated: %u\n", LatestRewindPos, EarliestRewindPos, AllocatedRewindStates);

  LatestRewindPos = (LatestRewindPos+1)%AllocatedRewindStates;

  if (LatestRewindPos == EarliestRewindPos) { RewindPosPassed = true; }

  RewindTimer = ActualRewindFrames;
  DblRewTimer += (DblRewTimer) ? 0 : ActualRewindFrames;
//  printf("New backup slot: #%u, timer %u, check %u\n", LatestRewindPos, RewindTimer, DblRewTimer);
}

void RestoreCVFrame()
{
  unsigned char *RewindBufferPos;

  if (LatestRewindPos != ((EarliestRewindPos+1)%AllocatedRewindStates))
  {
    if (DblRewTimer > ActualRewindFrames)
    {
      if (LatestRewindPos == 1 || AllocatedRewindStates == 1)
        { LatestRewindPos = AllocatedRewindStates-1; }
      else { LatestRewindPos = (LatestRewindPos) ? LatestRewindPos-2 : AllocatedRewindStates-2; }
    }
    else
    {
      LatestRewindPos = (LatestRewindPos) ? LatestRewindPos-1 : AllocatedRewindStates-1;
    }
  }
  else { LatestRewindPos = EarliestRewindPos; }

  RewindBufferPos = StateBackup + LatestRewindPos*rewind_state_size;
//  printf("Restoring from #%u, earliest: #%u\n", LatestRewindPos, EarliestRewindPos);

  if (MovieProcessing == 2)
  {
    zmv_rewind_load(LatestRewindPos, false);
  }
  else
  {
    if (MovieProcessing == 1)
    {
      zmv_rewind_load(LatestRewindPos, true);
    }

    if (PauseRewind || EMUPause)
    {
      PauseFrameMode = EMUPause = true;
    }
  }

  copy_state_data(RewindBufferPos, memcpyrinc, csm_load_rewind);
  ClearCacheCheck();

  LatestRewindPos = (LatestRewindPos+1)%AllocatedRewindStates;
  RewindTimer = ActualRewindFrames;
  DblRewTimer = 2*ActualRewindFrames;
}

void SetupRewindBuffer()
{
  //For special rewind case to help out pauses
  if (SpecialPauseBackup){ free(SpecialPauseBackup); }
  SpecialPauseBackup = doMemAlloc(rewind_state_size);

  //For standard rewinds
  if (StateBackup){ free(StateBackup); }
  for (; RewindStates; RewindStates--)
  {
    StateBackup = 0;
    StateBackup = (unsigned char *)malloc(rewind_state_size*RewindStates);
    if (StateBackup) { break; }
  }
  AllocatedRewindStates = RewindStates;
}

static size_t state_size;

static void state_size_tally(unsigned char **dest, void *src, size_t len)
{
  state_size += len;
}

void InitRewindVars()
{
  unsigned char almost_useless_array[1]; //An array is needed for copy_state_data to give the correct size
  state_size = 0;
  copy_state_data(almost_useless_array, state_size_tally, csm_save_rewind);
  rewind_state_size = state_size;

  SetupRewindBuffer();
  LatestRewindPos = 0;
  EarliestRewindPos = 0;
  RewindPosPassed = false;
  RewindTimer = 1;
  DblRewTimer = 1;
}

//This is used to preserve system load state between game loads
static unsigned char *BackupSystemBuffer = 0;

void BackupSystemVars()
{
  unsigned char *buffer;

  if (!BackupSystemBuffer)
  {
    state_size = 0;
    copy_snes_data(&buffer, state_size_tally);
    copy_spc_data(&buffer, state_size_tally);
    copy_extra_data(&buffer, state_size_tally);
    BackupSystemBuffer = doMemAlloc(state_size);
  }

  buffer = BackupSystemBuffer;
  copy_snes_data(&buffer, memcpyinc);
  copy_spc_data(&buffer, memcpyinc);
  copy_extra_data(&buffer, memcpyinc);
}

void RestoreSystemVars()
{
  unsigned char *buffer = BackupSystemBuffer;
  InitRewindVars();
  copy_snes_data(&buffer, memcpyrinc);
  copy_spc_data(&buffer, memcpyrinc);
  copy_extra_data(&buffer, memcpyrinc);
}

extern unsigned int spcBuffera;
extern unsigned int Voice0BufPtr, Voice1BufPtr, Voice2BufPtr, Voice3BufPtr;
extern unsigned int Voice4BufPtr, Voice5BufPtr, Voice6BufPtr, Voice7BufPtr;
extern unsigned int spcPCRam, spcRamDP;

void PrepareSaveState()
{
  spcPCRam -= (unsigned int)SPCRAM;
  spcRamDP -= (unsigned int)SPCRAM;

  Voice0BufPtr -= spcBuffera;
  Voice1BufPtr -= spcBuffera;
  Voice2BufPtr -= spcBuffera;
  Voice3BufPtr -= spcBuffera;
  Voice4BufPtr -= spcBuffera;
  Voice5BufPtr -= spcBuffera;
  Voice6BufPtr -= spcBuffera;
  Voice7BufPtr -= spcBuffera;
}

extern unsigned int SA1Stat;
extern unsigned char IRAM[2049], *SA1Ptr, *SA1RegPCS, *CurBWPtr, *SA1BWPtr;
extern unsigned char *SNSBWPtr;

void SaveSA1()
{
  SA1Stat &= 0xFFFFFF00;
  SA1Ptr -= (unsigned int)SA1RegPCS;

  if (SA1RegPCS == IRAM)
  {
    SA1Stat = (SA1Stat & 0xFFFFFF00) + 1;
  }

  if (SA1RegPCS == IRAM-0x3000)
  {
    SA1Stat = (SA1Stat & 0xFFFFFF00) + 2;
  }

  SA1RegPCS -= (unsigned int)romdata;
  CurBWPtr -= (unsigned int)romdata;
  SA1BWPtr -= (unsigned int)romdata;
  SNSBWPtr -= (unsigned int)romdata;
}

void RestoreSA1()
{
  SA1RegPCS += (unsigned int)romdata;
  CurBWPtr += (unsigned int)romdata;
  SA1BWPtr += (unsigned int)romdata;
  SNSBWPtr += (unsigned int)romdata;

  if ((SA1Stat & 0xFF) == 1)
  {
    SA1RegPCS = IRAM;
  }

  if ((SA1Stat & 0xFF) == 2)
  {
    SA1RegPCS = IRAM-0x3000;
  }

  SA1Ptr += (unsigned int)SA1RegPCS;
  SA1RAMArea = romdata + 4096*1024;
}

#define ResState(Voice_BufPtr) \
  Voice_BufPtr += spcBuffera; \
  if (Voice_BufPtr >= spcBuffera + 65536*4) \
  { \
    Voice_BufPtr = spcBuffera; \
  }

void ResetState()
{
  spcPCRam += (unsigned int)SPCRAM;
  spcRamDP += (unsigned int)SPCRAM;

  ResState(Voice0BufPtr);
  ResState(Voice1BufPtr);
  ResState(Voice2BufPtr);
  ResState(Voice3BufPtr);
  ResState(Voice4BufPtr);
  ResState(Voice5BufPtr);
  ResState(Voice6BufPtr);
  ResState(Voice7BufPtr);
}

unsigned char firstsaveinc = 0;

extern unsigned int statefileloc, CurrentHandle, SfxRomBuffer, SfxCROM;
extern unsigned int SfxLastRamAdr, SfxRAMMem, MsgCount, MessageOn;
extern unsigned char AutoIncSaveSlot, cbitmode, NoPictureSave;
extern char *Msgptr, fnamest[512];
extern unsigned short PrevPicture[64*56];

static FILE *fhandle;
void CapturePicture();

static void write_save_state_data(unsigned char **dest, void *data, size_t len)
{
  fwrite(data, 1, len, fhandle);
}

static const char zst_header_old[] = "ZSNES Save State File V0.6\x1a\x3c";
static const char zst_header_cur[] = "ZSNES Save State File V143\x1a\x8f";

void calculate_state_sizes()
{
  state_size = 0;
  copy_state_data(0, state_size_tally, csm_save_zst_new);
  cur_zst_size = state_size + sizeof(zst_header_cur)-1;

  state_size = 0;
  copy_state_data(0, state_size_tally, csm_load_zst_old);
  old_zst_size = state_size + sizeof(zst_header_old)-1;
}

static bool zst_save_compressed(FILE *fp)
{
  size_t data_size = cur_zst_size - (sizeof(zst_header_cur)-1);
  unsigned char *buffer = 0;

  bool worked = false;

  if ((buffer = (unsigned char *)malloc(data_size)))
  {
    //Compressed buffer which must be at least 0.1% larger than source buffer plus 12 bytes
    //We devide by 1000 then add an extra 1 as a quick way to get a buffer large enough when
    //using integer division
    unsigned long compressed_size = data_size + data_size/1000 + 13;
    unsigned char *compressed_buffer = 0;

    if ((compressed_buffer = (unsigned char *)malloc(compressed_size)))
    {
      copy_state_data(buffer, memcpyinc, csm_save_zst_new);
      if (compress2(compressed_buffer, &compressed_size, buffer, data_size, Z_BEST_COMPRESSION) == Z_OK)
      {
        fwrite3(compressed_size, fp);
        fwrite(compressed_buffer, 1, compressed_size, fp);
        worked = true;
      }
      free(compressed_buffer);
    }
    free(buffer);
  }

  if (!worked) //Compression failed for whatever reason
  {
    fwrite3(cur_zst_size | 0x00800000, fp); //Uncompressed ZST will never break 8MB
  }

  return(worked);
}

void zst_save(FILE *fp, bool Thumbnail, bool Compress)
{
  PrepareOffset();
  PrepareSaveState();
  unpackfunct();

  if (SFXEnable)
  {
    SfxRomBuffer -= SfxCROM;
    SfxLastRamAdr -= SfxRAMMem;
  }

  if (SA1Enable)
  {
    SaveSA1(); //Convert SA-1 stuff to standard, non displacement format
  }

  if (!Compress || !zst_save_compressed(fp)) //If we don't want compressed or compression failed
  {
    fwrite(zst_header_cur, 1, sizeof(zst_header_cur)-1, fp); //-1 for null

    fhandle = fp; //Set global file handle
    copy_state_data(0, write_save_state_data, csm_save_zst_new);

    if (Thumbnail)
    {
      CapturePicture();
      fwrite(PrevPicture, 1, 64*56*sizeof(unsigned short), fp);
    }
  }

  if (SFXEnable)
  {
    SfxRomBuffer += SfxCROM;
    SfxLastRamAdr += SfxRAMMem;
  }

  if (SA1Enable)
  {
    RestoreSA1(); //Convert back SA-1 stuff
  }

  ResetOffset();
  ResetState();
}


#define INSERT_POSITION_NUMBER(message, num)                          \
if (!num)                                                             \
{                                                                     \
  num = strchr(message, '-');                                         \
}                                                                     \
*num = (fnamest[statefileloc] == 't') ? '0' : fnamest[statefileloc];

void statesaver()
{
  static char txtsavemsg[] = "STATE - SAVED.";
  static char txtrrsvmsg[] = "RR STATE - SAVED.";

  static char *txtsavenum = 0;
  static char *txtrrsvnum = 0;

  //'Auto increment savestate slot' code
  if (AutoIncSaveSlot)
  {
    if (firstsaveinc) { firstsaveinc = 0; }
    else
    {
      switch (fnamest[statefileloc])
      {
        case 't': // ZST state
        case 'v': // ZMV movie
          fnamest[statefileloc] = '1';
          break;
        case '9':
          fnamest[statefileloc] = 't';
        case 's': // ZSS state
          break;
        default:
          fnamest[statefileloc]++;
      }
    }
  }

  //Get the state number
  INSERT_POSITION_NUMBER(txtsavemsg, txtsavenum);
  INSERT_POSITION_NUMBER(txtrrsvmsg, txtrrsvnum);

  if (MovieProcessing == 2)
  {
    bool mzt_save(char *, bool, bool);
    if (mzt_save((char *)fnamest+1, (cbitmode && !NoPictureSave) ? true : false, false))
    {
      Msgptr = txtrrsvmsg;
      MessageOn = MsgCount;
    }
    return;
  }

  clim();

  if ((fhandle = fopen_dir(ZSramPath, fnamest+1,"wb")))
  {
    zst_save(fhandle, (bool)(cbitmode && !NoPictureSave), false);
    fclose(fhandle);

    //Display message onscreen, 'STATE X SAVED.'
    Msgptr = txtsavemsg;
  }
  else
  {
    //Display message onscreen, 'UNABLE TO SAVE.'
    Msgptr = "UNABLE TO SAVE.";
  }

  MessageOn = MsgCount;

  stim();
}

extern unsigned int KeyLoadState, Totalbyteloaded, SfxMemTable[256], SfxCPB;
extern unsigned int SfxPBR, SfxROMBR, SfxRAMBR, SCBRrel, SfxSCBR;
extern unsigned char pressed[256+128+64], multchange, ioportval, SDD1Enable;
extern unsigned char nexthdma;

static void read_save_state_data(unsigned char **dest, void *data, size_t len)
{
  load_save_size += fread(data, 1, len, fhandle);
}

static bool zst_load_compressed(FILE *fp, size_t compressed_size)
{
  unsigned long data_size = cur_zst_size - (sizeof(zst_header_cur)-1);
  unsigned char *buffer = 0;
  bool worked = false;

  if ((buffer = (unsigned char *)malloc(data_size)))
  {
    unsigned char *compressed_buffer = 0;

    if ((compressed_buffer = (unsigned char *)malloc(compressed_size)))
    {
      fread(compressed_buffer, 1, compressed_size, fp);
      if (uncompress(buffer, &data_size, compressed_buffer, compressed_size) == Z_OK)
      {
        copy_state_data(buffer, memcpyrinc, csm_load_zst_new);
        worked = true;
      }
      free(compressed_buffer);
    }
    free(buffer);
  }
  return(worked);
}

bool zst_load(FILE *fp, size_t Compressed)
{
  size_t zst_version = 0;

  if (Compressed)
  {
    if (!zst_load_compressed(fp, Compressed))
    {
      return(false);
    }
  }
  else
  {
    char zst_header_check[sizeof(zst_header_cur)-1];

    Totalbyteloaded += fread(zst_header_check, 1, sizeof(zst_header_check), fp);

    if (!memcmp(zst_header_check, zst_header_cur, sizeof(zst_header_check)-2))
    {
      zst_version = 143; //v1.43+
    }

    if (!memcmp(zst_header_check, zst_header_old, sizeof(zst_header_check)-2))
    {
      zst_version = 60; //v0.60 - v1.42
    }

    if (!zst_version) { return(false); } //Pre v0.60 saves are no longer loaded

    load_save_size = 0;
    fhandle = fp; //Set global file handle
    copy_state_data(0, read_save_state_data, (zst_version == 143) ? csm_load_zst_new: csm_load_zst_old );
    Totalbyteloaded += load_save_size;
  }

  if (SFXEnable)
  {
    SfxCPB = SfxMemTable[(SfxPBR & 0xFF)];
    SfxCROM = SfxMemTable[(SfxROMBR & 0xFF)];
    SfxRAMMem = (unsigned int)sfxramdata + ((SfxRAMBR & 0xFF) << 16);
    SfxRomBuffer += SfxCROM;
    SfxLastRamAdr += SfxRAMMem;
    SCBRrel = (SfxSCBR << 10) + (unsigned int)sfxramdata;
  }

  if (SA1Enable)
  {
    RestoreSA1(); //Convert back SA-1 stuff
    SA1UpdateDPageC();
  }

  if (SDD1Enable)
  {
    UpdateBanksSDD1();
  }

  //Clear cache check if state loaded
  ClearCacheCheck();

  if (zst_version < 143) //Set new vars which old states did not have
  {
    prevoamptr = 0xFF;
    ioportval = 0xFF;
    spcnumread = 0;
    nexthdma = 0;
  }

  repackfunct();
  initpitch();
  ResetOffset();
  ResetState();
  procexecloop();

  return(true);
}

//Wrapper for above
bool zst_compressed_loader(FILE *fp)
{
  size_t data_size = fread3(fp);
  return((data_size & 0x00800000) ? zst_load(fp, 0) : zst_load(fp, data_size));
}

void zst_sram_load(FILE *fp)
{
  fseek(fp, sizeof(zst_header_cur)-1 + PH65816regsize + 199635, SEEK_CUR);
  if (spcon)	{ fseek(fp, PHspcsave + PHdspsave + sizeof(DSPMem), SEEK_CUR); }
  if (C4Enable)	{ fseek(fp, 8192, SEEK_CUR); }
  if (SFXEnable)	{ fseek(fp, PHnum2writesfxreg + 131072, SEEK_CUR); }
  if (SA1Enable)
  {
    fseek(fp, PHnum2writesa1reg, SEEK_CUR);
    fread(SA1RAMArea, 1, 131072, fp);	// SA-1 sram
    fseek(fp, 15, SEEK_CUR);
  }
  if (DSP1Type)	{ fseek(fp, 2874, SEEK_CUR); }
  if (SETAEnable)	{ fread(setaramdata, 1, 4096, fp); } // SETA sram
  if (SPC7110Enable)	{ fseek(fp, PHnum2writespc7110reg + 65536, SEEK_CUR); }
  fseek(fp, 227, SEEK_CUR);
  if (ramsize)	{ fread(sram, 1, ramsize, fp); } // normal sram
}

void zst_sram_load_compressed(FILE *fp)
{
  size_t compressed_size = fread3(fp);

  if (compressed_size & 0x00800000)
  {
    zst_sram_load(fp);
  }
  else
  {
    unsigned long data_size = cur_zst_size - (sizeof(zst_header_cur)-1);
    unsigned char *buffer = 0;

    if ((buffer = (unsigned char *)malloc(data_size)))
    {
      unsigned char *compressed_buffer = 0;
      if ((compressed_buffer = (unsigned char *)malloc(compressed_size)))
      {
        fread(compressed_buffer, 1, compressed_size, fp);
        if (uncompress(buffer, &data_size, compressed_buffer, compressed_size) == Z_OK)
        {
          unsigned char *data = buffer + PH65816regsize + 199635;
          if (spcon)  { data += PHspcsave + PHdspsave + sizeof(DSPMem); }
          if (C4Enable) { data += 8192; }
          if (SFXEnable)  { data += PHnum2writesfxreg + 131072; }
          if (SA1Enable)
          {
            data += PHnum2writesa1reg;
            memcpyrinc(&data, SA1RAMArea, 131072); // SA-1 sram
            data += 15;
          }
          if (DSP1Type) { data += 2874; }
          if (SETAEnable) { memcpyrinc(&data, setaramdata, 4096); } // SETA sram
          if (SPC7110Enable)  { data += PHnum2writespc7110reg + 65536; }
          data += 227;
          if (ramsize)  { memcpyrinc(&data, sram, ramsize); } // normal sram
        }
        free(compressed_buffer);
      }
      free(buffer);
    }
  }
}


extern unsigned char Voice0Disable, Voice1Disable, Voice2Disable, Voice3Disable;
extern unsigned char Voice4Disable, Voice5Disable, Voice6Disable, Voice7Disable;

void stateloader (char *statename, unsigned char keycheck, unsigned char xfercheck)
{
  extern unsigned char PauseLoad;

  static char txtloadmsg[] = "STATE - LOADED.";
  static char txtconvmsg[] = "STATE - TOO OLD.";
  static char txtnfndmsg[] = "UNABLE TO LOAD STATE -.";
  static char txtrrldmsg[] = "RR STATE - LOADED.";

  static char *txtloadnum = 0;
  static char *txtconvnum = 0;
  static char *txtnfndnum = 0;
  static char *txtrrldnum = 0;

  //Get the state number
  INSERT_POSITION_NUMBER(txtloadmsg, txtloadnum);
  INSERT_POSITION_NUMBER(txtconvmsg, txtconvnum);
  INSERT_POSITION_NUMBER(txtnfndmsg, txtnfndnum);
  INSERT_POSITION_NUMBER(txtrrldmsg, txtrrldnum);

  if (keycheck)
  {
    pressed[1] = 0;
    pressed[KeyLoadState] = 2;
    multchange = 1;
    MessageOn = MsgCount;
  }

  switch (MovieProcessing)
  {
    bool mzt_load(char *, bool);

    case 1:
      if (mzt_load(statename, true))
      {
        Msgptr = "CHAPTER LOADED.";
      }
      else
      {
        Msgptr = txtnfndmsg;
      }
      MessageOn = MsgCount;
      return;
    case 2:
      if (mzt_load(statename, false))
      {
        Msgptr = txtrrldmsg;
        MessageOn = MsgCount;

        if (PauseLoad || EMUPause)
        {
          PauseFrameMode = EMUPause = true;
        }
      }
      else
      {
        Msgptr = txtnfndmsg;
      }
      MessageOn = MsgCount;
      return;
  }

  clim();

  //Actual state loading code
  if ((fhandle = fopen_dir(ZSramPath, statename,"rb")))
  {
    if (xfercheck)      { Totalbyteloaded = 0; }

    if (zst_load(fhandle, 0))
    {
      Msgptr = txtloadmsg; // 'STATE X LOADED.'

      if (PauseLoad || EMUPause)
      {
        PauseFrameMode = EMUPause = true;
      }
    }
    else
    {
      Msgptr = txtconvmsg; // 'STATE X TOO OLD.' - I don't think this is always accurate -Nach
    }
    fclose(fhandle);
  }
  else
  {
    Msgptr = txtnfndmsg; // 'UNABLE TO LOAD STATE X.'
  }

  Voice0Disable = 1;
  Voice1Disable = 1;
  Voice2Disable = 1;
  Voice3Disable = 1;
  Voice4Disable = 1;
  Voice5Disable = 1;
  Voice6Disable = 1;
  Voice7Disable = 1;

  stim();
}

void debugloadstate()
{
  stateloader(fnamest+1, 0, 0);
}

void loadstate()
{
  stateloader(fnamest+1, 1, 0);
}

void loadstate2()
{
  stateloader(fnamest+1, 0, 1);
}

extern unsigned char CHIPBATT, sramsavedis;
extern char fnames[512];
void SaveCombFile();

// Sram saving
void SaveSramData()
{
  FILE *fp = 0;
  unsigned char special = 0;
  unsigned int *data_to_save;

  if (ramsize && !sramsavedis)
  {
    if (SFXEnable)
    {
      data_to_save=sfxramdata;
      special=1;
    }
    else if (SA1Enable)
    {
      data_to_save=(unsigned int *)SA1RAMArea;
      special=1;
    }
    else if (SETAEnable)
    {
      data_to_save=setaramdata;
      special=1;
    }
    else data_to_save=sram;

    if (!special || CHIPBATT)
    {
      clim();
      if ((fp = fopen_dir(ZSramPath,fnames+1,"wb")))
      {
        fwrite(data_to_save, 1, ramsize, fp);
        fclose(fp);
      }
      stim();
    }
  }
  SaveCombFile();
}
/*
SPC File Format - Invented by _Demo_ & zsKnight
Cleaned up by Nach

00000h-00020h - File Header : SNES-SPC700 Sound File Data v0.00 (33 bytes)
00021h-00023h - 0x1a,0x1a,0x1a (3 bytes)
00024h        - 10 (1 byte)
00025h        - PC Register value (1 Word)
00027h        - A Register Value (1 byte)
00028h        - X Register Value (1 byte)
00029h        - Y Register Value (1 byte)
0002Ah        - Status Flags Value (1 byte)
0002Bh        - Stack Register Value (1 byte)
0002Ch-0002Dh - Reserved (1 byte)
0002Eh-0004Dh - SubTitle/Song Name (32 bytes)
0004Eh-0006Dh - Title of Game (32 bytes)
0006Eh-0007Dh - Name of Dumper (32 bytes)
0007Eh-0009Dh - Comments (32 bytes)
0009Eh-000A1h - Date the SPC was Dumped (4 bytes)
000A2h-000A8h - Reserved (7 bytes)
000A9h-000ACh - Length of SPC in seconds (4 bytes)
000ADh-000AFh - Fade out length in milliseconds (3 bytes)
000B0h-000CFh - Author of Song (32 bytes)
000D0h        - Default Channel Disables (0 = enable, 1 = disable) (1 byte)
000D1h        - Emulator used to dump .spc file (1 byte)
                (0 = UNKNOWN, 1 = ZSNES, 2 = SNES9X)
                (Note : Contact the authors if you're an snes emu author with
                 an .spc capture in order to assign you a number)
000D2h-000FFh - Reserved (46 bytes)
00100h-100FFh - SPCRam (64 KB)
10100h-101FFh - DSPRam (256 bytes)
*/

extern unsigned char spcextraram[64];
extern unsigned char spcP, spcA, spcX, spcY, spcS, spcNZ;
extern unsigned int infoloc;

char spcsaved[16];
void savespcdata()
{
  setextension("spc");
  size_t fname_len = strlen(ZSaveName);
  unsigned int i = 0;

  while (i < 100)
  {
    if (i)
    {
      sprintf(ZSaveName-1+fname_len - ((i < 10) ? 0 : 1), "%d", i);
    }
    if (access_dir(ZSpcPath, ZSaveName, F_OK))
    {
      break;
    }
    i++;
  }
  if (i < 100)
  {
    FILE *fp = fopen_dir(ZSpcPath, ZSaveName, "wb");
    if (fp)
    {
      unsigned char ssdatst[256];
      time_t t = time(0);
      struct tm *lt = localtime(&t);

      //Assemble N/Z flags into P
      spcP &= 0xFD;
      if (!(spcNZ & 0xFF))
      {
        spcP |= 2;
      }
      spcP &= 0x7F;
      if (spcNZ & 0x80)
      {
        spcP |= 0x80;
      }

      strcpy((char *)ssdatst, "SNES-SPC700 Sound File Data v0.30"); //00000h - File Header : SNES-SPC700 Sound File Data v0.00
      ssdatst[0x21] = ssdatst[0x22] = ssdatst[0x23] = 0x1a; //00021h - 0x1a,0x1a,0x1a
      ssdatst[0x24] = 10;   //00024h - 10
      *((unsigned short *)(ssdatst+0x25)) = spcPCRam-(unsigned int)SPCRAM; //00025h - PC Register value (1 Word)
      ssdatst[0x27] = spcA; //00027h - A Register Value (1 byte)
      ssdatst[0x28] = spcX; //00028h - X Register Value (1 byte)
      ssdatst[0x29] = spcY; //00029h - Y Register Value (1 byte)
      ssdatst[0x2A] = spcP; //0002Ah - Status Flags Value (1 byte)
      ssdatst[0x2B] = spcS; //0002Bh - Stack Register Value (1 byte)

      ssdatst[0x2C] = 0; //0002Ch - Reserved
      ssdatst[0x2D] = 0; //0002Dh - Reserved

      PrepareSaveState();

      memset(ssdatst+0x2E, 0, 32); //0002Eh-0004Dh - SubTitle/Song Name
      memset(ssdatst+0x4E, 0, 32); //0004Eh-0006Dh - Title of Game
      memcpy(ssdatst+0x4E, ((unsigned char *)romdata)+infoloc-((infoloc == 0x40ffc0) ? 0x408000 : 0), 21);
      memset(ssdatst+0x6E, 0, 16); //0006Eh-0007Dh - Name of Dumper
      memset(ssdatst+0x7E, 0, 32); //0007Eh-0009Dh - Comments

      //0009Eh-000A1h - Date the SPC was Dumped
      ssdatst[0x9E] = lt->tm_mday;
      ssdatst[0x9F] = lt->tm_mon+1;
      ssdatst[0xA0] = (lt->tm_year+1900) & 0xFF;
      ssdatst[0xA1] = ((lt->tm_year+1900) >> 8) & 0xFF;

      memset(ssdatst+0xA2, 0, 7);  //000A2h-000A8h - Reserved
      memset(ssdatst+0xA9, 0, 4);  //000A9h-000ACh - Length of SPC in seconds
      memset(ssdatst+0xAD, 0, 3);  //000ADh-000AFh - Fade out time in milliseconds
      memset(ssdatst+0xB0, 0, 32); //000B0h-000CFh - Author of Song

      //Set Channel Disables
      ssdatst[0xD0] = 0; //000D0h - Default Channel Disables (0 = enable, 1 = disable)
      if (Voice0Disable) { ssdatst[0xD0] |= BIT(0); }
      if (Voice1Disable) { ssdatst[0xD0] |= BIT(1); }
      if (Voice2Disable) { ssdatst[0xD0] |= BIT(2); }
      if (Voice3Disable) { ssdatst[0xD0] |= BIT(3); }
      if (Voice4Disable) { ssdatst[0xD0] |= BIT(4); }
      if (Voice5Disable) { ssdatst[0xD0] |= BIT(5); }
      if (Voice6Disable) { ssdatst[0xD0] |= BIT(6); }
      if (Voice7Disable) { ssdatst[0xD0] |= BIT(7); }

      ssdatst[0xD1] = 1; //000D1h - Emulator used to dump .spc file
      memset(ssdatst+0xD2, 0, 46); //000D2h-000FFh - Reserved

      fwrite(ssdatst, 1, sizeof(ssdatst), fp);
      fwrite(SPCRAM, 1, 65536, fp); //00100h-100FFh - SPCRam
      fwrite(DSPMem, 1, 192, fp);   //10100h-101FFh - DSPRam
      fwrite(spcextraram, 1, 64, fp); //Seems DSPRam is split in two, but I don't get what's going on here
      fclose(fp);

      ResetState();

      sprintf(spcsaved, "%s FILE SAVED.", ZSaveName+fname_len-3);
    }
  }
}
