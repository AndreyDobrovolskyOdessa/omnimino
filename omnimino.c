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

#define _GNU_SOURCE 1

#include <features.h>

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>

#include <ncurses.h>


/**************************************

           Constants

**************************************/

#define MAX_FIGURE_SIZE 8

#define MAX_GLASS_WIDTH 32 /*UINT_WIDTH*/
#define MAX_GLASS_HEIGHT 256

enum GoalTypes {
  FILL_GOAL,
  TOUCH_GOAL,
  FLAT_GOAL,
  MAX_GOAL
};

#include "omnifunc.h"


/**************************************

           Game

**************************************/

static struct Omnimino G;


#define Aperture       (G.P.Aperture)
#define Metric         (G.P.Metric)
#define WeightMax      (G.P.WeightMax)
#define WeightMin      (G.P.WeightMin)
#define Gravity        (G.P.Gravity)
#define FlatFun        (G.P.FlatFun)
#define FullRowClear   (G.P.FullRowClear)
#define Goal           (G.P.Goal)
#define GlassWidth     (G.P.GlassWidth)
#define GlassHeightBuf (G.P.GlassHeightBuf)
#define FillLevel      (G.P.FillLevel)
#define FillRatio      (G.P.FillRatio)
#define SlotsUnique    (G.P.SlotsUnique)

#define LastFigure   (G.D.LastFigure)
#define CurFigure    (G.D.CurFigure)
#define NextFigure   (G.D.NextFigure)
#define Untouched    (G.D.Untouched)
#define TimeStamp    (G.D.TimeStamp)
#define GameType     (G.D.GameType)
#define FigureSize   (G.D.FigureSize)
#define TotalArea    (G.D.TotalArea)
#define FullRow      (G.D.FullRow)
#define GlassHeight  (G.D.GlassHeight)
#define GlassLevel   (G.D.GlassLevel)
#define GameOver     (G.D.GameOver)
#define GoalReached  (G.D.GoalReached)
#define GameModified (G.D.GameModified)
#define GameBufSize  (G.D.GameBufSize)
#define FillBuf      (G.D.FillBuf)
#define Figure       (G.D.Figure)
#define Block        (G.D.Block)
#define GlassRow     (G.D.GlassRow)
#define StoreBufSize (G.D.StoreBufSize)

#define MsgBuf     (G.S.MsgBuf)
#define GameName   (G.S.GameName)
#define ParentName (G.S.ParentName)
#define PlayerName (G.S.PlayerName)


void InitGame(void) {
  FillBuf = NULL;
  GameBufSize = 0;
}

static void FitWidth(struct Coord *B, int *Err){
  (*Err) |= ( (((B->x)>>1) < 0) || (((B->x)>>1) >= (int)GlassWidth) );
}

static void FitHeight(struct Coord *B, int *Err){
  (*Err) |= ( (((B->y)>>1) < 0) || (((B->y)>>1) >= (int)(GlassHeight+FigureSize+1)) );
}

static void FitGlass(struct Coord *B, int *Err){
  FitWidth(B,Err);
  FitHeight(B,Err);
}

static void AndGlass(struct Coord *B, int *Err){
  (*Err) |= ( GlassRow[(B->y)>>1] & (1<<((B->x)>>1)) );
}

static void PlaceIntoGlass(struct Coord *B, int *V){
  (void) V;
  GlassRow[(B->y)>>1] |= (1<<((B->x)>>1));
}


/**************************************

           New game functions

**************************************/

