#!/bin/sh

# This utility allows to choose among .mino files according to
# their content and write the list to stdout.
#
# $1 - desired type of .mino files.
#      "g[ame]": valid game records
#      "p[reset]": valid parameters
#      "e[rror]": damaged unusable files
#      "x": any
#
# $2 - string of up to 8 symbols, describing first 8 game parameters
#      in the following order:
#      Goal, Apperture, Metric, WeightMax, WeightMin, Gravity, SingleLayer,
#      ClearFullRows. "x" is placeholder
#
# $3 - GlassWidth
# $4 - GlassHeight
# $5 - FillLevel
# $6 - FillRatio
#
# Interactive output is directed to stderr. Each line shown represents
# the branch - group of .mino files with the same parent. Individual
# games are represented as their scores. Parent game is always the first
# in order. Branch can be selected by entering of its number (N).
# Individual game record can be selected with the second number, equal to
# its sequential number in the branch games list.
#
# Examples:
#
#    ./omnimino $(./minos p)
#         new game using one of the presets, selected interactively
#
#    rm -f $(echo | ./minos e 2>/dev/null)
#         remove all damaged .mino files
#
#    ./omnimino $(./minos p x 10)
#         select among presets with GlassWidth == 10
#
#    ./omnimino $(./minos g 2 x x x 8)
#         replay one of the recorded games with Goal == FLAT && FillRatio == 8
#

determine_keys() {
  MINOTYPE_KEY="$( echo $1 | cut -b 1 )"

  GOAL_KEY="$( echo $2 | cut -b 1 )"
  APERTURE_KEY="$( echo $2 | cut -b 2 )"
  METRIC_KEY="$( echo $2 | cut -b 3 )"
  FIGUREWEIGHTMAX_KEY="$( echo $2 | cut -b 4 )"
  FIGUREWEIGHTMIN_KEY="$( echo $2 | cut -b 5 )"
  GRAVITY_KEY="$( echo $2 | cut -b 6 )"
  FLATFUN_KEY="$( echo $2 | cut -b 7 )"
  FULLROW_KEY="$( echo $2 | cut -b 8 )"

  GLASSWIDTH_KEY="$3"
  GLASSHEIGHT_KEY="$4"
  FILLLEVEL_KEY="$5"
  FILLRATIO_KEY="$6"

  [ "$MINOTYPE_KEY" = "x" ] && MINOTYPE_KEY= 

  [ "$GOAL_KEY" = "x" ] && GOAL_KEY= 
  [ "$APERTURE_KEY" = "x" ] && APERTURE_KEY= 
  [ "$METRIC_KEY" = "x" ] && METRIC_KEY= 
  [ "$FIGUREWEIGHTMAX_KEY" = "x" ] && FIGUREWEIGHTMAX_KEY= 
  [ "$FIGUREWEIGHTMIN_KEY" = "x" ] && FIGUREWEIGHTMIN_KEY= 
  [ "$GRAVITY_KEY" = "x" ] && GRAVITY_KEY= 
  [ "$FLATFUN_KEY" = "x" ] && FLATFUN_KEY= 
  [ "$FULLROW_KEY" = "x" ] && FULLROW_KEY= 

  [ "$GLASSWIDTH_KEY" = "x" ] && GLASSWIDTH_KEY= 
  [ "$GLASSHEIGHT_KEY" = "x" ] && GLASSHEIGHT_KEY= 
  [ "$FILLLEVEL_KEY" = "x" ] && FILLLEVEL_KEY= 
  [ "$FILLRATIO_KEY" = "x" ] && FILLRATIO_KEY= 
}


parms_pass() {
  [ -n "$MINOTYPE_KEY" ] && [ "$MINOTYPE_KEY" != "$MINOTYPE" ] && return 1

  [ -n "$GOAL_KEY" ] && [ "$GOAL_KEY" != "$GOAL" ] && return 1
  [ -n "$APERTURE_KEY" ] && [ "$APERTURE_KEY" != "$APERTURE" ] && return 1
  [ -n "$METRIC_KEY" ] && [ "$METRIC_KEY" != "$METRIC" ] && return 1
  [ -n "$FIGUREWEIGHTMAX_KEY" ] && [ "$FIGUREWEIGHTMAX_KEY" != "$FIGUREWEIGHTMAX" ] && return 1
  [ -n "$FIGUREWEIGHTMIN_KEY" ] && [ "$FIGUREWEIGHTMIN_KEY" != "$FIGUREWEIGHTMIN" ] && return 1
  [ -n "$GRAVITY_KEY" ] && [ "$GRAVITY_KEY" != "$GRAVITY" ] && return 1
  [ -n "$FLATFUN_KEY" ] && [ "$FLATFUN_KEY" != "$FLATFUN" ] && return 1
  [ -n "$FULLROW_KEY" ] && [ "$FULLROW_KEY" != "$FULLROW" ] && return 1

  [ -n "$GLASSWIDTH_KEY" ] && [ "$GLASSWIDTH_KEY" -ne "$GLASSWIDTH" ] && return 1
  [ -n "$GLASSHEIGHT_KEY" ] && [ "$GLASSHEIGHT_KEY" -ne "$GLASSHEIGHT" ] && return 1
  [ -n "$FILLLEVEL_KEY" ] && [ "$FILLLEVEL_KEY" -ne "$FILLLEVEL" ] && return 1
  [ -n "$FILLRATIO_KEY" ] && [ "$FILLRATIO_KEY" -ne "$FILLRATIO" ] && return 1

  return 0
}

