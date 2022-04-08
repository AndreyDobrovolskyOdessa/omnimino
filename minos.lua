#!/usr/bin/env lua


if arg[1] == "-h" or arg[1] == "--help" then
  io.stderr:write( [[

Usage:	minos.lua [ -h | --help ]
	minos.lua [ search_key1 ... ]

Lists .mino files in the current directory, satisfying conditions,
described by $1 ... $7

$1 -	game type, one of "g[ame]", "p[reset]", "e[rror]" or "x" (any)

$2 -	string of up to 4 decimal digits, describing correspondingly:
	FigureWeightMax, Apperture, Metrics and FigureWeightMin

$3 -	string of up to 4 decimal digits, describing correspondingly:
	Goal, Gravity, SingleLayer and FullRowClear options

$4 -	GlassWidth

$5 - 	GlassHeight

$6 -	FillLevel

$7 -	FillRatio

.mino files are grouped in branches, having the same parent. Game records
have scores. Presets have no scores. For corrupted files the first line
of the error message is shown.

After descriptions  will be shown,  You will be asked  to choose sequentional
number of the branch desired (optional)  and sequential number of the game in
the branch (optional). Empty input means "list all". You can enter the new
filter keys and repeat the search.

Interactive output is written to stderr, resulting list is written to stdout.

]] )

  os.exit()

end

-------------------------------------
-- Try to locate "omnimino" binary --
-------------------------------------

local OmniminoName = "omnimino "

if not (os.execute("which omnimino > /dev/null")) then 
  OmniminoName = "./omnimino "
  if not (os.execute("test -x omnimino")) then
    io.stderr:write("\nThis utility needs 'omnimino' binary in the current directory or in the $PATH .\n\n")
    os.exit(1)
  end
end


--[[

Apperture	App
Metric		Metr
WeightMax	WMax
WeightMin	WMin
Gravity		Grav
SingleLayer	SL
FullRowClear	FRC
Goal		Goal
GlassWidth	GW
GlassHeight	GH
FillLevel	FL
FillRatio	FR
SlotsUnique	SU

Parameters order in .mino file

App	Metr	WMax	WMin	Grav	SL	FRC	Goal	GW	GH	FL	FR	SU


Parameters order for display

WMax	WMin	App	Metr	Goal	Grav	SL	FRC	GW	GH	FL	FR


Parameters order for keys

Type	<WMax	App	Metr	WMin>	<Goal	Grav	SL	FRC>	GW	GH	FL	FR


Parameters order for sort - internal data order

Type	WMax	Goal	App	Metr	Grav	SL	FRC	GW	GH	FL	FR	WMin

]]


--------------------
-- Reading .minos --
--------------------

io.stderr:write("\nReading .minos ..")


