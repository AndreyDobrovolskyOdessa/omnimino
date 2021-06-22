/******************************************************************************
	Omnimino is console version of polymino puzzle.
	Figure sizes, shapes, field size and gameplay options can be varied.

	Copyright (C) 2019-2021 Andrey Dobrovolsky

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
******************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <libgen.h>


#include <ncurses.h>

struct Coord{
  int x,y;
};

/**************************************

           Constants

**************************************/
#define MAX_FIGURE_SIZE 8

#define MAX_GLASS_WIDTH 32 /*UINT_WIDTH*/
#define MAX_GLASS_HEIGHT 256

enum GoalTypes {
  FILL_GOAL,
  TOUCH_GOAL,
  FLAT_GOAL,
  MAX_GOAL
};


/**************************************

           Game parameters

**************************************/

int FigureWeightMax;
int FigureWeightMin;
int Aperture;

int FigureMetric; /* 0 - abs(x1-x0)+abs(y1-y0), 1 - max(abs(x1-x0),abs(y1-y0)) */

int Gravity;
int FlatFun;      /* single-layer, moving figure can not overlap placed blocks */
int FullRowClear;

int Goal;

int GlassWidth;
int GlassHeightBuf; /* can change during game if FullRowClear */
int GlassFillLevel;
int GlassFillRatio; /* < GlassWidth */

int FigureSlotsUnique;

/**************************************

           Game data

**************************************/

int FigureNum;

int CurFigure;

#define GLASS_SIZE (MAX_GLASS_HEIGHT+MAX_FIGURE_SIZE+1)

unsigned int GlassRowBuf[GLASS_SIZE];

#define MAX_BLOCK_NUM ((MAX_GLASS_WIDTH*MAX_GLASS_HEIGHT)+(2*MAX_FIGURE_SIZE))

int BlockN[MAX_BLOCK_NUM+2];

struct Coord Block[MAX_BLOCK_NUM];

#define OM_STRLEN 80
char ParentName[OM_STRLEN]="none";

/**************************************

           Game variables

**************************************/

int FigureSize;

unsigned int GlassRow[GLASS_SIZE];

int NextFigure; /* used to navigate through figures */
int Untouched; /* lowest unmodified */

int GlassHeight; /* can change during game if FullRowClear */
int GlassLevel;	/* lowest free line */
unsigned int FullRow; /* template, defined once per game, depends on GlassWidth */
int GameOver;
int GoalReached;
int GameModified=0;


/**************************************

           Basic game functions

**************************************/

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

void ScaleUp(struct Coord *B,int *V){
  (B->x)<<=1; (B->y)<<=1;
}

void NegX(struct Coord *B,int *V){
  (B->x)=-(B->x);
}

void PlaceIntoGlass(struct Coord *B, int *V){
  GlassRow[(B->y)>>1] |= (1<<((B->x)>>1));
}

#pragma GCC diagnostic pop

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

void FitWidth(struct Coord *B, int *Err){
  (*Err) |= ( (((B->x)>>1) < 0) || (((B->x)>>1) >= GlassWidth) );
}

void FitHeight(struct Coord *B, int *Err){
  (*Err) |= ( (((B->y)>>1) < 0) || (((B->y)>>1) >= (GlassHeight+FigureSize+1)) );
}

void FitGlass(struct Coord *B, int *Err){
  FitWidth(B,Err);
  FitHeight(B,Err);
}

void AndGlass(struct Coord *B, int *Err){
  (*Err) |= ( GlassRow[(B->y)>>1] & (1<<((B->x)>>1)) );
}

int ForEachIn(int FN, void (*Func)(struct Coord *, int *), int V){
  int i;

  for(i=BlockN[FN];i<BlockN[FN+1];i++)
    (*Func)(Block+i,&V);

  return(V);
}

int CenterH(int FN){
  return((ForEachIn(FN,FindLeft,INT_MAX)+ForEachIn(FN,FindRight,INT_MIN))>>1);
}

int CenterV(int FN){
  return((ForEachIn(FN,FindBottom,INT_MAX)+ForEachIn(FN,FindTop,INT_MIN))>>1);
}

void Normalize(int FN,struct Coord *C){
  (C->x)=CenterH(FN);
  (C->y)=CenterV(FN);
  ForEachIn(FN,AddX,-(C->x));
  ForEachIn(FN,AddY,-(C->y));
}

