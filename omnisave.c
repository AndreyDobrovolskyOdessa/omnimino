#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>

#include "md5hash.h"

#include "omnitype.h"

static struct Omnimino *GG;

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

static void StoreBlockAddr(struct Coord *V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", (int)(V - Block), (char)Delim));
}

static void StorePointer(struct Coord **V, int Delim) {
  Adjust(snprintf(StorePtr, StoreFree, "%u%c", (int)(V - Figure), (char)Delim));
}

static void StoreString(char *S) {
  Adjust(snprintf(StorePtr, StoreFree, "%s\n", S));
}


void SaveGame(struct Omnimino *G){
  unsigned int *UPtr = (unsigned int *) (&(G->P));
  unsigned int i; 
  int Used;
  char *UserName;

  int fd;

  GG = G;

  StorePtr = (char *) GlassRow;
  StoreFree = StoreBufSize;


  for (i = 0; i < PARNUM; i++, UPtr++)
    StoreUnsigned(*UPtr, '\n');

  StoreString(ParentName);
  StorePointer(LastFigure, '\n');
  StorePointer(NextFigure, '\n');

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

