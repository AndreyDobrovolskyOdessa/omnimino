#!/usr/local/bin/lua

---------- Editable ------------

local Cflags = "-Os -Wall -Wextra -Wno-format-truncation -fno-asynchronous-unwind-tables"

local Deps = "ncursesw"

--------------------------------


assert(os.execute("redo-always"))

local f

if Deps:match("[^%s]") then
  f = assert(io.popen("pkg-config --cflags " .. Deps))
  Cflags = Cflags .. " " .. f:read()
  assert(f:close())
end

f = assert(io.open(arg[3], "w"))
assert(f:write(Cflags .. "\n" .. Deps .. "\n"))
f:close()

