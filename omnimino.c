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

struct Coord {
  int x, y;
};

struct Parameters {
  unsigned int Aperture;
  unsigned int Metric; /* 0 - abs(x1-x0)+abs(y1-y0), 1 - max(abs(x1-x0),abs(y1-y0)) */
  unsigned int WeightMax;
  unsigned int WeightMin;
  unsigned int Gravity;
  unsigned int FlatFun; /* single-layer, moving figure can not overlap placed blocks */
  unsigned int FullRowClear;
  unsigned int Goal;
  unsigned int GlassWidth;
  unsigned int GlassHeightBuf;
  unsigned int FillLevel;
  unsigned int FillRatio; /* < GlassWidth */
  unsigned int SlotsUnique;
};

typedef void (*bfunc) (struct Coord *, int *);

/**************************************

           Constants

**************************************/
#define MAX_FIGURE_SIZE 8

#define MAX_GLASS_WIDTH 32 /*UINT_WIDTH*/
#define MAX_GLASS_HEIGHT 256

#define GLASS_SIZE (MAX_GLASS_HEIGHT+MAX_FIGURE_SIZE+1)
#define MAX_BLOCK_NUM ((MAX_GLASS_WIDTH*MAX_GLASS_HEIGHT)+(2*MAX_FIGURE_SIZE))

#define OM_STRLEN 80

enum GoalTypes {
  FILL_GOAL,
  TOUCH_GOAL,
  FLAT_GOAL,
  MAX_GOAL
};


/**************************************

           Game parameters

**************************************/

struct Parameters P;

/**************************************

           Game data

**************************************/

char ParentName[OM_STRLEN + 1];

unsigned int FigureNum;
unsigned int CurFigure;


unsigned int *FillBuf;
unsigned int * BlockN;
struct Coord *Block;

char PlayerName[OM_STRLEN + 1];
unsigned int TimeStamp;

/**************************************

           Game variables

**************************************/

unsigned int GameType;

char MsgBuf[OM_STRLEN + 1] = "";
char *Msg = MsgBuf;

char GameName[OM_STRLEN + 1];

unsigned int FigureSize;
unsigned int TotalArea;
unsigned int FullRow; /* template, defined once per game, depends on GlassWidth */


unsigned int *GlassRow;       /* used by SaveGame too */
unsigned int GlassRowBufSize;

unsigned int NextFigure; /* used to navigate through figures */
unsigned int Untouched; /* lowest unmodified */

unsigned int GlassHeight; /* can change during game if FullRowClear */
unsigned int GlassLevel; /* lowest free line */

int GameOver;
int GoalReached;
int GameModified=0;


/**************************************

           Basic game functions

**************************************/

void ScaleUp(struct Coord *B,int *V){
  (void) V;
  (B->x)<<=1; (B->y)<<=1;
}

void FindLeft(struct Coord *B,int *V){
  if((*V)>(B->x)) (*V)=(B->x);
}

void FindBottom(struct Coord *B,int *V){
  if((*V)>(B->y)) (*V)=(B->y);
}

void FindRight(struct Coord *B,int *V){
  if((*V)<(B->x)) (*V)=(B->x);
}

void FindTop(struct Coord *B,int *V){
  if((*V)<(B->y)) (*V)=(B->y);
}

void AddX(struct Coord *B,int *V){
  (B->x)+=(*V);
}

void AddY(struct Coord *B,int *V){
  (B->y)+=(*V);
}

void NegX(struct Coord *B,int *V){
  (void) V;
  (B->x)=-(B->x);
}

void SwapXY(struct Coord *B,int *V){
  (*V)=(B->x);
  (B->x)=(B->y);
  (B->y)=(*V);
}

void RotCW(struct Coord *B,int *V){
  NegX(B,V);
  SwapXY(B,V);
}

void RotCCW(struct Coord *B,int *V){
  SwapXY(B,V);
  NegX(B,V);
}

void FitWidth(struct Coord *B, int *Err){
  (*Err) |= ( (((B->x)>>1) < 0) || (((B->x)>>1) >= (int)P.GlassWidth) );
}

void FitHeight(struct Coord *B, int *Err){
  (*Err) |= ( (((B->y)>>1) < 0) || (((B->y)>>1) >= (int)(GlassHeight+FigureSize+1)) );
}

void FitGlass(struct Coord *B, int *Err){
  FitWidth(B,Err);
  FitHeight(B,Err);
}

void AndGlass(struct Coord *B, int *Err){
  (*Err) |= ( GlassRow[(B->y)>>1] & (1<<((B->x)>>1)) );
}

