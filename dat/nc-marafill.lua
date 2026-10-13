-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.
-- NeonHack 2: a hold of the freighter Mara Sun (1F and 2F; the bottom
-- hold is nc-marahold).  Cargo bays off narrow passages, mostly dark.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("noflip")

for i = 1, 7 do
   des.room({ type = "ordinary", lit = 0, filled = 1 })
end
des.random_corridors()

-- the way out to the quay on 1F is added by the game
des.stair("up")
des.stair("down")

for i = 1, 2 + nh.rn2(3) do
   des.monster("smuggler")
end
if percent(50) then
   des.monster("corpsec trooper")   -- customs, Halcyon's
end
for i = 1, 2 do
   des.monster("sewer rat")
end
for i = 1, 3 do
   des.object()
end
des.object("large box")
