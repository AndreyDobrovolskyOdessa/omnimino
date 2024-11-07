#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "omnifunc.h"
#include "omnimem.h"

static struct Omnimino *GG;

#include "omnimino.def"


void InitGame(struct Omnimino *G) {
  memset(G, 0, sizeof(struct Omnimino)); /*   GameBufSize = 0;  */
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

  if (AllocateBuffers(G) != 0)
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