int CopyFigure(int DestN,int SrcN){
  int Len;

  Len=BlockN[SrcN+1]-BlockN[SrcN];
  if(DestN>SrcN)
    BlockN[DestN+1]=BlockN[DestN]+Len;
  memcpy(Block+BlockN[DestN],Block+BlockN[SrcN],Len*sizeof(struct Coord));

  return(DestN);
}


int Unfilled(void){
  int i,s;
  unsigned int r;

  for(i=0,s=0;i<GlassHeight;i++){
    for(r=(~GlassRow[i])&FullRow;r;r>>=1){
      if(r&1)
        s++;
    }
  }

  return(s);
}


void DetectGlassLevel(void){
  GlassLevel=GlassHeight+FigureSize+1;
  while((GlassLevel>0)&&(GlassRow[GlassLevel-1]==0))
    GlassLevel--;
}


/**************************************

           New game functions

**************************************/

void FillGlass(void){
  int Places,Blocks;

  memset(GlassRowBuf,0,sizeof(GlassRowBuf));
  for(GlassLevel=0;GlassLevel<GlassFillLevel;GlassLevel++){
    for(Places=GlassWidth,Blocks=GlassFillRatio;Places>0;Places--){
      GlassRowBuf[GlassLevel]<<=1;
      if((rand()%Places)<Blocks){
        GlassRowBuf[GlassLevel]|=1;Blocks--;
      }
    }
  }
}


int FindBlock(struct Coord *B,struct Coord *A,int Len){
  for(;Len&&(((B->x)!=(A->x))||((B->y)!=(A->y)));Len--,A++);
  return(Len);
}

int SelectSlots(struct Coord *Slot,struct Coord *FBlock,int Weight){
  int SlotNum=0,ApertureSize,HalfSize,SeedCnt,x,y,Delta;

  ApertureSize=(Aperture==0)?(Weight?3:1):Aperture;
  HalfSize=ApertureSize/2;
  for(SeedCnt=0;SeedCnt<(((Aperture==0)&&Weight)?Weight:1);SeedCnt++){
    for(x=-HalfSize;x<(ApertureSize-HalfSize);x++){
      Delta=FigureMetric?0:abs(x);
      for(y=-HalfSize+Delta;y<(ApertureSize-HalfSize-Delta);y++){
        Slot[SlotNum].x=x+(((Aperture==0)&&Weight)?FBlock[SeedCnt].x:0);
        Slot[SlotNum].y=y+(((Aperture==0)&&Weight)?FBlock[SeedCnt].y:0);
        if(((Weight>=FigureWeightMin)||(!FindBlock(Slot+SlotNum,FBlock,Weight)))&&
           ((!FigureSlotsUnique)||(!FindBlock(Slot+SlotNum,Slot,SlotNum))))
          SlotNum++;
      }
    }
  }

  return(SlotNum);
}

#define MAX_SLOTS (MAX_FIGURE_SIZE*MAX_FIGURE_SIZE)
#define MAX_SLOTS_COMPACT ((MAX_FIGURE_SIZE-1)*9)

#if MAX_SLOTS_COMPACT>MAX_SLOTS
#define MAX_SLOTS MAX_SLOTS_COMPACT
#endif

int NewFigure(struct Coord *F){
  int i;
  struct Coord Slot[MAX_SLOTS],*B;
  
  for(i=0,B=F;i<FigureWeightMax;i++){
    memcpy(B,Slot+(rand()%SelectSlots(Slot,F,B-F)),sizeof(struct Coord));
    if(!FindBlock(B,F,B-F))
      B++;
  }
  return(B-F);
}


/**************************************

             NewGame

**************************************/

int NewGame(void){
  int TotalArea;


  srand((unsigned int)time(NULL));

  FillGlass();

  TotalArea=(GlassWidth*GlassHeightBuf)-(GlassFillLevel*GlassFillRatio);
  for(FigureNum=0,BlockN[0]=0;BlockN[FigureNum]<TotalArea;FigureNum++){
    BlockN[FigureNum+1]=BlockN[FigureNum]+NewFigure(Block+BlockN[FigureNum]);
    ForEachIn(FigureNum,ScaleUp,0);
  }

  CurFigure=1; Untouched=NextFigure=0;

  return(0);
}


