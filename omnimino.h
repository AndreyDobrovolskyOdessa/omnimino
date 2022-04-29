#ifndef _OMNIMINO_H

#define _OMNIMINO_H 1


#include "omnitype.h"


void FitGlass(struct Coord *B, int *Err);
void AndGlass(struct Coord *B, int *Err);

void GetGlassState(struct Omnimino *G);

#define FitsGlass(F) (ForEachIn(F, FitGlass, 0) == 0)
#define Overlaps(F) (ForEachIn(F, AndGlass, 0) != 0)
#define Placeable(F) (FitsGlass(F) && (!Overlaps(F)))

#endif

