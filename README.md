# Omnimino

Console game inspired by polymino puzzle


## Usage

omnimino infile


## dependency

ncurses(w)


## configurable prior to compile

#define MAX_FIGURE_WEIGHT 8

#define MAX_GLASS_HEIGHT 256


## gameplay

Random sequence of figures is placed inside rectangular glass, using movements, rotations and mirror (vertical).

Figure is a set of blocks with fixed ralative placement. Number of blocks in each figure can vary from FigureWeightMax down to FigureWeightMin (these are game parameters, defined in game record file).

Figures can be generated using aperture, or using natural neighbourship. This is defined by Aperture parameter value. If it is 0, figure blocks are choosed as neighbours. If it is non-zero, aperture is filled with independently placed blocks, which can be separated in space.

Another game figures' sequence parameter is Metrics. Metrics=0 means, that block have 4 neighbours, Metric=1 means 8 neighbours. This is for Aperture=0 case.

If Aperture is non-zero, Metric=1 means square aperture with side length of value of Aperture parameter, in case Metric=0 this square loose its corners.


Examples of Aperture=5


Metric=1

1 1 1 1 1\
1 1 1 1 1\
1 1 1 1 1\
1 1 1 1 1\
1 1 1 1 1


Metric=0

0 0 1 0 0\
0 1 1 1 0\
1 1 1 1 1\
0 1 1 1 0\
0 0 1 0 0


So parameters, influencing random figures sequence are:


Aperture (0-MAX_FIGURE_WEIGHT)\
Metric (0,1)\
FigureWeightMax (1-MAX_FIGURE_WEIGHT)\
FigureWeightMin (1-FigureWeightMax)


Group of boolean parameters influences the gameplay. They are:


Gravity,SingleLayer,FullRowClear and Goal. 


Goal may be:


0 - fill glass in order to minimize empty cells number\
1 - touch the glass bottom (means glass prefill, see later)\
2 - get flat upper row.


The next parameter group describes glass:


GlassWidth\
GlassHeight\
GlassFillLevel\
GlassFillRatio (number of blocks in each prefilled row)


Additional parameter SlotsUnique influences the figure generation in case Aperture=0 only.


The set of 13 parameters is used to create game record file. 

Game record file consists of parameters, glass prefill state, and figures, sufficient to fill the glass up to top.
Sum of all figures' weights is >= (GlassWidth * GlassHeight) - (FillLevel * FillRatio)

Game ends if the figure was placed outside the glass (at least one of figure cubes is above the glass top) in any case. Or if additional goal (touch or flat) is achieved.

## Score

Depends on goal.

- Fill. Number of empty cells in the glass

- Touch. Number of figure, that touched the glass bottom

- Flat. Number of figure, which made the top row flat.

Score is the less, the better.

Short status is present in the right-bottom corner of the screen. '+' means that game goal is reached. '-' means that game is over - some blocks were placed above the glass top. Number represents estimated game score according to game settings.

## Controls

h - move left\
l - move right\
j - move down\
k - move up

a - rotate ccw\
s,d - vertical mirror\
f - rotate cw

SPACE - drop figure

u - undo move\
r - redo move\
^ - go to first figure\
$ - go to last modified figure

q - exit without save\
x - exit with save

## Format of .mino record file

Line No    Parameter or data

1          Aperture\
2          Metric\
3          FigureWeightMax\
4          FigureWeightMin\
5          Gravity\
6          SingleLayer\
7          FullRowClear\
8          Goal\
9          GlassWidth\
10         GlassHeight\
11         FillLevel\
12         FillRatio\
13         SlotsUnique\
14         ParentName\
15         Figure number\
16         Current figure\
17         Glass prefill rows, delimited with ;\
18         Figures' first block numbers, deliimited with ;\
19         Blocks coordinates. x,y;\
20         Player $USER\
21         Time of save

All data are in decimal representation.

The name of game record file is md5sum of its content.\
If file name is not equal to md5sum of its content, it is considered as preset for new game.\
For preset files parameters may be commented, without any delimiters. Only the first word is interpreted as data, the rest of line is ignored.\
play.preset assumes preset files to be names as p*.mino