void PlaceIntoGlass(struct Coord *B, int *V){
  (void) V;
  GlassRow[(B->y)>>1] |= (1<<((B->x)>>1));
}


int ForEachIn(int FN, bfunc Func, int V){
  unsigned int i;

  for (i = BlockN[FN]; i < BlockN[FN+1]; i++)
    (*Func)(Block+i, &V);

  return V;
}


int Dimension(int FN, bfunc FindMin, bfunc FindMax){
  return (ForEachIn(FN,FindMax,INT_MIN)-ForEachIn(FN,FindMin,INT_MAX))>>1;
}

int Center(int FN, bfunc FindMin, bfunc FindMax){
  return (ForEachIn(FN,FindMin,INT_MAX)+ForEachIn(FN,FindMax,INT_MIN))>>1;
}

void Normalize(int FN,struct Coord *C){
  (C->x)=Center(FN,FindLeft,FindRight);
  (C->y)=Center(FN,FindBottom,FindTop);
  ForEachIn(FN,AddX,-(C->x));
  ForEachIn(FN,AddY,-(C->y));
}

int CopyFigure(int DestN,int SrcN){
  int Len;

  Len=BlockN[SrcN+1]-BlockN[SrcN];
  if(DestN>SrcN)
    BlockN[DestN+1]=BlockN[DestN]+Len;
  memcpy(Block+BlockN[DestN],Block+BlockN[SrcN],Len*sizeof(struct Coord));

  return DestN;
}


unsigned int Unfilled(void){
  unsigned int i, s, r;

  for(i = 0, s = 0; i < GlassHeight; i++) {
    for(r = (~GlassRow[i]) & FullRow; r; r >>= 1) {
      if (r & 1)
        s++;
    }
  }

  return s;
}


void DetectGlassLevel(void) {
  GlassLevel = GlassHeight + FigureSize + 1;
  while ((GlassLevel > 0) && (GlassRow[GlassLevel - 1] == 0))
    GlassLevel--;
}


/**************************************

           New game functions

**************************************/

void FillGlass(void){
  unsigned int i, Places, Blocks;

  for (i = 0 ; i < P.FillLevel ; i++) {
    for (Places = P.GlassWidth, Blocks = P.FillRatio ; Places > 0 ; Places--) {
      FillBuf[i] <<= 1;
      if ((rand() % Places) < Blocks) {
        FillBuf[i] |= 1; Blocks--;
      }
    }
  }
}


int FindBlock(struct Coord *B,struct Coord *A,int Len){
  for (;Len && (((B->x) != (A->x)) || ((B->y) != (A->y))); Len--, A++);
  return Len;
}

