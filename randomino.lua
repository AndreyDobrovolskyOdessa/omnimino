---------------------------------------------------------------------
--
--    randomino.lua writes random .mino file to stdout
--
--    Usage: lua randomino.lua [ template.lua ... ] > random.mino
--
---------------------------------------------------------------------

if tonumber((_VERSION):match("%w+%.%w+")) < 5.4 then
  local addr = tostring(_G):match("0x.*")
  math.randomseed(os.time() + tonumber(addr))
end


local stack = {}

local var_mt = {
  __call = function(var)
    if var.value then
      return var.value
    end
    if stack[var] then
      io.stderr:write("dependency loop detected : ", var.name, "\n")
      io.stderr:write("stack : ", table.concat(stack, " "), "\n")
      os.exit(1)
    end

    stack[#stack + 1] = var.name
    stack[var] = true

    local success, result
    if type(var.init) == "function" then
      success, result = pcall(var.init)
      if not success then
        io.stderr:write("Warning : ", var.name, ".init() failed.\n")
      end
    else
      result = var.init
    end
    var.value = var.check(result)

    if result and var.value ~= result then
      io.stderr:write("Warning : ", var.name, ".init() output discarded.\n")
    end

    stack[#stack] = nil
    stack[var] = nil

    return var.value
  end
}

local Var = setmetatable( {}, {
  __newindex = function(t, k, v)
    v.name = k
    rawset(t, k, v)
    setmetatable(v, var_mt)
  end
  }
)


Var.Config = {}
Var.Goal = {}
Var.WeightMax = {}
Var.WeightMin = {}
Var.Aperture = {}
Var.FigureSize = {}
Var.GlassWidth = {}
Var.GlassHeight = {}
Var.FillLevel = {}
Var.FillRatio = {}

Var.Gravity = {}
Var.SingleLayer = {}
Var.DiscardFullRows = {}
Var.FixedSequence = {}
Var.Metric = {}


Var.Config.check = function(cfg)
  if type(cfg) ~= "table" or
     type(cfg.MaxFigureSize) ~= "number" or
     type(cfg.MaxGlassWidth) ~= "number" or
     type(cfg.MaxGlassHeight) ~= "number" then
    cfg = {
      MaxFigureSize = 8,
      MaxGlassWidth = 32,
      MaxGlassHeight = 256
    }
  end
  return cfg
end

Var.Config.init = function()
  local f = io.popen("echo | omnimino")
  if f then
    local s = f:read()
    f:close()
    if s then
      local fn = load("return {" .. s .. "}")
      if fn then
        return fn()
      end
    end
  end
end


Var.Goal.check = function(goal)
  if type(goal) ~= "number" or goal < 0 or goal > 2 then
    goal = math.random(0, 2)
  end
  return goal
end


Var.WeightMax.check = function(weight)
  local max_size = Var.Config().MaxFigureSize
  if type(weight) ~= "number" or weight < 2 or weight > max_size then
    weight = math.random(2, max_size)
  end
  return weight
end


Var.WeightMin.check = function(weight)
  local weight_max = Var.WeightMax()
  if type(weight) ~= "number" or weight < 1 or weight > weight_max then
    weight = math.random(1, weight_max)
  end
  return weight
end


Var.Aperture.check = function(aperture)
  local weight_max = Var.WeightMax()
  local metric = Var.Metric()
  local max_size = Var.Config().MaxFigureSize

  local aperture_area = function()
    local area = 0
    for i = 0, aperture - 1 do
      area = area + ((metric > 0) and aperture or (i | 1))
    end
    return area
  end

  if type(aperture) ~= "number" or aperture < 0 or aperture > max_size or
     (aperture > 0 and aperture_area() < weight_max) then
    aperture = math.random(0, 1)
    if aperture > 0 then
      aperture = 1
      while aperture_area() < weight_max do
        aperture = aperture + 1
      end
      aperture = math.random(aperture, max_size)
    end
  end
  return aperture
end


Var.FigureSize.check = function()
  local aperture = Var.Aperture()
  return aperture > 0 and aperture or Var.WeightMax()
end


Var.GlassWidth.check = function(width)
  local max_width = Var.Config().MaxGlassWidth
  local figure_size = Var.FigureSize()
  if type(width) ~= "number" or width < figure_size or width > max_width then
    width = math.random(figure_size, max_width)
  end
  return width
end


Var.GlassHeight.check = function(height)
  local max_height = Var.Config().MaxGlassHeight
  local figure_size = Var.FigureSize()
  if type(height) ~= "number" or height < figure_size or height > max_height then
    height = math.random(figure_size, max_height)
  end
  return height
end


Var.FillLevel.check = function(level)
  local glass_height = Var.GlassHeight()
  if type(level) ~= "number" or level < 0 or level > glass_height then
    level = math.random(0, glass_height)
  end
  return level
end


Var.FillRatio.check = function(ratio)
  local glass_width = Var.GlassWidth()
  if type(ratio) ~= "number" or ratio <= 0 or ratio >= glass_width then
    ratio = math.random(1, glass_width - 1)
  end
  return ratio
end


local checkBit = function(bit)
  if type(bit) ~= "number" or bit < 0 or bit > 1 then
    bit = math.random(0, 1)
  end
  return bit
end

Var.Gravity.check = checkBit
Var.SingleLayer.check = checkBit
Var.DiscardFullRows.check = checkBit
Var.FixedSequence.check = checkBit
Var.Metric.check = checkBit


local arg = {...}

for i, filename in ipairs(arg) do
  local fn = assert(loadfile(filename))
  fn(Var)
end


print(Var.Aperture())
print(Var.Metric())
print(Var.WeightMax())
print(Var.WeightMin())
print(Var.Gravity())
print(Var.SingleLayer())
print(Var.DiscardFullRows())
print(Var.Goal())
print(Var.GlassWidth())
print(Var.GlassHeight())
print(Var.FillLevel())
print(Var.FillRatio())
print(Var.FixedSequence())