/*     Errors    */

enum OM_Errors{
  OM_OK=0,
  OM_NEWWIN_ERROR,
  OM_GLASS_STATE_ERROR,
  OM_SAVE_GAME_ERROR,
  OM_LOAD_GAME_PAR_ERROR,
  OM_LOAD_GAME_DATA_ERROR,
  OM_MISSING_INPUT_FILE,
  OM_NEW_GAME_NOT_ALLOWED,
  OM_ERROR_MAX
};

int OM_Error=OM_OK;

char *OM_Message[]={
  "Ok",
  "ncurses error: newwin(0,0,0,0) failed",
  "PlayGame error: GetGlassState : figure out of glass or overlapping",
  "SaveGame error",
  "Load game parameters error",
  "Load game data error",
  "Missing input file",
  "New games allowed in interactive mode only",
  ""
};

/**************************************

	Screen functions
           
**************************************/

WINDOW *MyScr=NULL;


void StartCurses(void){
  initscr();
  noecho();
  raw();
  curs_set(0);
  keypad(stdscr,TRUE);
}


#define MAX_ROW_LEN ((MAX_GLASS_WIDTH+2)*2)

void PutRowImage(unsigned int Row,char *Image,int N){
  for(;N--;Row>>=1){
    *Image++=(Row&1)?'[':' ';
    *Image++=(Row&1)?']':' ';
  }
}

void DrawGlass(int GlassRowN){
  int RowN;
  char RowImage[MAX_ROW_LEN];

  GlassRowN-=(getmaxy(MyScr)-1);
  for(RowN=getmaxy(MyScr)-1;RowN>=0;RowN--,GlassRowN++){
    memset(RowImage,(GlassRowN>=GlassHeight)?' ':'#',MAX_ROW_LEN);
    if(GlassRowN>=0)
      PutRowImage(GlassRow[GlassRowN],RowImage+2,GlassWidth);
    mvwaddnstr(MyScr,RowN,0,RowImage,(GlassWidth+2)*2);
  }
}

int SelectGlassRow(void){
  int GlassRowN,FCV;

  FCV=CenterV((CurFigure<FigureNum)?CurFigure:(CurFigure-1))>>1;

  GlassRowN=FCV+(FigureSize/2)+1;
  if(GlassLevel>GlassRowN){
    GlassRowN=GlassLevel;
    if(GlassLevel>=getmaxy(MyScr)){
      if(GlassLevel>(FCV+(getmaxy(MyScr)/2))){
        GlassRowN=FCV+(getmaxy(MyScr)/2);
        if((getmaxy(MyScr)/2)>FCV){
          GlassRowN=getmaxy(MyScr)-1;
        }
      }
    }
  }

  return(GlassRowN);
}

void DrawBlock(struct Coord *B,int *GRN){
  mvwchgat(MyScr,(*GRN)-((B->y)>>1) ,((B->x)&(~1))+2,2,A_REVERSE,0,NULL);
}


void DrawQueue(void){
  int LeftMargin,PlacesH,PlacesV,x,y,N;
  struct Coord C;


  LeftMargin=(GlassWidth+2)*2;
  PlacesH=(getmaxx(MyScr)-LeftMargin)/(2*(FigureSize+2));
  PlacesV=getmaxy(MyScr)/(FigureSize+2);

  LeftMargin+=FigureSize+2;

  for(x=0,N=(CurFigure+1);(N<FigureNum)&&(x<PlacesH);x++){

    for(y=0;(N<FigureNum)&&(y<PlacesV);y++,N++){

      CopyFigure(FigureNum,N);
      Normalize(FigureNum,&C);

      ForEachIn(FigureNum,AddX,LeftMargin+(2*x*(FigureSize+2)));
      ForEachIn(FigureNum,AddY,-(FigureSize+2)-(2*y*(FigureSize+2))+1);

      ForEachIn(FigureNum,DrawBlock,0);

    }

  }
  
}


#define STATUS_MARK_POS 0
#define STATUS_MARK_LEN 1

#define STATUS_SCORE_POS 1
#define STATUS_SCORE_LEN 4

#define STATUS_LEN (STATUS_MARK_LEN+STATUS_SCORE_LEN)


