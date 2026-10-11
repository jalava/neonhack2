-- NeonHack 2: the story of Neo Yendor.
--
-- This file is data; src/story.c runs it.  See doc/story-format.md for
-- the format.  Phase 4 placeholder: a test quest that exercises every
-- kind of stage.  The real prologue replaces it.

story = {
   factions = {
      halcyon = "Halcyon Dynamics",
      jade = "the Jade Circuit",
      longroad = "the Long Road",
   },

   items = {
      testshard = { name = "scratched data shard" },
   },

   npcs = {
      okoye = { name = "Doc Okoye", monster = "ripperdoc", female = true,
                home = "nc-neonmile", stay = true,
                idle = { "Doc Okoye wipes down her tools." } },
      ratking = { name = "the Rat King", monster = "street punk",
                  hostile = true, home = "nc-neonmile" },
   },

   quests = {
      { id = "test", title = "Test Run", kind = "main", start = true,
        stages = {
           { goal = "Talk to Doc Okoye.", where = "nc-neonmile",
             talk = "okoye", dialogue = "okoye1",
             log = [[You woke up in a clinic on Neon Mile.]] },
           { goal = "Go to the Stacks.", arrive = "nc-stacks",
             start = { unlock = "nc-stacks" },
             log = [[Okoye says the answers are in the Stacks.]] },
           { goal = "Flatline the Rat King.", where = "nc-neonmile",
             kill = "ratking", start = { spawn = "ratking" } },
           { goal = "Bring the shard to Doc Okoye.", where = "nc-neonmile",
             talk = "okoye", give = "testshard",
             start = { item = "testshard" },
             remind = "\"Bring me that shard,\" says Doc Okoye." },
        },
        done = { credits = 100, rep = { jade = 5 } } },
   },

   dialogue = {
      okoye1 = { text = [[Easy, %n.  You've been out for a while.

Don't touch the stitches.]],
         choices = {
            { "Where am I?", go = "okoye2" },
            { "Not now.", wait = true },
         } },
      okoye2 = { text = [[Neon Mile.  My clinic.  Go see the Stacks.]] },
   },
}
