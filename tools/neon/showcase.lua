-- NeonGrid showcase: a room with every map feature, built where you
-- stand.  Wizard mode: copy this file into the playground, then
-- #wizloaddes and give its name.  The right end of the room is unlit, to
-- show dark and remembered floor.

des.level_flags("noflip")

local w, h = 31, 9
local x = math.max(1, math.min(u.ux - 15, 79 - w))
local y = math.max(1, math.min(u.uy - 2, 20 - h))

des.map({ x = x, y = y, map = [[
-------------------------------
|.{.K.\.......T.F.PPP.LLL.III.|
|.............................|
|.............................|
|...ZZ.WW.CC..................|
|.............................|
|....###......................|
|.............................|
-------------------------------]] })

des.region(selection.area(1, 1, 20, 7), "lit")
des.region(selection.area(21, 1, 29, 7), "unlit")

des.altar({ x = 8, y = 1, align = "law" })
des.altar({ x = 10, y = 1, align = "neutral" })
des.altar({ x = 12, y = 1, align = "chaos" })
des.altar({ x = 16, y = 3, align = "noalign" })

des.grave({ x = 2, y = 3, text = "Flatlined, but stylish" })
des.trap("web", 4, 3)
des.trap("teleport", 6, 3)
des.engraving({ x = 8, y = 3, type = "engrave", text = "Elbereth" })
des.object("boulder", 10, 3)
des.ladder("up", 12, 3)
des.ladder("down", 14, 3)

des.door({ state = "open", x = 3, y = 0 })
des.door({ state = "closed", x = 6, y = 0 })
des.door({ state = "broken", x = 9, y = 0 })
des.door({ state = "nodoor", x = 12, y = 0 })
des.door({ state = "open", x = 0, y = 4 })
des.door({ state = "closed", x = 0, y = 6 })
