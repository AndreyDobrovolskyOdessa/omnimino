#ifndef _OMNIFUNC_H

#define _OMNIFUNC_H 1

#include "omnitype.h"

void ScaleUp(struct Coord *B,int *V);
void FindLeft(struct Coord *B,int *V);
void FindBottom(struct Coord *B,int *V);
void FindRight(struct Coord *B,int *V);
void FindTop(struct Coord *B,int *V);
void AddX(struct Coord *B,int *V);
void AddY(struct Coord *B,int *V);
void AndX(struct Coord *B,int *V);
void NegX(struct Coord *B,int *V);
void SwapXY(struct Coord *B,int *V);
void RotCW(struct Coord *B,int *V);
void RotCCW(struct Coord *B,int *V);
int ForEachIn(struct Coord **F, bfunc Func, int V);
int Dimension(struct Coord **F, bfunc FindMin, bfunc FindMax);
int Center(struct Coord **F, bfunc FindMin, bfunc FindMax);
void Normalize(struct Coord **F,struct Coord *C);
struct Coord *CopyFigure(struct Coord **Dst, struct Coord **Src);
int FindBlock(struct Coord *B, struct Coord *A, int Len);

#endif
