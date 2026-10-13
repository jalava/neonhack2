-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.
-- NeonHack 2: a floor of Halcyon Tower (the top floor is nc-halctop).
-- Lit offices off clean corridors; corpsec on every floor.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("noflip")

for i = 1, 9 do
   des.room({ type = "ordinary", lit = 1, filled = 1 })
end
des.random_corridors()

-- the way out to the lobby on the ground floor is added by the game
des.stair("up")
des.stair("down")

for i = 1, 2 + nh.rn2(3) do
   des.monster("corpsec trooper")
end
if percent(50) then
   des.monster("corpsec sergeant")
end
des.monster("security drone")
if percent(30) then
   des.monster("sentry bot")
end
for i = 1, 2 do
   des.monster({ id = "salaryman", peaceful = 1 })
end
for i = 1, 4 do
   des.object()
end
