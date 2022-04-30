#include <limits.h>
#include <string.h>

#include "omnitype.h"


/**************************************

           Basic game functions

**************************************/

void ScaleUp(struct Coord *B,int *V){
  (void) V;
  (B->x)<<=1; (B->y)<<=1;
}

void FindLeft(struct Coord *B,int *V){
  if((*V)>(B->x)) (*V)=(B->x);
}

void FindBottom(struct Coord *B,int *V){
  if((*V)>(B->y)) (*V)=(B->y);
}

void FindRight(struct Coord *B,int *V){
  if((*V)<(B->x)) (*V)=(B->x);
}

void FindTop(struct Coord *B,int *V){
  if((*V)<(B->y)) (*V)=(B->y);
}

void AddX(struct Coord *B,int *V){
  (B->x)+=(*V);
}

void AddY(struct Coord *B,int *V){
  (B->y)+=(*V);
}

void AndX(struct Coord *B, int *Mask) {
  B->x &= *Mask;
}

void NegX(struct Coord *B,int *V){
  (void) V;
  (B->x)=-(B->x);
}

void SwapXY(struct Coord *B,int *V){
  (*V)=(B->x);
  (B->x)=(B->y);
  (B->y)=(*V);
}

void RotCW(struct Coord *B,int *V){
  NegX(B,V);
  SwapXY(B,V);
}

void RotCCW(struct Coord *B,int *V){
  SwapXY(B,V);
  NegX(B,V);
}

int ForEachIn(struct Coord **F, bfunc Func, int V){
  struct Coord *B = *F++;

  while (B < *F)
    (*Func)(B++, &V);

  return V;
}


int Dimension(struct Coord **F, bfunc FindMin, bfunc FindMax){
  return (ForEachIn(F,FindMax,INT_MIN)-ForEachIn(F,FindMin,INT_MAX))>>1;
}


int Center(struct Coord **F, bfunc FindMin, bfunc FindMax){
  return (ForEachIn(F,FindMin,INT_MAX)+ForEachIn(F,FindMax,INT_MIN))>>1;
}


void Normalize(struct Coord **F,struct Coord *C){
  (C->x)=Center(F,FindLeft,FindRight);
  (C->y)=Center(F,FindBottom,FindTop);
  ForEachIn(F,AddX,-(C->x));
  ForEachIn(F,AddY,-(C->y));
}


struct Coord *CopyFigure(struct Coord **Dst, struct Coord **Src) {
  int Len = Src[1] - Src[0];

  if((*Dst) >= (*Src))
    Dst[1]  = Dst[0] + Len;
  memmove(*Dst, *Src, Len * sizeof(struct Coord));

  return *Dst;
}


int FindBlock(struct Coord *B, struct Coord *A, int Len) {
  for (;Len && (((B->x) != (A->x)) || ((B->y) != (A->y))); Len--, A++);
  return Len;
}

