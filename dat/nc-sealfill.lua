-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.
-- NeonHack 2: the Seal Vault, 1F (the Loom below is nc-sealloom).
-- Halcyon's old R&D floors, broken by the quake and full of things
-- that came up from below.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "noflip")
des.level_init({ style = "mines", fg = ".", bg = " ", smoothed = true,
                 joined = true, lit = 0, walled = true })

-- the way out to Ashfall is added by the game
des.stair("up")
des.stair("down")

for i = 1, 3 + nh.rn2(3) do
   des.object("boulder")            -- rubble
end
for i = 1, 2 + nh.rn2(3) do
   des.monster("mutant runt")
end
des.monster("ghoul")
des.monster("feral")
if percent(50) then
   des.monster("scav zombie")
end
for i = 1, 4 do
   des.object()
end
