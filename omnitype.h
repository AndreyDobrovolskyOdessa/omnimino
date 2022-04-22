#ifndef _OMNITYPE_H

#define _OMNITYPE_H 1

/**************************************

           Omnimino types

**************************************/

struct Coord {
  int x, y;
};

typedef void (*bfunc) (struct Coord *, int *);

struct OmniParms {
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

#define PARNUM (sizeof(struct OmniParms) / sizeof(unsigned int))


struct OmniConsts {              /* Parameters' derivatives */
  unsigned int FigureSize;
  unsigned int TotalArea;
  unsigned int FullRow;
};


struct OmniData {
  struct Coord **LastFigure;
  struct Coord **NextFigure; /* used to navigate through figures */
  unsigned int TimeStamp;
};

struct OmniVars {
  struct Coord **CurFigure;
  struct Coord **LastTouched; /* latest modified */
  unsigned int GameType;
  unsigned int GlassHeight; /* can change during game if FullRowClear */
  unsigned int GlassLevel; /* lowest free line */
  unsigned int EmptyCells;
  int GameOver;
  int GoalReached;
  int GameModified;
};


struct OmniMem {
  size_t GameBufSize;
  unsigned int *FillBuf;
  struct Coord **Figure;
  struct Coord *Block;
  unsigned int *GlassRow;       /* used by SaveGame too */
  size_t StoreBufSize;
};


#define OM_STRLEN 80

struct OmniStrings {
  char MsgBuf[OM_STRLEN + 1];
  char GameName[OM_STRLEN + 1];
  char ParentName[OM_STRLEN + 1];
  char PlayerName[OM_STRLEN + 1];
};

struct Omnimino {
  struct OmniParms   P;
  struct OmniConsts  C;
  struct OmniData    D;
  struct OmniVars    V;
  struct OmniMem     M;
  struct OmniStrings S;
};

#endif

