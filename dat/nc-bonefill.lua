-- NeonHack 2: the Boneyard, 1F (the vault below is nc-bonevault).
-- The Long Road's scrapyard runs on underground through the old landfill.

des.level_init({ style = "solidfill", fg = " " })
des.level_flags("mazelevel", "noflip")
des.level_init({ style = "mines", fg = ".", bg = " ", smoothed = true,
                 joined = true, lit = 1, walled = true })

-- the way out to the yard is added by the game
des.stair("up")
des.stair("down")

for i = 1, 5 + nh.rn2(4) do
   des.object("boulder")            -- wrecks
end
for i = 1, 2 + nh.rn2(3) do
   des.monster("outrider")          -- friendly or not, as the Road decides
end
des.monster("trash robot")
if percent(50) then
   des.monster("spider bot")
end
for i = 1, 4 do
   des.object()
end
des.object("(")
