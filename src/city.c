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
staticfn s_level *district_level(int);
staticfn int district_count(void);
staticfn boolean district_open(int);
staticfn void citymap_art(winid, int);
staticfn int citymap_menu(int, const char *, boolean);
staticfn boolean taxi_refused(void);
staticfn long taxi_fare(int, int);
staticfn void city_travel(int, boolean);

/* arrival spec for the next level change (metro, taxi, respawn) */
static struct city_arrival {
    int kind;
    coordxy x, y;
} arrival;

/* city state that goes into the save file; districts are numbered by
   their level in Neo Yendor (bit n is district n) */
static struct city_state {
    unsigned long unlocked; /* districts you can travel to */
    unsigned long visited;  /* districts you have been to */
} cstate;

/* where each district sits on the city map: column, row */
static const struct district_spot {
    const char *proto;
    int col, row;
} citylayout[] = {
    { "nc-fringe", 2, 0 },   { "nc-docks", 24, 0 },
    { "nc-ashfall", 2, 2 },  { "nc-neonmile", 24, 2 },
    { "nc-lantern", 46, 2 }, { "nc-stacks", 2, 4 },
    { "nc-chrome", 24, 4 },  { "nc-zenith", 46, 4 },
};
#define CITYMAP_W 64
#define CITYMAP_H 5

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

/* the same, for use inside a sentence: "the Stacks", "Block 7 3F" */
char *
city_place_phrase(d_level *lev, char *buf)
{
    (void) city_place_name(lev, buf, FALSE);
    if (!strncmp(buf, "The ", 4))
        buf[0] = 't';
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
               city_place_phrase(&u.uz, here),
               In_city(&u.uz0) ? city_place_phrase(&u.uz0, from)
                               : svd.dungeons[u.uz0.dnum].dname);
    u_on_rndspot(0);
}

/*
 * City state: which districts are open and visited.
 */

staticfn s_level *
district_level(int n)
{
    d_level lev;

    lev.dnum = city_dnum;
    lev.dlevel = n;
    return Is_special(&lev);
}

staticfn int
district_count(void)
{
    if (city_dnum < 0 || city_dnum >= svn.n_dgns)
        return 0;
    return svd.dungeons[city_dnum].num_dunlevs;
}

staticfn boolean
district_open(int n)
{
    return (boolean) ((cstate.unlocked & (1UL << n)) != 0);
}

/* new game: districts marked "locked" in dungeon.lua start closed */
void
city_init_state(void)
{
    int n;
    s_level *sp;

    cstate.unlocked = cstate.visited = 0UL;
    for (n = 1; n <= district_count(); n++)
        if ((sp = district_level(n)) != 0 && !sp->locked)
            cstate.unlocked |= 1UL << n;
}

/* open a district for travel (story progress, or wizard mode) */
void
city_unlock(int n, boolean verbose)
{
    char nbuf[BUFSZ];
    d_level lev;

    if (n < 1 || n > district_count() || district_open(n))
        return;
    cstate.unlocked |= 1UL << n;
    if (verbose) {
        lev.dnum = city_dnum, lev.dlevel = n;
        pline("%s is open to you now.", upstart(city_place_phrase(&lev, nbuf)));
    }
}

/* district number by its level name ("nc-stacks"), 0 if none */
int
city_district_num(const char *proto)
{
    s_level *sp = find_level(proto);

    if (sp && In_district(&sp->dlevel))
        return sp->dlevel.dlevel;
    return 0;
}

/* called after every arrival on a level, and at the start of a game */
void
city_arrived(void)
{
    if (In_district(&u.uz))
        cstate.visited |= 1UL << u.uz.dlevel;
    story_arrived();
}

/* save: append the city's lines */
int
city_state_lines(char **lines, int n)
{
    lines[n++] = dupstr("city 1"); /* format version */
    {
        char buf[BUFSZ];

        Sprintf(buf, "districts %lx %lx", cstate.unlocked, cstate.visited);
        lines[n++] = dupstr(buf);
    }
    return n;
}

/* restore: one line; False if it isn't the city's */
boolean
city_state_line(const char *line)
{
    unsigned long unl, vis;

    if (!strncmp(line, "city ", 5))
        return TRUE;
    if (sscanf(line, "districts %lx %lx", &unl, &vis) == 2) {
        cstate.unlocked = unl;
        cstate.visited = vis;
        return TRUE;
    }
    return FALSE;
}

