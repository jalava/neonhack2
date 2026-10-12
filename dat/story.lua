-- NeonHack 2: The Splinter -- the story of Neo Yendor.
--
-- This file is data; src/story.c runs it.  doc/story-format.md has the
-- format.  Check it in a debug-mode game with #wizstorycheck.
--
-- The prologue (Neon Mile), act 1 (Lantern Market, the Stacks, Block 7),
-- act 2 (three splinters: the Jade Circuit's, the Long Road's and
-- Halcyon's, each by a deal or the hard way), act 3 (Ashfall and the Seal
-- Vault), act 4 (the Zenith Spire) and three endings; side jobs and
-- gigs between.

story = {
   -- a faction's members are friendly in the city when you're on good
   -- terms with it (reputation 15 or more) and hostile on bad terms
   -- (-15 or less)
   factions = {
      halcyon = { name = "Halcyon Dynamics",
                  members = { "corpsec trooper", "corpsec sergeant",
                              "corpsec lieutenant", "corpsec captain",
                              "security guard", "security chief",
                              "security drone", "sentry bot" } },
      jade = { name = "the Jade Circuit", members = { "yakuza" } },
      longroad = { name = "the Long Road", members = { "outrider" } },
   },

   items = {
      coldcase = { name = "frosted cold case" },
      envelope = { name = "sealed envelope marked MAGS" },
      hexcore = { name = "scorched deck core" },
      crate = { name = "crate stamped SEIZED - HALCYON CUSTOMS" },
      navcore = { name = "nav core marked DUSTMOTHER" },
      splinter_jade = { name = "splinter in a jade locket" },
      splinter_road = { name = "splinter on a leather cord" },
      splinter_halcyon = { name = "splinter in a Halcyon cryo-vial" },
      splinterkey = { name = "Splinter key" },
   },

   ----------------------------------------------------------------------
   npcs = {
      okoye = {
         name = "Doc Okoye", monster = "ripperdoc", female = true,
         home = "nc-neonmile", stay = true,
         idle = {
            "Doc Okoye is sterilising a bone saw and doesn't look up.",
            "\"Keep the stitches dry,\" says Doc Okoye.",
            "\"If you're bleeding, take a number.  If not, out.\"",
         },
      },
      mags = {
         name = "Mags Vance", monster = "infiltrator", female = true,
         home = "nc-neonmile", stay = true, fixer = true,
         idle = {
            "Mags Vance is reading three feeds at once.",
            "\"Work comes to those who sit still,\" says Mags.  \"So sit.\"",
            "Mags pours you something blue.  You don't ask.",
         },
      },
      sato = {
         name = "Dr. Sato", monster = "ripperdoc",
         home = "nc-lantern", stay = true,
         idle = {
            "Dr. Sato is soldering something that twitches.",
            "\"Come back when something falls off,\" says Dr. Sato.",
         },
      },
      benny = {
         name = "Benny Two-Tone", monster = "salvager",
         home = "nc-lantern", stay = true, faction = "jade",
         fixer = true, chat = "benny_chat",
         idle = {
            "\"Everything's half price,\" says Benny.  \"For you, full price.\"",
            "Benny Two-Tone polishes a stolen optic on his sleeve.",
         },
      },
      hex = {
         name = "Hex", monster = "netrunner", female = true,
         home = "nc-b7roof", stay = true,
         idle = {
            "Hex is three screens deep and doesn't hear you.",
            "\"Whatever it is, it can wait,\" says Hex.  \"Unless it's coffee.\"",
         },
      },
      lazarus = {
         name = "Saint Lazarus", monster = "juicer", level = 5,
         hostile = true, home = "Block 7:2",
      },

      -- act 2: the keepers of the other splinters
      lin = {
         name = "Matron Lin", monster = "yakuza", female = true, level = 12,
         home = "Jade Den:3", stay = true, faction = "jade",
         idle = {
            "Matron Lin pours tea for nobody in particular.",
            "\"The Circuit remembers,\" says Matron Lin.  \"Everything.\"",
         },
         died = {
            say = "Matron Lin falls, and her jade locket cracks on the floor.",
            rep = { jade = -30 },
            -- the splinter, unless you already have it
            cases = { { need = { notdone = "jade" }, item = "splinter_jade",
                        say = "Inside the locket is a splinter.  You take it." } },
            when = { { need = { active = "jade_deal" }, fail = "jade_deal" } },
         },
      },
      juno = {
         name = "Old Juno", monster = "nomad", female = true,
         home = "nc-fringe", stay = true, faction = "longroad",
         chat = "juno_chat",
         idle = {
            "Old Juno is mending a fan belt with her teeth.",
            "\"The road goes on,\" says Old Juno.  \"So do we.\"",
         },
      },
      rook = {
         name = "Rook", monster = "outrider", level = 8,
         home = "nc-fringe", stay = true, faction = "longroad", fixer = true,
         chat = "rook_chat",
         idle = {
            "Rook feeds the fire another chair leg.",
            "\"Work's work,\" says Rook.  \"The road don't pave itself.\"",
         },
      },
      vos = {
         name = "Adrian Vos", monster = "corporate exec", level = 8,
         home = "nc-chrome", stay = true, faction = "halcyon", fixer = true,
         chat = "vos_chat",
         idle = {
            "Adrian Vos stirs his coffee anticlockwise.",
            "\"Everything has a price,\" says Vos.  \"Halcyon just pays it first.\"",
         },
      },
      brandt = {
         name = "Captain Brandt", monster = "corpsec captain", female = true,
         home = "The Boneyard:2", spot = "near", stay = true,
         faction = "halcyon",
      },

      -- act 3: whoever took Hex
      shen = {
         name = "Red Pole Shen", monster = "yakuza", level = 12,
         hostile = true, home = "Seal Vault:2", near = "hex", faction = "jade",
         died = { flag = "hex_free",
                  say = "Red Pole Shen drops, still reaching for Hex." },
      },
      kask = {
         name = "Director Kask", monster = "corpsec captain", level = 12,
         hostile = true, home = "Seal Vault:2", near = "hex",
         faction = "halcyon",
         died = { flag = "hex_free",
                  say = "Director Kask goes down, and his override key with him." },
      },
      sable = {
         name = "Brother Sable", monster = "outrider", level = 12,
         hostile = true, home = "Seal Vault:2", near = "hex",
         faction = "longroad",
         died = { flag = "hex_free",
                  say = "Brother Sable falls, and the road goes quiet." },
      },
      -- never seen: the voice from below
      voice = { name = "UNKNOWN CALLER", monster = "black ICE" },

      -- act 4
      arbiter = {
         name = "ARBITER", monster = "battle mech", level = 18,
         hostile = true, home = "Zenith Spire:1",
         died = { say = "ARBITER's chassis locks up, and the sanctum door unseals with a sigh." },
      },
   },

   ----------------------------------------------------------------------
   quests = {

      -- the prologue: wake up, meet Okoye and Mags
      { id = "prologue", title = "Cold Start", kind = "main", start = true,
        stages = {
           { goal = "Talk to Doc Okoye.", where = "nc-neonmile",
             talk = "okoye", dialogue = "okoye_wake",
             start = { text = [[Neo Yendor, 2188.

A year ago somebody went down into the Undercity, the sealed arcology under the city's feet, and didn't come back up.  Since then the Grid has been twitchy: metro lines that stop for no reason, corpsec drones that forget where they are, money that leaves one account and turns up in another.

You don't remember the last week.  You remember antiseptic, a woman's voice counting backwards, and a word that wasn't yours: SPLINTER.]] },
             log = [[I woke up on a ripperdoc's table on Neon Mile with fresh stitches behind my ear.  I don't remember the last week.]] },
           { goal = "Find Mags Vance in the Static Lounge.",
             where = "nc-neonmile", talk = "mags", dialogue = "mags_meet",
             log = [[Doc Okoye wants 200 credits for patching me up.  She says Mags Vance, at the Static Lounge next door, has work for people who don't ask questions.]] },
        },
        done = { start = { "act1", "bill", "rent" } } },

      -- act 1: the cold case, Sato's scan, Hex on the roof of Block 7
      { id = "act1", title = "Street Cred", kind = "main",
        stages = {
           { goal = "Deliver the cold case to Dr. Sato in Lantern Market.",
             where = "nc-lantern", talk = "sato", give = "coldcase",
             dialogue = "sato_case",
             remind = "Dr. Sato looks at your empty hands.  \"Mags said there'd be a case.\"",
             log = [[Mags gave me a sealed cold case for Dr. Sato in Lantern Market.  Don't open it, don't drop it.  The metro station is at the west end of Neon Mile.]] },
           { goal = "Find the netrunner Hex on the roof of Block 7.",
             where = "nc-b7roof", talk = "hex", dialogue = "hex_meet",
             log = [[Sato scanned me.  The thing in my head isn't cyberware; it's part of a key, and it's calling home.  He says a netrunner called Hex can read it: she's holed up on the roof of Block 7, in the Stacks, with the Chrome Saints on the stairs.]] },
           { goal = "Take Hex's deck core back from Saint Lazarus.",
             where = "Block 7:2", kill = "lazarus",
             start = { spawn = "lazarus" },
             done = { item = "hexcore",
                      say = "Something clatters out of Saint Lazarus's coat: a scorched deck core." },
             log = [[Hex will read the splinter if I get her deck core back.  Saint Lazarus, the Chrome Saints' boss, took it.  He's on the fourth floor.]] },
           { goal = "Bring the deck core to Hex.", where = "nc-b7roof",
             talk = "hex", give = "hexcore", dialogue = "hex_read",
             remind = "\"No core, no reading,\" says Hex.  \"Fourth floor.  Follow the screaming.\"" },
        },
        done = { xp = 150, start = "act2" } },

      -- act 2: three splinters, three keepers, a deal or the hard way
      { id = "act2", title = "Three Splinters", kind = "main",
        stages = {
           { goal = "Get the other three splinters: the Jade Circuit's, the Long Road's and Halcyon's.",
             start = { start = { "jade", "road", "halcyon" } },
             log = [[Hex told me what I'm carrying: one of four splinters of a copy of the Grid's root keys.  Halcyon keeps one in its tower in Chrome Heights, Matron Lin of the Jade Circuit wears one under Lantern Market, and the nomads of the Long Road took the last one west.]] },
           { goal = "Get the last two splinters." },
           { goal = "Get the last splinter." },
        },
        done = { start = "act3" } },

      { id = "jade", title = "The Matron", kind = "main",
        stages = {
           { goal = "Get the Jade Circuit's splinter from Matron Lin, at the bottom of the Jade Den.  Benny Two-Tone can get you in.",
             where = "nc-lantern", carry = "splinter_jade",
             log = [[Matron Lin wears the Jade Circuit's splinter.  Her den is under Lantern Market, and Benny Two-Tone is the Circuit's eyes in the market: he can get me an audience.  Or I can let myself in, and the Circuit will never forgive me.]] },
        },
        done = { advance = "act2" } },
      { id = "jade_deal", title = "The Matron's Crate", kind = "main",
        stages = {
           { goal = "Meet Matron Lin at the bottom of the Jade Den.",
             where = "Jade Den:3", talk = "lin", dialogue = "lin_meet",
             log = [[Benny made his call.  The Circuit's people will let me through the den, and Matron Lin will see me at the bottom of it.]] },
           { goal = "Get Lin's crate out of the customs cage in the Mara Sun's bottom hold, at Rustwater Docks.",
             where = "Mara Sun:3", carry = "crate",
             log = [[Halcyon customs seized one of the Circuit's crates off the freighter Mara Sun and caged it in her bottom hold.  Lin wants it back, unopened, and she'll trade her splinter for it.  The Mara Sun is berthed at Rustwater Docks.]] },
           { goal = "Bring the crate to Matron Lin.", where = "Jade Den:3",
             talk = "lin", give = "crate", dialogue = "lin_deal",
             remind = "\"The crate,\" says Matron Lin.  \"Then we talk.\"" },
        } },
      { id = "jade_force", title = "Taking the Den", kind = "main",
        stages = {
           { goal = "Take the splinter from Matron Lin, at the bottom of the Jade Den.",
             where = "Jade Den:3", kill = "lin",
             start = { anger = "lin" },
             log = [[No deals with the Circuit.  Lin wears the splinter at the bottom of her den, and her people will die for her.]] },
        } },

      { id = "road", title = "The Long Road", kind = "main",
        stages = {
           { goal = "Find Old Juno at the Long Road camp in the Fringe.",
             where = "nc-fringe", talk = "juno", dialogue = "juno_meet",
             log = [[The nomads of the Long Road took the last splinter west, Hex says.  They're camped on the Fringe, at the end of the metro line, and Old Juno speaks for them.]] },
           { goal = "Get the Long Road's splinter.", where = "nc-fringe",
             carry = "splinter_road" },
        },
        done = { advance = "act2" } },
      { id = "road_deal", title = "The Dustmother", kind = "main",
        stages = {
           { goal = "Find the Dustmother's nav core in the storm drains under Rustwater Docks.",
             where = "Storm Drains:3", carry = "navcore",
             log = [[The Drain Rats hijacked the convoy rig Dustmother and dragged her into the storm drains under Rustwater Docks.  The outfall is at the west end of the quay.  Her nav core holds every route the Long Road ever drove, and Juno will trade her splinter for it.]] },
           { goal = "Bring the nav core to Old Juno.", where = "nc-fringe",
             talk = "juno", give = "navcore", dialogue = "juno_deal",
             remind = "\"The Dustmother's core, %n,\" says Old Juno.  \"Then we'll talk.\"" },
        } },
      { id = "road_betray", title = "The Raid", kind = "main",
        stages = {
           { goal = "Meet Captain Brandt in the Long Road's vault, under the Boneyard in the Fringe.",
             where = "The Boneyard:2", talk = "brandt", dialogue = "brandt_meet",
             start = { rep = { halcyon = 15, longroad = -50 },
                       anger = { "juno", "rook" }, spawn = "brandt",
                       when = { { need = { active = "road_deal" },
                                  fail = "road_deal" } } },
             log = [[I told Vos where the Long Road keeps its splinter.  Halcyon raided the Boneyard, and Captain Brandt is waiting for me in the vault under it.  The Long Road will know who talked.]] },
        } },

      { id = "halcyon", title = "Halcyon's Splinter", kind = "main",
        stages = {
           { goal = "Get Halcyon's splinter: deal with Adrian Vos at Cafe Meridian in Chrome Heights, or take it from the vault at the top of Halcyon Tower.",
             where = "nc-chrome", carry = "splinter_halcyon",
             log = [[Halcyon keeps its splinter in the vault on the top floor of its tower in Chrome Heights.  Hex says Halcyon's man for deniable problems is Adrian Vos, who keeps a table at Cafe Meridian.  A deal, or a break-in.]] },
        },
        done = { when = {
                    { need = { noflag = "vos_deal" },
                      rep = { halcyon = -40 }, heat = 50, anger = "vos",
                      call = "vos",
                      text = [[That was Halcyon property, %n, and you took it out of our own tower.  I'm told the vault footage is very clear.

Corpsec has your face now.  So do I.]] },
                    { need = { active = "halcyon_deal" }, fail = "halcyon_deal" },
                 },
                 advance = "act2" } },
      { id = "halcyon_deal", title = "Vos's Offer", kind = "main",
        stages = {
           { goal = "Get the seized crate out of the customs cage in the Mara Sun's bottom hold, at Rustwater Docks.",
             where = "Mara Sun:3", carry = "crate",
             log = [[Vos wants a crate that Halcyon customs caged in the Mara Sun's bottom hold, before the Jade Circuit gets it back.  "Customs is a department," he says.  "Departments lose things."]] },
           { goal = "Bring the crate to Adrian Vos at Cafe Meridian.",
             where = "nc-chrome", talk = "vos", give = "crate",
             dialogue = "vos_deal",
             remind = "\"No crate, no splinter,\" says Vos, stirring his coffee." },
           { goal = "Collect Halcyon's splinter from the vault at the top of Halcyon Tower.",
             where = "Halcyon Tower:1", carry = "splinter_halcyon",
             log = [[Vos has cleared me with tower security.  The splinter is in the vault on the top floor, and I can walk in and take it.]] },
        } },

      -- act 3: the splinters sing, and someone takes Hex
      { id = "act3", title = "Ashfall", kind = "main",
        stages = {
           { goal = "Free Hex from her captor in the Seal Vault, under Ashfall.",
             where = "Seal Vault:2", flag = "hex_free",
             start = {
                unlock = "nc-ashfall", move = { hex = "Seal Vault:2" },
                cases = {
                   { need = { worst = "halcyon" }, spawn = "kask", call = "mags",
                     text = [[%n.  Listen.  Hex is gone.

Corpsec took her off the roof of Block 7 an hour ago: a dropship, a director in a long coat, the works.  Halcyon hasn't forgotten what you did to them.  They want what's in your head, and they want her to take it out.

My people say they flew her to Ashfall, to the old Halcyon lab.  The Seal Vault.  I've opened the line for you.  Go.]] },
                   { need = { worst = "jade" }, spawn = "shen", call = "mags",
                     text = [[%n.  Listen.  Hex is gone.

The Circuit took her: Red Pole Shen and a van full of Lin's boys.  The Jade Circuit hasn't forgotten what you did to them, and they want what's in your head.  They want her to take it out.

They took her to Ashfall, to the old Halcyon lab.  The Seal Vault.  I've opened the line for you.  Go.]] },
                   { spawn = "sable", call = "mags",
                     text = [[%n.  Listen.  Hex is gone.

Brother Sable and a dozen outriders took her off the roof of Block 7.  The Long Road hasn't forgotten what you did to them, and they want what's in your head.  They want her to take it out.

They rode for Ashfall, for the old Halcyon lab.  The Seal Vault.  I've opened the line for you.  Go.]] },
                } },
             log = [[The splinters started to sing to each other the moment I had all four, and someone heard.  Hex has been taken to the Seal Vault, under the ruins of Ashfall, where the splinters were cut sixty years ago.]] },
           { goal = "Talk to Hex at the Loom.", where = "Seal Vault:2",
             talk = "hex", dialogue = "hex_loom" },
        },
        done = { say = "Hex jacks the last cable into the Loom's line, and the Grid takes her somewhere you can't follow.",
                 call = "voice",
                 text = [[Little splinter.

You have done very well, carrying your copy of a copy across this city.  Sixty years ago they cut it from something much older, and that something is still where it was left, at the very bottom, under your feet.

The Shaft is open now.  Go and finish your little war.  When you tire of keys made of glass, come down and take the real one.  I will be waiting.  I am always waiting.]],
                 flag = "shaft_open", unlock = "nc-zenith",
                 move = { hex = "Zenith Spire:1" }, start = "act4" } },

      -- act 4: the Spire
      { id = "act4", title = "Zenith", kind = "main",
        stages = {
           { goal = "Get into the Zenith Spire, on the island of Zenith.",
             where = "nc-zenith", arrive = "Zenith Spire",
             log = [[Hex has gone ahead through the Grid, to the root console at the top of the Zenith Spire.  The Splinter key only works there.  The metro runs to Zenith now.]],
             done = { when = {
                { need = { rep = { jade = 20 } }, allies = { yakuza = 3 },
                  say = "Three of the Circuit's soldiers fall in beside you.  \"The Matron sends her regards.\"" },
                { need = { rep = { longroad = 20 } }, allies = { outrider = 2 },
                  say = "Two outriders shoulder through the lobby doors.  \"Juno says the Road rides with you.\"" },
                { need = { rep = { halcyon = 20 } },
                  allies = { ["corpsec lieutenant"] = 1 },
                  say = "A corpsec lieutenant salutes you.  \"Mr. Vos's compliments.  I'm to see you to the top.\"" },
             } } },
           { goal = "Climb to the crown of the Spire and destroy ARBITER.",
             where = "Zenith Spire:1", kill = "arbiter",
             log = [[ARBITER, Halcyon's guardian, holds the crown of the Spire.  Hex is waiting behind it, at the root console.]] },
           { goal = "Talk to Hex at the root console.", where = "Zenith Spire:1",
             talk = "hex", dialogue = "hex_console" },
        },
        done = { xp = 1000 } },

      -- after the end, if you stay
      { id = "undercity", title = "The Original", kind = "main",
        stages = {
           { goal = "Find the Amulet of Yendor at the bottom of the Undercity.  The Shaft is in Ashfall.",
             where = "nc-ashfall",
             log = [[The voice from below said the splinters were only a copy of a copy, and that the real keys are still at the very bottom of the Undercity.  The Shaft in Ashfall is open.]] },
        } },

      -- side job: Okoye's bill
      { id = "bill", title = "Okoye's Bill", kind = "side",
        stages = {
           { goal = "Pay Doc Okoye her 200 credits.", where = "nc-neonmile",
             talk = "okoye", dialogue = "okoye_bill",
             log = [[I owe Doc Okoye 200 credits for the stitches.  She'll be in her operating room on Neon Mile.]] },
        } },

      -- gig: Mags wants Benny's protection money
      { id = "rent", title = "Rent Day", kind = "gig",
        stages = {
           { goal = "Collect Benny Two-Tone's money in Lantern Market.",
             where = "nc-lantern", talk = "benny", dialogue = "benny_rent",
             log = [[Mags wants this week's money from Benny Two-Tone, who runs a stall in Lantern Market.  She'll give me a cut.]] },
           { goal = "Bring Benny's envelope to Mags.", where = "nc-neonmile",
             talk = "mags", give = "envelope", dialogue = "mags_rent" },
        } },
   },

   ----------------------------------------------------------------------
   -- how it ends; if you stay in the city, "stay" runs too
   endings = {
      corporate = { title = "Corporate Order",
         text = [[You put the key in, and Halcyon's logo fills the console.

By morning the Grid is quiet.  The metro runs on time.  The drones remember where they are.  The accounts balance, to the cent, and the cents go where Halcyon says.  In the Stacks the lights stay on for the first time in years, and so do the cameras.

Adrian Vos sends you a bottle of something very old and a card that says only THANK YOU FOR YOUR SERVICE.  Nobody ever finds out what was in Matron Lin's crate.

Neo Yendor is safe.  Neo Yendor is managed.  Neo Yendor is Halcyon's.]],
         stay = { start = "undercity" } },
      crown = { title = "Street Crown",
         text = [[You put the key in, and the console turns the colour of jade.

The Circuit doesn't fix the Grid; it rents it out.  Every lock in Neo Yendor opens for the right price, and the price is paid in Lantern Market, over tea.  Halcyon's towers go dark one floor at a time, and nobody on the street is sorry.

Corpsec still patrols Chrome Heights.  It just checks with the Circuit first.

The Matron's people bow to you in the market now.  Benny Two-Tone gives you a discount, and means it.]],
         stay = { start = "undercity" } },
      open = { title = "Open Grid",
         text = [[You put the key in, and Hex starts to laugh.

Then the key is everywhere: on every terminal, in every inbox, painted on the walls of the Stacks by kids who can't read it yet.  Every lock in Neo Yendor opens for everybody, and for a week the city is the strangest place on earth.

It doesn't fix everything.  It breaks a few things that won't be fixed.  But the Long Road's convoys roll out of the Fringe with the gates open behind them, and no one in Neo Yendor is ever locked out of their own life again.

Old Juno sends you a road map with every road on it.]],
         stay = { start = "undercity" } },
   },

   ----------------------------------------------------------------------
   -- ways the story keeps shut until a flag is set
   locks = {
      ["The Undercity"] = { flag = "shaft_open", text = [[Blast doors close the Shaft, welded shut, with a Halcyon seal glowing on every weld.  Not yet.]] },
   },

   ----------------------------------------------------------------------
   -- districts (and sites, by dungeon name): who turns up at random
   -- (monster = weight), the civilians kept around (monster = how many),
   -- and things you see and hear now and then.
   districts = {
      ["nc-neonmile"] = {
         monsters = { ["sewer rat"] = 3, pickpocket = 2, grifter = 2,
                      ["street punk"] = 2, ["trash robot"] = 2, jackal = 1,
                      ["grid bug"] = 1, bat = 1 },
         people = { citizen = 6, salaryman = 2, ["street vendor"] = 1 },
         sights = {
            "A holo-ad for Halcyon Life Insurance blooms over the street: PEACE OF MIND, PRE-APPROVED.",
            "Rain starts, stops, and leaves the neon running down the walls.",
            "A delivery drone weaves past at head height, blaring its horn.",
            "Somebody's dumped a broken cyberarm in a doorway.  It's still twitching.",
            "The sign over the Static Lounge buzzes and goes out, then comes back brighter.",
            "A billboard flickers to a wanted poster for a second, then back to noodles.",
         },
         sounds = {
            "Bass thumps through a wall somewhere.",
            "A siren rises across the city and fades.",
            "Somebody is laughing too hard, a street away.",
            "You hear the metro grind into the station.",
         },
      },
      ["nc-lantern"] = {
         monsters = { pickpocket = 3, grifter = 2, ["cat burglar"] = 1,
                      ["sewer rat"] = 2, raven = 1, yakuza = 1,
                      ["street punk"] = 1, ["printer drone"] = 1 },
         people = { citizen = 5, ["street vendor"] = 3, salaryman = 1 },
         sights = {
            "Paper lanterns sway over the stalls, each one running an ad.",
            "Steam rolls out of a dumpling cart and turns pink under the neon.",
            "Two yakuza walk the market slowly; the stallholders find something else to look at.",
            "A kid sells counterfeit firmware from a shoebox.",
         },
         sounds = {
            "Stallholders shout prices over each other.",
            "A wok hisses somewhere behind the stalls.",
            "Somewhere a radio plays a song you almost remember.",
         },
      },
      ["nc-stacks"] = {
         monsters = { ["street punk"] = 5, ["giant rat"] = 2, ["rabid rat"] = 1,
                      juicer = 1, ["black hat"] = 1, ["cable crawler"] = 1,
                      ["trash robot"] = 1, ["mutant runt"] = 1 },
         people = { citizen = 3 },
         sights = {
            "Forty floors up, somebody throws something out of a window.",
            "Fresh Chrome Saints tags drip down the side of Block 5.",
            "A stripped car sits on its rims, still smoking.",
            "Laundry and comm cables sag between the megablocks.",
         },
         sounds = {
            "Gunfire, three blocks over.  Then nothing.",
            "A dog barks itself hoarse somewhere in Block 9.",
            "An elevator cable screams inside Block 7.",
         },
      },
      ["Block 7"] = {
         monsters = { ["street punk"] = 5, ["black hat"] = 1, ["giant rat"] = 2,
                      juicer = 1, ["cable crawler"] = 1 },
      },
      ["Jade Den"] = {
         monsters = { yakuza = 5, ["black hat"] = 1, grifter = 1 },
      },
      ["nc-docks"] = {
         monsters = { smuggler = 4, ["sewer rat"] = 2, ["giant rat"] = 2,
                      ["street punk"] = 1, juicer = 1, ["trash robot"] = 1,
                      outrider = 1 },
         people = { citizen = 3, ["street vendor"] = 1 },
         sights = {
            "A crane swings a container overhead, and its shadow slides across the yard.",
            "Somebody is fishing off the quay.  You decide not to ask what for.",
            "A customs drone scans a container, beeps twice, and moves on.",
            "Oil rainbows drift on the black water.",
         },
         sounds = {
            "A ship's horn moans out in the harbour.",
            "Chains rattle somewhere among the containers.",
            "Water slaps against the quay.",
         },
      },
      ["nc-fringe"] = {
         monsters = { outrider = 3, coyote = 2, jackal = 2, ["trash robot"] = 1,
                      ["spider bot"] = 1, ["mutant runt"] = 1 },
         people = { citizen = 2, ["street vendor"] = 1 },
         sights = {
            "Dust devils spin along the fence and fall apart.",
            "A convoy's lights crawl along the horizon, heading west.",
            "Someone has painted LONG ROAD 3000 KM on a dead tree.",
            "A rig's engine coughs, catches and settles into a growl.",
         },
         sounds = {
            "The wind moans through the fence.",
            "Someone at the fire is singing a road song, badly.",
            "A dog howls out past the fence.",
         },
      },
      ["nc-chrome"] = {
         monsters = { ["corpsec trooper"] = 3, ["security drone"] = 2,
                      ["black hat"] = 1, pickpocket = 1, grifter = 1 },
         people = { ["corporate exec"] = 4, salaryman = 4, citizen = 1 },
         sights = {
            "A holo-ad turns to follow you: HALCYON - WE KEEP THE LIGHTS ON.",
            "A cleaning drone polishes the spot where you just stood.",
            "Someone's limousine glides past without touching the ground.",
            "The fountain's water rises in a perfect, silent arch.",
         },
         sounds = {
            "A calm voice announces that the plaza is a safe and happy zone.",
            "Glass doors hiss open and shut, open and shut.",
            "Somebody laughs at a joke about the Stacks.",
         },
      },
      ["nc-ashfall"] = {
         monsters = { ["mutant runt"] = 3, feral = 2, ghoul = 1,
                      ["scav zombie"] = 2, ["giant rat"] = 2, ["rabid rat"] = 1,
                      ["cable crawler"] = 1 },
         people = { citizen = 2 },
         sights = {
            "Ash drifts down like grey snow.",
            "A wall that was leaning yesterday isn't there today.",
            "Something moves in a window of a building with no floors.",
            "Steam rises from the Shaft's ring wall.",
         },
         sounds = {
            "The ground grumbles, deep down.",
            "Concrete groans and settles somewhere nearby.",
            "From the Shaft comes a sound like breathing.",
         },
      },
      ["nc-zenith"] = {
         monsters = { ["corpsec sergeant"] = 3, ["security drone"] = 2,
                      ["sentry bot"] = 1, ["combat synth"] = 1 },
         people = { ["corporate exec"] = 4, salaryman = 2 },
         sights = {
            "The Spire's lights pulse in time with the city's heartbeat.",
            "A yacht slides into the harbour without a sound.",
            "A checkpoint scanner sweeps over you.  It doesn't like what it sees.",
         },
         sounds = {
            "A string quartet is playing in the sky bar.",
            "The sea breathes against the island's wall.",
         },
      },
      ["Mara Sun"] = {
         monsters = { smuggler = 4, ["sewer rat"] = 2, ["corpsec trooper"] = 1 },
      },
      ["Storm Drains"] = {
         monsters = { ["street punk"] = 4, ["sewer rat"] = 3, ["giant rat"] = 2,
                      juicer = 1 },
      },
      ["The Boneyard"] = {
         monsters = { outrider = 3, ["trash robot"] = 2, ["spider bot"] = 1 },
      },
      ["Halcyon Tower"] = {
         monsters = { ["corpsec trooper"] = 4, ["corpsec sergeant"] = 1,
                      ["security drone"] = 2, ["sentry bot"] = 1 },
      },
      ["Seal Vault"] = {
         monsters = { ["mutant runt"] = 3, ghoul = 2, feral = 1,
                      ["scav zombie"] = 2 },
      },
      ["Zenith Spire"] = {
         monsters = { ["corpsec sergeant"] = 3, ["combat synth"] = 2,
                      ["sentry bot"] = 1, ["black ICE"] = 1 },
      },
   },

   ----------------------------------------------------------------------
   -- gigs: repeatable jobs.  A fixer (an NPC with fixer = true) offers
   -- up to three when you talk to them.  In the text, %T is the target,
   -- %P the place, %I the item and %F the fixer; $target, $item, $place,
   -- $fixer, $fixerhome and $reward stand for their ids and the pay.
   gigs = {
      { id = "courier", title = "Delivery to %T",
        fixers = { "mags" }, place = "district", spot = "metro",
        target = { "citizen", "salaryman", "street vendor" },
        names = { "Juno Reyes", "Old Pak", "Sister Venn", "Taz Okafor",
                  "Mina Kade", "Rusty Ilves", "Bo Tranh", "Kit Halloran" },
        items = { "sealed package", "insulated medcase", "box of bootleg chips",
                  "taped-up briefcase", "crate of synth-whisky" },
        reward = 30,
        stages = {
           { goal = "Deliver the %I to %T, waiting by the metro in %P.",
             where = "$place", talk = "$target", give = "$item",
             start = { item = "$item", spawn = "$target" },
             remind = "\"That's not what I'm waiting for,\" says %T." },
        },
        done = { credits = "$reward", despawn = "$target", call = "$fixer",
                 text = [[%T has the %I, no questions asked.  Your cut is on its way: $reward credits.  Come see me for more.]] } },

      { id = "bounty", title = "Bounty: %T",
        fixers = { "mags" }, place = "site", hostile = true,
        target = { "street punk", "black hat", "juicer" },
        names = { "Knuckles Mbeki", "Vex", "Moth", "Static Pete", "Lulu Razor",
                  "Doc Nails", "Two-Bit Ramos", "Wiremouth" },
        reward = 40,
        stages = {
           { goal = "Flatline %T, holed up in %P.", where = "$place",
             kill = "$target", start = { spawn = "$target" } },
        },
        done = { credits = "$reward", call = "$fixer",
                 text = [[Word on the wire says %T won't be collecting anymore.  $reward credits, as promised.]] } },

      { id = "retrieval", title = "Retrieval: the %I",
        fixers = { "mags" }, place = "site", drop = true,
        items = { "client's data shard", "stolen keycard", "black-box recorder",
                  "encrypted ledger", "prototype optic" },
        reward = 35,
        stages = {
           { goal = "Find the %I somewhere in %P.", where = "$place",
             carry = "$item" },
           { goal = "Bring the %I to %F.", where = "$fixerhome",
             talk = "$fixer", give = "$item",
             remind = "\"No %I, no pay,\" says %F." },
        },
        done = { credits = "$reward" } },

      -- the factions' fixers: their jobs move your standing with them
      { id = "collection", title = "Collection: %T",
        fixers = { "benny" }, place = "district", spot = "metro",
        target = { "citizen", "street vendor", "salaryman" },
        names = { "Lucky Feng", "Dory Maas", "Sunny Okonkwo", "Paz Delgado",
                  "Ivo Brenner", "Grandma Tsai" },
        items = { "envelope of Circuit money", "jade tally token",
                  "tin of \"tea\"" },
        reward = 30,
        stages = {
           { goal = "Collect what %T owes the Circuit, by the metro in %P.",
             where = "$place", talk = "$target",
             start = { spawn = "$target" },
             done = { item = "$item", despawn = "$target",
                      say = "%T hands it over without a word." } },
           { goal = "Bring the %I to %F.", where = "$fixerhome",
             talk = "$fixer", give = "$item",
             remind = "\"Where's the %I?\" says %F." },
        },
        done = { credits = "$reward", rep = { jade = 3 } } },
      { id = "circuit_bounty", title = "Circuit business: %T",
        fixers = { "benny" }, place = "site", hostile = true,
        target = { "smuggler", "black hat", "street punk" },
        names = { "Slick Moreau", "Fingers Oyelaran", "the Weasel",
                  "Kat Szabo", "Lefty Ng" },
        reward = 45,
        stages = {
           { goal = "Flatline %T, who cheated the Circuit, holed up in %P.",
             where = "$place", kill = "$target",
             start = { spawn = "$target" } },
        },
        done = { credits = "$reward", rep = { jade = 3 }, call = "$fixer",
                 text = [[The Matron heard about %T.  She was pleased.  Your money's at my stall: $reward credits.]] } },
      { id = "parts_run", title = "Parts run: the %I",
        fixers = { "rook" }, place = "site", drop = true,
        items = { "fuel cell", "injector rack", "radio mast", "roll of solar film" },
        reward = 35,
        stages = {
           { goal = "Find the %I somewhere in %P.", where = "$place",
             carry = "$item" },
           { goal = "Bring the %I to %F.", where = "$fixerhome",
             talk = "$fixer", give = "$item",
             remind = "\"The %I, %n,\" says %F." },
        },
        done = { credits = "$reward", rep = { longroad = 3 } } },
      { id = "road_bounty", title = "Drain Rat: %T",
        fixers = { "rook" }, place = "site", hostile = true,
        target = { "street punk", "juicer" },
        names = { "Gutter Tom", "Mother Rust", "Pipe", "Little Sump",
                  "Grease Kowalski" },
        reward = 40,
        stages = {
           { goal = "Flatline %T, a Drain Rat who robbed a convoy, holed up in %P.",
             where = "$place", kill = "$target",
             start = { spawn = "$target" } },
        },
        done = { credits = "$reward", rep = { longroad = 3 }, call = "$fixer",
                 text = [[%T won't be robbing any more convoys.  The Road pays its debts: $reward credits.]] } },
      { id = "asset_recovery", title = "Asset recovery: the %I",
        fixers = { "vos" }, place = "site", drop = true,
        items = { "prototype optic", "encrypted drive", "sealed sample case",
                  "executive's tablet" },
        reward = 45,
        stages = {
           { goal = "Recover Halcyon's %I, lost somewhere in %P.",
             where = "$place", carry = "$item" },
           { goal = "Bring the %I to %F.", where = "$fixerhome",
             talk = "$fixer", give = "$item",
             remind = "\"The %I,\" says %F.  \"I'm not paying for nothing.\"" },
        },
        done = { credits = "$reward", rep = { halcyon = 3 } } },
      { id = "termination", title = "Termination: %T",
        fixers = { "vos" }, place = "site", hostile = true,
        target = { "black hat", "smuggler" },
        names = { "Null Byte", "Ada Kettering", "Spoofy", "Dmitri Vale",
                  "the Locksmith" },
        reward = 50,
        stages = {
           { goal = "Terminate %T, who stole from Halcyon, holed up in %P.",
             where = "$place", kill = "$target",
             start = { spawn = "$target" } },
        },
        done = { credits = "$reward", rep = { halcyon = 3 }, call = "$fixer",
                 text = [[Halcyon thanks you for resolving the %T situation.  $reward credits have been transferred, net of nothing at all.]] } },
   },

   ----------------------------------------------------------------------
   dialogue = {
      -- prologue
      okoye_wake = {
         text = [[Easy.  Don't sit up yet.

You're in my clinic on Neon Mile.  Somebody dumped you on my doorstep three nights ago with a hole behind your ear and a chip in it that isn't one of mine.  I closed you up.  You're welcome.

The bill is 200 credits.]],
         choices = {
            { "What chip?", go = "okoye_chip" },
            { "Who brought me in?", go = "okoye_who" },
            { "I don't have 200 credits.", go = "okoye_work" },
         } },
      okoye_ask = {
         text = [[Doc Okoye waits, arms folded.]],
         choices = {
            { "What chip?", go = "okoye_chip" },
            { "Who brought me in?", go = "okoye_who" },
            { "About the money...", go = "okoye_work" },
         } },
      okoye_chip = {
         text = [[Military encryption, no maker's mark, and wired into your auditory nerve by someone with shaky hands.  It wrote one word into your boot log before I shut the port: SPLINTER.

I don't know what that means and I don't want to.  Don't let anybody scan you.]],
         choices = { { "Go on.", go = "okoye_ask" } } },
      okoye_who = {
         text = [[A nomad kid in road leathers.  Paid cash for the stitches, not for the rest of it, and was gone before the anaesthetic wore off.  No name.  They never leave a name.]],
         choices = { { "Go on.", go = "okoye_ask" } } },
      okoye_work = {
         text = [[Then you'll earn it.  Mags Vance runs the back of the Static Lounge, right next door, across the alley.  She always needs runners who don't ask questions.

Tell her Okoye sent you.  And come back when you've got my money.]],
         choices = { { "I'll go and see her." } } },

      mags_meet = {
         text = [[So you're Okoye's stray.  She says you owe her, which means you'll work cheap.

I've got a cold case for Dr. Sato in Lantern Market.  Medical.  Sealed.  You don't open it, you don't drop it, you don't stop for noodles.  Sato pays on delivery.]],
         choices = {
            { "What's in it?", go = "mags_case" },
            { "I'll take it.", item = "coldcase", unlock = "nc-lantern",
              say = "Mags slides a frosted case across the bar." },
            { "Not right now.", wait = true },
         } },
      mags_case = {
         text = [[Kidneys, I hope.  Do you want the job or not?]],
         choices = {
            { "I'll take it.", item = "coldcase", unlock = "nc-lantern",
              say = "Mags slides a frosted case across the bar." },
            { "Not right now.", wait = true },
         } },

      -- act 1
      sato_case = {
         text = [[Still cold.  Good.  150, as agreed.

...Hold still.  Okoye told me you've got a passenger.  Let me look.

Hm.  That isn't cyberware.  It's a key, or a piece of one, and it's calling home: every time you pass a Halcyon node it whispers where you are.  Somebody is going to come for it, and they won't take it out gently.

You need a real netrunner to tell you what it is.  The best one I know used to code for Halcyon.  Hex.  Last I heard she'd locked herself in a server room on the roof of Block 7, in the Stacks.  The Chrome Saints have been sitting on the stairwells for a week.]],
         choices = {
            { "Thanks, Doc.", credits = 150, unlock = "nc-stacks" },
         } },

      hex_meet = {
         text = [[Stop right there.

...Oh.  Oh, you're the splinter.  I can hear it from here.

Sit.  No, not on that, it's holding the building's grid together.  The Saints have been trying to get in for a week.  Their boss, Saint Lazarus, took my deck core when they jumped me on the fourth floor.  Without it I can't read anything, including you.

Get it back and I'll tell you what you're carrying.]],
         choices = {
            { "Where do I find him?", go = "hex_where" },
            { "I'm on it." },
         } },
      hex_where = {
         text = [[Fourth floor.  Follow the screaming.]],
         choices = { { "I'm on it." } } },

      hex_read = {
         text = [[Okay.  Okay okay okay.  Hold still.

...You know the old story about the Amulet of Yendor?  The root keys of the Grid, buried at the bottom of the Undercity, right under our feet?  It isn't a story.  And sixty years ago Halcyon Dynamics cut a copy of those keys.  Too dangerous to keep in one place, so they split it into four splinters and gave them to four keepers.

One of them is in your head.

Halcyon kept one in its tower in Chrome Heights.  One went to the Jade Circuit as the price of a truce, and Matron Lin wears it in her den under Lantern Market.  The last one went west with the nomads of the Long Road.

Put the four back together and you hold Neo Yendor's Grid: every lock, every camera, every account in the city.  Everyone who knows about the splinters is going to want yours, %n.  So you'd better decide who gets theirs.]],
         choices = {
            { "Then I'd better find the other three first.",
              unlock = { "nc-docks", "nc-fringe", "nc-chrome" } },
         } },

      -- act 2: the Jade Circuit
      benny_chat = {
         text = [[Benny Two-Tone looks up from a tray of optics.  "What do you need?"]],
         choices = {
            { "Get me in to see Matron Lin.",
              need = { active = "jade", noflag = "jade_route", rep = { jade = -14 } },
              go = "benny_lin" },
            { "Get me in to see Matron Lin.",
              need = { active = "jade", noflag = "jade_route", norep = { jade = -14 } },
              go = "benny_nolin" },
            { "Got any work for me?", gigs = true },
            { "Nothing.  See you, Benny." },
         } },
      benny_lin = {
         text = [[Lin doesn't see just anybody.  But you've done right by the market, more or less, so I'll make the call.

Go down through the den.  The boys will let you pass.  Don't touch anything, don't stare, and don't make me look bad.]],
         choices = {
            { "Thanks, Benny.", flag = "jade_route", rep = { jade = 15 },
              start = "jade_deal" },
            { "Forget it.  I'll take the splinter my own way.",
              flag = "jade_route", start = "jade_force" },
         } },
      benny_nolin = {
         text = [[See Lin?  Lin wants to see you too: in pieces.  After what you did to Circuit people, nobody's making that call for you.  If you go down into that den, you go down fighting.]],
         choices = {
            { "Then I go down fighting.", flag = "jade_route",
              start = "jade_force" },
            { "Maybe later." },
         } },
      lin_meet = {
         text = [[So.  The splinter-carrier.  Benny says you have manners.  We will see.

Sit.  Tea?  ...No?  Then business.

Three days ago Halcyon customs took a crate of mine off the Mara Sun and locked it in a cage in her bottom hold.  What is in it is mine.  Bring it to me unopened, and I will give you what I wear around my neck.  The Circuit keeps its bargains.]],
         choices = {
            { "I'll bring you your crate.", need = { noflag = "crate_gone" } },
            { "Halcyon has the crate now.  I gave it to Vos.",
              need = { flag = "crate_gone" }, go = "lin_nocrate" },
            { "I'll take the splinter now, thanks.", go = "lin_war" },
            { "Let me think about it.", wait = true },
         } },
      lin_war = {
         text = [[Matron Lin sets down her cup, very carefully.

Then take it.]],
         choices = {
            { "...", rep = { jade = -20 }, fail = "jade_deal",
              start = "jade_force" },
         } },
      lin_nocrate = {
         text = [[Then you sold the Circuit to Halcyon, and you came here to tell me so.  That is either very brave or very stupid.

Get out of my house while you can still walk.  The next time we meet, it will be the last time.]],
         choices = {
            { "...", rep = { jade = -20 }, fail = "jade_deal",
              start = "jade_force" },
         } },
      lin_deal = {
         text = [[Matron Lin runs a fingernail along the customs seal, and it falls apart.  She doesn't open the lid.

Good.  Unopened, as agreed.

She lifts a jade locket over her head and puts it in your hand.  It is heavier than it looks.

Thirty years I have worn it.  The Circuit keeps its bargains, %n.  See that you keep yours.]],
         choices = {
            { "Thank you, Matron.", flag = "crate_gone", item = "splinter_jade",
              rep = { jade = 20 },
              when = { { need = { active = "halcyon_deal" },
                         fail = "halcyon_deal" } } },
         } },

      -- act 2: the Long Road
      juno_meet = {
         text = [[Old Juno looks at you for a long time, the way people look at the weather.

So the splinter found its way back to the Road.  Funny.  We're the ones who brought you to Okoye, you know.  Found you in a ditch on the coast road with that thing in your head and nobody's name on you.

Ours?  We keep it in the Boneyard, in the vault under the scrap, where it's been kept for forty years.  We never wanted it.  You can have it.  But nothing's free on the Road.

Three weeks ago the Drain Rats hijacked the Dustmother on the coast road and dragged her into the storm drains under the Docks.  Her nav core has every route the Long Road ever drove.  Bring it home, and the splinter is yours.]],
         choices = {
            { "I'll bring back the nav core.",
              flag = { "boneyard_known", "road_route" }, start = "road_deal" },
            { "I'll think about it.", flag = "boneyard_known" },
         } },
      juno_chat = {
         text = [[Old Juno nods at you.  "%n."]],
         choices = {
            { "About the Dustmother...",
              need = { active = "road", noflag = "road_route" },
              flag = "road_route", start = "road_deal",
              say = "\"Storm drains under the Docks,\" says Juno.  \"Bring her heart home.\"" },
            { "Nothing.  Ride safe." },
         } },
      juno_deal = {
         text = [[Juno turns the nav core over in her hands, and for a moment she's somewhere else, a long way west.

Forty years of roads in here.  Thank you, %n.

She pulls a leather cord over her head.  On it is a splinter, worn smooth.

Keep it close.  And whatever you end up doing with the four of them, remember the people who just want to drive.]],
         choices = {
            { "I'll remember.", item = "splinter_road", rep = { longroad = 20 } },
         } },
      rook_chat = {
         text = [[Rook looks up from the fire.  "Need something?"]],
         choices = {
            { "Got any work?", gigs = true },
            { "Just warming my hands." },
         } },
      brandt_meet = {
         text = [[Captain Brandt is standing over a broken strongbox, rifle slung, as relaxed as if this were her own office.

Clean sweep.  The nomads won't be using this vault again.

She tosses you a splinter on a leather cord.

Mr. Vos says that's yours, for now.  He'll be in touch about the rest.]],
         choices = {
            { "Pleasure doing business.", item = "splinter_road",
              flag = "road_betrayed" },
         } },

      -- act 2: Halcyon
      vos_chat = {
         text = [[Adrian Vos looks up from his coffee and smiles the way a bank smiles.  "Ah.  The walking security breach."]],
         choices = {
            { "About Halcyon's splinter...",
              need = { active = "halcyon", noflag = "halcyon_route" },
              go = "vos_offer" },
            { "The Long Road keeps their splinter in the Boneyard.",
              need = { flag = "boneyard_known", noflag = "road_route",
                       active = "road" },
              go = "vos_betray" },
            { "Got any work?", gigs = true },
            { "Nothing.  Enjoy your coffee." },
         } },
      vos_offer = {
         text = [[Here is Halcyon's position.  That splinter in your head is Halcyon property.  We could have it removed, but surgery is so messy.

So: a favour.  The Jade Circuit smuggled a crate in on the Mara Sun.  Our customs people caged it in her bottom hold, and Matron Lin wants it back badly enough that I want it more.  Bring it to me, and I'll lend you our splinter.  Lend.  When the four are together, we'll talk about who holds them.]],
         choices = {
            { "Deal.", need = { noflag = "crate_gone" },
              flag = "halcyon_route", start = "halcyon_deal" },
            { "The crate's gone.  Lin has it.", need = { flag = "crate_gone" },
              go = "vos_nocrate" },
            { "No deal." },
         } },
      vos_nocrate = {
         text = [[Then you'll have to get ours the hard way.  I'd advise against it, %n.  Halcyon has a long memory and a very large security budget.]] },
      vos_betray = {
         text = [[The Boneyard.  Under the Fringe scrapyard.  How very nomadic of them.

Thank you, %n.  Halcyon will go in tonight.  Meet Captain Brandt in the vault afterwards; she'll have something of theirs for you, as a gesture.  The Long Road will be... upset.]],
         choices = {
            { "...", flag = "road_route", start = "road_betray" },
            { "On second thought, forget I said anything.", wait = true },
         } },
      vos_deal = {
         text = [[Vos lifts the lid an inch, looks, and closes it again.  Whatever is inside, he seems pleased.

Splendid.  I've told tower security to expect you.  Our splinter is in the vault on the top floor of Halcyon Tower; walk in, take it, walk out.  Nobody will stop you.

Try not to steal anything else while you're up there.]],
         choices = {
            { "Pleasure.", flag = { "vos_deal", "crate_gone" },
              rep = { halcyon = 20, jade = -20 },
              when = { { need = { active = "jade_deal" },
                         fail = "jade_deal" } } },
         } },

      -- act 3
      hex_loom = {
         text = [[Hex is sitting at the Loom's console with her hands in its guts, and she doesn't look round.

You came.  Of course you came.  Do you know where we are?  This is where they cut the splinters, sixty years ago.  The Loom.  It can put them back together.

Give me the three you've got.  The fourth one stays in your head; it's the one that'll turn the key.]],
         choices = {
            { "Here.  All three.",
              need = { item = { "splinter_jade", "splinter_road", "splinter_halcyon" } },
              take = { "splinter_jade", "splinter_road", "splinter_halcyon" },
              item = "splinterkey", go = "hex_loom2" },
            { "Not yet.", wait = true },
         } },
      hex_loom2 = {
         text = [[The Loom wakes up with a sound like a cathedral clearing its throat.  Light runs along the splinters, then through them, then through you.

When it's over there's a key in your hand: one piece of black glass, warm as skin.

It works in one place, %n.  The root console, at the crown of the Zenith Spire.  I'll go ahead through the Grid and open what I can.  You bring the key, and your body, and whoever still likes you.]] },

      -- act 4: the end
      hex_console = {
         text = [[Hex is jacked into the root console, three cables in her neck and her eyes full of light.

There you are.  It's all here, %n: every lock, every camera, every account in Neo Yendor, waiting for a key.  Put it in and choose.  I'll do whatever you say.  I just want it to be you who says it.]],
         choices = {
            { "Give the keys to Halcyon.  Order beats chaos.",
              take = "splinterkey", ending = "corporate" },
            { "Give them to the Jade Circuit.  The street should run the street.",
              take = "splinterkey", ending = "crown" },
            { "Broadcast them.  Everyone gets the keys.",
              take = "splinterkey", ending = "open" },
            { "Not yet.", wait = true },
         } },

      -- side job
      okoye_bill = {
         text = [[Come to settle up?]],
         choices = {
            { "Here's your 200.", need = { credits = 200 }, credits = -200,
              go = "okoye_paid" },
            { "Not yet.", wait = true },
         } },
      okoye_paid = {
         text = [[Pleasure doing business.  Here, on the house; you look like you'll need them.  And if you flatline, crawl back here.  I'll bill you again.]],
         choices = {
            { "Thanks, Doc.", object = "2 uncursed medkits" },
         } },

      -- gig
      benny_rent = {
         text = [[Mags sent you?  Of course she did.

Look, business is dead.  The Circuit already took their cut this week, and I've got nothing for her.  Nothing!]],
         choices = {
            { "Pay up, Benny, or I take the stall apart.",
              go = "benny_threat", item = "envelope", rep = { jade = -5 } },
            { "I'll cover it this week.  (100 credits)",
              need = { credits = 100 }, credits = -100, item = "envelope",
              rep = { jade = 5 }, go = "benny_grateful" },
            { "Not my problem.", wait = true },
         } },
      benny_threat = {
         text = [[All right, all right!  Here, take it to her.  It's sealed, so don't get ideas.  And when the Circuit asks who's been leaning on their people, I'm giving them your face.]] },
      benny_grateful = {
         text = [[You'd do that?  For Benny?  ...Here, it's sealed, Mags likes it that way.  I won't forget this.  Neither will Matron Lin; she likes people who look after her people.]] },
      mags_rent = {
         text = [[Benny paid?  Huh.  I had a bet with myself that you'd come back bleeding.  Here's your cut.]],
         choices = { { "Pleasure.", credits = 60 } } },
   },
}
