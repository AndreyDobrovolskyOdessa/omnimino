#define _GNU_SOURCE 1

#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "omnitype.h"

#include "md5hash.h"
#include "omnifunc.h"
#include "omnimem.h"

static struct Omnimino *GG;

#include "omnimino.def"

/**************************************

           LoadGame

**************************************/


static char *LoadPtr;


static int ReadInt(int *V, int Delim) {
  char *EndPtr;

  *V = (int)strtol(LoadPtr, &EndPtr, 10);
  if (LoadPtr == EndPtr)
    return 1;

  if (Delim != 0) {
    if (*EndPtr++ != Delim)
      return 1;
  } else {
    while (*EndPtr && (*EndPtr++ != '\n'));
  }

  LoadPtr = EndPtr;

  return 0; 
}


static int ReadBlockAddr(struct Coord **P, int Delim) {
  int V;

  if (ReadInt(&V, Delim) != 0)
    return 1;

  *P = Block + V;

  return 0; 
}


static int ReadPointer(struct Coord ***P, int Delim) {
  int V;

  if (ReadInt(&V, Delim) != 0)
    return 1;

  *P = Figure + V;

  return 0; 
}


static void ReadString(char *S) {
  int i;

  for (i = 0; *LoadPtr; LoadPtr++) {
    if (*LoadPtr == '\n') {
      LoadPtr++; break;
    }
    if (i < OM_STRLEN) {
      *S++ = *LoadPtr;
    }
  }

  *S = '\0';
}


static int ReadParameters(void) {
  unsigned int i;
  unsigned int *Par = (unsigned int *) (&(GG->P));

  for (i = 0; i < PARNUM; i++, Par++){
    if(ReadInt((int *)Par, 0) != 0) {
      snprintf(MsgBuf, OM_STRLEN, "[%d] Parameter load error.", i+1);
      return 1;
    }
  }

  return 0;
}

int CheckParameters(void){
  if (Aperture > MAX_FIGURE_SIZE){
    snprintf(MsgBuf, OM_STRLEN, "[1] Aperture (%d) > MAX_FIGURE_SIZE (%d)", Aperture, MAX_FIGURE_SIZE);
  } else if (Metric > 1){
    snprintf(MsgBuf, OM_STRLEN, "[2] Metric (%d) can be 0 or 1", Metric);
  } else if (WeightMax > MAX_FIGURE_SIZE){
    snprintf(MsgBuf, OM_STRLEN, "[3] WeightMax (%d) > MAX_FIGURE_SIZE (%d)", WeightMax, MAX_FIGURE_SIZE);
  } else if (WeightMin < 1){
    snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) < 1", WeightMin);
  } else if (WeightMin > WeightMax){
    snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) > [3] WeightMax (%d)", WeightMin, WeightMax);
  } else if (Gravity > 1){
    snprintf(MsgBuf, OM_STRLEN, "[5] Gravity (%d) can be 0 or 1", Gravity);
  } else if (SingleLayer > 1){
    snprintf(MsgBuf, OM_STRLEN, "[6] SingleLayer (%d) can be 0 or 1", SingleLayer);
  } else if (DiscardFullRows > 1){
    snprintf(MsgBuf, OM_STRLEN, "[7] DiscardFullRows (%d) can be 0 or 1", DiscardFullRows);
  } else if (Goal >= MAX_GOAL){
    snprintf(MsgBuf, OM_STRLEN, "[8] Goal (%d) > 2", Goal);
  } else {
    FigureSize = (Aperture == 0) ? WeightMax : Aperture;

    if (GlassWidth < FigureSize){
      snprintf(MsgBuf, OM_STRLEN, "[9] GlassWidth (%d) < FigureSize (%d)", GlassWidth, FigureSize);
    } else if (GlassWidth > MAX_GLASS_WIDTH){
      snprintf(MsgBuf, OM_STRLEN, "[9] GlassWidth (%d) > MAX_GLASS_WIDTH (%d)", GlassWidth, MAX_GLASS_WIDTH);
    } else if (GlassHeightBuf < FigureSize){
      snprintf(MsgBuf, OM_STRLEN, "[10] GlassHeight (%d) < FigureSize (%d)", GlassHeightBuf, FigureSize);
    } else if (GlassHeightBuf > MAX_GLASS_HEIGHT){
      snprintf(MsgBuf, OM_STRLEN, "[10] GlassHeight (%d) > MAX_GLASS_HEIGHT (%d)", GlassHeightBuf, MAX_GLASS_HEIGHT);
    } else if (FillLevel > GlassHeightBuf){
      snprintf(MsgBuf, OM_STRLEN, "[11] FillLevel (%d) > [10] GlassHeight (%d)", FillLevel, GlassHeightBuf);
    } else if (FillRatio >= GlassWidth){
      snprintf(MsgBuf, OM_STRLEN, "[12] FillRatio (%d) >= [9] GlassWidth (%d)", FillRatio, GlassWidth);
    } else if (FixedSequence > 1){
      snprintf(MsgBuf, OM_STRLEN, "[13] FixedSequence (%d) can be 0 or 1", FixedSequence);
    } else {
      unsigned int i, Area;

      FullRow = ((1 << (GlassWidth - 1)) << 1) - 1;
      TotalArea = GlassWidth * GlassHeightBuf;

      for (Area = 0, i = 0; i < Aperture; i++)
        Area += Metric ? Aperture : (i | 1);

      if (Aperture && (Area < WeightMin)) {
        snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) > Aperture Area (%d)", WeightMin, Area);
      } else {
        return 0;
      }
    }
  }

  return 1;
}


