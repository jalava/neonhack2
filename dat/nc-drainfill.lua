-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.
-- NeonHack 2: the storm drains under Rustwater Docks (1F and 2F; the
-- junction at the bottom is nc-drainhub).  Tunnels and black water.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "noflip")
des.level_init({ style = "mines", fg = ".", bg = " ", smoothed = true,
                 joined = true, lit = 0, walled = true })

-- the way out on 1F is added by the game
des.stair("up")
des.stair("down")

for i = 1, 3 + nh.rn2(3) do
   des.monster("street punk")       -- Drain Rats
end
for i = 1, 3 do
   des.monster("sewer rat")
end
des.monster("giant rat")
if percent(40) then
   des.monster("juicer")
end
for i = 1, 3 do
   des.object()
end