static void FillGlass(void){
  unsigned int i, Places, Blocks;

  for (i = 0 ; i < FillLevel ; i++) {
    FillBuf[i] = 0;
    for (Places = GlassWidth, Blocks = FillRatio ; Places > 0 ; Places--) {
      FillBuf[i] <<= 1;
      if ((rand() % Places) < Blocks) {
        FillBuf[i] |= 1; Blocks--;
      }
    }
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
           ((!SlotsUnique) || (!FindBlock(Slot + SlotNum, Slot, SlotNum))))
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

static void NewGame(void)
{
  srand((unsigned int)time(NULL));

  FillGlass();

  for (LastFigure = Figure, Figure[0] = Block; ((*LastFigure) - Figure[0]) < (int)TotalArea; LastFigure++){
    LastFigure[1] = (*LastFigure) + NewFigure(*LastFigure);
    ForEachIn(LastFigure, ScaleUp, 0);
  }

  CurFigure = Figure + 1; Untouched = NextFigure = Figure;

  strcpy(ParentName, "none");
}


/**************************************

	Screen functions
           
**************************************/

static SCREEN *Screen = NULL;
static WINDOW *MyScr = NULL;


static void StartCurses(void) {
  Screen = newterm(NULL, stderr, stdin);
  if (Screen) {
    set_term(Screen);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
  }
}

static void DeleteMyScr(void) {
  if (MyScr) {
    delwin(MyScr);
    MyScr = NULL;
  }
}

static void StopCurses() {
  if (Screen) {
    DeleteMyScr();
    endwin();
    delscreen(Screen);
    Screen = NULL;
  }
}


#define MAX_ROW_LEN ((MAX_GLASS_WIDTH+2)*2)

static void PutRowImage(unsigned int Row, char *Image, int N) {
  for (;N--; Row >>= 1) {
    *Image++ = (Row & 1) ? '[' : ' ';
    *Image++ = (Row & 1) ? ']' : ' ';
  }
}

static void DrawGlass(int GlassRowN) {
  int RowN;
  char RowImage[MAX_ROW_LEN];

  GlassRowN -= (getmaxy(MyScr) - 1);
  for (RowN = getmaxy(MyScr) - 1; RowN >= 0; RowN--, GlassRowN++) {
    memset(RowImage, (GlassRowN >= (int)GlassHeight) ? ' ' : '#', MAX_ROW_LEN);
    if ((GlassRowN >= 0) && (GlassRowN < (int)(GlassHeight + FigureSize +1)))
      PutRowImage(GlassRow[GlassRowN], RowImage+2, GlassWidth);
    mvwaddnstr(MyScr, RowN, 0, RowImage, (GlassWidth + 2) * 2);
  }
}


static int SelectGlassRow(void) {
  int FCV = Center((CurFigure < LastFigure) ? CurFigure : (CurFigure - 1), FindBottom, FindTop) >> 1;
  unsigned int GlassRowN = FCV + (FigureSize / 2) + 1;

  if (GlassLevel > GlassRowN){
    GlassRowN=GlassLevel;
    if (GlassLevel >= (unsigned int)getmaxy(MyScr)){
      int Half = getmaxy(MyScr) / 2;
      if (GlassLevel > (unsigned int)(FCV+Half)) {
        GlassRowN=FCV+Half;
        if(Half>FCV){
          GlassRowN=getmaxy(MyScr)-1;
        }
      }
    }
  }

  return GlassRowN;
}

static void DrawBlock(struct Coord *B,int *GRN) {
  mvwchgat(MyScr, (*GRN) - ((B->y) >> 1) ,((B->x) & (~1)) + 2, 2, A_REVERSE, 0, NULL);
}


static void DrawQueue(void) {
  int x, y;
  struct Coord C;

  int SideLen = FigureSize + 2;
  int TwiSide = SideLen * 2;
  int LeftMargin = (GlassWidth + 2) * 2;
  int PlacesH = (getmaxx(MyScr) - LeftMargin) / TwiSide;
  int PlacesV = getmaxy(MyScr) / SideLen;

  struct Coord **N = CurFigure + 1;

  int OffsetX = LeftMargin + SideLen;
  int OffsetYInit = -SideLen + 1;

  for (x = 0; (N < LastFigure) && (x < PlacesH); x++, OffsetX += TwiSide) {

    int OffsetY = OffsetYInit;

    for (y = 0; (N < LastFigure) && (y < PlacesV); y++, OffsetY -= TwiSide, N++){
      CopyFigure(LastFigure, N);
      Normalize(LastFigure, &C);
      ForEachIn(LastFigure, AddX, OffsetX);
      ForEachIn(LastFigure, AddY, OffsetY); 
      ForEachIn(LastFigure, DrawBlock, 0);
    }
  }
}



static void DrawStatus(void) {
  int MsgLen = strlen(MsgBuf);
  int StatusY = getmaxy(MyScr) - 1;
  int StatusX = getmaxx(MyScr) - MsgLen;
  char *Sym = " TF-+";

  if (GameOver) {
    Sym += 3;
    if (GoalReached)
      Sym++;
  } else
    Sym += Goal;

  mvwaddnstr(MyScr, StatusY, StatusX  - 2, Sym, 1);

  mvwaddnstr(MyScr, StatusY, StatusX , MsgBuf, MsgLen);
}

static int ShowScreen(void) {
  if (Screen) {
    if (MyScr == NULL) {
      if ((MyScr = newwin(0, 0, 0, 0)) == NULL)
        return 0;
      refresh();
    }

    werase(MyScr);

    if (((unsigned int)getmaxx(MyScr) < ((GlassWidth + 2) * 2)) || ((unsigned int)getmaxy(MyScr) < (MAX_FIGURE_SIZE * 2))){
      mvwaddstr(MyScr, 0, 0, "Screen too small");
    } else {
      int GlassRowN = SelectGlassRow();

      DrawGlass(GlassRowN);
      if (CurFigure < LastFigure)
        ForEachIn(CurFigure, DrawBlock, GlassRowN);
      DrawQueue();
      DrawStatus();
    }

    wrefresh(MyScr);
  }

  return 1;
}

/**************************************

        Various game functions

**************************************/


static unsigned int Unfilled(void){
  unsigned int i, s, r;

  for(i = 0, s = 0; i < GlassHeight; i++) {
    for(r = (~GlassRow[i]) & FullRow; r; r >>= 1) {
      if (r & 1)
        s++;
    }
  }

  return s;
}


static void DetectGlassLevel(void) {
  GlassLevel = GlassHeight + FigureSize + 1;
  while ((GlassLevel > 0) && (GlassRow[GlassLevel - 1] == 0))
    GlassLevel--;
}


static void Drop(struct Coord **FigN) {
  CopyFigure(LastFigure, FigN);
  if(Gravity){
    if(!FlatFun){
      ForEachIn(LastFigure,AddY,-(ForEachIn(LastFigure,FindBottom,INT_MAX)&(~1)));
      while((ForEachIn(LastFigure,FitGlass,0)==0)&&(ForEachIn(LastFigure,AndGlass,0)!=0))
        ForEachIn(LastFigure,AddY,2);
    }else{
      while((ForEachIn(LastFigure,FitGlass,0)==0)&&(ForEachIn(LastFigure,AndGlass,0)==0))
        ForEachIn(LastFigure,AddY,-2);
      ForEachIn(LastFigure,AddY,2);
    }
  }
  ForEachIn(LastFigure,PlaceIntoGlass,0);
  DetectGlassLevel();
}


static void ClearFullRows(void) {
  unsigned int r, w, FullRowNum, GlassSize;

  GlassSize = GlassHeight + FigureSize + 1;

  for(r = w = 0 ; r < GlassHeight ; r++){
    if (GlassRow[r] != FullRow)
      GlassRow[w++] = GlassRow[r];
  }

  FullRowNum = r - w;

  for(; r < GlassSize ; r++, w++)
    GlassRow[w] = GlassRow[r];

  for(; w < GlassSize ; w++)
    GlassRow[w] = 0;

  GlassHeight -= FullRowNum;
  GlassLevel -= FullRowNum;
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
      if(Unfilled() == 0)
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

  GlassLevel = FillLevel;
  CurFigure = Figure;
  GameOver = 0;
  GoalReached = 0;
}


void GetGlassState(void) {

  if (NextFigure < CurFigure)
    RewindGlassState();

  while (CurFigure < NextFigure) {
    if ((ForEachIn(CurFigure, FitGlass, 0) != 0) || (ForEachIn(CurFigure, AndGlass, 0) != 0)) {
      Untouched = NextFigure = CurFigure;
      break;
    }
    Drop(CurFigure++);
    if (FullRowClear)
      ClearFullRows();
    CheckGameState();
    if (GameOver)
      NextFigure = CurFigure;
  }

  if(CurFigure >= Untouched){
    if(CurFigure < LastFigure)
      Deploy(CurFigure);
    Untouched = CurFigure + 1;
  }

  snprintf(MsgBuf, OM_STRLEN, "%d", (Goal == FILL_GOAL) ? Unfilled() : (unsigned int)(CurFigure - Figure));
}

/**************************************

           PlayGame

**************************************/

static int KeepPlaying;

static void ExitWithSave(void){
  KeepPlaying = 0;
}

static void ExitWithoutSave(void){
  GameModified=0;
  KeepPlaying = 0;
}

static void Attempt(bfunc F, int V) {
  if (!GameOver) {
    struct Coord C;

    CopyFigure(LastFigure, CurFigure);
    Normalize(LastFigure, &C);
    ForEachIn(LastFigure, F, V);
    ForEachIn(LastFigure, AddX, C.x);
    ForEachIn(LastFigure, AddY, C.y);
    if((ForEachIn(LastFigure,FitGlass,0)==0)&&
       ((!FlatFun) || (ForEachIn(LastFigure,AndGlass,0)==0))){
      CopyFigure(CurFigure,LastFigure);
      Untouched=CurFigure+1;
      GameModified=1;
    }
  }
}

static void MoveCurLeft(void){
  Attempt(AddX, -2);
}

static void MoveCurRight(void){
  Attempt(AddX, 2);
}

static void MoveCurDown(void){
  if (!Gravity)
    Attempt(AddY, -2);
}

static void MoveCurUp(void){
  if (!Gravity)
    Attempt(AddY, 2);
}

static void RotateCurCW(void){
  Attempt(RotCW, 0);
}

static void RotateCurCCW(void){
  Attempt(RotCCW, 0);
}

static void MirrorCurVert(void){
  Attempt(NegX, 0);
}

static void DropCur(void){
  if((!GameOver)&&
     (ForEachIn(CurFigure,FitGlass,0)==0)&&
     (ForEachIn(CurFigure,AndGlass,0)==0)){
    NextFigure=CurFigure+1;
  }
}

static void UndoFigure(void) {
  if (CurFigure > Figure)
    NextFigure = CurFigure - 1;
}

static void RedoFigure(void) {
  if ((CurFigure + 1) < Untouched)
    NextFigure = CurFigure + 1;
}

static void Rewind(void) {
  NextFigure = Figure;
}

static void LastPlayed(void) {
  NextFigure = Untouched - 1;
}

static struct KBinding {
  int Key;
  void (*Action)(void);
} KBindList[] = {
  {'q', ExitWithoutSave},
  {'x', ExitWithSave},
  {'h', MoveCurLeft},
  {'l', MoveCurRight},
  {'k', MoveCurUp},
  {'j', MoveCurDown},
  {KEY_LEFT, MoveCurLeft},
  {KEY_RIGHT, MoveCurRight},
  {KEY_UP, MoveCurUp},
  {KEY_DOWN, MoveCurDown},
  {'a', RotateCurCCW},
  {'f', RotateCurCW},
  {'s', MirrorCurVert},
  {'d', MirrorCurVert},
  {' ', DropCur},
  {'^', Rewind},
  {'$', LastPlayed},

  {'u', UndoFigure},
  {'r', RedoFigure},

  {KEY_RESIZE, DeleteMyScr},

  {0, NULL}
};


static void ExecuteCmd(void){
  int Key;
  struct KBinding *P;
  void (*Func)(void);

  do {
    Key=getch();
    for(P=KBindList;((Func=(P->Action))!=NULL)&&(Key!=(P->Key));P++);
  } while(Func==NULL);

  (*Func)();
}


int PlayGame(void){
  if (GameType == 2)
    NewGame();

  StartCurses();

  KeepPlaying = 1;

  do {
    GetGlassState();
    if (ShowScreen() == 0)
      break;
    ExecuteCmd();
  } while (KeepPlaying);

  StopCurses();

  return GameModified;
}


#include "md5hash.h"

/**************************************

           SaveGame

**************************************/


static char *StorePtr;

static int StoreFree;

static void Adjust(int Done) {
  if (Done >= StoreFree)
    Done = StoreFree;

  /* Done--; */

  StoreFree -= Done;
  StorePtr += Done;
}

static void StoreInt(int V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%d%c", V, (char)Delim));
}

static void StoreUnsigned(unsigned int V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", V, (char)Delim));
}

static void StoreBlockAddr(struct Coord *V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", (int)(V - Block), (char)Delim));
}

static void StorePointer(struct Coord **V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", (int)(V - Figure), (char)Delim));
}

static void StoreString(char *S) {
  Adjust(snprintf(StorePtr, StoreFree, "%s\n", S));
}


void SaveGame(void){
  unsigned int *UPtr = (unsigned int *) (&G.P);
  unsigned int i; 
  int Used;
  char *UserName;

  int fd;


  StorePtr = (char *) GlassRow;
  StoreFree = StoreBufSize;


  for (i = 0; i < PARNUM; i++, UPtr++)
    StoreUnsigned(*UPtr, '\n');

  if ((GameType == 1) && (strcmp(ParentName, "none") == 0))
    strcpy(ParentName, GameName);

  GameType = 1;

  StoreString(ParentName);
  StorePointer(LastFigure, '\n');
  StorePointer(CurFigure, '\n');

  for (i = 0; i < FillLevel; i++)
    StoreUnsigned(FillBuf[i], ';');
  StoreString("");

  struct Coord **F;

  for(F = Figure; F <= LastFigure; F++)
    StoreBlockAddr(*F, ';');
  StoreString("");

  struct Coord *B;

  for(B = Block; B < (*LastFigure); B++) {
    StoreInt(B->x, ',');
    StoreInt(B->y, ';');
  }
  StoreString("");

  UserName = getenv("USER");
  if (!UserName)
    UserName = "anonymous";
  snprintf(PlayerName,OM_STRLEN,"%s",UserName);
  StoreString(PlayerName);

  TimeStamp = (unsigned int)time(NULL);
  StoreUnsigned(TimeStamp, '\n');

  Used = StoreBufSize - StoreFree;
  md5hash(GlassRow, Used, GameName);
  strcat(GameName, ".mino");

  fd = open(GameName, O_CREAT | O_WRONLY, 0666);
  if (fd < 0) {
    snprintf(MsgBuf, OM_STRLEN, "Can not open for write %s.", GameName);
    GameType = 3;
  } else {
    if (write(fd, GlassRow, Used) != Used) {
      snprintf(MsgBuf, OM_STRLEN, "Error writing %s.", GameName);
      GameType = 3;
    }
    close(fd);
  }
}


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
  unsigned int *Par = (unsigned int *) (&G.P);

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
  } else if ((CurFigure > LastFigure) && (GameType != 1)) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurFigure (%d) > [15] LastFigure (%d).",(int)(CurFigure - Figure), (int)(LastFigure - Figure));
  } else if (CurFigure < Figure) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurtFigure must not be < 0.");
  } else
    return 0;

  return 1;
}


