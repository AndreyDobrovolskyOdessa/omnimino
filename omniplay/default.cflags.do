#!/usr/local/bin/lua

---------- Editable ------------

local Cflags = "-Os -Wall -Wextra -Wno-format-truncation -fno-asynchronous-unwind-tables"

local Deps = "ncursesw"

--------------------------------

local f

if Deps:match("[^%s]") then
  assert(os.execute("redo-always"))
  f = assert(io.popen("pkg-config --cflags " .. Deps))
  Cflags = Cflags .. " " .. f:read()
  assert(f:close())
end

f = assert(io.open(arg[3], "w"))
assert(f:write(Cflags .. "\n" .. Deps .. "\n"))
f:close()

