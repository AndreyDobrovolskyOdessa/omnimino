# Omnimino


![Simple omnimino](/samples/3.png)


Console game inspired by polymino puzzle


## Usage

omnimino infile


## Build

You will need gcc, pkg-config and ncursesw-dev

    ./build.sh

Optionally You can use redo build system. Current .do files are written in Lua for https://github.com/AndreyDobrovolskyOdessa/redo-c version of https://github.com/leahneukirchen/redo-c.

    (. ./redo.do; redo ...) 


## configurable prior to compile

omnitype.h:

    #define MAX_FIGURE_SIZE 8

    #define MAX_GLASS_HEIGHT 256

omniplay.c:

    #define GOAL_SYM (":;=")

    #define LOOSE_SYM '-'

    #define WIN_SYM '+'

    #define BELOW_SYM '?' /* invisible part of the glass */

    #define THICKNESS 4 /* glass walls */


## gameplay

Random sequence of figures is placed inside rectangular glass, using movements, rotations, mirror (vertical) and circlar shifts of the figures' queue (if enabled).

Figure is a set of blocks with fixed ralative placement. Number of blocks in each figure can vary from FigureWeightMax down to FigureWeightMin (these are game parameters, defined in game record file).

Figures can be generated using aperture, or using natural neighbourship. This is defined by Aperture parameter value. If it is 0, figure blocks are choosed as neighbours. If it is non-zero, aperture is filled with independently placed blocks, which can be separated in space.

Another game figures' sequence parameter is Metric. Metric = 0 means, that block have 4 neighbours, Metric=1 means 8 neighbours. This is for Aperture=0 case.

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


Gameplay options:


Gravity (0, 1)\
SingleLayer (0, 1)\
DiscardFullRows (0, 1)\
Goal (0 - 2)\
FixedSequence (0, 1)

Goal may be:


0 - fill glass in order to minimize empty cells number\
1 - touch the glass bottom (means glass prefill, see later)\
2 - get flat upper row.


The next parameter group describes glass:


GlassWidth\
GlassHeight\
GlassFillLevel\
GlassFillRatio (number of blocks in each prefilled row)

GlassFillRatio == 0 assumes that prefill values are present below in the .mino file (line 17) and will be constant, not created randomly (see "omnifill" utility).

The set of 13 parameters is used to create game record file. 

Game record file consists of parameters, glass prefill state, and figures, sufficient to fill the glass up to top.
Sum of all figures' weights is >= (GlassWidth * GlassHeight) - (FillLevel * FillRatio)

Game ends if the figure was placed outside the glass (at least one of figure cubes is above the glass top) in any case. Or if additional goal (touch or flat) is achieved.

You are being prompted about the current game goal and game status (game is over, additional goals reached) with the help of symbols used to draw the glass and 4 symbols, appearing in the left-bottom corner of the screen and meaning:

G - gravity on\
S - single layer mode\
D - full rows will be discarded\
F - figures' sequence is fixed

Default settings for the glass symbols are:

':' - fill\
';' - touch\
'=' - flat\
'-' - game is over\
'+' - success

You can change the defaults defined in omniplay.c according to Your taste before compiling.

An outer side of the right glass wall is used as the visibility stripe - invisible part of the glass below the screen is displayed as BELOW_SYM (default is '?').

## Score

Depends on goal.

- Fill. Number of empty cells in the glass

- Touch or Flat. Number of cubes played

Score is the less, the better. Number of free cells and cubes played are displayed in the right-bottom corner of the screen.


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

n - skip foward\
N,p - skip backward

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
7          DiscardFullRows\
8          Goal\
9          GlassWidth\
10         GlassHeight\
11         FillLevel\
12         FillRatio\
13         FixedSequence\
14         ParentName\
15         Figure number\
16         Current figure\
17         Glass prefill rows, delimited with ;\
18         Figures' first block numbers, delimited with ;\
19         Blocks coordinates. x,y;\
20         Player $USER\
21         Time of save

All data are in decimal representation.

The name of game record file is md5sum of its content.\
If file name is not equal to md5sum of its content, it is considered as preset for new game.\
For preset files parameters may be commented, without any delimiters. Only the first word is interpreted as data, the rest of line is ignored.


### minos.lua utility

Can be used to select .mino files according to their content. Command line parameters are some search keys, output (stdout) is the list of .mino files names, satisfying requested conditions. See source for details.

    ./omnimino $(./minos.lua p)

Manually select and play one of the existing presets.


    ./omnimino $(./minos.lua g)

Replay one of the existing recorded games.


    ./omnimino $(ls -t $(echo | ./minos.lua g 2>/dev/null) | head -n 1)

Replay the latest recorded game.


    rm -f $(echo | ./minos.lua e 2>/dev/null)

Remove all corrupted .mino files in the current directory.

    echo x | ./omnimino p1.mino

Create new game record instance according to p1.mino preset.

    echo x | ./omnimino p2.mino | cat

Create new game instance and see its description in Lua notation..


Although You can take a look at

    minos.lua -h

Of course You need Lua installed in Your system.


### omnifill

Utility intended for editing constant prefill of the preset files (FillRatio == 0). Uses any preset .mino file as the template. Editing takes place as omnimino game with single-block figures with Gravity = 0. In case of exit with save ('x' command) new preset file is written with the glass gathered field as the glass field preset. Retuns 0 in case valid preset file was successfully loaded, modified and successfully saved. In all other cases returns 1.

Usage:

    ./omnifill infile

Preset file is named according to its content md5 sum with ".preset" suffix added. You can change the newly created file name with the help of for example

    ./omnifill old.preset.mino && mv $(./fre.sh) new.preset.mino 


Andrey Dobrovolsky <andrey.dobrovolsky.odessa@gmail.com>


