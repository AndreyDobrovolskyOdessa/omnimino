#include <stdio.h>
#include <string.h>

#include "omnifunc.h"

#include "omnimino.def"


void ExportLua(struct Omnimino *GG, FILE *fout) {
  unsigned int i, Order[] = {2,7,0,1,4,5,6,8,9,10,11,12,3};
  unsigned int *Par = (unsigned int *)(&(GG->P));


  if ((GameType != 1) || (strcmp(ParentName, "none") == 0))
    strcpy(ParentName, GameName);

  if (GameType == 2)
    MsgBuf[0] = '\0';

  fprintf(fout,"{\n");

  fprintf(fout,"  Data = {");

  fprintf(fout,"\"%s\", ", GameName);
  fprintf(fout,"\"%s\", ", PlayerName);
  fprintf(fout,"%u, ", TimeStamp);
  fprintf(fout, GameType == 1 ? "%s, " : "\"%s\", ", MsgBuf);

  fprintf(fout,"},\n");

  fprintf(fout,"  Parameters = {");

  fprintf(fout,"%u, ", GameType);
  for (i = 0; i < PARNUM ; i++){
    fprintf(fout,"%d, ", Par[Order[i]]);
  }
  fprintf(fout,"\"%s\", ", ParentName);

  fprintf(fout,"},\n");

  fprintf(fout,"},\n");
}