void DrawStatus(void){
  char StatusMark[STATUS_MARK_LEN+1];
  char StatusScore[STATUS_SCORE_LEN+1];
  int CurScore = (Goal==FILL_GOAL)?Unfilled():CurFigure;
  int StatusY = getmaxy(MyScr)-1;
  int StatusX = getmaxx(MyScr)-STATUS_LEN;

  StatusMark[0]=GoalReached?'+':(GameOver?'-':' ');
  snprintf(StatusScore,STATUS_SCORE_LEN+1,"%-*d",STATUS_SCORE_LEN,CurScore);

  mvwaddnstr(MyScr,StatusY,StatusX+STATUS_MARK_POS,StatusMark,STATUS_MARK_LEN);
  mvwaddnstr(MyScr,StatusY,StatusX+STATUS_SCORE_POS,StatusScore,STATUS_SCORE_LEN);
}

int ShowScreen(void){
  int GlassRowN;

  if(MyScr==NULL){
    if((MyScr=newwin(0,0,0,0))==NULL){
      OM_Error=OM_NEWWIN_ERROR; return(1);
    }
    refresh();
  }

  werase(MyScr);

  if((getmaxx(MyScr)<((GlassWidth+2)*2))||(getmaxy(MyScr)<(MAX_FIGURE_SIZE*2))){
    mvwaddstr(MyScr,0,0,"Screen too small");
  }else{
    GlassRowN=SelectGlassRow();
    DrawGlass(GlassRowN);
    if(CurFigure<FigureNum)
      ForEachIn(CurFigure,DrawBlock,GlassRowN);
    DrawQueue();
    DrawStatus();
  }

  wrefresh(MyScr);

  return(0);
}

/**************************************

           Various game functions

**************************************/

void Drop(int FigN){
  CopyFigure(FigureNum,FigN);
  if(Gravity){
    if(!FlatFun){
      ForEachIn(FigureNum,AddY,-(ForEachIn(FigureNum,FindBottom,INT_MAX)&(~1)));
      while((ForEachIn(FigureNum,FitGlass,0)==0)&&(ForEachIn(FigureNum,AndGlass,0)!=0))
        ForEachIn(FigureNum,AddY,2);
    }else{
      while((ForEachIn(FigureNum,FitGlass,0)==0)&&(ForEachIn(FigureNum,AndGlass,0)==0))
        ForEachIn(FigureNum,AddY,-2);
      ForEachIn(FigureNum,AddY,2);
    }
  }
  ForEachIn(FigureNum,PlaceIntoGlass,0);
  DetectGlassLevel();
}

void ClearFullRows(void){
  int r,w,FullRowNum;

  for(r=w=0;r<GlassHeight;r++){
    if(GlassRow[r]!=FullRow)
      GlassRow[w++]=GlassRow[r];
  }

  FullRowNum=r-w;

  for(;w<(GlassHeight+FigureSize+1);w++)
    GlassRow[w]=0;

  GlassHeight-=FullRowNum;
  GlassLevel-=FullRowNum;
}

void Deploy(int FN,int NewX,int NewY){
  struct Coord C;

  Normalize(FN,&C);
  ForEachIn(FN,AddX,NewX<<1);
  ForEachIn(FN,AddY,NewY<<1);
}

void CheckGame(void){

  switch(Goal){
    case TOUCH_GOAL:
      if((ForEachIn(FigureNum,FindBottom,INT_MAX)>>1)==0)
        GoalReached=1;
      break;
    case FLAT_GOAL:
      if((GlassLevel==0)||(GlassRow[GlassLevel-1]==FullRow))
        GoalReached=1;
      break;
    default:
      if(Unfilled()==0)
        GoalReached=1;
  }

  if(GoalReached||(GlassLevel>GlassHeight))
    GameOver=1;
}

int GetGlassState(void){

  if(NextFigure<CurFigure){
    memcpy(GlassRow,GlassRowBuf,sizeof(GlassRow));
    GlassHeight=GlassHeightBuf;
    DetectGlassLevel();
    CurFigure=0;
    GameOver=0;
    GoalReached=0;
  }

  while(CurFigure<NextFigure){
    if((ForEachIn(CurFigure,FitGlass,0)!=0)||(ForEachIn(CurFigure,AndGlass,0)!=0)){
      OM_Error=OM_GLASS_STATE_ERROR; return(1);
    }
    Drop(CurFigure++);
    CheckGame();
    if(GameOver){
        NextFigure=CurFigure;
        break;
    }
    if(FullRowClear)
      ClearFullRows();
  }

  if((CurFigure<FigureNum)&&(CurFigure>=Untouched)){
    Deploy(CurFigure,GlassWidth/2,GlassLevel+(FigureSize/2)+1);
    Untouched=CurFigure+1;
    GameModified=1;
  }

  return(0);
}

