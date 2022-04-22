#ifndef _OMNIMINO_H

#define _OMNIMINO_H 1


#include "omnitype.h"


void InitGame(struct Omnimino *G);

void FitGlass(struct Coord *B, int *Err);
void AndGlass(struct Coord *B, int *Err);

void GetGlassState(struct Omnimino *G);

#endif

