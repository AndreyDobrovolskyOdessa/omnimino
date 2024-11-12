#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "omnifunc.h"

#include "omnimino.def"

int AllocateBuffers(struct Omnimino *GG) {

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