void
city_state_reset(void)
{
    cstate.unlocked = cstate.visited = 0UL;
    arrival.kind = CITY_ARRIVE_NONE;
}

/*
 * The NeonHack 2 part of the save file: text lines from the city and
 * (later) the story, so the format can grow without breaking.
 */

#define NEON2_MAXLINES 4096

int
neon2_state_save(char ***linesp)
{
    char **lines = (char **) alloc(NEON2_MAXLINES * sizeof (char *));
    int n = 0;

    n = city_state_lines(lines, n);
    n = story_state_lines(lines, n, NEON2_MAXLINES);
    *linesp = lines;
    return n;
}

void
neon2_state_reset(void)
{
    city_state_reset();
    story_load(); /* the story's data, before its state is restored */
}

void
neon2_state_line(const char *line)
{
    if (city_state_line(line) || story_state_line(line))
        return;
    impossible("Unknown NeonHack 2 save line: \"%.40s\"", line);
}

void
neon2_state_free(void)
{
    story_free();
}

/*
 * The city map, the metro and taxis.
 */

/* draw the city map at the top of a menu; 'here' is the hero's district
   (0 when not in one) */
staticfn void
citymap_art(winid win, int here)
{
    char grid[CITYMAP_H][CITYMAP_W + 1];
    char row[BUFSZ * 2], *p;
    const char *dot, *colon, *hz, *vt, *tl, *tr, *bl, *br, *title;
    /* curses wraps menu lines by bytes, so it gets the ASCII map */
    boolean utf8 = neon_utf8_terminal() && WINDOWPORT(tty);
    int i, j, k, n, len, marker;
    s_level *sp;

    for (i = 0; i < CITYMAP_H; i++) {
        (void) memset(grid[i], ' ', CITYMAP_W);
        grid[i][CITYMAP_W] = '\0';
    }
    /* roads: dots along each row from the first district to the last,
       colons down each column of districts */
    for (k = 0; k < (int) SIZE(citylayout); k++) {
        int last = -1;

        i = citylayout[k].row;
        for (n = 0; n < (int) SIZE(citylayout); n++)
            if (citylayout[n].row == i && citylayout[n].col > last)
                last = citylayout[n].col;
        for (j = citylayout[k].col; j < last && j < CITYMAP_W; j++)
            grid[i][j] = '.';
        if (i + 2 < CITYMAP_H && citylayout[k].col + 5 < CITYMAP_W)
            grid[i + 1][citylayout[k].col + 5] = ':';
    }
    /* district names, each with a marker in front */
    for (k = 0; k < (int) SIZE(citylayout); k++) {
        if ((n = city_district_num(citylayout[k].proto)) == 0
            || (sp = district_level(n)) == 0)
            continue;
        title = *sp->title ? sp->title : sp->proto;
        marker = (n == here) ? '@'
                 : story_district_marker(n) ? '*'
                   : !district_open(n) ? 'x' : ' ';
        i = citylayout[k].row, j = citylayout[k].col;
        grid[i][j] = (char) marker;
        len = (int) strlen(title);
        if (j + 1 + len + 1 > CITYMAP_W)
            len = CITYMAP_W - j - 2;
        (void) memcpy(&grid[i][j + 1], title, len);
        grid[i][j + 1 + len] = ' ';
        if (j > 0 && grid[i][j - 1] == '.')
            grid[i][j - 1] = ' ';
    }

    dot = utf8 ? "\u00b7" : ".";       /* · */
    colon = utf8 ? "\u2502" : ":";     /* │ */
    hz = utf8 ? "\u2500" : "-";        /* ─ */
    vt = utf8 ? "\u2502" : "|";        /* │ */
    tl = utf8 ? "\u250c" : "+";        /* ┌ */
    tr = utf8 ? "\u2510" : "+";        /* ┐ */
    bl = utf8 ? "\u2514" : "+";        /* └ */
    br = utf8 ? "\u2518" : "+";        /* ┘ */

    /* top: ┌──── NEO YENDOR ────┐ */
    p = row;
    p = eos(strcpy(p, " ")), p = eos(strcpy(p, tl));
    for (j = 0; j < CITYMAP_W + 2; j++) {
        if (j == (CITYMAP_W + 2 - 12) / 2) {
            p = eos(strcpy(p, " NEO YENDOR "));
            j += 11;
        } else
            p = eos(strcpy(p, hz));
    }
    (void) strcpy(p, tr);
    add_menu_str(win, row);
    for (i = 0; i < CITYMAP_H; i++) {
        p = row;
        p = eos(strcpy(p, " ")), p = eos(strcpy(p, vt));
        p = eos(strcpy(p, " "));
        for (j = 0; j < CITYMAP_W; j++) {
            char c = grid[i][j];
            char one[2];

            /* district names never contain '.' or ':' */
            if (c == '.')
                p = eos(strcpy(p, dot));
            else if (c == ':')
                p = eos(strcpy(p, colon));
            else if (c == '*' && utf8)
                p = eos(strcpy(p, "\u2605")); /* ★ */
            else if (c == 'x' && utf8 && (j + 1 < CITYMAP_W
                                         && grid[i][j + 1] >= 'A'
                                         && grid[i][j + 1] <= 'Z'))
                p = eos(strcpy(p, "\u00d7")); /* × */
            else
                one[0] = c, one[1] = '\0', p = eos(strcpy(p, one));
        }
        p = eos(strcpy(p, " "));
        (void) strcpy(p, vt);
        add_menu_str(win, row);
    }
    p = row;
    p = eos(strcpy(p, " ")), p = eos(strcpy(p, bl));
    for (j = 0; j < CITYMAP_W + 2; j++)
        p = eos(strcpy(p, hz));
    (void) strcpy(p, br);
    add_menu_str(win, row);
    Sprintf(row, "  @ you   %s objective   %s closed off",
            utf8 ? "\u2605" : "*", utf8 ? "\u00d7" : "x");
    add_menu_str(win, row);
    add_menu_str(win, "");
}

