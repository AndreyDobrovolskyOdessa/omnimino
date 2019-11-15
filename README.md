# omnimino

Console game inspired by polymino puzzle


# dependency

ncurses(w)


# configurable prior to compile

#define MAX_FIGURE_WEIGHT 8

#define MAX_GLASS_HEIGHT 256


# gameplay

Random sequence of figures is placed inside rectangular glass, using movements, rotations and mirror (vertical).

Figure is a set of blocks with fixed ralative placement. Number of blocks in each figure can vary from FigureWeightMax down to FigureWeightMin (these are game parameters, defined in game record file).

Figures can be generated using aperture, or using natural neighbourship. This is defined by Aperture parameter value. If it is 0, figure blocks are choosed as neighbours. If it is non-zero, aperture is filled with independently placed blocks, which can be separated in space.

Another game figures' sequence parameter is Metrics. Metrics=0 means, that black have 4 neighbours, Metric=1 means 8 neighbours. This is for Aperture=0 case.

If Aperture is non-zero, Metric=1 means square aperture with side length of value of Aperture parameter, in case Metric=0 this square loose its corners.


Examples of Aperture=5


Metric=0       Metric=1


1 1 1 1 1      0 0 1 0 0

1 1 1 1 1      0 1 1 1 0

1 1 1 1 1      1 1 1 1 1

1 1 1 1 1      0 1 1 1 0

1 1 1 1 1      0 0 1 0 0


So parameters, influencing random figures sequence are:


Aperture (1-MAX_FIGURE_WEIGHT)

Metric (0,1)

FigureWeightMax (1-MAX_FIGURE_WEIGHT)

FigureWeightMin (1-FigureWeightMax)


Group of boolean parameters influences the gameplay. They are:


Gravity,SingleLayer,FullRowClear and Goal. 


Goal may be:


0 - fill glass in order to minimize empty cells number

1 - touch the glass bottom (means glass prefill, see later)

2 - get flat upper row.


The next parameter group describes glass:


GlassWidth

GlassHeight

GlassFillLevel

GlassFillRatio (number of blocks in each prefilled row)


Additional parameter SlotsUnique influences the figure generation in case Aperture=0 only.


The set of 13 parameters is used to create game record file. 

Game record file consists of parameters, glass prefill state, and figures, sufficient to fill the glass up to top.
Sum of all figures' weights is >= (GlassWidth * GlassHeight) - (FillLevel * FillRatio)

Game ends if the figure was placed outside the glass (at least one of figure cubes is above the glass top) in any case. Or if additional goal (touch or flat) is achieved.

# Score

Depends on goal.


- Fill. Number of empty cells in the glass

- Touch. Number of figure, that touched the glass bottom

- Flat. Number of figure, which made the top row flat.


Score is the less, the better.

# Commands


h - move left

l - move right

j - move down

k - move up


a - rotate ccw

g - rotate cw

s,d - vertical mirror


SPACE - drop figure


u - undo move

r - redo move

^ - go to first figure

$ - go to last modified figure


q - exit without save

x - exit with save