/**************************************

           PlayGame

**************************************/

int ExitWithSave(void){
  return(0);
}

int ExitWithoutSave(void){
  GameModified=0;
  return(0);
}

void MoveBufLeft(void){
  ForEachIn(FigureNum,AddX,-2);
}

void MoveBufRight(void){
  ForEachIn(FigureNum,AddX,2);
}

void MoveBufDown(void){
  ForEachIn(FigureNum,AddY,-2);
}

void MoveBufUp(void){
  ForEachIn(FigureNum,AddY,2);
}

void CompoundOp(void (*F)(struct Coord *,int *)){
  struct Coord C;

  Normalize(FigureNum,&C);
  ForEachIn(FigureNum,F,0);
  ForEachIn(FigureNum,AddX,C.x);
  ForEachIn(FigureNum,AddY,C.y);
}

void MirrorBufVert(void){
  CompoundOp(NegX);
}

void RotateBufCW(void){
  CompoundOp(RotCW);
}

void RotateBufCCW(void){
  CompoundOp(RotCCW);
}

int Attempt(void (*F)(void)){
  if(!GameOver){
    CopyFigure(FigureNum,CurFigure);
    (*F)();
    if((ForEachIn(FigureNum,FitGlass,0)==0)&&
       ((!FlatFun) || (ForEachIn(FigureNum,AndGlass,0)==0))){
      CopyFigure(CurFigure,FigureNum);
      Untouched=CurFigure+1;
      GameModified=1;
    }
  }
  return(1); 
}

int MoveCurLeft(void){
  return(Attempt(MoveBufLeft));
}

int MoveCurRight(void){
  return(Attempt(MoveBufRight));
}

int MoveCurDown(void){
  return(Gravity?1:Attempt(MoveBufDown));
}

int MoveCurUp(void){
  return(Gravity?1:Attempt(MoveBufUp));
}

int RotateCurCW(void){
  return(Attempt(RotateBufCW));
}

int RotateCurCCW(void){
  return(Attempt(RotateBufCCW));
}

int MirrorCurVert(void){
  return(Attempt(MirrorBufVert));
}

int DropCur(void){
  if((!GameOver)&&
     (ForEachIn(CurFigure,FitGlass,0)==0)&&
     (ForEachIn(CurFigure,AndGlass,0)==0)){
    NextFigure=CurFigure+1;
  }
  return(1);
}

int ScreenResize(void){

  delwin(MyScr); MyScr=NULL;

  return(1);
}

int UndoFigure(void){
  if(CurFigure>0)
    NextFigure=CurFigure-1;
  return(1);
}

int RedoFigure(void){
  if((CurFigure+1)<Untouched)
    NextFigure=CurFigure+1;
  return(1);
}

int Rewind(void){
  NextFigure=0;  return(1);
}

int LastPlayed(void){
  NextFigure=Untouched-1;  return(1);
}

struct KBinding{
  int Key;
  int (*Action)(void);
} KBindList[]={
  {'q',ExitWithoutSave},
  {'x',ExitWithSave},
  {'h',MoveCurLeft},
  {'l',MoveCurRight},
  {'k',MoveCurUp},
  {'j',MoveCurDown},
  {KEY_LEFT,MoveCurLeft},
  {KEY_RIGHT,MoveCurRight},
  {KEY_UP,MoveCurUp},
  {KEY_DOWN,MoveCurDown},
  {'a',RotateCurCCW},
  {'f',RotateCurCW},
  {'s',MirrorCurVert},
  {'d',MirrorCurVert},
  {' ',DropCur},
  {'^',Rewind},
  {'$',LastPlayed},

  {'u',UndoFigure},
  {'r',RedoFigure},

  {KEY_RESIZE,ScreenResize},

  {0,NULL}
};


