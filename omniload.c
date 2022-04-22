#define _GNU_SOURCE 1

#include <features.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "md5hash.h"

#include "omnifunc.h"

static struct Omnimino *GG;

#include "omnimino.def"

/**************************************

           LoadGame

**************************************/


static int CheckParameters(void){
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
  } else if (FlatFun > 1){
    snprintf(MsgBuf, OM_STRLEN, "[6] FlatFun (%d) can be 0 or 1", FlatFun);
  } else if (FullRowClear > 1){
    snprintf(MsgBuf, OM_STRLEN, "[7] FullRowClear (%d) can be 0 or 1", FullRowClear);
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
    } else if (SlotsUnique > 1){
      snprintf(MsgBuf, OM_STRLEN, "[13] SlotsUnique (%d) can be 0 or 1", SlotsUnique);
    } else {
      unsigned int i,Area = FigureSize;

      FullRow=((1<<(GlassWidth-1))<<1)-1;
      TotalArea = (GlassWidth * GlassHeightBuf) - (FillLevel * FillRatio);

      if (Aperture != 0){
        Area=Aperture * Aperture;
        if(Metric == 0){
          if ((Aperture % 2) == 0)
            Area -= Aperture;
          for (i = 1; i < ((Aperture - 1) / 2); i++)
            Area -= 4*i;
        }
      }

      if (Area < WeightMin){
        snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) > Aperture Area (%d)", WeightMin, Area);
      } else {
        return 0;
      }
    }
  }

  return 1;
}


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


static int LoadParameters(void) {
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

static int WrongUnits(unsigned int R){
  unsigned int n;

  if ((R & (~FullRow)) != 0)
    return 1;

  for (n = 0; R; R >>= 1)
    n += R & 1;

  return n != FillRatio;
}

static int ReadGlassFill(void) {
  unsigned int i;

  for (i = 0; i < FillLevel; i++) {
    if (ReadInt((int *)(FillBuf + i), ';') != 0) {
      snprintf(MsgBuf, OM_STRLEN, "[17] GlassRow[%d] load error.", i); return 1;
    }
  }

  return 0;
}


static int CheckGlassFill(void) {
  unsigned int i;

  for (i = 0; i < FillLevel; i++) {
    if (WrongUnits(FillBuf[i])) {
      snprintf(MsgBuf, OM_STRLEN, "[17] Wrong GlassRow[%d] = %d.", i, FillBuf[i]); return 1;
    }
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
    if ((F[0] - Block) >= (int)TotalArea) {
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

  if (ReadPointer(&LastFigure, 0) != 0) {
    snprintf(MsgBuf, OM_STRLEN, "[15] LastFigure : load error.");
  } else if (ReadPointer(&NextFigure, 0) != 0) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurFigure : load error.");
  } else if ((CheckData() == 0) &&
             (ReadGlassFill() == 0) &&
             (CheckGlassFill() == 0) &&
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


static int AllocateBuffers() {

  /* FillBuf, BlockN, Block, GlassRow = StoreBuf */

  unsigned int MaxFigure = TotalArea / WeightMin + 3;
  unsigned int MaxBlock = TotalArea + 2 * MAX_FIGURE_SIZE;

  size_t FillBufSize = FillLevel * sizeof(int);
  size_t FigureBufSize = MaxFigure * sizeof(struct Coord *); 
  size_t BlockBufSize  = MaxBlock * sizeof(struct Coord);

/*
  Sizes of the parameters and data text representations

  Parameters: 8*(1+1) + 4*(10+1) + (1+1) = 62
  ParentName: OM_STRLEN+1 = 81
  FigureNum:  10+1 = 11
  CurFigure:  10+1 = 11
  FillBuf:    FillLevel * (10+1) = FillLevel * 11
  BlockN:     (FigureNum+2) * (10+1) = MaxFigure * 11
  Block:      MaxBlock * (2+1,4+1) = MaxBlock * 8
  PlayerName: OM_STRLEN+1 = 81
  TimeStamp:  10+1 = 11
*/

  StoreBufSize = 62 + 81 + 11 + 11 + FillLevel * 11 +
                 MaxFigure * 11 + MaxBlock * 11 + 81 + 11;

  size_t NewGameBufSize = FillBufSize + FigureBufSize + BlockBufSize + StoreBufSize;

  if (FillBuf == NULL) {
    GameBufSize = NewGameBufSize;
    FillBuf = malloc(GameBufSize);
    if (FillBuf == NULL) {
      snprintf(MsgBuf, OM_STRLEN, "Failed to allocate %ld byte buffer.", (long)GameBufSize);
      return 1;
    }
  } else {
    if (NewGameBufSize > GameBufSize) {
      GameBufSize = NewGameBufSize;
      FillBuf = realloc(FillBuf, GameBufSize);
      if (FillBuf == NULL) {
        snprintf(MsgBuf, OM_STRLEN, "Failed to reallocate %ld byte buffer.", (long)GameBufSize);
        return 1;
      }
    }
  }

  Figure = (struct Coord **) (FillBuf + FillLevel);
  Block = (struct Coord *) (Figure + MaxFigure);
  GlassRow = (unsigned int *) (Block + MaxBlock);

  return 0;
}



#include <sys/stat.h>
#include <sys/mman.h>


static int DoLoad(char *BufAddr, size_t BufLen) {
  char BufName[OM_STRLEN + 1];

  md5hash(BufAddr, BufLen, BufName);
  strcat(BufName, ".mino");

  LoadPtr = BufAddr;

  if ((LoadParameters() != 0) || (CheckParameters() != 0) || (AllocateBuffers() != 0))
    return 1;

  LastFigure = Figure; /* mark missing game data */

  if (strcmp(BufName, GameName) != 0) {
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


int CheckGame(struct Omnimino *G) {
  GG = G;

  return (CheckParameters() ||
          CheckData() ||
          CheckGlassFill() ||
          CheckFigures() ||
          CheckBlocks()); 
}


