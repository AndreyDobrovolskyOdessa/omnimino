#include <stdio.h>
#include <string.h>

#include "omnifunc.h"

#include "omnimino.def"


void ExportLua(struct Omnimino *GG, FILE *fout) {
  unsigned int i, Order[] = {2,7,0,1,4,5,6,8,9,10,11,12,3};
  unsigned int *Par = (unsigned int *)(&(GG->P));
  char *sfmt = "[[%s]], ";
  char *MsgFmt = (GameType == 1) ? "%s, " : sfmt;
  char *ParentExported = ParentName;


  if (strcmp(ParentName, "none") == 0)
    ParentExported = GameName;

  fprintf(fout,"{\n");

  fprintf(fout,"  Data = {");

  fprintf(fout, sfmt, GameName);
  fprintf(fout, sfmt, PlayerName);
  fprintf(fout,"%u, ", TimeStamp);
  fprintf(fout, MsgFmt, MsgBuf);

  fprintf(fout,"},\n");

  fprintf(fout,"  Parameters = {");

  fprintf(fout,"%u, ", GameType);
  for (i = 0; i < PARNUM ; i++){
    fprintf(fout,"%d, ", Par[Order[i]]);
  }
  fprintf(fout, sfmt, ParentExported);

  fprintf(fout,"},\n");

  fprintf(fout,"},\n");
}