show_branch_parms(){
  [ -n "$MINOTYPE_KEY" ] && printf "  -" || printf "%3s" $MINOTYPE
  if [ "$MINOTYPE" = "e" ]
  then
    printf "  %s" $PARENT
  else
    [ -n "$GOAL_KEY" ] && printf "   -" || printf "%4d" $GOAL 
    [ -n "$APERTURE_KEY" ] && printf "   -" || printf "%4d" $APERTURE
    [ -n "$METRIC_KEY" ] && printf "   -" || printf "%4d" $METRIC
    printf "   ["
    [ -n "$FIGUREWEIGHTMAX_KEY" ] && printf "-" || printf "%1d" $FIGUREWEIGHTMAX
    printf "-"
    [ -n "$FIGUREWEIGHTMIN_KEY" ] && printf "-" || printf "%1d" $FIGUREWEIGHTMIN
    printf "]"
    [ -n "$GRAVITY_KEY" ] && printf "   -" || printf "%4d" $GRAVITY
    [ -n "$FLATFUN_KEY" ] && printf "   -" || printf "%4d" $FLATFUN
    [ -n "$FULLROW_KEY" ] && printf "   -" || printf "%4d" $FULLROW
    [ -n "$GLASSWIDTH_KEY" ] && printf "  --" || printf "%4d" $GLASSWIDTH
    printf "*"
    [ -n "$GLASSHEIGHT_KEY" ] && printf "-- " || printf "%-3d" $GLASSHEIGHT
    [ -n "$FILLLEVEL_KEY" ] && printf "  --" || printf "%4d" $FILLLEVEL
    printf "*"
    [ -n "$FILLRATIO_KEY" ] && printf "-- " || printf "%-3d" $FILLRATIO
  fi
}

clear_game_parms() {
  APERTURE="0";
  METRIC="0";
  FIGUREWEIGHTMAX="0";
  FIGUREWEIGHTMIN="0";
  GRAVITY="0";
  FLATFUN="0";
  FULLROW="0";
  GOAL="0";
  GLASSWIDTH="0" ;
  GLASSHEIGHT="0";
  FILLLEVEL="0";
  FILLRATIO="0";
  SLOTSUNIQUE="0";
}


read_game_parms() {
  read APERTURE REST;
  read METRIC REST;
  read FIGUREWEIGHTMAX REST;
  read FIGUREWEIGHTMIN REST;
  read GRAVITY REST;
  read FLATFUN REST;
  read FULLROW REST;
  read GOAL REST;
  read GLASSWIDTH REST;
  read GLASSHEIGHT REST;
  read FILLLEVEL REST;
  read FILLRATIO REST;
  read SLOTSUNIQUE REST;
}

make_index() {

  PARENT_PREV=

  for MINO in $(ls *.mino)
  do
    SCORE=$(./omnimino $MINO 2>/dev/null)
    case $? in
      0)
        PARENT=$(sed -n 14p $MINO)
        if [ "$PARENT" = "none" ]; then
          echo g $MINO 0 $MINO $SCORE
        else
          echo g $PARENT 1 $MINO $SCORE
        fi
        ;;
      7)
        echo p $MINO 0 $MINO $SCORE
        ;;
      *)
        echo e $MINO 0 $MINO 0
        ;;
    esac
  done | sort |

  while read MINOTYPE PARENT FLAG MINO SCORE
  do
    [ -z "$MINO" ] && continue
    if [ "$PARENT" != "$PARENT_PREV" ]; then
      [ -n "$PARENT_PREV" ] && echo
      PARENT_PREV=$PARENT
      printf "%1s " $MINOTYPE
      if [ "$MINOTYPE" = "e" ]
      then
        clear_game_parms
      else
        read_game_parms < $MINO
      fi
      printf "%1d %1d %1d %1d %1d %1d %1d %1d %2d %3d %3d %2d %1d %s "\
              $GOAL $APERTURE $METRIC $FIGUREWEIGHTMAX $FIGUREWEIGHTMIN\
              $GRAVITY $FLATFUN $FULLROW $GLASSWIDTH $GLASSHEIGHT\
              $FILLLEVEL $FILLRATIO $SLOTSUNIQUE $PARENT
    fi
    printf "%s %d " $MINO $SCORE
 done | sort 

}


read_branch_data() {
  read MINOTYPE GOAL APERTURE METRIC FIGUREWEIGHTMAX FIGUREWEIGHTMIN GRAVITY FLATFUN\
       FULLROW GLASSWIDTH GLASSHEIGHT FILLLEVEL FILLRATIO SLOTSUNIQUE\
       PARENT GAMELIST
}



determine_keys $@

GAMES=$(mktemp)

COUNT=1

echo >&2
echo "  N  Type  G   A   M   Weight  Gr  S   C  Glass    Fill  Scores">&2
echo >&2

make_index | 

while read_branch_data
do
  if parms_pass; then
    printf "[%2d] " $COUNT
    COUNT=$((COUNT+1))
    show_branch_parms
    if [ "$MINOTYPE" = "e" ]
    then
      echo "$PARENT" >> $GAMES
    else
      ISNAME="1"
      for WORD in $GAMELIST
      do
        if [ "$ISNAME" = "1" ]
        then
          echo -n "$WORD " >> $GAMES
          ISNAME="0"
        else
          printf " %d;" "$WORD"
          ISNAME="1"
        fi
      done
      echo >> $GAMES
    fi
    echo
  fi
done >&2

echo >&2

read -p "Enter [ branch [ game ] ] : " BNUM GNUM 

STR=

[ -n "$BNUM" ] && STR="$(sed -n -e ${BNUM}p $GAMES)" || STR="$(cat $GAMES)"

[ -n "$GNUM" ] && STR="$(echo $STR | cut -d ' ' -f $GNUM)"

rm -f $GAMES

echo $STR

