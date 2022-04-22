#include <stdio.h>
#include <unistd.h>

#include "omnimino.h"
#include "omniload.h"
#include "omniplay.h"
#include "omnisave.h"
#include "omnilua.h"
#include "omninew.h"

#define stringize(s) stringyze(s)
#define stringyze(s) #s


void Report(struct Omnimino *G) {
  if (G->V.GameType == 1) {
    if ((G->P.Goal != FILL_GOAL) && (!G->V.GoalReached))
      snprintf(G->S.MsgBuf, OM_STRLEN,  "%d", (int)(G->D.LastFigure - G->M.Figure));
  } else if (G->V.GameType == 2) {
    G->S.MsgBuf[0] = '\0';
  }

  if (isatty(fileno(stdout))) {
    fprintf(stdout, "%s\n", G->S.MsgBuf);
  } else {
    ExportLua(G, stdout);
  }
}


#define USAGE "Omnimino 0.5 Copyright (C) 2019-2022 Andrey Dobrovolsky\n\n\
Usage: omnimino infile\n\
       ls *.mino | omnimino > outfile\n\n"


int main(int argc,char *argv[]){
  int argi;
  char FName[OM_STRLEN + 1];

  struct Omnimino Game;

  InitGame(&Game);

  if (argc > 1) {
    for (argi = 1; argi < argc; argi++){
      if (LoadGame(&Game, argv[argi]) == 0) {
        if ((Game.V.GameType == 1) || (NewGame(&Game) == 0)) {
          if (PlayGame(&Game))
            SaveGame(&Game);
        }
      }
      Report(&Game);
    }
  } else {
    if (isatty(fileno(stdin))) {
      if (isatty(fileno(stdout))) {
        fprintf(stdout, USAGE);
      }
    } else {
      while (!feof(stdin)) {
        if (fscanf(stdin, "%" stringize(OM_STRLEN) "s%*[^\n]", FName) > 0) {
          if (LoadGame(&Game, FName) == 0) {
            if (Game.V.GameType == 1) {
              Game.V.CurFigure = Game.D.NextFigure + 1;
              GetGlassState(&Game);
              }
          }
          Report(&Game);
        }
      }
    }
  }

  return 0;
}

