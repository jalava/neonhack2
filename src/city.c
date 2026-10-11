/* NeonHack 2   city.c   Neo Yendor: districts, sites and getting around */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The city is a set of "city" dungeons (dat/dungeon.lua):
 *
 *   - "Neo Yendor", whose levels are the districts.  Districts don't
 *     connect by stairs; you travel between them by metro or taxi.
 *   - one dungeon per site (a building, the storm drains, ...), entered
 *     from a lobby on its parent district's map.
 *
 * City levels take their difficulty from a danger rating instead of
 * their depth, and the usual shortcuts between levels (level teleport,
 * digging down, falling, rising through the ceiling) don't cross
 * between districts.
 */

#include "hack.h"

staticfn int site_floor(d_level *);

/* arrival spec for the next level change (metro, taxi, respawn) */
static struct city_arrival {
    int kind;
    coordxy x, y;
} arrival;

boolean
In_city(d_level *lev)
{
    return (boolean) (lev->dnum >= 0 && lev->dnum < svn.n_dgns
                      && svd.dungeons[lev->dnum].flags.city);
}

boolean
In_district(d_level *lev)
{
    return (boolean) (city_dnum >= 0 && lev->dnum == city_dnum
                      && lev->dlevel > 0);
}

/* the district a site's lobby is in; the district itself for a district */
boolean
city_district_of(d_level *lev, d_level *district)
{
    if (In_district(lev)) {
        assign_level(district, lev);
        return TRUE;
    }
    if (In_city(lev) && svd.dungeons[lev->dnum].parent.dlevel > 0) {
        assign_level(district, &svd.dungeons[lev->dnum].parent);
        return TRUE;
    }
    return FALSE;
}

/* difficulty of a city level: the district's danger, or the site's
   danger plus one for each floor away from its entrance */
int
city_difficulty(d_level *lev)
{
    dungeon *dptr = &svd.dungeons[lev->dnum];
    s_level *sp;
    int res;

    if (In_district(lev)) {
        sp = Is_special(lev);
        res = (sp && sp->danger) ? sp->danger : 1;
    } else {
        res = dptr->danger ? dptr->danger : 1;
        res += abs(lev->dlevel - dptr->entry_lev);
    }
    return res;
}

/* floor number within a site: towers count up from the lobby (1F) on
   the district map, basements and tunnels count down (B1, B2, ...) */
staticfn int
site_floor(d_level *lev)
{
    dungeon *dptr = &svd.dungeons[lev->dnum];

    if (builds_up(lev))
        return dptr->entry_lev - lev->dlevel + 2;
    return -(lev->dlevel - dptr->entry_lev + 1);
}

/* name of a city level: "Lantern Market", or "Block 7 3F" ('brief' gives
   the short form used on the status line); returns buf */
char *
city_place_name(d_level *lev, char *buf, boolean brief)
{
    dungeon *dptr = &svd.dungeons[lev->dnum];
    s_level *sp;
    int floor;

    if (In_district(lev)) {
        sp = Is_special(lev);
        if (!sp)
            Sprintf(buf, "%s", dptr->dname);
        else if (brief && *sp->shortname)
            Sprintf(buf, "%s", sp->shortname);
        else if (*sp->title)
            Sprintf(buf, "%s", sp->title);
        else
            Sprintf(buf, "%s", sp->proto);
        return buf;
    }
    floor = site_floor(lev);
    Sprintf(buf, (floor < 0) ? "%s B%d" : "%s %dF",
            (brief && *dptr->shortname) ? dptr->shortname : dptr->dname,
            abs(floor));
    return buf;
}

/* resolve a stair destination given in a level file:
     "nc-ashfall"        a special level by name
     "Block 7"           the entrance of a dungeon, by name
     "The Undercity:1"   a dungeon and level */
boolean
city_resolve_dest(const char *name, d_level *dest)
{
    char buf[BUFSZ], *colon;
    s_level *sp;
    xint16 dnum;

    copynchars(buf, name, (int) sizeof buf - 1);
    if ((colon = strchr(buf, ':')) != 0) {
        *colon = '\0';
        if ((dnum = dname_lookup(buf)) < 0)
            return FALSE;
        dest->dnum = dnum;
        dest->dlevel = atoi(colon + 1);
        return (boolean) (dest->dlevel >= 1
                          && dest->dlevel <= svd.dungeons[dnum].num_dunlevs);
    }
    if ((sp = find_level(buf)) != 0) {
        assign_level(dest, &sp->dlevel);
        return TRUE;
    }
    if ((dnum = dname_lookup(buf)) >= 0) {
        dest->dnum = dnum;
        dest->dlevel = svd.dungeons[dnum].entry_lev;
        return TRUE;
    }
    return FALSE;
}

/* where a new game starts: the first district */
void
city_start_level(void)
{
    if (citystart_level.dlevel <= 0)
        return; /* no city in dungeon.lua; NetHack's start */
    assign_level(&u.uz, &citystart_level);
    u.utolev = u.uz;
    if (!svd.dungeons[u.uz.dnum].dunlev_ureached)
        svd.dungeons[u.uz.dnum].dunlev_ureached = u.uz.dlevel;
}

/* the up stairs on Undercity 1 lead back to the shaft in Ashfall */
boolean
city_shaft_level(d_level *lev)
{
    if (ashfall_level.dlevel <= 0)
        return FALSE;
    assign_level(lev, &ashfall_level);
    return TRUE;
}

/* the city blocks the shortcuts between districts and sites */
boolean
city_no_shortcuts(d_level *lev)
{
    return In_district(lev);
}

/* set where the hero lands on the next level change */
void
city_set_arrival(int kind, coordxy x, coordxy y)
{
    arrival.kind = kind;
    arrival.x = x;
    arrival.y = y;
}

/* place the hero according to a pending arrival spec; False if none */
boolean
city_place_arrival(void)
{
    int kind = arrival.kind;
    stairway *stway;
    char nbuf[BUFSZ];

    arrival.kind = CITY_ARRIVE_NONE;
    switch (kind) {
    case CITY_ARRIVE_XY:
        if (isok(arrival.x, arrival.y)) {
            u_on_newpos(arrival.x, arrival.y);
            return TRUE;
        }
        break;
    case CITY_ARRIVE_METRO:
        for (stway = gs.stairs; stway; stway = stway->next)
            if (stway->kind == STWAY_METRO) {
                u_on_newpos(stway->sx, stway->sy);
                return TRUE;
            }
        impossible("No metro station in %s?",
                   city_place_name(&u.uz, nbuf, FALSE));
        break;
    case CITY_ARRIVE_SHAFT:
        for (stway = gs.stairs; stway; stway = stway->next)
            if (stway->tolev.dnum == 0 && stway->tolev.dlevel == 1) {
                u_on_newpos(stway->sx, stway->sy);
                return TRUE;
            }
        impossible("No shaft down to the Undercity in %s?",
                   city_place_name(&u.uz, nbuf, FALSE));
        break;
    default:
        return FALSE;
    }
    u_on_rndspot(0);
    return TRUE;
}

/* arriving by stairs in the city without stairs leading back: the level
   files disagree about where their stairs go */
void
city_lost_stairs(void)
{
    char here[BUFSZ], from[BUFSZ];

    impossible("No stairs in %s lead back to %s.",
               city_place_name(&u.uz, here, FALSE),
               In_city(&u.uz0) ? city_place_name(&u.uz0, from, FALSE)
                               : svd.dungeons[u.uz0.dnum].dname);
    u_on_rndspot(0);
}

/*city.c*/
