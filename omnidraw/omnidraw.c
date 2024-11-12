#include <string.h>

#include <curses.h>

#include "../omnifunc.h"



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


void OpenScreen(void) {
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


void CloseScreen(void) {
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
  int Wall = GameOver ? (GoalReached ? WIN_SYM : LOOSE_SYM) : GOAL_SYM[Goal];

  Visible = (getmaxy(MyScr) * getmaxy(MyScr)) / (GlassRowN + 1);

  for (RowN = 0; RowN < getmaxy(MyScr); RowN++, GlassRowN--) {
    memset(RowImage, (GlassRowN < (int)GlassHeight) ? Wall : ' ', RowWidth);
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


int ShowScreen(struct Omnimino *G) {
  GG = G; 

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

int ReadKey(void) {
  int Key = getch();

  switch (Key) {
  case KEY_LEFT:	Key = 'h'; break;
  case KEY_RIGHT:	Key = 'l'; break;
  case KEY_UP:		Key = 'k'; break;
  case KEY_DOWN:	Key = 'j'; break;
  case KEY_RESIZE:	Key = '*'; DeleteMyScr(); break;
  }

  return Key;
}

