#ifndef _OMNIMINO_H

#define _OMNIMINO_H 1


#include "omnitype.h"


void InitGame(void);
int LoadGame(char *Name);
int PlayGame(void);
void SaveGame(void);
void Report(void);
int CheckGame(void);
void EvaluateGame(void);

#endif

