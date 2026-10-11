-- NeonHack 2: a floor of the Jade Circuit's den under Lantern Market.
-- (Act 2; for now a gang den of rooms and halls.)

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("noflip")

for i = 1, 7 do
   des.room({ type = "ordinary", filled = 1 })
end
des.random_corridors()
des.stair("up")
des.stair("down")
for i = 1, 4 do
   des.monster("yakuza")
end
for i = 1, 4 do
   des.object()
end
