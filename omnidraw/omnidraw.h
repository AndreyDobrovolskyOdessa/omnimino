#ifndef _OMNIDRAW_H

#define _OMNIDRAW_H 1

#include "../omnitype.h"

void OpenScreen(void);
void RefreshScreen(void);
void CloseScreen(void);
int ShowScreen(struct Omnimino *G);
int ReadKey(void);

#endif