/* show the city map; with 'travel', districts can be picked.  Returns
   the picked district or 0. */
staticfn int
citymap_menu(int here, const char *prompt, boolean travel)
{
    winid win;
    anything any;
    menu_item *pick = (menu_item *) 0;
    char buf[BUFSZ], note[BUFSZ];
    const char *title;
    int n, res = 0;
    char let = 'a';
    s_level *sp;

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    citymap_art(win, here);
    for (n = 1; n <= district_count(); n++) {
        if ((sp = district_level(n)) == 0)
            continue;
        title = *sp->title ? sp->title : sp->proto;
        note[0] = '\0';
        if (n == here)
            Strcpy(note, "you are here");
        else if (!district_open(n))
            Strcpy(note, "closed off");
        else if (!(cstate.visited & (1UL << n)))
            Strcpy(note, "not been there yet");
        story_district_note(n, note, (int) sizeof note);
        Snprintf(buf, sizeof buf, "%-16s %s", title, note);
        any = cg.zeroany;
        if (travel && district_open(n) && n != here) {
            any.a_int = n;
            add_menu(win, &nul_glyphinfo, &any, let, 0, ATR_NONE, NO_COLOR,
                     buf, MENU_ITEMFLAGS_NONE);
        } else {
            char line[BUFSZ];

            Snprintf(line, sizeof line, "    %s", buf);
            add_menu_str(win, line);
        }
        let = (let == 'z') ? 'A' : let + 1;
    }
    end_menu(win, prompt);
    if (select_menu(win, travel ? PICK_ONE : PICK_NONE, &pick) > 0) {
        res = pick[0].item.a_int;
        free((genericptr_t) pick);
    }
    destroy_nhwindow(win);
    return res;
}

/* reasons a cab won't come; True if refused (after saying why) */
staticfn boolean
taxi_refused(void)
{
    struct monst *mtmp;

    if (u.uheat >= HEAT_WANTED) {
        pline("Every cab in the city has your face on its blocklist.");
        return TRUE;
    }
    if (u.uswallow || u.utrap || u.ustuck) {
        You("can't flag a cab down right now.");
        return TRUE;
    }
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || mtmp->mpeaceful || mtmp->mtame)
            continue;
        if (distu(mtmp->mx, mtmp->my) <= 2 && canspotmon(mtmp)) {
            pline("No cab will stop with %s on your back.", mon_nam(mtmp));
            return TRUE;
        }
    }
    return FALSE;
}

