#include <stdio.h>
#include <unistd.h>

#include "omnimino.h"


#define stringize(s) stringyze(s)
#define stringyze(s) #s


#define USAGE "Omnimino 0.5 Copyright (C) 2019-2022 Andrey Dobrovolsky\n\n\
Usage: omnimino infile\n\
       ls *.mino | omnimino > outfile\n\n"


int main(int argc,char *argv[]){
  int argi;
  char FName[OM_STRLEN + 1];

  InitGame();

  if (argc > 1) {
    for (argi = 1; argi < argc; argi++){
      if (LoadGame(argv[argi]) == 0) {
        if (PlayGame())
          SaveGame();
      }
      Report();
    }
  } else {
    if (isatty(fileno(stdin))) {
      if (isatty(fileno(stdout))) {
        fprintf(stdout, USAGE);
      }
    } else {
      while (!feof(stdin)) {
        if (fscanf(stdin, "%" stringize(OM_STRLEN) "s%*[^\n]", FName) > 0) {
          if (LoadGame(FName) == 0) {
            if (CheckGame() == 0)
              EvaluateGame();
          }
          Report();
        }
      }
    }
  }

  return 0;
}

