#include <stdio.h>
#include <string.h>

#include "../omnitype.h"

#include "../omnimino.h"
#include "../omniload.h"
#include "../omniplay.h"
#include "../omnisave.h"
#include "../omninew.h"

int main(int argc,char *argv[]){
  struct Omnimino Game, Game2;

  InitGame(&Game); InitGame(&Game2);

  if (argc > 1) {
    if (LoadGame(&Game, argv[1]) == 0) {
      if (Game.V.GameType == 2) {
        Game2.P = Game.P;

        Game.P.Aperture = 0;
        Game.P.Metric = 0;
        Game.P.WeightMax = 1;
        Game.P.WeightMin = 1;
        Game.P.Gravity = 0;
        Game.P.FlatFun = 0;
        Game.P.FullRowClear = 0;
        Game.P.Goal = 0;
        Game.P.FillLevel = 0;
        Game.P.FillRatio = 0;
        Game.P.SlotsUnique = 0;

        Game.C.TotalArea = Game.P.GlassWidth * Game.P.GlassHeightBuf;

        if (NewGame(&Game) == 0) {
          if (PlayGame(&Game)) {
            Game.P = Game2.P;
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

