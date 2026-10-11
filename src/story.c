/* NeonHack 2   story.c   quests, NPCs and the journal */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* city map: True if a district has an objective of the tracked quest */
boolean
story_district_marker(int district UNUSED)
{
    return FALSE;
}

/* city map: append objectives in a district to a note */
void
story_district_note(int district UNUSED, char *buf UNUSED, int bufsz UNUSED)
{
    return;
}

/*story.c*/
