#include <string.h>

#include <curses.h>


#include "../omnitype.h"

#include "../omnifunc.h"
#include "../omnimino.h"
#include "../omninew.h"


static struct Omnimino *GG;

#include "../omnimino.def"


/*******************************************************

	Symbols, used to draw glass' walls and floor
	depending on the Goal, GameOver and GoalReached
	variables values.
           
*******************************************************/

#define GOAL_SYM  (":;=")
#define LOOSE_SYM '-'
#define WIN_SYM   '+'
#define BELOW_SYM '?'

#define THICKNESS 4


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


#define MAX_ROW_LEN ((MAX_GLASS_WIDTH + THICKNESS) * 2)

static void PutRowImage(unsigned int Row, char *Image, int N) {
  for (;N--; Row >>= 1) {
    *Image++ = (Row & 1) ? '[' : ' ';
    *Image++ = (Row & 1) ? ']' : ' ';
  }
}


static void DrawGlass(int GlassRowN) {
  int RowN;
  char RowImage[MAX_ROW_LEN];
  int RowWidth = (GlassWidth + THICKNESS) * 2;
  int Visible;

  Visible = (getmaxy(MyScr) * getmaxy(MyScr)) / (GlassRowN + 1);

  for (RowN = 0; RowN < getmaxy(MyScr); RowN++, GlassRowN--) {
    int Sym;

    if (GlassRowN >= (int)GlassHeight)
      Sym = ' '; 
    else if (!GameOver)
      Sym = GOAL_SYM[Goal];
    else if (!GoalReached)
      Sym = LOOSE_SYM;
    else
      Sym = WIN_SYM;

    memset(RowImage, Sym, RowWidth);
    if ((GlassRowN >= 0) && (GlassRowN < (int)FieldSize))
      PutRowImage(GlassRow[GlassRowN], RowImage+THICKNESS, GlassWidth);
    if (RowN >= Visible)
      RowImage[RowWidth - 1] = BELOW_SYM;
    mvwaddnstr(MyScr, RowN, 0, RowImage, RowWidth);
  }
}


static int SelectGlassRow(void) {
  int FCV = Center(CurFigure, FindBottom, FindTop) >> 1;
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


static void DrawBlock(struct Coord *B, int *GRN) {
  mvwchgat(MyScr, (*GRN) - ((B->y) >> 1), B->x, 2, A_REVERSE, 0, NULL);
}


static void DrawFigure(struct Coord **F, struct Coord *Buf, int OffsetX, int OffsetY) {
  CopyFigure(FigureBuf, F);
  if (Buf)
    Normalize(FigureBuf, Buf);
  ForEachIn(FigureBuf, AndX, ~1);
  ForEachIn(FigureBuf, AddX, OffsetX);
  ForEachIn(FigureBuf, DrawBlock, OffsetY);
}


static void DrawQueue(void) {
  int x, y;
  struct Coord C;

  int SideLen = FigureSize + 2;
  int TwiSide = SideLen * 2;
  int LeftMargin = (GlassWidth + THICKNESS) * 2;
  int PlacesH = (getmaxx(MyScr) - LeftMargin) / TwiSide;
  int PlacesV = getmaxy(MyScr) / SideLen;

  struct Coord **F = CurFigure + 1;

  int OffsetX = LeftMargin + SideLen;
  int OffsetYInit = SideLen / 2;

  for (x = 0; (F < LastFigure) && (x < PlacesH); x++, OffsetX += TwiSide) {

    int OffsetY = OffsetYInit;

    for (y = 0; (F < LastFigure) && (y < PlacesV); y++, OffsetY += SideLen, F++){
      DrawFigure(F, &C, OffsetX, OffsetY);
    }
  }
}


#define SCORE_WIDTH 12
#define SCORE_FORMAT "  %-5d%5d"

static void DrawStatus(void) {
  int ScoreX = getmaxx(MyScr) - SCORE_WIDTH;

  mvwhline(MyScr, getmaxy(MyScr) - 2, ScoreX, ' ', SCORE_WIDTH);
  mvwprintw(MyScr, getmaxy(MyScr) - 1, ScoreX, SCORE_FORMAT, EmptyCells, TotalArea - EmptyCells);

  if (Gravity)
    mvwaddch(MyScr, getmaxy(MyScr) - 4, 0, 'G');

  if (SingleLayer)
    mvwaddch(MyScr, getmaxy(MyScr) - 3, 0, 'S');

  if (DiscardFullRows)
    mvwaddch(MyScr, getmaxy(MyScr) - 2, 0, 'D');

  if (FixedSequence)
    mvwaddch(MyScr, getmaxy(MyScr) - 1, 0, 'F');
}


static int ShowScreen(void) {
  if (Screen) {
    if (MyScr == NULL) {
      if ((MyScr = newwin(0, 0, 0, 0)) == NULL)
        return 0;
      refresh();
    }

    werase(MyScr);

    if ((getmaxx(MyScr) < SCORE_WIDTH) ||
        (getmaxx(MyScr) < (int)((GlassWidth + THICKNESS) * 2)) ||
        (getmaxy(MyScr) < (int)((FigureSize * 2) + 2))){
      mvwaddstr(MyScr, 0, 0, "small");
    } else {
      int GlassRowN = SelectGlassRow();

      DrawGlass(GlassRowN);
      DrawFigure(CurFigure, NULL, THICKNESS, GlassRowN);
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

    CopyFigure(FigureBuf, CurFigure);
    Normalize(FigureBuf, &C);
    ForEachIn(FigureBuf, F, V);
    ForEachIn(FigureBuf, AddX, C.x);
    ForEachIn(FigureBuf, AddY, C.y);
    if(FitsGlass(FigureBuf) && ((!SingleLayer) || (!Overlaps(FigureBuf)))){
      CopyFigure(CurFigure,FigureBuf);
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
  if((!GameOver) && Placeable(CurFigure)) {
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

static void SkipForward(void) {
  struct Coord **F;

  if (!FixedSequence) {
    CopyFigure(FigureBuf, CurFigure);
    memmove(CurFigure[0], CurFigure[1], (LastFigure[0] - CurFigure[1]) * sizeof(struct Coord));
    for (F = CurFigure + 1; F < LastFigure; F++)
      F[0] = F[-1] + (F[1] - F[0]);
    CopyFigure(LastFigure - 1, FigureBuf);

    LastTouched = CurFigure - 1;
  }
}

static void SkipBackward(void) {
  struct Coord **F;

  if (!FixedSequence) {
    CopyFigure(FigureBuf, LastFigure - 1);
    for (F = LastFigure - 1; F > CurFigure; F--)
      F[0] = F[1] - (F[0] - F[-1]);
    memmove(CurFigure[1], CurFigure[0], (LastFigure[0] - CurFigure[1]) * sizeof(struct Coord));
    CopyFigure(CurFigure, FigureBuf);

    LastTouched = CurFigure - 1;
  }
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

  {'n', SkipForward},
  {'N', SkipBackward},
  {'p', SkipBackward},

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


