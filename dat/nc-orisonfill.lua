-- NeonHack 2: Orison, deck 2 (the cold open's middle floor): labs and
-- server halls, lit and guarded.
-- Copyright (c) 2026 jalava
-- NetHack may be freely redistributed.  See license for details.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("noflip", "noteleport")

for i = 1, 8 do
   des.room({ type = "ordinary", lit = 1, filled = 0 })
end
des.random_corridors()

des.stair("up")
des.stair("down")

for i = 1, 3 + nh.rn2(2) do
   des.monster({ id = "corpsec trooper", peaceful = 0 })
end
des.monster({ id = "corpsec sergeant", peaceful = 0 })
for i = 1, 2 do
   des.monster({ id = "combat synth", peaceful = 0 })
end
des.monster({ id = "security drone", peaceful = 0 })
des.monster({ id = "sentry bot", peaceful = 0 })
des.object("medkit")
for i = 1, 3 do
   des.object()
end