int GetCmd(void){
  int Key;
  struct KBinding *P;
  int (*Func)(void)=NULL;

  while(Func==NULL){
    Key=getch();
    for(P=KBindList;((Func=(P->Action))!=NULL)&&(Key!=(P->Key));P++);
  }

  return((*Func)());
}


int Score(void){
  return((Goal==FILL_GOAL)?Unfilled():(GoalReached?CurFigure:FigureNum));
}


int PlayGame(void){
  int Tty=isatty(fileno(stdout));

  FullRow=((1<<(GlassWidth-1))<<1)-1;

  FigureSize=(Aperture==0)?FigureWeightMax:Aperture;

  if(Tty)
    StartCurses();

  while((GetGlassState()==0)&&Tty&&(ShowScreen()==0)&&GetCmd());

  if(Tty)
    endwin();
  else if(GameModified)
    OM_Error=OM_NEW_GAME_NOT_ALLOWED;

  fprintf(stdout,"%d\n",Score());

  return(OM_Error);
}


/**************************************

           SaveGame

**************************************/

int *GameParAddr[]={
  &Aperture,
  &FigureMetric,
  &FigureWeightMax,
  &FigureWeightMin,
  &Gravity,
  &FlatFun,
  &FullRowClear,
  &Goal,
  &GlassWidth,
  &GlassHeightBuf,
  &GlassFillLevel,
  &GlassFillRatio,
  &FigureSlotsUnique,
  NULL
};

char *GameParName[]={
  "Aperture",
  "FigureMetric",
  "FigureWeightMax",
  "FigureWeightMin",
  "Gravity",
  "FlatFun",
  "FullRowClear",
  "Goal",
  "GlassWidth",
  "GlassHeight",
  "GlassFillLevel",
  "GlassFillRatio",
  "SlotsUnique",
  NULL
};

#define OM_HASH_LEN 80
#define OM_MAX_PATH 1024

char OM_ext[]=".mino";
char OM_tmp[]="tmp.mino";

int GetHash(char *FileName, char *HashStr){
  FILE *f;
  char Buf[10+OM_MAX_PATH]="md5sum ",Fmt[20];

  if(strlen(FileName)>=OM_MAX_PATH){
    fprintf(stderr,"file name too long : %s\n",FileName); return(1);
  }

  strcat(Buf,FileName);

  if((f=popen(Buf,"r"))==NULL){
    fprintf(stderr,"can't open pipe '%s'\n",Buf); return(1);
  }

  sprintf(Fmt,"%%%ds%%*[^\\n]",OM_HASH_LEN-1-((int)sizeof(OM_ext)));

  if(fscanf(f,Fmt,HashStr)!=1){
    fprintf(stderr,"error reading hash from pipe.\n"); return(1);
  }

  strcat(HashStr,OM_ext);

  if(pclose(f)!=0){
    fprintf(stderr,"error closing pipe.\n"); return(1);
  }

  return(0);
}



int SaveGame(void){
  FILE *fout;
  int i;
  char HashStr[OM_HASH_LEN];

  if(!GameModified)
    return(0);

  OM_Error=OM_SAVE_GAME_ERROR;

  if((fout=fopen(OM_tmp,"w"))==NULL){
    fprintf(stderr,"can't open file for write : %s\n",OM_tmp); return(1);
  }

  for(i=0;(ferror(fout)==0)&&(GameParAddr[i]!=NULL);i++)
    fprintf(fout,"%u\n",*GameParAddr[i]);

  if(ferror(fout)==0) fprintf(fout,"%s\n",ParentName);
  if(ferror(fout)==0) fprintf(fout,"%u\n",FigureNum);
  if(ferror(fout)==0) fprintf(fout,"%u\n",CurFigure);

  for(i=0;(ferror(fout)==0)&&(i<GlassFillLevel);i++)
    fprintf(fout,"%u;",GlassRowBuf[i]);
  if(ferror(fout)==0) fprintf(fout,"\n");

  for(i=0;(ferror(fout)==0)&&(i<=FigureNum);i++)
    fprintf(fout,"%u;",BlockN[i]);
  if(ferror(fout)==0) fprintf(fout,"\n");

  for(i=0;(ferror(fout)==0)&&(i<BlockN[FigureNum]);i++)
    fprintf(fout,"%d,%d;",Block[i].x,Block[i].y);
  if(ferror(fout)==0) fprintf(fout,"\n");

  if(ferror(fout)==0) fprintf(fout,"%s\n",getenv("USER"));
  if(ferror(fout)==0) fprintf(fout,"%u\n",(unsigned int)time(NULL));

  if(ferror(fout)!=0){
    fprintf(stderr,"write data error.\n"); fclose(fout); return(1);
  }

  if(fclose(fout)!=0){
    fprintf(stderr,"close file error\n"); return(1);
  }

  if(GetHash(OM_tmp,HashStr)!=0){
    return(1);
  }

  if(rename(OM_tmp,HashStr)==-1){
    fprintf(stderr,"rename error.\n"); return(1);
  }

  OM_Error=OM_OK;

  return(0);
}


