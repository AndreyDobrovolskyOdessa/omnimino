/******************************************************************************
	Omnimino is console version of polymino puzzle.
	Figure sizes, shapes, field size and gameplay options can be varied.

	Copyright (C) 2019-2024 Andrey Dobrovolsky

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
#include <string.h>

#include "omnigame.h"
#include "omniload.h"
#include "omnisave.h"
#include "omnilua.h"
#include "omninew.h"

#define stringize(s) stringyze(s)
#define stringyze(s) #s


void Report(struct Omnimino *G) {
  if ((G->V.GameType != 3) && (G->D.LastFigure != G->M.Figure)) { /* game data present */
    int Score = G->V.EmptyCells;
    if (G->P.Goal != FILL_GOAL) {
      Score = G->C.TotalArea;
      if (G->V.GoalReached)
        Score -= G->V.EmptyCells;
    }
    snprintf(G->S.MsgBuf, OM_STRLEN,  "%d", Score);
  }

  if (isatty(fileno(stdout))) {
    fprintf(stdout, "%s\n", G->S.MsgBuf);
  } else {
    ExportLua(G, stdout);
  }
}


#define COPYRIGHT "Omnimino 0.6.3 Copyright (C) 2019-2024 Andrey Dobrovolsky\n\n"
#define USAGE "Usage: omnimino infile\n       ls *.mino | omnimino > outfile\n\n"


int main(int argc,char *argv[]){
  int argi;
  char FName[OM_STRLEN + 1];

  char *PName = basename(argv[0]);

  struct Omnimino Game;
  struct OmniParms PBuf;


  InitGame(&Game);

  if (strcmp(PName, "omnimino") == 0) {

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
        fprintf(stdout, COPYRIGHT USAGE);
      } else {
        while (fscanf(stdin, "%" stringize(OM_STRLEN) "s%*[^\n]", FName) > 0) {
          if (LoadGame(&Game, FName) == 0) {
            if (Game.V.GameType == 1) {
              Game.V.CurFigure = Game.D.NextFigure + 1;
              GetGlassState(&Game);
            }
          }
          Report(&Game);
        }
      }
      fprintf(stdout, "MaxFigureSize = %d, MaxGlassWidth = %d, MaxGlassHeight = %d\n\n",
                       MAX_FIGURE_SIZE,    MAX_GLASS_WIDTH,    MAX_GLASS_HEIGHT);
    }

    return 0;

  }

  if (argc > 1) {
    if (LoadGame(&Game, argv[1]) == 0) {
      if (Game.V.GameType == 2) {
        PBuf = Game.P;

        Game.P.Aperture = 0;
        Game.P.Metric = 0;
        Game.P.WeightMax = 1;
        Game.P.WeightMin = 1;
        Game.P.Gravity = 0;
        Game.P.SingleLayer = 0;
        Game.P.DiscardFullRows = 0;
        Game.P.Goal = 0;
        Game.P.FillLevel = 0;
        Game.P.FillRatio = 0;
        Game.P.FixedSequence = 0;

        Game.C.TotalArea = Game.P.GlassWidth * Game.P.GlassHeightBuf;

        if (NewGame(&Game) == 0) {
          if (PlayGame(&Game)) {
            Game.P = PBuf;
            Game.P.FillLevel = Game.P.GlassHeightBuf;
            while ((Game.P.FillLevel > 0) && (Game.M.GlassRow[Game.P.FillLevel - 1] == 0))
              Game.P.FillLevel--;
            memcpy(Game.M.FillBuf, Game.M.GlassRow, Game.P.FillLevel * sizeof(int));
            Game.P.FillRatio = 0;
            Game.V.GameType = 2;
            SaveGame(&Game);
            if (Game.V.GameType != 3)
              return 0;
          }
        }
      }
    }
    fprintf(stdout, "%s\n", Game.S.MsgBuf);
  } else {
    fprintf(stdout, "\nUsage:  omnifill infile\n\n");
  }

  return 1;

}