static unsigned int CountUnits(unsigned int R){
  unsigned int n;

  for (n = 0; R; R >>= 1)
    n += R & 1;

  return n;
}


static int ReadGlassFill(void) {
  unsigned int i;

  for (i = 0; i < FillLevel; i++) {
    if (ReadInt((int *)(FillBuf + i), ';') != 0) {
      snprintf(MsgBuf, OM_STRLEN, "[17] GlassRow[%d] load error.", i); return 1;
    }
    FillBuf[i] &= FullRow;
  }

  return 0;
}


static int CheckGlassFill(void) {
  unsigned int i, Units;

  for (i = 0; i < FillLevel; i++) {
    Units = CountUnits(FillBuf[i]);
    if (FillRatio != 0) {
      if (Units != FillRatio) {
        snprintf(MsgBuf, OM_STRLEN, "[17] Wrong GlassRow[%d] = %d.", i, FillBuf[i]); return 1;
      }
    }
    TotalArea -= Units;
  }

  return 0;
}


static int ReadFigures(void) {
  unsigned int i;
  struct Coord **F;

  for (i = 0, F = Figure; F <= LastFigure; F++, i++) {
    if(ReadBlockAddr(F, ';') != 0){
      snprintf(MsgBuf, OM_STRLEN, "[18] Figure[%d] load error.", i); return 1;
    }
  }

  return 0;
}


static int CheckFigures(void) {
  unsigned int i, FW;
  struct Coord **F;

  if (Figure[0] != Block) {
    snprintf(MsgBuf, OM_STRLEN, "[18] Figure[0] must be 0."); return 1;
  }

  for (i = 0, F = Figure; F < LastFigure; F++, i++) {
    FW = F[1] - F[0];
    if (FW < WeightMin) {
      snprintf(MsgBuf, OM_STRLEN, "[18] Weight[%d] (%d) < WeightMin (%d)", i, FW, WeightMin); return 1;
    }
    if (FW > WeightMax) {
      snprintf(MsgBuf, OM_STRLEN, "[18] Weight[%d] (%d) > WeightMax (%d)", i, FW, WeightMax); return 1;
    }
    if (FixedSequence && ((F[0] - Block) >= (int)TotalArea)) {
      snprintf(MsgBuf, OM_STRLEN, "[18] Figure[%d] is unnecessary.", i); return 1;
    }
  }

  if (((*LastFigure) - Block) < (int)TotalArea) {
    snprintf(MsgBuf, OM_STRLEN, "[18] Not enough blocks to cover the glass free area."); return 1;
  }

  return 0;
}


static int ReadBlocks(void) {
  unsigned int i;
  struct Coord *B;

  for (i = 0, B = Block; B < *LastFigure; B++, i++) {
    if ((ReadInt(&(B->x), ',') != 0) || (ReadInt(&(B->y), ';') != 0)) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Block[%d] : load error.", i); return 1;
    }
  }

  return 0;
}


