#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "omnitype.h"

#include "omnifunc.h"

static struct Omnimino *GG;

#include "omnimino.def"


void InitGame(struct Omnimino *G) {
  memset(G, 0, sizeof(struct Omnimino)); /*   GameBufSize = 0;  */
}


int CheckParameters(struct Omnimino *G){
  GG = G;

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


static int ReallyAllocateBuffers(void) {

  /* Figure, Block, GlassRow = StoreBuf */

  unsigned int MaxFigure = TotalArea / WeightMin + 4;
  unsigned int MaxBlock = TotalArea + 2 * MAX_FIGURE_SIZE;

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

  size_t NewGameBufSize = FigureBufSize + BlockBufSize + StoreBufSize;

  if (GameBufSize == 0) {
    Figure = malloc(NewGameBufSize);
    if (Figure == NULL) {
      snprintf(MsgBuf, OM_STRLEN, "Failed to allocate %ld byte buffer.", (long)NewGameBufSize);
      return 1;
    }
    GameBufSize = NewGameBufSize;
  } else {
    if (NewGameBufSize > GameBufSize) {
      void *NewBuf = realloc(Figure, NewGameBufSize);
      if (NewBuf == NULL) {
        snprintf(MsgBuf, OM_STRLEN, "Failed to reallocate %ld byte buffer.", (long)NewGameBufSize);
        return 1;
      }
      Figure = NewBuf;
      GameBufSize = NewGameBufSize;
    }
  }

  Block = (struct Coord *) (Figure + MaxFigure);
  GlassRow = (unsigned int *) (Block + MaxBlock);

  return 0;
}


int AllocateBuffers(struct Omnimino *G) {
  GG = G;

  return ReallyAllocateBuffers();
}


/**************************************

           New game functions

**************************************/

static void FillGlass(void){
  unsigned int i, Places, Blocks;

  if (FillRatio != 0) {
    for (i = 0 ; i < FillLevel ; i++) {
      FillBuf[i] = 0;
      for (Places = GlassWidth, Blocks = FillRatio ; Places > 0 ; Places--) {
        FillBuf[i] <<= 1;
        if ((rand() % Places) < Blocks) {
          FillBuf[i] |= 1; Blocks--;
        }
      }
    }
    TotalArea -= FillRatio * FillLevel;
  }
}


static int SelectSlots(struct Coord *Slot, struct Coord *FBlock, int Weight) {
  int SlotNum = 0, SeedCnt, x, y;
  int UseNeighbours = ((Aperture == 0) && Weight);
  int ApertureSize = ((Aperture == 0) ? (Weight ? 3 : 1) : Aperture);
  int HalfSize = ApertureSize / 2;
  int SeedMax = (UseNeighbours ? Weight : 1);
  int MaxX = ApertureSize - HalfSize;

  for(SeedCnt = 0; SeedCnt < SeedMax; SeedCnt++){
    for(x = -HalfSize; x < MaxX; x++){
      int Delta = (Metric ? 0 : abs(x));
      int MaxY = MaxX - Delta;
      for(y = -HalfSize + Delta; y < MaxY; y++){
        Slot[SlotNum].x = x + (UseNeighbours ? FBlock[SeedCnt].x : 0);
        Slot[SlotNum].y = y + (UseNeighbours ? FBlock[SeedCnt].y : 0);
        if (((Weight >= (int)WeightMin) || (!FindBlock(Slot + SlotNum, FBlock, Weight))) &&
           (!FindBlock(Slot + SlotNum, Slot, SlotNum)))
          SlotNum++;
      }
    }
  }

  return SlotNum;
}

#define MAX_SLOTS (MAX_FIGURE_SIZE*MAX_FIGURE_SIZE)
#define MAX_SLOTS_COMPACT ((MAX_FIGURE_SIZE-1)*9)

#if MAX_SLOTS_COMPACT>MAX_SLOTS
#define MAX_SLOTS MAX_SLOTS_COMPACT
#endif

static int NewFigure(struct Coord *F) {
  unsigned int i;
  struct Coord Slot[MAX_SLOTS], *B;
  
  for (i = 0, B = F; i < WeightMax; i++){
    memcpy(B, Slot + (rand() % SelectSlots(Slot, F, B-F)),sizeof(struct Coord));
    if (!FindBlock(B, F, B-F))
      B++;
  }
  return B-F;
}


/**************************************

             NewGame

**************************************/

int NewGame(struct Omnimino *G){

  GG = G;

  if (ReallyAllocateBuffers() != 0)
    return 1;

  srand((unsigned int)time(NULL));

  FillGlass();

  for (LastFigure = Figure, Figure[0] = Block; ((*LastFigure) - Figure[0]) < (int)TotalArea; LastFigure++){
    LastFigure[1] = (*LastFigure) + NewFigure(*LastFigure);
    ForEachIn(LastFigure, ScaleUp, 0);
  }

  NextFigure = Figure;
  LastTouched = Figure - 1;

  strcpy(ParentName, "none");

  return 0;
}


