#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "md5hash.h"

#include "omnitype.h"

#include "omnimino.def"

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

static void StoreString(char *S) {
  Adjust(snprintf(StorePtr, StoreFree, "%s\n", S));
}


void SaveGame(struct Omnimino *GG){
  unsigned int *UPtr = (unsigned int *) (&(GG->P));
  unsigned int i; 
  int Used;
  char *UserName;

  FILE *fout;


  if (GameType == 3)
    return;

  StorePtr = (char *) GlassRow;
  StoreFree = StoreBufSize;

  do {
    for (i = 0; i < PARNUM; i++, UPtr++)
      StoreUnsigned(*UPtr, '\n');

    if ((GameType == 2) && (FillRatio != 0))
        break;

    StoreString(ParentName);
    StoreInt((int)(LastFigure - Figure), '\n');
    StoreInt((int)(NextFigure - Figure), '\n');

    for (i = 0; i < FillLevel; i++)
      StoreUnsigned(FillBuf[i], ';');
    StoreString("");

    if (GameType == 2)
      break;

    struct Coord **F;

    for(F = Figure; F <= LastFigure; F++)
      StoreInt((int)(*F - Block), ';');
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

  } while(0);

  Used = StoreBufSize - StoreFree;
  md5hash(GlassRow, Used, GameName);
  strcat(GameName, ".mino");

  if (GameType == 2)
    GameName[0] = 'p';


  fout = fopen(GameName, "w");
  if (fout == NULL) {
    snprintf(MsgBuf, OM_STRLEN, "Can not open for write %s.", GameName);
    GameType = 3;
  } else {
    if(fwrite(GlassRow, sizeof(char), Used, fout) != (size_t)Used) {
      snprintf(MsgBuf, OM_STRLEN, "Error writing %s.", GameName);
      GameType = 3;
    }
    fclose(fout);
  }
}