/**************************************

           LoadGame

**************************************/


int CheckParameters(void){
  int i,Area,Err=0;

  if(Aperture>MAX_FIGURE_SIZE){
    fprintf(stderr,"[1] Aperture (%d) > MAX_FIGURE_SIZE (%d).\n",Aperture,MAX_FIGURE_SIZE);
    Err=1;
  }

  if(FigureMetric>1){
    fprintf(stderr,"[2] FigureMetric (%d) can be 0 or 1 .\n",FigureMetric);
    Err=1;
  }

  if(FigureWeightMax>MAX_FIGURE_SIZE){
    fprintf(stderr,"[3] FigureWeightMax (%d) > MAX_FIGURE_SIZE (%d).\n",FigureWeightMax,MAX_FIGURE_SIZE);
    Err=1;
  }

  if(FigureWeightMin<1){
    fprintf(stderr,"[4] FigureWeightMin (%d) < 1 .\n",FigureWeightMin);
    Err=1;
  }

  if(FigureWeightMin>FigureWeightMax){
    fprintf(stderr,"[4] FigureWeightMin (%d) > [3] FigureWeightMax (%d).\n",FigureWeightMin,FigureWeightMax);
    Err=1;
  }

  if(Aperture!=0){
    Area=Aperture*Aperture;
    if(FigureMetric==0){
      if((Aperture%2)==0)
        Area-=Aperture;
      for(i=1;i<((Aperture-1)/2);i++)
        Area-=4*i;
    }
    if(Area<FigureWeightMin){
      fprintf(stderr,"[4] FigureWeightMin (%d) > Aperture Area (%d).\n",FigureWeightMin,Area);
      Err=1;
    }
  }

  if(Gravity>1){
    fprintf(stderr,"[5] Gravity (%d) can be 0 or 1 .\n",Gravity);
    Err=1;
  }

  if(FlatFun>1){
    fprintf(stderr,"[6] FlatFun (%d) can be 0 or 1 .\n",FlatFun);
    Err=1;
  }

  if(FullRowClear>1){
    fprintf(stderr,"[7] FullRowClear (%d) can be 0 or 1 .\n",FullRowClear);
    Err=1;
  }

  if(Goal>=MAX_GOAL){
    fprintf(stderr,"[8] Goal (%d) > 2 .\n",Goal);
    Err=1;
  }

  FigureSize=(Aperture==0)?FigureWeightMax:Aperture;

  if(GlassWidth<FigureSize){
    fprintf(stderr,"[9] GlassWidth (%d) < FigureSize (%d).\n",GlassWidth,FigureSize);
    Err=1;
  }

  if(GlassWidth>MAX_GLASS_WIDTH){
    fprintf(stderr,"[9] GlassWidth (%d) > MAX_GLASS_WIDTH (%d).\n",GlassWidth,MAX_GLASS_WIDTH);
    Err=1;
  }

  if(GlassHeightBuf<FigureSize){
    fprintf(stderr,"[10] GlassHeight (%d) < FigureSize (%d).\n",GlassHeightBuf,FigureSize);
    Err=1;
  }

  if(GlassHeightBuf>MAX_GLASS_HEIGHT){
    fprintf(stderr,"[10] GlassHeight (%d) > MAX_GLASS_HEIGHT (%d).\n",GlassHeightBuf,MAX_GLASS_HEIGHT);
    Err=1;
  }

  if(GlassFillLevel>GlassHeightBuf){
    fprintf(stderr,"[11] GlassFillLevel (%d) > [10] GlassHeight (%d).\n",GlassFillLevel,GlassHeightBuf);
    Err=1;
  }

  if(GlassFillRatio<1){
    fprintf(stderr,"[12] GlassFillRatio (%d) < 1 .\n",GlassFillRatio);
    Err=1;
  }

  if(GlassFillRatio>=GlassWidth){
    fprintf(stderr,"[12] GlassFillRatio (%d) >= [9] GlassWidth (%d).\n",GlassFillRatio,GlassWidth);
    Err=1;
  }

  if(FigureSlotsUnique>1){
    fprintf(stderr,"[13] FigureSlotsUnique (%d) can be 0 or 1 .\n",FigureSlotsUnique);
    Err=1;
  }

  return(Err);
}

