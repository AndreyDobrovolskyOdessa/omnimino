#include <stdio.h>
#include <string.h>

#include <ncurses.h>


#include "omnifunc.h"
#include "omnimino.h"
#include "omninew.h"


static struct Omnimino *GG;

#include "omnimino.def"


/*******************************************************

	Symbols, used to draw glass' walls and floor
	depending on the Goal, GameOver and GoalReached
	variables values.
           
*******************************************************/

#define GOAL_SYM  ("+:=")
#define LOOSE_SYM '-'
#define WIN_SYM   '#'


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
    int Sym;

    if (GlassRowN >= (int)GlassHeight)
      Sym = ' '; 
    else if (!GameOver)
      Sym = GOAL_SYM[Goal];
    else if (!GoalReached)
      Sym = LOOSE_SYM;
    else
      Sym = WIN_SYM;

    memset(RowImage, Sym, MAX_ROW_LEN);
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
  if (Gravity)
    mvwaddch(MyScr, getmaxy(MyScr) - 3, 0, 'G');

  if (FlatFun)
    mvwaddch(MyScr, getmaxy(MyScr) - 2, 0, 'F');

  if (FullRowClear)
    mvwaddch(MyScr, getmaxy(MyScr) - 1, 0, 'C');

  mvwaddstr(MyScr, getmaxy(MyScr) - 1, getmaxx(MyScr) - strlen(MsgBuf) - 1, MsgBuf);
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
      LastTouched = CurFigure;
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
  if (CurFigure < LastTouched)
    NextFigure = CurFigure + 1;
}

static void Rewind(void) {
  NextFigure = Figure;
}

static void LastPlayed(void) {
  NextFigure = LastTouched;
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


static int ExecuteCmd(void){
  int Key;
  struct KBinding *P;
  void (*Func)(void);

  KeepPlaying = 1;

  do {
    Key=getch();
    for(P=KBindList;((Func=(P->Action))!=NULL)&&(Key!=(P->Key));P++);
  } while(Func==NULL);

  (*Func)();

  return KeepPlaying;
}


int PlayGame(struct Omnimino *G){

  GG = G;

  CurFigure = NextFigure + 1; /* forces RewindGlassState() */

  StartCurses();

  do
    GetGlassState(G);
  while (ShowScreen() && ExecuteCmd());

  StopCurses();

  if (GameModified) {
    if ((GameType == 1) && (strcmp(ParentName, "none") == 0))
      strcpy(ParentName, GameName);
    GameType = 1;
  }

  return GameModified;
}


