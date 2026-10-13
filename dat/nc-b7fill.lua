-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.
-- NeonHack 2: a floor of Block 7 (2F to 4F).
-- Capsule flats off narrow halls; the Chrome Saints have taken over.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("noflip")

for i = 1, 8 do
   des.room({ type = "ordinary", filled = 1 })
end
des.random_corridors()

-- stairs between floors; the way out to the street on 2F is added by
-- the game (place_site_exit)
des.stair("up")
des.stair("down")

-- the Saints
for i = 1, 2 + nh.rn2(3) do
   des.monster("street punk")
end
if percent(25) then
   des.monster("black hat")
end
for i = 1, 4 do
   des.object()
end
