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
the branch (optional). Empty input means "list all".

Interactive output is written to stderr, resulting list is written to stdout.

]] )

os.exit()

end

-------------------------------------
-- Try to locate "omnimino" binary --
-------------------------------------

local OmniminoName

if (os.execute("which omnimino > /dev/null")) then 
  OmniminoName = "omnimino "
else
  if (os.execute("test -x omnimino")) then
    OmniminoName = "./omnimino "
  else
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


-------------------------------------------------
-- Make search keys of commannd-line arguments --
-------------------------------------------------

local Key={}

local KeyNum = 14

local KeyPos = { 1, 2, 4, 5, 13, 3, 6, 7, 8, 9, 10, 11, 12 , 14 }

if arg[1] then
  Key[1] = string.sub(arg[1],1,1)
  if Key[1] == 'x' then Key[1] = nil end
end

local StringToKeys = function(str, start)
  if not str then return end
  local strlen = string.len(str)
  if strlen > 4 then strlen = 4 end
  for i=1,strlen do
    local NewKey = string.sub(str,i,i)
    if NewKey == 'x' then
      NewKey = nil
    else
      NewKey = tonumber(NewKey)
    end
    Key[KeyPos[start+i]] = NewKey;
  end
end

StringToKeys(arg[2],1)
StringToKeys(arg[3],5)

for i=0,3 do
  if arg[4 + i] and arg[4 + i] ~= 'x' then
    Key[KeyPos[10 + i]] = tonumber(arg[4 + i])
  end
end



--------------------
-- Reading .minos --
--------------------

local ParPos = { 4, 5, 2, 13, 6, 7, 8, 3, 9, 10, 11, 12, 14 }


local ReadParameters = function(FileName, Parameter)

  local MinoFile = io.open(FileName,"r")
  local Parent

  if Parameter[1] ~= "e" then
    for i = 1, 13 do
      local MinoLine = MinoFile:read("l")
      local Value = string.match(MinoLine,"%w+")
      Parameter[ParPos[i]] = tonumber(Value)
    end
  end

  if Parameter[1] == "g" then
    Parent = MinoFile:read("l")
  end

  if (not Parent) or (Parent == "none") then
    Parent = FileName
  end

  MinoFile:close()

  return Parent
end


local ParmsMatchKey = function(Parameter, Key)
  for i = 1,KeyNum do
    if Key[i] and Key[i] ~= Parameter[i] then return false end
  end
  return true
end


io.stderr:write("\nReading .minos ..")


local Branch={}


local ScoreFileName = os.tmpname()
local MessageFileName = os.tmpname()
local Redirection = " 2> "  .. MessageFileName .. " > " .. ScoreFileName 

local f = io.popen("ls *.mino 2>/dev/null", "r")

for MinoName in f:lines() do

  io.stderr:write(".")

  local Success, ExitType, ExitCode = os.execute(OmniminoName .. MinoName .. Redirection)

  local CurGame = {}
  local Parent
  local Parameter = {}

  CurGame.Name = MinoName

  if ExitCode == 0 then
    Parameter[1] = "g"
    local ScoreFile = io.open(ScoreFileName,"r")
    CurGame.Data = ScoreFile:read("n")
    ScoreFile:close()
  elseif ExitCode == 7 then
    Parameter[1] = "p"
  else
    Parameter[1] = "e"
    local MessageFile = io.open(MessageFileName)
    CurGame.Data = MessageFile:read("l")
    MessageFile:close()
  end

  Parent = ReadParameters(MinoName, Parameter)

  if ParmsMatchKey(Parameter, Key) then
    if Branch[Parent] then
      table.insert(Branch[Parent],CurGame)
    else
      Branch[Parent] = {}
      Parameter[15] = Parent
      Branch[Parent].Parms = Parameter
      Branch[Parent][1] = CurGame
    end
  end
end

local SBranch = {}

for i,j in pairs(Branch) do
  table.insert(SBranch,j)
end


if #SBranch == 0 then

io.stderr:write(" not found\n\n")
os.exit()

end

io.stderr:write(" Ok\n")



----------------------
-- Sorting branches --
----------------------


local CompareBranches = function(B1, B2)
  local Q1, Q2

  for i=1,KeyNum+1 do
    Q1 = B1.Parms[i]
    Q2 = B2.Parms[i]
    if Q1 ~= Q2 then
      if Q1 then
        if Q2 then
          return Q1 < Q2
        else
          return false
        end
      end
      if Q2 then
        return true
      end
    end
  end
  return false
end

table.sort(SBranch, CompareBranches)


------------------
-- Show results --
------------------


local CompareData = function(D1, D2)
  if D1.Data ~= D2.Data then
    return D1.Data < D2.Data
  end
  return D1.Name < D2.Name
end

io.stderr:write("\n")
io.stderr:write("    Weight  A  M   G   Gr S  C   Glass   Fill   Scores\n")
io.stderr:write("\n")



for i, Br in ipairs(SBranch) do

  local Exhibit = function(ListOfPairs)

    local First, Second

    for i, ThePair in ipairs(ListOfPairs) do
      First = ThePair[1]
      Second = ThePair[2]
      io.stderr:write(string.format("%" .. tostring(Second)  .. "s", Key[First] and " " or tostring(Br.Parms[First]))) 
    end
  end


  io.stderr:write(string.format("%2d", i))

  if Br.Parms[1] ~= "e" then
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

io.stderr:write("\nEnter [ branch [ game ] ] : ")
local AnsL = io.read("l")

local Ans={}

for i in string.gmatch(AnsL,"%w+") do
  table.insert(Ans,i)
end


--------------------------------------
-- Writing list of .mino file names --
--------------------------------------

local BN = tonumber(Ans[1])
local GN = tonumber(Ans[2])

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

