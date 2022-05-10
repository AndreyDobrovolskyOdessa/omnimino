#!/usr/bin/env lua


--------- Editable -----------

local Linker = "gcc"

local Libs = ""

------------------------------


local Sanitize = function(P)
  local D, N = P:match("(.-)([^/]*)$")

  local S = {}

  for part in D:gmatch("([^/]*)/") do
    if part == ".." and #S > 0 and S[#S] ~= ".." then
      table.remove(S)
    else
      table.insert(S, part)
    end
  end

  table.insert(S, "")
  D = table.concat(S, "/")

  return D .. N
end


local BinName = arg[2]

local TDir, TName = arg[2]:match("(.-)([^/]*)$")

if TName == "main" then
  local DirName = TDir
  if DirName == "" then
    local f = assert(io.popen("pwd"))
    DirName = f:read() .. "/"
    assert(f:close())
  end
  BinName = TDir .. DirName:match("([^/]*)/$")
end

local CName = arg[2] .. ".c"
local RName = arg[2] .. ".require"

if not os.execute("test -e " .. CName) then
  io.stderr:write("Missing " .. CName .. "\n")
  os.exit(1)
end

assert(os.execute("redo-ifchange " .. RName))

local f = assert(io.open(RName))

local RUniq = {}
local DUniq = {}

for n in f:lines() do
  if n:match("%.o$") then
    table.insert(RUniq, Sanitize(TDir .. n))
  else
    table.insert(DUniq, n)
  end
end

f:close()

local RList = table.concat(RUniq, " ")
local DList = table.concat(DUniq, " ")

if DList ~= "" then
  f = assert(io.popen("pkg-config --libs " .. DList))
  Libs = Libs .. " " .. f:read()
  assert(f:close())
end

assert(os.execute("redo-ifchange " .. RList))
assert(os.execute(Linker .. " -o " .. BinName .. " " .. RList .. " " .. Libs))
assert(os.execute("redo-ifchange " .. BinName))

