---------------------------------------------------------
--
-- template.lua consists of init functions for variables
-- declared in randomino.lua
--
---------------------------------------------------------


local Var = ...

Var.WeightMax.init = function()
  return math.random(3, Var.Config().MaxFigureSize)
end

Var.WeightMin.init = 3

Var.Aperture.init = function()
  local weight = Var.WeightMax()
  local metric = Var.Metric()

  local ap_min = {
    [0] = {1, 2, 4, 4, 4, 4, 5, 5},
    [1] = {1, 2, 3, 3, 3, 3, 4, 4}
  }

  local ap_max = {
    [0] = {1, 2, 5, 5, 5, 6, 6, 6},
    [1] = {1, 2, 4, 4, 5, 5, 5, 5}
  }

  local aperture = math.random(weight == 3 and 1 or 0, 1)

  if aperture > 0 then
    aperture = math.random(ap_min[metric][weight], ap_max[metric][weight])
  end

  return aperture
end

Var.GlassWidth.init = function()
  return 2 * Var.FigureSize()
end

Var.GlassHeight.init = function()
  local k = 2
  if Var.Goal() == 1 then
    k = 5
  end
  return k * Var.FigureSize()
end

Var.DiscardFullRows.init = function()
  -- if Var.Goal() == 1 or Var.SingleLayer() == 1 then
  if Var.Goal() == 1 then
    return 1
  end
end

Var.FillLevel.init = function()
  if Var.Goal() == 1 then
    return 2 * Var.FigureSize()
  end
  return 0
end

Var.FillRatio.init = function()
  local ratio = Var.FigureSize() + 2

  if Var.SingleLayer() > 0 then
    ratio = ratio // 2
  end

  return ratio
end