local ReadParameters = function(FileName, Parameter)

  local ParmPos = { 4, 5, 2, 13, 6, 7, 8, 3, 9, 10, 11, 12, 14 }

  local MinoFile = io.open(FileName,"r")

  for i, Pos in ipairs(ParmPos) do
    Parameter[Pos] = Parameter.Success and tonumber(string.match(MinoFile:read("l"),"%w+")) or -1
  end

  table.insert(Parameter,FileName)

  if Parameter[1] == 1 then
    local Parent = MinoFile:read("l")
    if Parent ~= "none" then Parameter[#Parameter] = Parent end
  end

  MinoFile:close()

  return Parameter[#Parameter]
end



local Branch={}


local f = io.popen("ls *.mino 2>/dev/null", "r")

for MinoName in f:lines() do

  io.stderr:write(".")

  local MinoPipe = io.popen(OmniminoName .. MinoName .. " 2>&1")
  local CurGame = {Name = MinoName, Data = MinoPipe:read()}
  local Parameter = {Success = MinoPipe:close()}

  Parameter[1] = Parameter.Success and (CurGame.Data and 1 or 2) or 3

  local Parent = ReadParameters(MinoName, Parameter)

  if Branch[Parent] then
    table.insert(Branch[Parent], CurGame)
  else
    Branch[Parent] = {Parms = Parameter, CurGame}
  end
end

io.stderr:write(f:close() and " Ok\n" or " error\n")


-------------------------------------------------
-- Make filter keys of commannd-line arguments --
-------------------------------------------------

local MakeKey = function(arg)
  Key = {}

  local GameType = {["g"] = 1, ["p"] = 2, ["e"] = 3}

  local SelectGameType = function(Arg, ArgPos)
    Key[ArgPos] = GameType[string.sub(Arg,1,1)]
  end

  local Quad = function(Arg, ArgPos)
    for i=1,4 do
      Key[ArgPos[i]] = tonumber(string.sub(Arg,i,i))
    end
  end

  local Single = function(Arg, ArgPos)
    Key[ArgPos] = tonumber(Arg)
  end

  local Decoder = {{SelectGameType, 1}, {Quad, {2, 4, 5, 13}}, {Quad, {3, 6, 7, 8}},
                   {Single, 9}, {Single, 10 }, {Single, 11}, {Single, 12}}

  for i,D in ipairs(Decoder) do
    if arg[i] then
      D[1](arg[i], D[2])
    end
  end

  return Key
end


local SelectBranchesMatching = function(Key)
  local SBranch = {}

  local ParmsMatchKey = function(Parameter)
    for i, V in ipairs(Parameter) do
      if Key[i] and Key[i] ~= V then return false end
    end
    return true
  end

  for i,j in pairs(Branch) do
    if ParmsMatchKey(j.Parms) then
      table.insert(SBranch,j)
    end
  end

  return SBranch
end


local CompareBranches = function(B1, B2)
  local Q = B2.Parms

  for i, V in ipairs(B1.Parms) do
    if V ~= Q[i] then
      return V < Q[i]
    end
  end
  return false
end


local ReadAndSplitInputLine = function()
  io.stderr:write("\nEnter branch [ game ] or new filter keys : ")
  local AnsL = io.stdin:read("l") or "" -- if input pipe is empty

  local Ans = {}

  for i in string.gmatch(AnsL,"%w+") do
    table.insert(Ans,i)
  end

  return Ans
end


local ShowSortedBranches = function(SBranch)
  local BrParms

  local Exhibit = function(ListOfPairs)
    for i, Pair in ipairs(ListOfPairs) do
      io.stderr:write(string.format("%" .. tostring(Pair[2])  .. "s", Key[Pair[1]] and " " or tostring(BrParms[Pair[1]]))) 
    end
  end

  local CompareData = function(D1, D2)
    if D1.Data ~= D2.Data then
      return D1.Data < D2.Data
    end
    return D1.Name < D2.Name
  end


  io.stderr:write("\n    Weight  A  M  Goal G  S  C   Glass   Fill   Scores\n\n")

  for i, Br in ipairs(SBranch) do

    BrParms = Br.Parms

    io.stderr:write(string.format("%2d", i))

    if Br.Parms.Success then
      Exhibit{{2,4}}
      io.stderr:write(">=")
      Exhibit{{13, 1}, {4, 4}, {5, 3}, {3, 4}, {6, 4}, {7, 3}, {8, 3}, {9, 5}}
      io.stderr:write("*")
      Exhibit{{10, -3}, {11, 4}}
      io.stderr:write("*")
      Exhibit{{12, -3}}
    end

    table.sort(Br,CompareData)

    for j, k in ipairs(Br) do
      if k.Data then
        io.stderr:write(" ", k.Data)
      end
    end
    io.stderr:write("\n")
  end
end


local SBranch

local Ans = arg
local BN
local GN

repeat ------------------------------------------------------------------------

  SBranch = SelectBranchesMatching(MakeKey(Ans))

  table.sort(SBranch, CompareBranches)

  ShowSortedBranches(SBranch)

  Ans = ReadAndSplitInputLine()

  BN = tonumber(Ans[1])
  GN = tonumber(Ans[2])

until #Ans == 0 or BN ---------------------------------------------------------

io.stderr:write("\n")

--------------------------------------
-- Writing list of .mino file names --
--------------------------------------

for i, Br in ipairs(SBranch) do
  if not BN or BN == i then
    for j, k in ipairs(Br) do
      if not GN or GN == j then
        io.stdout:write(" ", k.Name)
      end
    end
  end
end

io.stdout:write("\n")