static int LoadData(void) {

  ReadString(ParentName);

  if (ReadPointer(&LastFigure, 0) != 0) {
    snprintf(MsgBuf, OM_STRLEN, "[15] LastFigure : load error.");
  } else if (ReadPointer(&CurFigure, 0) != 0) {
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

      NextFigure=CurFigure;
      CurFigure=Untouched=NextFigure+1;
      GameType = 1;

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


int LoadGame(char *Name) {
  unsigned int i;
  unsigned int *Par = (unsigned int *)(&G.P);

  struct stat st;


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


int CheckGame(void) {
  return (CheckParameters() ||
          CheckData() ||
          CheckGlassFill() ||
          CheckFigures() ||
          CheckBlocks()); 
}


static void ExportGame(void){
  unsigned int i, Order[] = {2,7,0,1,4,5,6,8,9,10,11,12,3};
  unsigned int *Par = (unsigned int *)(&G.P);


  if ((GameType != 1) || (strcmp(ParentName, "none") == 0))
    strcpy(ParentName, GameName);


  fprintf(stdout,"{\n");

  fprintf(stdout,"  Data = {");

  fprintf(stdout,"\"%s\", ", GameName);
  fprintf(stdout,"\"%s\", ", PlayerName);
  fprintf(stdout,"%u, ", TimeStamp);
  fprintf(stdout, GameType == 1 ? "%s, " : "\"%s\", ", MsgBuf);

  fprintf(stdout,"},\n");

  fprintf(stdout,"  Parameters = {");

  fprintf(stdout,"%u, ", GameType);
  for (i = 0; i < PARNUM ; i++){
    fprintf(stdout,"%d, ", Par[Order[i]]);
  }
  fprintf(stdout,"\"%s\", ", ParentName);

  fprintf(stdout,"},\n");

  fprintf(stdout,"},\n");
}


void Report() {
  if (GameType == 1) {
    if ((Goal != FILL_GOAL) && (!GoalReached))
      snprintf(MsgBuf, OM_STRLEN,  "%d", (int)(LastFigure - Figure));
  } else if (GameType == 2) {
    MsgBuf[0] = '\0';
  }

  if (isatty(fileno(stdout))) {
    fprintf(stdout, "%s\n", MsgBuf);
  } else {
    ExportGame();
  }
}