int LoadParameters(FILE *fin){
  int i;

  for(i=0;GameParAddr[i]!=NULL;i++){
    if(fscanf(fin,"%u%*[^\n]\n",GameParAddr[i])!=1){
      fprintf(stderr,"[%d] %s : load error.\n",i+1,GameParName[i]); return(1);
    }
  }

  return(0);
}

#define MAX_BLOCK ((MAX_GLASS_WIDTH*MAX_GLASS_HEIGHT)+MAX_FIGURE_SIZE)


int LoadGameData(FILE *fin){
  int i;
  char Fmt[80];

  sprintf(Fmt,"%%%ds\\n",OM_STRLEN);

  if(fscanf(fin,Fmt,ParentName)!=1){
    fprintf(stderr,"[19] ParentName : load error.\n"); return(1);
  }

  if(fscanf(fin,"%u",&FigureNum)!=1){
    fprintf(stderr,"[14] FigureNum : load error.\n"); return(1);
  }

  if(fscanf(fin,"%u",&CurFigure)!=1){
    fprintf(stderr,"[15] CurFigure : load error.\n"); return(1);
  }

  NextFigure=CurFigure;
  CurFigure=Untouched=NextFigure+1;

  for(i=0;i<GlassFillLevel;i++){
    if(fscanf(fin,"%u;",GlassRowBuf+i)!=1){
      fprintf(stderr,"[16] GlassRow[%d] : load error.\n",i); return(1);
    }
  }

  for(i=0;i<=FigureNum;i++){
    if(fscanf(fin,"%u;",BlockN+i)!=1){
      fprintf(stderr,"[17] BlockN[%d] : load error.\n",i); return(1);
    }
  }

  for(i=0;i<BlockN[FigureNum];i++){
    if(fscanf(fin,"%d,%d;",&(Block[i].x),&(Block[i].y))!=2){
      fprintf(stderr,"[18] Block[%d] : load error.\n",i); return(1);
    }
  }

  return(0);
}



int LoadGame(char *Name){
  FILE *fin;
  char HashStr[OM_HASH_LEN];

  OM_Error=OM_LOAD_GAME_PAR_ERROR;

  if(GetHash(Name,HashStr)!=0){
    return(1);
  }

  if((fin=fopen(Name,"r"))==NULL){
    fprintf(stderr,"can't open file '%s' for read.\n",Name); return(1);
  }

  if((LoadParameters(fin)!=0)||(CheckParameters()!=0)){
    fclose(fin); return(1);
  }

  OM_Error=OM_OK;

  if(strcmp(HashStr,basename(Name))!=0){
    fclose(fin); strcpy(ParentName,"none"); return(NewGame());
  }

  OM_Error=OM_LOAD_GAME_DATA_ERROR;

  if(LoadGameData(fin)!=0){
    fclose(fin); return(1);
  }

  fclose(fin);

  if(strcmp(ParentName,"none")==0)
    strcpy(ParentName,basename(Name));

  OM_Error=OM_OK;

  return(0);
}


int ShowUsage(void){
  fputs(
"Omnimino 0.2 Copyright (C) 2019-2021 Andrey Dobrovolsky\n\n\
Usage: omnimino infile\n\n",
  stderr);

  OM_Error=OM_MISSING_INPUT_FILE;

  return(1);
}


int main(int argc,char *argv[]){

  (void)(((argc>1)?LoadGame(argv[1]):ShowUsage()) || PlayGame() || SaveGame());

  fprintf(stderr,"%s\n",OM_Message[OM_Error]);

  return(OM_Error);
}