/* a cab ride costs more the further it goes up the danger scale */
staticfn long
taxi_fare(int from, int to)
{
    s_level *a = district_level(from), *b = district_level(to);
    int da = a ? a->danger : 1, db = b ? b->danger : 1;

    return 10L + 5L * (long) abs(db - da);
}

staticfn void
city_travel(int n, boolean metro)
{
    d_level dest;
    char nbuf[BUFSZ];

    dest.dnum = city_dnum;
    dest.dlevel = n;
    (void) city_place_phrase(&dest, nbuf);
    if (metro)
        You("ride the metro to %s.", nbuf);
    else
        You("ride a cab across the city to %s.", nbuf);
    city_set_arrival(CITY_ARRIVE_METRO, 0, 0);
    schedule_goto(&dest, UTOTYPE_NONE, (char *) 0, (char *) 0);
}

/* '>' on a metro station */
int
city_metro(void)
{
    int n;

    if (!In_district(&u.uz)) {
        impossible("A metro station outside the districts?");
        return ECMD_OK;
    }
    if (!next_to_u()) {
        You("are held back by your pet!");
        return ECMD_OK;
    }
    n = citymap_menu(u.uz.dlevel, "Ride the metro to where?", TRUE);
    if (!n)
        return ECMD_OK;
    city_travel(n, TRUE);
    return ECMD_TIME;
}

/* the #citymap command: look at the map, maybe call a cab */
int
docitymap(void)
{
    int n, here = In_district(&u.uz) ? u.uz.dlevel : 0;
    long fare;
    stairway *stway = stairway_at(u.ux, u.uy);
    char qbuf[QBUFSZ], nbuf[BUFSZ];
    d_level dest;

    if (!district_count()) {
        pline("There is no city here.");
        return ECMD_OK;
    }
    if (stway && stway->kind == STWAY_METRO)
        return city_metro();
    if (!In_city(&u.uz)) {
        (void) citymap_menu(0, "Neo Yendor, far above you", FALSE);
        return ECMD_OK;
    }
    if (!here) {
        (void) citymap_menu(0, "Neo Yendor (get to the street for a cab)",
                            FALSE);
        return ECMD_OK;
    }
    n = citymap_menu(here, "Call a cab to where?", TRUE);
    if (!n || taxi_refused())
        return ECMD_OK;
    fare = taxi_fare(here, n);
    dest.dnum = city_dnum, dest.dlevel = n;
    Snprintf(qbuf, sizeof qbuf, "The ride to %s costs %ld %s.  Pay?",
             city_place_phrase(&dest, nbuf), fare, currency(fare));
    if (money_cnt(gi.invent) < fare) {
        pline("The ride to %s costs %ld %s, more than you have.", nbuf,
              fare, currency(fare));
        return ECMD_OK;
    }
    if (y_n(qbuf) != 'y')
        return ECMD_OK;
    if (!next_to_u()) {
        You("are held back by your pet!");
        return ECMD_OK;
    }
    (void) neon2_pay(fare);
    city_travel(n, FALSE);
    return ECMD_TIME;
}

/* hand over 'amount' credits to nobody in particular; False if short */
boolean
neon2_pay(long amount)
{
    struct obj *ygold = findgold(gi.invent);

    if (amount <= 0)
        return TRUE;
    if (!ygold || ygold->quan < amount)
        return FALSE;
    if (ygold->quan > amount)
        ygold = splitobj(ygold, amount);
    else if (ygold->owornmask)
        remove_worn_item(ygold, FALSE);
    freeinv(ygold);
    obfree(ygold, (struct obj *) 0);
    disp.botl = TRUE;
    return TRUE;
}

/* #wizcity: open every district */
int
wiz_city(void)
{
    int n;

    if (!wizard)
        return ECMD_OK;
    for (n = 1; n <= district_count(); n++)
        city_unlock(n, FALSE);
    pline("Every district of Neo Yendor is open.");
    return ECMD_OK;
}

/*city.c*/
