#ifndef _OMNINEW_H

#define _OMNINEW_H 1


#include "omnitype.h"

void InitGame(struct Omnimino *G);

int CheckParameters(struct Omnimino *G);
int AllocateBuffers(struct Omnimino *G);
int NewGame(struct Omnimino *G);

#endif

