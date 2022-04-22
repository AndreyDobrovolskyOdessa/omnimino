/******************************************************************************
	Omnimino is console version of polymino puzzle.
	Figure sizes, shapes, field size and gameplay options can be varied.

	Copyright (C) 2019-2022 Andrey Dobrovolsky

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
******************************************************************************/

#include <stdio.h>
#include <string.h>
#include <limits.h>

#include "omnifunc.h"


/**************************************

           Game

**************************************/

static struct Omnimino *GG;

#include "omnimino.def"


void InitGame(struct Omnimino *G) {
  memset(G, 0, sizeof(struct Omnimino));
}

static void FitWidth(struct Coord *B, int *Err){
  (*Err) = (*Err) || ( (((B->x)>>1) < 0) || (((B->x)>>1) >= (int)GlassWidth) );
}

static void FitHeight(struct Coord *B, int *Err){
  (*Err) = (*Err) || ( (((B->y)>>1) < 0) || (((B->y)>>1) >= (int)(GlassHeight+FigureSize+1)) );
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

  unsigned int GlassSize = GlassHeight + FigureSize + 1;
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
    for(; r < GlassSize ; r++, w++)
      GlassRow[w] = GlassRow[r];
*/
    memmove(GlassRow + w, GlassRow + r, (GlassSize -r) * sizeof(int));
/*
    for(; w < GlassSize ; w++)
      GlassRow[w] = 0;
*/
    memset(GlassRow + (GlassSize - FullRowNum), 0, FullRowNum * sizeof(int));

    GlassHeight -= FullRowNum;
    GlassLevel -= FullRowNum;
  }
}


static void Drop(struct Coord **FigN) {
  unsigned int Top, Bottom, y;

  CopyFigure(LastFigure, FigN);
  y = ForEachIn(LastFigure, FindBottom, INT_MAX) & (~1);

  if(Gravity){
    Bottom = y >> 1;
    if (FlatFun) {
      for (y = Bottom; y > 0; y--) {
        ForEachIn(LastFigure, AddY, -2);
        if (ForEachIn(LastFigure,AndGlass,0) != 0) {
          ForEachIn(LastFigure, AddY, 2);
          break;
        }
      }
    } else {
      ForEachIn(LastFigure,AddY,-y);
      for (y = 0; (y < Bottom) && (ForEachIn(LastFigure, AndGlass, 0) != 0); y++) {
        ForEachIn(LastFigure,AddY,2);
      }
    }
  } else {
    y >>= 1;
  }

  ForEachIn(LastFigure,PlaceIntoGlass,0);
  EmptyCells -= ForEachIn(LastFigure, CountInner, 0);
  Top = (ForEachIn(LastFigure, FindTop, INT_MIN) >> 1) + 1;
  if (Top > GlassLevel)
    GlassLevel = Top;
  if (FullRowClear)
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
      if ((ForEachIn(LastFigure, FindBottom, INT_MAX) >> 1) == 0)
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
  unsigned int i, GlassSize;

  for (i=0; i < FillLevel; i++)
    GlassRow[i] = FillBuf[i];

  GlassHeight=GlassHeightBuf;
  GlassSize = GlassHeight + FigureSize + 1;

  for (; i < GlassSize; i++)
    GlassRow[i] = 0;

  EmptyCells = TotalArea;
  GlassLevel = FillRatio ? FillLevel : 0;
  CurFigure = Figure;
  GameOver = 0;
  GoalReached = 0;
}


void GetGlassState(struct Omnimino *G) {
  GG = G;

  if (CurFigure > NextFigure)
    RewindGlassState();

  while (CurFigure < NextFigure) {
    if ((ForEachIn(CurFigure, FitGlass, 0) != 0) || (ForEachIn(CurFigure, AndGlass, 0) != 0)) {
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
    if(CurFigure < LastFigure)
      Deploy(CurFigure);
    LastTouched = CurFigure;
  }

  snprintf(MsgBuf, OM_STRLEN, "%d", (Goal == FILL_GOAL) ? EmptyCells : (unsigned int)(CurFigure - Figure));
}