static int CheckBlocks(void) {
  unsigned int i, FS;
  struct Coord **F;

  for (i = 0, F = Figure; F < LastFigure; F++, i++) {
    FS = Dimension(F, FindLeft, FindRight);
    if (FS > FigureSize) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Figure[%d] width (%d) > FigureSize (%d)", i, FS, FigureSize); return 1;
    }
    FS = Dimension(F, FindBottom, FindTop);
    if (FS > FigureSize) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Figure[%d] height (%d) > FigureSize (%d)", i, FS, FigureSize); return 1;
    }
  }

  return 0;
}


static int ReadData(void) {

  if (ReadPointer(&LastFigure, 0) != 0) {
    snprintf(MsgBuf, OM_STRLEN, "[15] LastFigure : load error.");
  } else if (ReadPointer(&NextFigure, 0) != 0) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurFigure : load error.");
  } else {
    return 0;
  }

  return 1;
}


static int CheckData(void) {
  int MaxFigure = TotalArea / WeightMin + 1;

  if ((LastFigure - Figure) > MaxFigure) {
    snprintf(MsgBuf, OM_STRLEN, "[15] LastFigure (%d) > MaxFigure (%d).", (int)(LastFigure - Figure), MaxFigure);
  } else if (LastFigure <= Figure) {
    snprintf(MsgBuf, OM_STRLEN, "[15] LastFigure must not be <= 0.");
  } else if (NextFigure > LastFigure) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurFigure (%d) > [15] LastFigure (%d).",(int)(NextFigure - Figure), (int)(LastFigure - Figure));
  } else if (NextFigure < Figure) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurtFigure must not be < 0.");
  } else
    return 0;

  return 1;
}


static int LoadData(void) {

  ReadString(ParentName);

  if ((AllocateBuffers(GG) == 0) &&
      (ReadData() == 0) &&
      (ReadGlassFill() == 0) &&
      (CheckGlassFill() == 0) &&
      (CheckData() == 0) &&
      (ReadFigures() == 0) &&
      (CheckFigures() == 0) &&
      (ReadBlocks() == 0) &&
      (CheckBlocks() == 0)) {
    ReadString(PlayerName); /* skip new line following block descriptions */
    ReadString(PlayerName);

    if (ReadInt((int *)&TimeStamp, 0) != 0) {
      snprintf(MsgBuf, OM_STRLEN, "[21] TimeStamp read error.");
    } else {
      GameType = 1;

      LastTouched = NextFigure;

      return 0;
    }
  }

  return 1;
}


#include <sys/stat.h>
#include <sys/mman.h>


static int DoLoad(char *BufAddr, size_t BufLen) {
  char BufName[OM_STRLEN + 1];

  md5hash(BufAddr, BufLen, BufName);
  strcat(BufName, ".mino");

  LoadPtr = BufAddr;

  if ((ReadParameters() != 0) || (CheckParameters() != 0))
    return 1;

  if (strcmp(BufName, GameName) != 0) {
    if (FillRatio == 0) {
      char Dummy[OM_STRLEN + 1];

      ReadString(Dummy);
      ReadString(Dummy);
      ReadString(Dummy);

      if ((ReadGlassFill() != 0) || (CheckGlassFill() != 0))
        return 1;  
    }
    GameType = 2;
    return 0;
  }

  return LoadData();
}


int LoadGame(struct Omnimino *G, char *Name) {
  unsigned int i;
  unsigned int *Par = (unsigned int *)(&(G->P));

  struct stat st;

  GG = G;

  snprintf(GameName, OM_STRLEN, "%s", basename(Name));

  for (i = 0; i < PARNUM; i++)
    Par[i] = -1;

  MsgBuf[0] = '\0';
  PlayerName[0] = '\0';
  TimeStamp = 0;
  GameType = 3;
  GameModified=0;
  LastFigure = Figure; /* mark missing game data */
  strcpy(ParentName, "none");

  if (stat(Name, &st) < 0) {
    snprintf(MsgBuf, OM_STRLEN, "Can not stat file %s.", Name);
  } else {
    int fd = open(Name, O_RDONLY);
    if (fd < 0) {
      snprintf(MsgBuf, OM_STRLEN, "Can not open for read %s.", Name);
    } else {
      char *Buf = mmap(NULL, st.st_size + 1, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
      close(fd);
      if (Buf == MAP_FAILED) {
        snprintf(MsgBuf, OM_STRLEN, "mmap failed.");
      } else {
        Buf[st.st_size] = '\0';
        int Err = DoLoad(Buf, st.st_size);
        munmap(Buf, st.st_size + 1);
        return Err;
      }
    }
  }

  return 1;
}

