-- NeonHack 2: The Splinter -- the story of Neo Yendor.
--
-- This file is data; src/story.c runs it.  doc/story-format.md has the
-- format.  Check it in a debug-mode game with #wizstorycheck.
--
-- So far: the prologue (Neon Mile), act 1 (Lantern Market, the Stacks,
-- Block 7), a side job and a gig.  Acts 2-4 come later.

story = {
   factions = {
      halcyon = "Halcyon Dynamics",
      jade = "the Jade Circuit",
      longroad = "the Long Road",
   },

   items = {
      coldcase = { name = "frosted cold case" },
      envelope = { name = "sealed envelope marked MAGS" },
      hexcore = { name = "scorched deck core" },
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
         home = "nc-neonmile", stay = true,
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

      -- act 2 is not written yet
      { id = "act2", title = "Three Splinters", kind = "main",
        stages = {
           { goal = "Find the other three splinters (act 2 is coming in a later build).",
             log = [[Hex told me what I'm carrying: one of four splinters of a copy of the Grid's root keys.  Halcyon has one in Chrome Heights, Matron Lin of the Jade Circuit wears one under Lantern Market, and the nomads of the Long Road took the last one west.]] },
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