int SelectSlots(struct Coord *Slot,struct Coord *FBlock,int Weight){
  int SlotNum = 0, ApertureSize, HalfSize, SeedCnt, x, y, Delta;

  ApertureSize = (P.Aperture == 0) ? (Weight ? 3 : 1) : P.Aperture;
  HalfSize = ApertureSize / 2;
  for(SeedCnt = 0; SeedCnt < (((P.Aperture == 0) && Weight) ? Weight : 1); SeedCnt++){
    for(x = -HalfSize; x < (ApertureSize - HalfSize); x++){
      Delta = P.Metric ? 0 : abs(x);
      for(y = -HalfSize+Delta; y < (ApertureSize - HalfSize - Delta); y++){
        Slot[SlotNum].x = x + (((P.Aperture == 0) && Weight) ? FBlock[SeedCnt].x : 0);
        Slot[SlotNum].y = y + (((P.Aperture == 0) && Weight) ? FBlock[SeedCnt].y : 0);
        if (((Weight >= (int)P.WeightMin) || (!FindBlock(Slot + SlotNum, FBlock, Weight))) &&
           ((!P.SlotsUnique) || (!FindBlock(Slot + SlotNum, Slot, SlotNum))))
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

int NewFigure(struct Coord *F) {
  unsigned int i;
  struct Coord Slot[MAX_SLOTS], *B;
  
  for (i = 0, B = F; i < P.WeightMax; i++){
    memcpy(B, Slot + (rand() % SelectSlots(Slot, F, B-F)),sizeof(struct Coord));
    if (!FindBlock(B, F, B-F))
      B++;
  }
  return B-F;
}


/**************************************

             NewGame

**************************************/

void NewGame(void)
{
  srand((unsigned int)time(NULL));

  FillGlass();

  for (FigureNum = 0, BlockN[0] = 0; BlockN[FigureNum] < TotalArea; FigureNum++){
    BlockN[FigureNum+1] = BlockN[FigureNum] + NewFigure(Block + BlockN[FigureNum]);
    ForEachIn(FigureNum, ScaleUp, 0);
  }

  CurFigure = 1; Untouched = NextFigure = 0;

  strcpy(ParentName, "none");
}


/**************************************

	Screen functions
           
**************************************/

SCREEN *Screen = NULL;
WINDOW *MyScr = NULL;


void StartCurses(void) {
  Screen = newterm(NULL, stderr, stdin);
  if (Screen) {
    set_term(Screen);
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
  }
}

void DeleteMyScr(void) {
  if (MyScr) {
    delwin(MyScr);
    MyScr = NULL;
  }
}

void StopCurses() {
  if (Screen) {
    DeleteMyScr();
    endwin();
    delscreen(Screen);
    Screen = NULL;
  }
}


#define MAX_ROW_LEN ((MAX_GLASS_WIDTH+2)*2)

void PutRowImage(unsigned int Row, char *Image, int N) {
  for (;N--; Row >>= 1) {
    *Image++ = (Row & 1) ? '[' : ' ';
    *Image++ = (Row & 1) ? ']' : ' ';
  }
}

void DrawGlass(int GlassRowN) {
  int RowN;
  char RowImage[MAX_ROW_LEN];

  GlassRowN -= (getmaxy(MyScr) - 1);
  for (RowN = getmaxy(MyScr) - 1; RowN >= 0; RowN--, GlassRowN++) {
    memset(RowImage, (GlassRowN >= (int)GlassHeight) ? ' ' : '#', MAX_ROW_LEN);
    if ((GlassRowN >= 0) && (GlassRowN < (int)(GlassHeight + FigureSize +1)))
      PutRowImage(GlassRow[GlassRowN], RowImage+2, P.GlassWidth);
    mvwaddnstr(MyScr, RowN, 0, RowImage, (P.GlassWidth + 2) * 2);
  }
}


int SelectGlassRow(void) {
  int FCV = Center((CurFigure < FigureNum) ? CurFigure : (CurFigure - 1), FindBottom, FindTop) >> 1;
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

void DrawBlock(struct Coord *B,int *GRN) {
  mvwchgat(MyScr, (*GRN) - ((B->y) >> 1) ,((B->x) & (~1)) + 2, 2, A_REVERSE, 0, NULL);
}


void DrawQueue(void) {
  int x, y;
  struct Coord C;

  int SideLen = FigureSize + 2;
  int TwiSide = SideLen * 2;
  int LeftMargin = (P.GlassWidth + 2) * 2;
  int PlacesH = (getmaxx(MyScr) - LeftMargin) / TwiSide;
  int PlacesV = getmaxy(MyScr) / SideLen;

  unsigned int N = CurFigure + 1;

  int OffsetX = LeftMargin + SideLen;
  int OffsetYInit = -SideLen + 1;

  for (x = 0; (N < FigureNum) && (x < PlacesH); x++, OffsetX += TwiSide) {

    int OffsetY = OffsetYInit;

    for (y = 0; (N < FigureNum) && (y < PlacesV); y++, OffsetY -= TwiSide, N++){
      CopyFigure(FigureNum, N);
      Normalize(FigureNum, &C);
      ForEachIn(FigureNum, AddX, OffsetX);
      ForEachIn(FigureNum, AddY, OffsetY); 
      ForEachIn(FigureNum, DrawBlock, 0);
    }
  }
}


#define STATUS_MARK_POS 0
#define STATUS_MARK_LEN 1

#define STATUS_SCORE_POS 1
#define STATUS_SCORE_LEN 4

#define STATUS_LEN (STATUS_MARK_LEN+STATUS_SCORE_LEN)


void DrawStatus(void) {
  char StatusMark[STATUS_MARK_LEN + 1];
  char StatusScore[STATUS_SCORE_LEN + 1];
  int CurScore = (P.Goal == FILL_GOAL) ? Unfilled() : CurFigure;
  int StatusY = getmaxy(MyScr) - 1;
  int StatusX = getmaxx(MyScr) - STATUS_LEN;

  StatusMark[0] = GoalReached ? '+' : (GameOver ? '-' : ' ');
  snprintf(StatusScore, STATUS_SCORE_LEN+1, "%-*d", STATUS_SCORE_LEN, CurScore);

  mvwaddnstr(MyScr, StatusY, StatusX + STATUS_MARK_POS, StatusMark, STATUS_MARK_LEN);
  mvwaddnstr(MyScr, StatusY, StatusX + STATUS_SCORE_POS, StatusScore, STATUS_SCORE_LEN);
}

int ShowScreen(void) {
  if (Screen) {
    if (MyScr == NULL) {
      if ((MyScr = newwin(0, 0, 0, 0)) == NULL)
        return 0;
      refresh();
    }

    werase(MyScr);

    if (((unsigned int)getmaxx(MyScr) < ((P.GlassWidth + 2) * 2)) || ((unsigned int)getmaxy(MyScr) < (MAX_FIGURE_SIZE * 2))){
      mvwaddstr(MyScr, 0, 0, "Screen too small");
    } else {
      int GlassRowN = SelectGlassRow();

      DrawGlass(GlassRowN);
      if (CurFigure < FigureNum)
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

void Drop(int FigN){
  CopyFigure(FigureNum,FigN);
  if(P.Gravity){
    if(!P.FlatFun){
      ForEachIn(FigureNum,AddY,-(ForEachIn(FigureNum,FindBottom,INT_MAX)&(~1)));
      while((ForEachIn(FigureNum,FitGlass,0)==0)&&(ForEachIn(FigureNum,AndGlass,0)!=0))
        ForEachIn(FigureNum,AddY,2);
    }else{
      while((ForEachIn(FigureNum,FitGlass,0)==0)&&(ForEachIn(FigureNum,AndGlass,0)==0))
        ForEachIn(FigureNum,AddY,-2);
      ForEachIn(FigureNum,AddY,2);
    }
  }
  ForEachIn(FigureNum,PlaceIntoGlass,0);
  DetectGlassLevel();
}

void ClearFullRows(void) {
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

void Deploy(int FN) {
  struct Coord C;

  Normalize(FN, &C);
  ForEachIn(FN, AddX, P.GlassWidth); /* impliciltly divided by 2 */
  ForEachIn(FN, AddY, ((GlassLevel + 1) << 1) + FigureSize);
  GameModified = 1;
}

void CheckGame(void) {
  switch(P.Goal){
    case TOUCH_GOAL:
      if ((ForEachIn(FigureNum, FindBottom, INT_MAX) >> 1) == 0)
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

void RewindGlassState(void) {
  unsigned int i, GlassSize;

  for (i=0; i < P.FillLevel; i++)
    GlassRow[i] = FillBuf[i];

  GlassHeight=P.GlassHeightBuf;
  GlassSize = GlassHeight + FigureSize + 1;

  for (; i < GlassSize; i++)
    GlassRow[i] = 0;

  GlassLevel = P.FillLevel;
  CurFigure = 0;
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
    if (P.FullRowClear)
      ClearFullRows();
    CheckGame();
    if (GameOver)
      NextFigure = CurFigure;
  }

  if(CurFigure >= Untouched){
    if(CurFigure < FigureNum)
      Deploy(CurFigure);
    Untouched = CurFigure + 1;
  }
}

/**************************************

           PlayGame

**************************************/

int KeepPlaying;

void ExitWithSave(void){
  KeepPlaying = 0;
}

void ExitWithoutSave(void){
  GameModified=0;
  KeepPlaying = 0;
}

void Attempt(bfunc F, int V) {
  if (!GameOver) {
    struct Coord C;

    CopyFigure(FigureNum, CurFigure);
    Normalize(FigureNum, &C);
    ForEachIn(FigureNum, F, V);
    ForEachIn(FigureNum, AddX, C.x);
    ForEachIn(FigureNum, AddY, C.y);
    if((ForEachIn(FigureNum,FitGlass,0)==0)&&
       ((!P.FlatFun) || (ForEachIn(FigureNum,AndGlass,0)==0))){
      CopyFigure(CurFigure,FigureNum);
      Untouched=CurFigure+1;
      GameModified=1;
    }
  }
}

void MoveCurLeft(void){
  Attempt(AddX, -2);
}

void MoveCurRight(void){
  Attempt(AddX, 2);
}

void MoveCurDown(void){
  if (!P.Gravity)
    Attempt(AddY, -2);
}

void MoveCurUp(void){
  if (!P.Gravity)
    Attempt(AddY, 2);
}

void RotateCurCW(void){
  Attempt(RotCW, 0);
}

void RotateCurCCW(void){
  Attempt(RotCCW, 0);
}

void MirrorCurVert(void){
  Attempt(NegX, 0);
}

void DropCur(void){
  if((!GameOver)&&
     (ForEachIn(CurFigure,FitGlass,0)==0)&&
     (ForEachIn(CurFigure,AndGlass,0)==0)){
    NextFigure=CurFigure+1;
  }
}

void UndoFigure(void){
  if(CurFigure>0)
    NextFigure=CurFigure-1;
}

void RedoFigure(void){
  if((CurFigure+1)<Untouched)
    NextFigure=CurFigure+1;
}

void Rewind(void){
  NextFigure=0;
}

void LastPlayed(void){
  NextFigure=Untouched-1;
}

struct KBinding {
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


void ExecuteCmd(void){
  int Key;
  struct KBinding *P;
  void (*Func)(void);

  do {
    Key=getch();
    for(P=KBindList;((Func=(P->Action))!=NULL)&&(Key!=(P->Key));P++);
  } while(Func==NULL);

  (*Func)();
}


void PlayGame(void){
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
}


#include "md5hash.h"

/**************************************

           SaveGame

**************************************/


#define stringize(s) stringyze(s)
#define stringyze(s) #s


char *StorePtr;

int StoreFree;

void Adjust(int Done) {
  if (Done >= StoreFree)
    Done = StoreFree;

  /* Done--; */

  StoreFree -= Done;
  StorePtr += Done;
}

void StoreInt(int V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%d%c", V, (char)Delim));
}

void StoreUnsigned(unsigned int V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", V, (char)Delim));
}

void StoreString(char *S) {
  Adjust(snprintf(StorePtr, StoreFree, "%s\n", S));
}

#define PARNUM (sizeof(P) / sizeof(unsigned int))

void SaveGame(void){
  unsigned int *UPtr = (unsigned int *) (&P);
  unsigned int i; 
  int Used;
  char *UserName;

  int fd;


  StorePtr = (char *) GlassRow;
  StoreFree = GlassRowBufSize;


  for (i = 0; i < PARNUM; i++, UPtr++)
    StoreUnsigned(*UPtr, '\n');

  if ((GameType == 1) && (strcmp(ParentName, "none") == 0))
    strcpy(ParentName, GameName);

  GameType = 1;

  StoreString(ParentName);
  StoreUnsigned(FigureNum, '\n');
  StoreUnsigned(CurFigure, '\n');

  for (i = 0; i < P.FillLevel; i++)
    StoreUnsigned(FillBuf[i], ';');
  StoreString("");

  for(i = 0; i <= FigureNum; i++)
    StoreUnsigned(BlockN[i], ';');
  StoreString("");

  for(i = 0; i < BlockN[FigureNum]; i++) {
    StoreInt(Block[i].x, ',');
    StoreInt(Block[i].y, ';');
  }
  StoreString("");

  UserName = getenv("USER");
  if (!UserName)
    UserName = "anonymous";
  snprintf(PlayerName,OM_STRLEN,"%s",UserName);
  StoreString(PlayerName);

  TimeStamp = (unsigned int)time(NULL);
  StoreUnsigned(TimeStamp, '\n');

  Used = GlassRowBufSize - StoreFree;
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


int CheckParameters(void){
  if (P.Aperture > MAX_FIGURE_SIZE){
    snprintf(MsgBuf, OM_STRLEN, "[1] Aperture (%d) > MAX_FIGURE_SIZE (%d)", P.Aperture, MAX_FIGURE_SIZE);
  } else if (P.Metric > 1){
    snprintf(MsgBuf, OM_STRLEN, "[2] Metric (%d) can be 0 or 1", P.Metric);
  } else if (P.WeightMax > MAX_FIGURE_SIZE){
    snprintf(MsgBuf, OM_STRLEN, "[3] WeightMax (%d) > MAX_FIGURE_SIZE (%d)", P.WeightMax, MAX_FIGURE_SIZE);
  } else if (P.WeightMin < 1){
    snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) < 1", P.WeightMin);
  } else if (P.WeightMin > P.WeightMax){
    snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) > [3] WeightMax (%d)", P.WeightMin, P.WeightMax);
  } else if (P.Gravity > 1){
    snprintf(MsgBuf, OM_STRLEN, "[5] Gravity (%d) can be 0 or 1", P.Gravity);
  } else if (P.FlatFun > 1){
    snprintf(MsgBuf, OM_STRLEN, "[6] FlatFun (%d) can be 0 or 1", P.FlatFun);
  } else if (P.FullRowClear > 1){
    snprintf(MsgBuf, OM_STRLEN, "[7] FullRowClear (%d) can be 0 or 1", P.FullRowClear);
  } else if (P.Goal >= MAX_GOAL){
    snprintf(MsgBuf, OM_STRLEN, "[8] Goal (%d) > 2", P.Goal);
  } else {
    FigureSize = (P.Aperture == 0) ? P.WeightMax : P.Aperture;

    if (P.GlassWidth < FigureSize){
      snprintf(MsgBuf, OM_STRLEN, "[9] GlassWidth (%d) < FigureSize (%d)", P.GlassWidth, FigureSize);
    } else if (P.GlassWidth > MAX_GLASS_WIDTH){
      snprintf(MsgBuf, OM_STRLEN, "[9] GlassWidth (%d) > MAX_GLASS_WIDTH (%d)", P.GlassWidth, MAX_GLASS_WIDTH);
    } else if (P.GlassHeightBuf < FigureSize){
      snprintf(MsgBuf, OM_STRLEN, "[10] GlassHeight (%d) < FigureSize (%d)", P.GlassHeightBuf, FigureSize);
    } else if (P.GlassHeightBuf > MAX_GLASS_HEIGHT){
      snprintf(MsgBuf, OM_STRLEN, "[10] GlassHeight (%d) > MAX_GLASS_HEIGHT (%d)", P.GlassHeightBuf, MAX_GLASS_HEIGHT);
    } else if (P.FillLevel > P.GlassHeightBuf){
      snprintf(MsgBuf, OM_STRLEN, "[11] FillLevel (%d) > [10] GlassHeight (%d)", P.FillLevel, P.GlassHeightBuf);
    } else if (P.FillRatio >= P.GlassWidth){
      snprintf(MsgBuf, OM_STRLEN, "[12] FillRatio (%d) >= [9] GlassWidth (%d)", P.FillRatio, P.GlassWidth);
    } else if (P.SlotsUnique > 1){
      snprintf(MsgBuf, OM_STRLEN, "[13] SlotsUnique (%d) can be 0 or 1", P.SlotsUnique);
    } else {
      unsigned int i,Area = FigureSize;

      FullRow=((1<<(P.GlassWidth-1))<<1)-1;
      TotalArea = (P.GlassWidth * P.GlassHeightBuf) - (P.FillLevel * P.FillRatio);

      if (P.Aperture != 0){
        Area=P.Aperture * P.Aperture;
        if(P.Metric == 0){
          if ((P.Aperture % 2) == 0)
            Area -= P.Aperture;
          for (i = 1; i < ((P.Aperture - 1) / 2); i++)
            Area -= 4*i;
        }
      }

      if (Area < P.WeightMin){
        snprintf(MsgBuf, OM_STRLEN, "[4] WeightMin (%d) > Aperture Area (%d)", P.WeightMin, Area);
      } else {
        return 0;
      }
    }
  }

  return 1;
}


char *LoadPtr;


int ReadInt(int *V, int Delim) {
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


void ReadString(char *S) {
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


int LoadParameters(void) {
  unsigned int i;
  unsigned int *Par = (unsigned int *) (&P);

  for (i = 0; i < PARNUM; i++, Par++){
    if(ReadInt((int *)Par, 0) != 0) {
      snprintf(MsgBuf, OM_STRLEN, "[%d] Parameter load error.", i+1);
      return 1;
    }
  }

  return 0;
}

int WrongUnits(unsigned int R){
  unsigned int n;

  if ((R & (~FullRow)) != 0)
    return 1;

  for (n = 0; R; R >>= 1)
    n += R & 1;

  return n != P.FillRatio;
}

int ReadGlassFill(void) {
  unsigned int i;

  for (i = 0; i < P.FillLevel; i++) {
    if (ReadInt((int *)(FillBuf + i), ';') != 0) {
      Msg = "[17] GlassRow load error."; return 1;
    }
    if (WrongUnits(FillBuf[i])) {
      snprintf(MsgBuf, OM_STRLEN, "[17] Wrong GlassRow[%d] = %d.",i,FillBuf[i]); return 1;
    }
  }

  return 0;
}


int ReadFigures(void) {
  unsigned int i, FW;

  for (i = 0; i <= FigureNum; i++) {
    if(ReadInt((int *)(BlockN+i), ';') != 0){
      Msg = "[18] BlockN load error."; return 1;
    }
    if (i == 0) {
      if (BlockN[0] != 0) {
        Msg = "[18] BlockN[0] must be 0."; return 1;
      }
    } else {
      FW = BlockN[i] - BlockN[i-1];
      if (FW < P.WeightMin) {
        snprintf(MsgBuf, OM_STRLEN, "[18] Weight[%d] (%d) < WeightMin (%d)", i - 1, FW, P.WeightMin); return 1;
      }
      if (FW > P.WeightMax) {
        snprintf(MsgBuf, OM_STRLEN, "[18] Weight[%d] (%d) > WeightMax (%d)", i - 1, FW, P.WeightMax); return 1;
      }
      if (BlockN[i - 1] >= TotalArea) {
        snprintf(MsgBuf, OM_STRLEN, "[18] Figure[%d] is unnecessary.", i - 1); return 1;
      }
    }
  }

  if (BlockN[FigureNum] < TotalArea) {
    Msg = "[18] Not enough blocks to cover the glass free area."; return 1;
  }

  return 0;
}


int ReadBlocks(void) {
  unsigned int i;

  for (i = 0; i < BlockN[FigureNum]; i++) {
    if ((ReadInt(&(Block[i].x), ',') != 0) || (ReadInt(&(Block[i].y), ';') != 0)) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Block[%d] : load error.", i); return 1;
    }
  }

  return 0;
}


int CheckFigures(void) {
  unsigned int i, FS;

  for (i = 0; i < FigureNum; i++) {
    FS = Dimension(i, FindLeft, FindRight);
    if (FS > FigureSize) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Figure[%d] width (%d) > FigureSize (%d)", i, FS, FigureSize); return 1;
    }
    FS = Dimension(i, FindBottom, FindTop);
    if (FS > FigureSize) {
      snprintf(MsgBuf, OM_STRLEN, "[19] Figure[%d] height (%d) > FigureSize (%d)", i, FS, FigureSize); return 1;
    }
  }

  return 0;
}


int LoadData(void) {

  ReadString(ParentName);

  if (ReadInt((int *)&FigureNum, 0) != 0) {
    Msg = "[15] FigureNum : load error.";
  } else if (FigureNum > MAX_BLOCK_NUM) {
    snprintf(MsgBuf, OM_STRLEN, "[15] FigureNum (%d) > MAX_BLOCK_NUM (%d).",FigureNum,MAX_BLOCK_NUM);
  } else if (FigureNum == 0) {
    Msg = "[15] FigureNum must not be 0.";
  } else if (ReadInt((int *)&CurFigure, 0) != 0) {
    Msg = "[16] CurFigure : load error.";
  } else if (CurFigure > FigureNum) {
    snprintf(MsgBuf, OM_STRLEN, "[16] CurFigure (%d) > [15] FigureNum (%d).",CurFigure,FigureNum);
  } else if ((ReadGlassFill() == 0) &&
             (ReadFigures() == 0) &&
             (ReadBlocks() == 0) &&
             (CheckFigures() == 0)) {
    ReadString(PlayerName); /* skip new line following block descriptions */
    ReadString(PlayerName);

    if (ReadInt((int *)&TimeStamp, 0) != 0) {
      Msg = "[21] TimeStamp read error.";
    } else {

      NextFigure=CurFigure;
      CurFigure=Untouched=NextFigure+1;
      GameType = 1;

      return 0;
    }
  }

  return 1;
}


void  *GameBuf = NULL;
size_t GameBufSize = 0;


int AllocateBuffers() {
  unsigned int NewGameBufSize;

  /* FillBuf, BlockN, Block, GlassRow = StoreBuf */

  unsigned int FillBufSize = P.FillLevel * sizeof(int);
  unsigned int MaxBlockN = TotalArea + 2 * MAX_FIGURE_SIZE;
  unsigned int BlockBufSize  = MaxBlockN * sizeof(struct Coord);
  unsigned int MaxFigure = MaxBlockN / P.WeightMin + 3;
  unsigned int BlockNBufSize = MaxFigure * sizeof(struct Coord); 

/*
  Sizes of the parameters and data text representations

  Parameters: 8*(1+1) + 4*(10+1) + (1+1) = 62
  ParentName: OM_STRLEN+1 = 81
  FigureNum:  10+1 = 11
  CurFigure:  10+1 = 11
  FillBuf:    FillLevel * (10+1) = FillLevel * 11
  BlockN:     MaxFigure * (10+1) = MaxFigure * 11
  Block:      MaxBlockN * (2+1,4+1) = MaxBlockN * 8
  PlayerName: OM_STRLEN+1 = 81
  TimeStamp:  10+1 = 11
*/

  unsigned int StoreBufSize = 62 + 81 + 11 + 11 +
                              P.FillLevel * 11 +
                              MaxFigure * 11 +
                              MaxBlockN * 11 +
                              81 + 11;

  NewGameBufSize = FillBufSize + BlockNBufSize + BlockBufSize + StoreBufSize;

  if (GameBuf == NULL) {
    GameBufSize = NewGameBufSize;
    GameBuf = malloc(GameBufSize);
    if (GameBuf == NULL) {
      snprintf(MsgBuf, OM_STRLEN, "Failed to allocate %d byte buffer.", GameBufSize);
      return 1;
    }
  } else {
    if (NewGameBufSize > GameBufSize) {
      GameBufSize = NewGameBufSize;
      GameBuf = realloc(GameBuf, GameBufSize);
      if (GameBuf == NULL) {
        snprintf(MsgBuf, OM_STRLEN, "Failed to reallocate %d byte buffer.", GameBufSize);
        return 1;
      }
    }
  }

  FillBuf = (unsigned int *) GameBuf;
  BlockN = (unsigned int *) (FillBuf + P.FillLevel);
  Block = (struct Coord *) (BlockN + MaxFigure);
  GlassRow = (unsigned int *) (Block + MaxBlockN);

  GlassRowBufSize = StoreBufSize;

  return 0;
}



#include <sys/stat.h>
#include <sys/mman.h>


int DoLoad(char *BufAddr, size_t BufLen) {
  char BufName[OM_STRLEN + 1];

  md5hash(BufAddr, BufLen, BufName);
  strcat(BufName, ".mino");

  LoadPtr = BufAddr;

  if ((LoadParameters() != 0) || (CheckParameters() != 0) || (AllocateBuffers() != 0))
    return 1;

  if (strcmp(BufName, GameName) != 0) {
    GameType = 2;
    return 0;
  }

  return LoadData();
}


int LoadGame(char *Name) {
  unsigned int i;
  unsigned int *Par = (unsigned int *)(&P);

  struct stat st;

  for (i = 0; i < PARNUM; i++)
    Par[i] = -1;

  MsgBuf[0] = '\0';
  Msg = MsgBuf;
  PlayerName[0] = '\0';
  TimeStamp = 0;

  GameType = 3;


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
        Msg = "mmap failed.";
      } else {
        Buf[st.st_size] = '\0';
        snprintf(GameName, OM_STRLEN, "%s", basename(Name));
        int Err = DoLoad(Buf, st.st_size);
        munmap(Buf, st.st_size + 1);
        return Err;
      }
    }
  }

  return 1;
}


void ExportGame(void){
  unsigned int i, Order[] = {2,7,0,1,4,5,6,8,9,10,11,12,3};
  unsigned int *Par = (unsigned int *)(&P);


  if ((GameType != 1) || (strcmp(ParentName, "none") == 0))
    strcpy(ParentName, GameName);


  fprintf(stdout,"{\n");

  fprintf(stdout,"  Data = {");

  fprintf(stdout,"\"%s\", ", GameName);
  fprintf(stdout,"\"%s\", ", PlayerName);
  fprintf(stdout,"%u, ", TimeStamp);
  fprintf(stdout, GameType == 1 ? "%s," : "\"%s\", ", Msg);

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


void Report(void) {
  if (GameType == 1) {
    unsigned int Score = (P.Goal == FILL_GOAL) ? Unfilled() : (GoalReached ? CurFigure : FigureNum);
    snprintf(MsgBuf, OM_STRLEN,  "%d", Score);
  }

  if (isatty(fileno(stdout))) {
    fprintf(stdout, "%s\n", Msg);
  } else {
    ExportGame();
  }
}


#define USAGE "Omnimino 0.5 Copyright (C) 2019-2022 Andrey Dobrovolsky\n\n\
Usage: omnimino infile\n\
       ls *.mino | omnimino > outfile\n"

int main(int argc,char *argv[]){
  int argi;
  char FName[OM_STRLEN + 1];

  if (argc > 1) {
    for (argi = 1; argi < argc; argi++){
      if (LoadGame(argv[argi]) == 0) {
        PlayGame();
        if (GameModified)
          SaveGame();
      }
      Report();
    }
  } else {
    if (isatty(fileno(stdin))) {
      Msg = USAGE;
      if (isatty(fileno(stdout))) {
        fprintf(stdout, "%s\n", Msg);
      }
    } else {
      while (!feof(stdin)) {
        if (fscanf(stdin, "%" stringize(OM_STRLEN) "s%*[^\n]", FName) > 0) {
          if (LoadGame(FName) == 0) {
            if (GameType == 1)
              GetGlassState();
          }
          Report();
        }
      }
    }
  }

  return 0;
}

