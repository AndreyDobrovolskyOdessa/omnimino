#include "omnitype.h"

#include <string.h>
#include <limits.h>

#include "omnifunc.h"
#include "omnigame.h"

/**************************************

           Game

**************************************/

static struct Omnimino *GG;

#include "omnimino.def"


static void FitWidth(struct Coord *B, int *Err){
  (*Err) = (*Err) || (((B->x)>>1) < 0) || (((B->x)>>1) >= (int)GlassWidth);
}

static void FitHeight(struct Coord *B, int *Err){
  (*Err) = (*Err) || (((B->y)>>1) < 0) || (((B->y)>>1) >= (int)FieldSize);
}

void FitGlass(struct Coord *B, int *Err){
  FitWidth(B,Err);
  FitHeight(B,Err);
}

void AndGlass(struct Coord *B, int *Err){
  (*Err) = (*Err) || ( GlassRow[(B->y)>>1] & (1<<((B->x)>>1)) );
}

static void PlaceIntoGlass(struct Coord *B, int *V){
  (void) V;
  GlassRow[(B->y)>>1] |= (1<<((B->x)>>1));
}

static void CountInner(struct Coord *B, int *Cnt){
  if (((B->y)>>1) < (int)GlassHeight)
    (*Cnt)++;
}


/**************************************

        Various game functions

**************************************/

static void ClearFullRows(unsigned int From, unsigned int To) {
  unsigned int r, w, FullRowNum;

  unsigned int Upper = GlassLevel;

  if (Goal == FLAT_GOAL)
    Upper--;

  if (To < Upper)
    Upper = To;

  for(r = w = From ; r < Upper ; r++){
    if (GlassRow[r] != FullRow)
      GlassRow[w++] = GlassRow[r];
  }

  FullRowNum = r - w;

  if ((int)FullRowNum > 0) {
/*
    for(; r < FieldSize ; r++, w++)
      GlassRow[w] = GlassRow[r];
*/

    memmove(GlassRow + w, GlassRow + r, (FieldSize - r) * sizeof(int));

    GlassHeight -= FullRowNum;
    FieldSize -= FullRowNum;
    GlassLevel -= FullRowNum;
  }
}


static void Drop(struct Coord **FigN) {
  unsigned int Top, Bottom, y;

  CopyFigure(FigureBuf, FigN);
  y = ForEachIn(FigureBuf, FindBottom, INT_MAX) & (~1);

  if(Gravity){
    Bottom = y >> 1;
    if (SingleLayer) {
      for (y = Bottom; y > 0; y--) {
        ForEachIn(FigureBuf, AddY, -2);
        if (Overlaps(FigureBuf)) {
          ForEachIn(FigureBuf, AddY, 2);
          break;
        }
      }
    } else {
      ForEachIn(FigureBuf, AddY, -y);
      for (y = 0; (y < Bottom) && Overlaps(FigureBuf); y++) {
        ForEachIn(FigureBuf,AddY,2);
      }
    }
  } else {
    y >>= 1;
  }

  ForEachIn(FigureBuf,PlaceIntoGlass,0);
  EmptyCells -= ForEachIn(FigureBuf, CountInner, 0);
  Top = (ForEachIn(FigureBuf, FindTop, INT_MIN) >> 1) + 1;
  if (Top > GlassLevel)
    GlassLevel = Top;
  if (DiscardFullRows)
    ClearFullRows(y, Top);
}


static void Deploy(struct Coord **F) {
  struct Coord C;

  Normalize(F, &C);
  ForEachIn(F, AddX, GlassWidth); /* impliciltly divided by 2 */
  ForEachIn(F, AddY, ((GlassLevel + 1) << 1) + FigureSize);
  GameModified = 1;
}


static void CheckGameState(void) {
  switch(Goal){
    case TOUCH_GOAL:
      if ((ForEachIn(FigureBuf, FindBottom, INT_MAX) >> 1) == 0)
        GoalReached = 1;
      break;
    case FLAT_GOAL:
      if ((GlassLevel == 0) || (GlassRow[GlassLevel - 1] == FullRow))
        GoalReached = 1;
      break;
    default:
      if(EmptyCells == 0)
        GoalReached = 1;
  }

  if(GoalReached || (GlassLevel > GlassHeight))
    GameOver = 1;
}


static void RewindGlassState(void) {
  unsigned int i;

  for (i=0; i < FillLevel; i++)
    GlassRow[i] = FillBuf[i];

  GlassHeight=GlassHeightBuf;
  FieldSize = GlassHeight + FigureSize + 1;

  for (; i < FieldSize; i++)
    GlassRow[i] = 0;

  EmptyCells = TotalArea;
  GlassLevel = FillLevel;
  CurFigure = Figure;
  FigureBuf = LastFigure + 1;
  *FigureBuf = *LastFigure;
  GameOver = 0;
  GoalReached = 0;
}


void GetGlassState(struct Omnimino *G) {
  GG = G;

  if (CurFigure > NextFigure)
    RewindGlassState();

  while (CurFigure < NextFigure) {
    if (!Placeable(CurFigure)) {
      NextFigure = CurFigure;
      LastTouched = CurFigure - 1;
      break;
    }
    Drop(CurFigure++);
    CheckGameState();
    if (GameOver)
      NextFigure = CurFigure;
  }

  if(CurFigure > LastTouched){
    Deploy(CurFigure);
    LastTouched = CurFigure;
  }
}


