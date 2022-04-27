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

$3 -	string of up to 5 decimal digits, describing correspondingly:
	Goal, Gravity, SingleLayer, FullRowClear and OrderFixed options

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

-- local OmniminoName = "./omnimino "


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


local Branch={}


local pipe = io.popen("ls *.mino | " .. OmniminoName, "r")
local chunk = assert(pipe:read("a"))
assert(pipe:close())
local f = load("_G = nil _ENV = nil return {" .. chunk .. "}")

if not f then
  print("Bad chunk") os.exit(1)
end

local IBranch = f()

for i,B in ipairs(IBranch) do
  local P = B.Parameters
  local Parent = P[#P]
  if Branch[Parent] then
    table.insert(Branch[Parent], B.Data)
  else
    Branch[Parent] = {Parms = P, B.Data}
  end
end

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
    for i,Pos in ipairs(ArgPos) do
      Key[Pos] = tonumber(string.sub(Arg,i,i))
    end
  end

  local Single = function(Arg, ArgPos)
    Key[ArgPos] = tonumber(Arg)
  end

  local Decoder = {{SelectGameType, 1}, {Quad, {2, 4, 5, 14}}, {Quad, {3, 6, 7, 8, 13}},
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


local ReadAndSplitInputLine = function(Subject)
  io.stderr:write("\nEnter " .. Subject .. " or new filter keys : ")
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
    for i = #D1, 1, -1 do
      if D1[i] ~= D2[i] then
        return D1[i] < D2[i]
      end
    end
    return false
  end


  io.stderr:write("\n")
  io.stderr:write("       Metric  Gravity  FlatFun      OrderFixed\n")
  io.stderr:write("             \\     \\   /                  |\n")
  io.stderr:write("   Aperture  |      \\  |  ClearFullRows   |\n")
  io.stderr:write("           \\ |       \\ | /                |\n")
  io.stderr:write("    Weight A M  Goal G F C  Glass   Fill  O  Scores\n\n")

  for i, Br in ipairs(SBranch) do

    BrParms = Br.Parms

    io.stderr:write(string.format("%2d", i))

    if Br.Parms[1] ~= 3 then
      Exhibit{{2,4}}
      io.stderr:write(">=")
      Exhibit{{14, 1}, {4, 3}, {5, 2}, {3, 4}, {6, 4}, {7, 2}, {8, 2}, {9, 4}}
      io.stderr:write("*")
      Exhibit{{10, -3}, {11, 4}}
      io.stderr:write("*")
      Exhibit{{12, -3}, {13,-3}}
    end

    table.sort(Br,CompareData)

    for j, k in ipairs(Br) do
      if k[4] then
        io.stderr:write(k[4], " ")
      end
    end
    io.stderr:write("\n")
  end
end


local ShowSortedGames = function(B)

  io.stderr:write("\n    Score          Date         Player\n\n")
  for i = 1, #B do
    io.stderr:write(string.format("%2d",i))
    io.stderr:write(string.format("%7s",B[i][4]))
    io.stderr:write(string.format("%20s",os.date("%y-%m-%d %H:%M:%S",B[i][3])))
    io.stderr:write(string.format("   %s",B[i][2]))
    io.stderr:write("\n")
  end
  io.stderr:write("\n")
end


local SBranch

local Ans = arg
local BN
local GN

repeat ------------------------------------------------------------------------

repeat -----------------------------------------

  SBranch = SelectBranchesMatching(MakeKey(Ans))

  table.sort(SBranch, CompareBranches)

  ShowSortedBranches(SBranch)

  Ans = ReadAndSplitInputLine("branch [ game ]")

  BN = tonumber(Ans[1])
  GN = tonumber(Ans[2])

until #Ans == 0 or BN --------------------------

  io.stderr:write("\n")

  if #Ans == 0 or GN or (not SBranch[BN]) or #SBranch[BN] < 2 then break end

  ShowSortedGames(SBranch[BN])

  Ans = ReadAndSplitInputLine("game")

  GN = tonumber(Ans[1])

until #Ans == 0 or GN ---------------------------------------------------------

  io.stderr:write("\n")

--------------------------------------
-- Writing list of .mino file names --
--------------------------------------

for i, Br in ipairs(SBranch) do
  if not BN or BN == i then
    for j, k in ipairs(Br) do
      if not GN or GN == j then
        io.stdout:write(k[1],"\n")
      end
    end
  end
end


