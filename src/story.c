/* NeonHack 2   story.c   quests, NPCs, dialogue and the journal */
/* NetHack may be freely redistributed.  See license for details. */
/* Copyright (c) 2026 jalava */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * The story is data: dat/story.lua describes factions, story items,
 * NPCs, quests (made of stages) and dialogue.  This file reads it at
 * the start of a game (and again on restore) and runs it.  Only the
 * state goes into the save file, as text lines keyed by the ids used in
 * story.lua, so the story can be edited between sessions.
 *
 * A stage is finished by one of:
 *   talk = npc      talking to the NPC (after its dialogue, if any);
 *                   with give = item, only while carrying the item,
 *                   which the NPC takes
 *   arrive = place  arriving there ("nc-stacks", "Block 7:2", ...)
 *   kill = npc      the NPC dying
 *   carry = item    having the item in your pack
 *   flag = name     the story flag being set
 * or by an "advance" effect.  Effects ("start", "done", dialogue
 * choices) are listed in story_effects() below.
 */

#include "hack.h"

/* a flattened Lua table entry: credits = 50 -> ("credits", 0, 50);
   start = "act1" -> ("start", "act1", 0); rep = { jade = 5 } ->
   ("rep", "jade", 5); unlock = { "a", "b" } -> ("unlock", "a"), ...;
   move = { hex = "Seal Vault:2" } -> ("move", "hex", val "Seal Vault:2");
   cases = { { need = {...}, <effects> }, ... } -> ("cases", cases[]) */
struct st_case;
struct st_kv {
    char *key, *str;
    long num;
    char *val;             /* name = "string" in a sub-table */
    int ncases;            /* cases and when */
    struct st_case *cases;
};
struct st_kvl {
    int n;
    struct st_kv *kv;
};
struct st_case {
    struct st_kvl need, eff;
};

struct st_item {
    char *id, *name;
    d_level drop;    /* gigs: left lying somewhere on this level */
    boolean dropped; /* ... and it has been put there */
};
struct st_faction {
    char *id, *name;
    int nmembers;
    int *members; /* monster types that belong to it */
    long rep;     /* state */
};
/* reputation at which a faction's people turn friendly or hostile */
#define REP_FRIENDLY 15
#define REP_HOSTILE (-15)
#define st_stance(r) ((r) >= REP_FRIENDLY ? 1 : (r) <= REP_HOSTILE ? -1 : 0)
struct st_npc {
    char *id, *name, *monster, *home, *faction, *chat;
    char *nearnpc; /* appears next to this NPC, if it's there */
    boolean female, stay, hostile;
    boolean fixer; /* offers gigs */
    xint8 spot;    /* where it appears when spawned: SPOT_xxx */
    int level; /* experience level, if not the monster type's own */
    int nidle;
    char **idle;
    struct st_kvl died; /* effects when it is killed */
    d_level homelev;
    /* state */
    unsigned m_id;
    xint8 status;    /* NPC_xxx */
    boolean pending; /* spawn when the hero reaches its home */
    boolean angry;   /* turned hostile by the story */
    boolean moved;   /* its home was changed by the story */
};
#define SPOT_RANDOM 0
#define SPOT_METRO 1 /* next to the district's metro station */
#define SPOT_NEAR 2  /* next to the hero */
#define NPC_UNMADE 0
#define NPC_ALIVE 1
#define NPC_DEAD 2
#define NPC_GONE 3

struct st_stage {
    char *goal, *where, *talk, *give, *carry, *kill, *arrive, *flag;
    char *dialogue, *remind, *log;
    struct st_kvl start, done;
    d_level wherelev, arrivelev;
};
struct st_quest {
    char *id, *title, *kind;
    boolean autostart;
    int nstages;
    struct st_stage *stages;
    struct st_kvl done;
    /* state */
    xint8 status; /* Q_xxx */
    int stage;    /* current stage, 0-based */
    long ended;   /* turn it was finished or failed */
};
#define Q_INACTIVE 0
#define Q_ACTIVE 1
#define Q_DONE 2
#define Q_FAILED 3

struct st_choice {
    char *text, *go;
    boolean wait;
    struct st_kvl need, eff;
};
struct st_node {
    char *id, *text;
    int nchoices;
    struct st_choice *choices;
};

/* gigs: repeatable jobs from fixers, made from templates in story.lua
   into a few slots kept at the end of the quest, NPC and item arrays */
#define GIG_SLOTS 8      /* gigs that can exist at once, finished included */
#define GIG_ACTIVE_MAX 3 /* gigs you can have running */
struct st_gigtpl {
    char *id, *place; /* place: "district" or "site" */
    boolean hostile, drop, metro;
    int reward;       /* plus 10 per point of danger */
    int nfixers, nmonsters, nnames, ninames;
    char **fixers, **monsters, **names, **inames;
    struct st_quest q; /* title, stages, done; with $ and % holes */
};
struct st_gig {
    boolean used;
    long seq;  /* gig<seq> is the quest's id */
    int tpl;   /* template */
    int fixer; /* NPC index */
    d_level place;
    long reward;
    int pm;    /* target's monster type */
    char tname[64], iname[64];
};

/* what a district (or a site, by dungeon name) is like */
struct st_district {
    char *key;      /* "nc-stacks" or "Block 7" */
    d_level lev;    /* a district; dlevel 0 for a whole site dungeon */
    int nmon, npeople, nsight, nsound;
    int *mon, *monw;       /* random monsters and their weights */
    int *people, *peoplen; /* civilians and how many to keep around */
    char **sight, **sound; /* ambience */
};

struct st_log {
    int quest;
    long turn;
    char *text;
    struct st_log *next;
};
struct st_flag {
    char *name;
    long val;
    struct st_flag *next;
};

/* the way into a site or the Undercity, closed until a flag is set */
struct st_lock {
    char *key, *flag, *text;
    d_level lev; /* dlevel 0: the whole dungeon */
};

/* the story's endings */
struct st_ending {
    char *id, *title, *text;
    struct st_kvl stay; /* effects when you stay in the city */
};

#define STALE_MAX 16 /* old copies of moved NPCs, to remove */

static struct story_data {
    boolean loaded;
    int nquests, nnpcs, nnodes, nitems, nfactions;
    int nstatq, nstatn, nstati; /* story.lua's own; gig slots follow */
    int ntpls, ndist, nlocks, nendings;
    struct st_gigtpl *tpls;
    struct st_district *dist;
    struct st_lock *locks;
    struct st_ending *endings;
    struct st_npc *talking; /* in a conversation with */
    char walkaway[BUFSZ];   /* how the game ended, if you walked away */
    unsigned stale[STALE_MAX];
    int nstale;
    long lastamb; /* turn of the last ambient message */
    struct st_gig gigs[GIG_SLOTS];
    long gigseq;
    struct st_quest *quests;
    struct st_npc *npcs;
    struct st_node *nodes;
    struct st_item *items;
    struct st_faction *factions;
    /* state */
    struct st_flag *flags;
    struct st_log *log, *logtail;
    int tracked; /* quest index, -1 for none */
    int busy;    /* effects in progress; defers re-checks */
} st;

#define STORY_FILE "story.lua"

staticfn void st_free(genericptr_t);
staticfn int st_count_keys(lua_State *);
staticfn int st_check_kvl(struct st_kvl *, const char *, winid);
staticfn struct st_kv *st_kv_push(struct st_kvl *, int *, const char *);
staticfn void st_read_cases(lua_State *, struct st_kv *);
staticfn void st_read_kvl(lua_State *, struct st_kvl *, const char *const *);
staticfn void st_read_kvl_field(lua_State *, const char *, struct st_kvl *);
staticfn void st_free_kvl(struct st_kvl *);
staticfn void st_read_npcs(lua_State *);
staticfn void st_read_quests(lua_State *);
staticfn void st_read_quest(lua_State *, struct st_quest *, int);
staticfn char **st_read_strlist(lua_State *, const char *, int *);
staticfn void st_read_gigs(lua_State *);
staticfn void st_add_gig_slots(void);
staticfn void st_free_quest(struct st_quest *);
staticfn char *st_gigsub(const char *, struct st_gig *);
staticfn void st_gigsub_kvl(struct st_kvl *, struct st_kvl *, struct st_gig *);
staticfn void st_place_spec(d_level *, char *);
staticfn int st_gig_build(int, long, int, int, d_level *, long, int,
                          const char *, const char *, boolean);
staticfn boolean st_gig_place(struct st_gigtpl *, d_level *);
staticfn int st_gig_free_slot(void);
staticfn void st_gig_board(struct st_npc *);
staticfn void st_drop_items(void);
staticfn void st_read_districts(lua_State *);
staticfn void st_read_locks(lua_State *);
staticfn void st_read_endings(lua_State *);
staticfn int st_ending_idx(const char *);
staticfn void st_ending(int);
staticfn void st_rep_add(int, long, boolean);
staticfn void st_faction_sync(void);
staticfn int st_member_of(struct monst *);
staticfn void st_anger(struct st_npc *);
staticfn void st_move_npc(struct st_npc *, const char *);
staticfn void st_allies(const char *, long);
staticfn void st_stale_add(unsigned);
staticfn void st_stale_remove(void);
staticfn int st_rank_faction(boolean);
staticfn boolean st_site_locked(xint16);
staticfn int st_read_weights(lua_State *, const char *, int **, int **);
staticfn struct st_district *st_here(void);
staticfn void st_read_nodes(lua_State *);
staticfn void st_read_named(lua_State *, const char *);
staticfn void st_resolve(void);
staticfn int st_quest_idx(const char *);
staticfn int st_npc_idx(const char *);
staticfn int st_node_idx(const char *);
staticfn int st_item_idx(const char *);
staticfn int st_faction_idx(const char *);
staticfn struct st_npc *st_npc_by_mon(struct monst *);
staticfn struct monst *st_npc_mon(struct st_npc *);
staticfn struct st_flag *st_flag_get(const char *, boolean);
staticfn long st_flag_val(const char *);
staticfn void st_flag_set(const char *, long);
staticfn void st_log_add(int, long, const char *);
staticfn char *st_subst(const char *, char *, int);
staticfn boolean st_need(struct st_kvl *);
staticfn boolean st_need_shown(struct st_kvl *, char *);
staticfn struct obj *st_carried(const char *);
staticfn void st_give_item(const char *);
staticfn void st_take_item(const char *);
staticfn void st_spawn(struct st_npc *, boolean);
staticfn void st_despawn(struct st_npc *);
staticfn void st_start_quest(int, boolean);
staticfn void st_begin_stage(int);
staticfn void st_finish_stage(int);
staticfn void st_end_quest(int, xint8, const char *);
staticfn void st_recheck(void);
staticfn struct st_stage *st_cur(int);
staticfn void st_show_text(const char *, const char *);
staticfn int st_dialogue(struct st_npc *, const char *);
staticfn void st_wrap_menu(winid, const char *);
staticfn void st_quest_details(int);
staticfn const char *st_place(d_level *, char *);
staticfn boolean st_on(d_level *);

/*
 * Reading story.lua
 */

staticfn void
st_free(genericptr_t p)
{
    if (p)
        free(p);
}

/* room for one more entry in a list being read */
staticfn struct st_kv *
st_kv_push(struct st_kvl *out, int *cap, const char *key)
{
    struct st_kv *kv;

    if (out->n >= *cap) {
        struct st_kv *bigger;

        *cap *= 2;
        bigger = (struct st_kv *) alloc(*cap * sizeof *bigger);
        (void) memcpy(bigger, out->kv, out->n * sizeof *bigger);
        free((genericptr_t) out->kv);
        out->kv = bigger;
    }
    kv = &out->kv[out->n++];
    (void) memset((genericptr_t) kv, 0, sizeof *kv);
    kv->key = dupstr(key);
    return kv;
}

/* cases = { { need = {...}, <effects> }, ... } at the top of the stack */
staticfn void
st_read_cases(lua_State *L, struct st_kv *kv)
{
    static const char *const case_skip[] = { "need", 0 };
    int len, j;

    lua_len(L, -1);
    len = (int) lua_tointeger(L, -1);
    lua_pop(L, 1);
    kv->cases = (struct st_case *) alloc((len + 1) * sizeof *kv->cases);
    (void) memset(kv->cases, 0, (len + 1) * sizeof *kv->cases);
    for (j = 0; j < len; j++) {
        lua_pushinteger(L, j + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) == LUA_TTABLE) {
            struct st_case *cp = &kv->cases[kv->ncases++];

            st_read_kvl_field(L, "need", &cp->need);
            st_read_kvl(L, &cp->eff, case_skip);
        }
        lua_pop(L, 1);
    }
}

/* flatten the table at the top of the stack, skipping 'skip' keys */
staticfn void
st_read_kvl(lua_State *L, struct st_kvl *out, const char *const *skip)
{
    int cap = 8, i;
    const char *key;
    struct st_kv *kv;

    out->n = 0;
    out->kv = (struct st_kv *) alloc(cap * sizeof (struct st_kv));
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        /* key at -2, value at -1 */
        if (lua_type(L, -2) != LUA_TSTRING) {
            lua_pop(L, 1);
            continue;
        }
        key = lua_tostring(L, -2);
        for (i = 0; skip && skip[i]; i++)
            if (!strcmp(key, skip[i]))
                break;
        if (skip && skip[i]) {
            lua_pop(L, 1);
            continue;
        }
        if (lua_type(L, -1) == LUA_TTABLE
            && (!strcmp(key, "cases") || !strcmp(key, "when"))) {
            kv = st_kv_push(out, &cap, key);
            st_read_cases(L, kv);
        } else if (lua_type(L, -1) == LUA_TTABLE) {
            /* sub-table: array of strings, or name = number|string */
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                kv = st_kv_push(out, &cap, key);
                if (lua_type(L, -2) == LUA_TSTRING) {
                    kv->str = dupstr(lua_tostring(L, -2));
                    if (lua_type(L, -1) == LUA_TNUMBER)
                        kv->num = (long) lua_tointeger(L, -1);
                    else if (lua_type(L, -1) == LUA_TBOOLEAN)
                        kv->num = lua_toboolean(L, -1);
                    else if (lua_type(L, -1) == LUA_TSTRING)
                        kv->val = dupstr(lua_tostring(L, -1));
                } else if (lua_type(L, -1) == LUA_TSTRING) {
                    kv->str = dupstr(lua_tostring(L, -1));
                }
                lua_pop(L, 1);
            }
        } else {
            kv = st_kv_push(out, &cap, key);
            if (lua_type(L, -1) == LUA_TSTRING)
                kv->str = dupstr(lua_tostring(L, -1));
            else if (lua_type(L, -1) == LUA_TNUMBER)
                kv->num = (long) lua_tointeger(L, -1);
            else if (lua_type(L, -1) == LUA_TBOOLEAN)
                kv->num = lua_toboolean(L, -1);
        }
        lua_pop(L, 1);
    }
}

/* flatten t[name] of the table at the top of the stack */
staticfn void
st_read_kvl_field(lua_State *L, const char *name, struct st_kvl *out)
{
    lua_getfield(L, -1, name);
    if (lua_type(L, -1) == LUA_TTABLE) {
        st_read_kvl(L, out, (const char *const *) 0);
    } else {
        out->n = 0;
        out->kv = 0;
    }
    lua_pop(L, 1);
}

staticfn void
st_free_kvl(struct st_kvl *l)
{
    int i, j;

    for (i = 0; i < l->n; i++) {
        free((genericptr_t) l->kv[i].key);
        if (l->kv[i].str)
            free((genericptr_t) l->kv[i].str);
        if (l->kv[i].val)
            free((genericptr_t) l->kv[i].val);
        for (j = 0; j < l->kv[i].ncases; j++) {
            st_free_kvl(&l->kv[i].cases[j].need);
            st_free_kvl(&l->kv[i].cases[j].eff);
        }
        if (l->kv[i].cases)
            free((genericptr_t) l->kv[i].cases);
    }
    if (l->kv)
        free((genericptr_t) l->kv);
    l->n = 0, l->kv = 0;
}

/* number of string-keyed entries in the table at the top of the stack */
staticfn int
st_count_keys(lua_State *L)
{
    int n = 0;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING)
            n++;
        lua_pop(L, 1);
    }
    return n;
}

/* factions = { id = "name" }, items = { id = { name = "..." } } */
staticfn void
st_read_named(lua_State *L, const char *section)
{
    int n, i = 0;
    boolean factions = !strcmp(section, "factions");

    lua_getfield(L, -1, section);
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    if (factions) {
        st.factions = (struct st_faction *) alloc((n + 1) * sizeof *st.factions);
        (void) memset(st.factions, 0, (n + 1) * sizeof *st.factions);
    } else {
        st.items = (struct st_item *) alloc((n + 1) * sizeof *st.items);
        (void) memset(st.items, 0, (n + 1) * sizeof *st.items);
    }
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && i < n) {
            const char *id = lua_tostring(L, -2);
            char *name = 0;

            if (lua_type(L, -1) == LUA_TSTRING)
                name = dupstr(lua_tostring(L, -1));
            else if (lua_type(L, -1) == LUA_TTABLE)
                name = get_table_str_opt(L, "name", (char *) 0);
            if (!name)
                name = dupstr(id);
            if (factions) {
                struct st_faction *fp = &st.factions[i];

                fp->id = dupstr(id);
                fp->name = name;
                if (lua_type(L, -1) == LUA_TTABLE) {
                    int nm, k, gend = 0;
                    char **mnames = st_read_strlist(L, "members", &nm);

                    fp->members = (int *) alloc((nm + 1) * sizeof (int));
                    for (k = 0; k < nm; k++) {
                        int pm = name_to_mon(mnames[k], &gend);

                        if (pm >= LOW_PM)
                            fp->members[fp->nmembers++] = pm;
                        else
                            impossible("story.lua: unknown monster \"%s\"",
                                       mnames[k]);
                        free((genericptr_t) mnames[k]);
                    }
                    st_free(mnames);
                }
            } else {
                st.items[i].id = dupstr(id);
                st.items[i].name = name;
            }
            i++;
        }
        lua_pop(L, 1);
    }
    if (factions) {
        int j;

        st.nfactions = i;
        /* Lua tables have no order; keep the factions in id order, so
           that ties (the "worst" faction) always go the same way */
        for (i = 1; i < st.nfactions; i++)
            for (j = i; j > 0 && strcmp(st.factions[j - 1].id,
                                        st.factions[j].id) > 0; j--) {
                struct st_faction tmp = st.factions[j];

                st.factions[j] = st.factions[j - 1];
                st.factions[j - 1] = tmp;
            }
    } else {
        st.nitems = i;
    }
    lua_pop(L, 1);
}

/* npcs = { id = { name =, monster =, home =, ... } } */
staticfn void
st_read_npcs(lua_State *L)
{
    int n, i = 0, j;

    lua_getfield(L, -1, "npcs");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    st.npcs = (struct st_npc *) alloc((n + 1) * sizeof *st.npcs);
    (void) memset(st.npcs, 0, (n + 1) * sizeof *st.npcs);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE
            && i < n) {
            struct st_npc *np = &st.npcs[i++];

            np->id = dupstr(lua_tostring(L, -2));
            np->name = get_table_str_opt(L, "name", (char *) 0);
            if (!np->name)
                np->name = dupstr(np->id);
            np->monster = get_table_str_opt(L, "monster", (char *) 0);
            np->home = get_table_str_opt(L, "home", (char *) 0);
            np->faction = get_table_str_opt(L, "faction", (char *) 0);
            np->chat = get_table_str_opt(L, "chat", (char *) 0);
            np->nearnpc = get_table_str_opt(L, "near", (char *) 0);
            np->female = get_table_boolean_opt(L, "female", FALSE);
            np->stay = get_table_boolean_opt(L, "stay", FALSE);
            np->hostile = get_table_boolean_opt(L, "hostile", FALSE);
            np->level = get_table_int_opt(L, "level", 0);
            np->fixer = get_table_boolean_opt(L, "fixer", FALSE);
            {
                char *spot = get_table_str_opt(L, "spot", (char *) 0);

                np->spot = !spot ? SPOT_RANDOM
                           : !strcmp(spot, "metro") ? SPOT_METRO
                           : !strcmp(spot, "near") ? SPOT_NEAR : SPOT_RANDOM;
                st_free(spot);
            }
            st_read_kvl_field(L, "died", &np->died);
            lua_getfield(L, -1, "idle");
            if (lua_type(L, -1) == LUA_TTABLE) {
                lua_len(L, -1);
                np->nidle = (int) lua_tointeger(L, -1);
                lua_pop(L, 1);
                np->idle = (char **) alloc((np->nidle + 1) * sizeof (char *));
                for (j = 0; j < np->nidle; j++) {
                    lua_pushinteger(L, j + 1);
                    lua_gettable(L, -2);
                    np->idle[j] = dupstr(lua_type(L, -1) == LUA_TSTRING
                                         ? lua_tostring(L, -1) : "...");
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1);
        }
        lua_pop(L, 1);
    }
    st.nnpcs = i;
    lua_pop(L, 1);
}

/* read the quest table at the top of the stack into qp */
staticfn void
st_read_quest(lua_State *L, struct st_quest *qp, int i)
{
    int j;

    qp->id = get_table_str_opt(L, "id", (char *) 0);
    if (!qp->id) {
        char buf[20];

        Sprintf(buf, "quest%d", i + 1);
        qp->id = dupstr(buf);
    }
    qp->title = get_table_str_opt(L, "title", (char *) 0);
    if (!qp->title)
        qp->title = dupstr(qp->id);
    qp->kind = get_table_str_opt(L, "kind", (char *) 0);
    if (!qp->kind)
        qp->kind = dupstr("side");
    qp->autostart = get_table_boolean_opt(L, "start", FALSE);
    st_read_kvl_field(L, "done", &qp->done);
    lua_getfield(L, -1, "stages");
    if (lua_type(L, -1) == LUA_TTABLE) {
        lua_len(L, -1);
        qp->nstages = (int) lua_tointeger(L, -1);
        lua_pop(L, 1);
        qp->stages = (struct st_stage *) alloc((qp->nstages + 1)
                                               * sizeof *qp->stages);
        (void) memset(qp->stages, 0, (qp->nstages + 1) * sizeof *qp->stages);
        for (j = 0; j < qp->nstages; j++) {
            struct st_stage *sp = &qp->stages[j];

            lua_pushinteger(L, j + 1);
            lua_gettable(L, -2);
            if (lua_type(L, -1) == LUA_TTABLE) {
                sp->goal = get_table_str_opt(L, "goal", (char *) 0);
                sp->where = get_table_str_opt(L, "where", (char *) 0);
                sp->talk = get_table_str_opt(L, "talk", (char *) 0);
                sp->give = get_table_str_opt(L, "give", (char *) 0);
                sp->carry = get_table_str_opt(L, "carry", (char *) 0);
                sp->kill = get_table_str_opt(L, "kill", (char *) 0);
                sp->arrive = get_table_str_opt(L, "arrive", (char *) 0);
                sp->flag = get_table_str_opt(L, "flag", (char *) 0);
                sp->dialogue = get_table_str_opt(L, "dialogue", (char *) 0);
                sp->remind = get_table_str_opt(L, "remind", (char *) 0);
                sp->log = get_table_str_opt(L, "log", (char *) 0);
                st_read_kvl_field(L, "start", &sp->start);
                st_read_kvl_field(L, "done", &sp->done);
            } else {
                impossible("story.lua: %s stage %d is not a table", qp->id,
                           j + 1);
            }
            lua_pop(L, 1);
        }
    } else {
        impossible("story.lua: quest %s has no stages", qp->id);
    }
    lua_pop(L, 1); /* stages */
}

/* quests = { { id =, title =, stages = { {...}, ... } }, ... } */
staticfn void
st_read_quests(lua_State *L)
{
    int n, i;

    lua_getfield(L, -1, "quests");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    lua_len(L, -1);
    n = (int) lua_tointeger(L, -1);
    lua_pop(L, 1);
    st.quests = (struct st_quest *) alloc((n + 1) * sizeof *st.quests);
    (void) memset(st.quests, 0, (n + 1) * sizeof *st.quests);
    for (i = 0; i < n; i++) {
        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TTABLE) {
            impossible("story.lua: quests[%d] is not a table", i + 1);
            st.quests[i].id = dupstr("");
            st.quests[i].title = dupstr("");
            st.quests[i].kind = dupstr("side");
        } else {
            st_read_quest(L, &st.quests[i], i);
        }
        lua_pop(L, 1); /* quest */
    }
    st.nquests = n;
    lua_pop(L, 1);
}

/* t[name] as an array of strings; a single string is a list of one */
staticfn char **
st_read_strlist(lua_State *L, const char *name, int *n)
{
    char **list = 0;
    int i, len;

    *n = 0;
    lua_getfield(L, -1, name);
    if (lua_type(L, -1) == LUA_TSTRING) {
        list = (char **) alloc(2 * sizeof (char *));
        list[0] = dupstr(lua_tostring(L, -1));
        *n = 1;
    } else if (lua_type(L, -1) == LUA_TTABLE) {
        lua_len(L, -1);
        len = (int) lua_tointeger(L, -1);
        lua_pop(L, 1);
        list = (char **) alloc((len + 1) * sizeof (char *));
        for (i = 0; i < len; i++) {
            lua_pushinteger(L, i + 1);
            lua_gettable(L, -2);
            if (lua_type(L, -1) == LUA_TSTRING)
                list[(*n)++] = dupstr(lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return list;
}

/* gigs = { { id =, place =, fixers = {...}, target = {...}, names = {...},
   items = {...}, reward =, hostile =, drop =, spot =, title =,
   stages = {...}, done = {...} }, ... } */
staticfn void
st_read_gigs(lua_State *L)
{
    int n, i;
    char *spot;

    lua_getfield(L, -1, "gigs");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    lua_len(L, -1);
    n = (int) lua_tointeger(L, -1);
    lua_pop(L, 1);
    st.tpls = (struct st_gigtpl *) alloc((n + 1) * sizeof *st.tpls);
    (void) memset(st.tpls, 0, (n + 1) * sizeof *st.tpls);
    for (i = 0; i < n; i++) {
        struct st_gigtpl *tp = &st.tpls[st.ntpls];

        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) == LUA_TTABLE) {
            st_read_quest(L, &tp->q, i);
            tp->id = dupstr(tp->q.id);
            tp->place = get_table_str_opt(L, "place", (char *) 0);
            if (!tp->place)
                tp->place = dupstr("district");
            spot = get_table_str_opt(L, "spot", (char *) 0);
            tp->metro = (spot && !strcmp(spot, "metro"));
            st_free(spot);
            tp->hostile = get_table_boolean_opt(L, "hostile", FALSE);
            tp->drop = get_table_boolean_opt(L, "drop", FALSE);
            tp->reward = get_table_int_opt(L, "reward", 20);
            tp->fixers = st_read_strlist(L, "fixers", &tp->nfixers);
            tp->monsters = st_read_strlist(L, "target", &tp->nmonsters);
            tp->names = st_read_strlist(L, "names", &tp->nnames);
            tp->inames = st_read_strlist(L, "items", &tp->ninames);
            st.ntpls++;
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/* t[name] = { ["street punk"] = 3, urchin = 1 }: monster types and
   numbers; returns how many */
staticfn int
st_read_weights(lua_State *L, const char *name, int **pms, int **wts)
{
    int n = 0, gend = 0, pm;

    *pms = *wts = (int *) 0;
    lua_getfield(L, -1, name);
    if (lua_type(L, -1) == LUA_TTABLE) {
        int cap = st_count_keys(L);

        *pms = (int *) alloc((cap + 1) * sizeof (int));
        *wts = (int *) alloc((cap + 1) * sizeof (int));
        lua_pushnil(L);
        while (lua_next(L, -2) != 0) {
            if (lua_type(L, -2) == LUA_TSTRING && n < cap) {
                pm = name_to_mon(lua_tostring(L, -2), &gend);
                if (pm >= LOW_PM) {
                    (*pms)[n] = pm;
                    (*wts)[n] = (int) lua_tointeger(L, -1);
                    if ((*wts)[n] > 0)
                        n++;
                } else {
                    impossible("story.lua: unknown monster \"%s\"",
                               lua_tostring(L, -2));
                }
            }
            lua_pop(L, 1);
        }
    }
    lua_pop(L, 1);
    return n;
}

/* districts = { ["nc-stacks"] = { monsters = {...}, people = {...},
   sights = {...}, sounds = {...} }, ["Block 7"] = {...} } */
staticfn void
st_read_districts(lua_State *L)
{
    int n;

    lua_getfield(L, -1, "districts");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    st.dist = (struct st_district *) alloc((n + 1) * sizeof *st.dist);
    (void) memset(st.dist, 0, (n + 1) * sizeof *st.dist);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE
            && st.ndist < n) {
            struct st_district *dp = &st.dist[st.ndist++];

            dp->key = dupstr(lua_tostring(L, -2));
            dp->nmon = st_read_weights(L, "monsters", &dp->mon, &dp->monw);
            dp->npeople = st_read_weights(L, "people", &dp->people,
                                          &dp->peoplen);
            dp->sight = st_read_strlist(L, "sights", &dp->nsight);
            dp->sound = st_read_strlist(L, "sounds", &dp->nsound);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/* locks = { ["The Undercity"] = { flag = "shaft_open", text = [[...]] } } */
staticfn void
st_read_locks(lua_State *L)
{
    int n;

    lua_getfield(L, -1, "locks");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    st.locks = (struct st_lock *) alloc((n + 1) * sizeof *st.locks);
    (void) memset(st.locks, 0, (n + 1) * sizeof *st.locks);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE
            && st.nlocks < n) {
            struct st_lock *lp = &st.locks[st.nlocks++];

            lp->key = dupstr(lua_tostring(L, -2));
            lp->flag = get_table_str_opt(L, "flag", (char *) 0);
            lp->text = get_table_str_opt(L, "text", (char *) 0);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/* endings = { open = { title =, text =, stay = {...} }, ... } */
staticfn void
st_read_endings(lua_State *L)
{
    int n;

    lua_getfield(L, -1, "endings");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    st.endings = (struct st_ending *) alloc((n + 1) * sizeof *st.endings);
    (void) memset(st.endings, 0, (n + 1) * sizeof *st.endings);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE
            && st.nendings < n) {
            struct st_ending *ep = &st.endings[st.nendings++];

            ep->id = dupstr(lua_tostring(L, -2));
            ep->title = get_table_str_opt(L, "title", (char *) 0);
            if (!ep->title)
                ep->title = dupstr(ep->id);
            ep->text = get_table_str_opt(L, "text", (char *) 0);
            st_read_kvl_field(L, "stay", &ep->stay);
        }
        lua_pop(L, 1);
    }
    lua_pop(L, 1);
}

/* room for gigs at the end of the quest, NPC and item arrays; done once,
   right after loading, so no pointers into the arrays exist yet */
staticfn void
st_add_gig_slots(void)
{
    struct st_quest *q2;
    struct st_npc *n2;
    struct st_item *i2;
    int k;

    st.nstatq = st.nquests, st.nstatn = st.nnpcs, st.nstati = st.nitems;
    q2 = (struct st_quest *) alloc((st.nquests + GIG_SLOTS + 1) * sizeof *q2);
    (void) memset(q2, 0, (st.nquests + GIG_SLOTS + 1) * sizeof *q2);
    if (st.nquests)
        (void) memcpy(q2, st.quests, st.nquests * sizeof *q2);
    n2 = (struct st_npc *) alloc((st.nnpcs + GIG_SLOTS + 1) * sizeof *n2);
    (void) memset(n2, 0, (st.nnpcs + GIG_SLOTS + 1) * sizeof *n2);
    if (st.nnpcs)
        (void) memcpy(n2, st.npcs, st.nnpcs * sizeof *n2);
    i2 = (struct st_item *) alloc((st.nitems + GIG_SLOTS + 1) * sizeof *i2);
    (void) memset(i2, 0, (st.nitems + GIG_SLOTS + 1) * sizeof *i2);
    if (st.nitems)
        (void) memcpy(i2, st.items, st.nitems * sizeof *i2);
    st_free(st.quests), st_free(st.npcs), st_free(st.items);
    st.quests = q2, st.npcs = n2, st.items = i2;
    for (k = 0; k < GIG_SLOTS; k++) {
        q2[st.nquests + k].id = dupstr("");
        q2[st.nquests + k].title = dupstr("");
        q2[st.nquests + k].kind = dupstr("gig");
        n2[st.nnpcs + k].id = dupstr("");
        n2[st.nnpcs + k].name = dupstr("");
        i2[st.nitems + k].id = dupstr("");
        i2[st.nitems + k].name = dupstr("");
    }
    st.nquests += GIG_SLOTS, st.nnpcs += GIG_SLOTS, st.nitems += GIG_SLOTS;
}

/* dialogue = { id = { text = [[...]], choices = { { "text", go =,
   need = {...}, wait = true, <effects> }, ... } } } */
staticfn void
st_read_nodes(lua_State *L)
{
    static const char *const choice_skip[] = { "go", "need", "wait", 0 };
    int n, i = 0, j;

    lua_getfield(L, -1, "dialogue");
    if (lua_type(L, -1) != LUA_TTABLE) {
        lua_pop(L, 1);
        return;
    }
    n = st_count_keys(L);
    st.nodes = (struct st_node *) alloc((n + 1) * sizeof *st.nodes);
    (void) memset(st.nodes, 0, (n + 1) * sizeof *st.nodes);
    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) == LUA_TSTRING && lua_type(L, -1) == LUA_TTABLE
            && i < n) {
            struct st_node *np = &st.nodes[i++];

            np->id = dupstr(lua_tostring(L, -2));
            np->text = get_table_str_opt(L, "text", (char *) 0);
            lua_getfield(L, -1, "choices");
            if (lua_type(L, -1) == LUA_TTABLE) {
                lua_len(L, -1);
                np->nchoices = (int) lua_tointeger(L, -1);
                lua_pop(L, 1);
                np->choices = (struct st_choice *) alloc(
                                 (np->nchoices + 1) * sizeof *np->choices);
                (void) memset(np->choices, 0,
                              (np->nchoices + 1) * sizeof *np->choices);
                for (j = 0; j < np->nchoices; j++) {
                    struct st_choice *cp = &np->choices[j];

                    lua_pushinteger(L, j + 1);
                    lua_gettable(L, -2);
                    if (lua_type(L, -1) == LUA_TTABLE) {
                        lua_pushinteger(L, 1);
                        lua_gettable(L, -2);
                        cp->text = dupstr(lua_type(L, -1) == LUA_TSTRING
                                          ? lua_tostring(L, -1) : "...");
                        lua_pop(L, 1);
                        cp->go = get_table_str_opt(L, "go", (char *) 0);
                        cp->wait = get_table_boolean_opt(L, "wait", FALSE);
                        st_read_kvl_field(L, "need", &cp->need);
                        st_read_kvl(L, &cp->eff, choice_skip);
                    } else if (lua_type(L, -1) == LUA_TSTRING) {
                        cp->text = dupstr(lua_tostring(L, -1));
                    }
                    lua_pop(L, 1);
                }
            }
            lua_pop(L, 1); /* choices */
        }
        lua_pop(L, 1);
    }
    st.nnodes = i;
    lua_pop(L, 1);
}

/* level specs to d_levels, once the dungeon exists */
staticfn void
st_resolve(void)
{
    int i, j;
    s_level *slev;
    xint16 dn;

    for (i = 0; i < st.ndist; i++) {
        struct st_district *dp = &st.dist[i];

        if ((slev = find_level(dp->key)) != 0) {
            dp->lev = slev->dlevel;
        } else if ((dn = dname_lookup(dp->key)) >= 0) {
            dp->lev.dnum = dn, dp->lev.dlevel = 0;
        } else {
            impossible("story.lua: unknown district or site \"%s\"", dp->key);
            dp->lev.dnum = -1;
        }
    }

    for (i = 0; i < st.nlocks; i++) {
        struct st_lock *lp = &st.locks[i];

        if (strchr(lp->key, ':') || find_level(lp->key)) {
            if (!city_resolve_dest(lp->key, &lp->lev))
                lp->lev.dnum = -1, lp->lev.dlevel = 0;
        } else if ((dn = dname_lookup(lp->key)) >= 0) {
            lp->lev.dnum = dn, lp->lev.dlevel = 0;
        } else {
            lp->lev.dnum = -1, lp->lev.dlevel = 0;
        }
    }
    for (i = 0; i < st.nnpcs; i++)
        if (st.npcs[i].home
            && !city_resolve_dest(st.npcs[i].home, &st.npcs[i].homelev))
            st.npcs[i].homelev.dlevel = 0;
    for (i = 0; i < st.nquests; i++)
        for (j = 0; j < st.quests[i].nstages; j++) {
            struct st_stage *sp = &st.quests[i].stages[j];

            if (!sp->where || !city_resolve_dest(sp->where, &sp->wherelev))
                sp->wherelev.dlevel = 0;
            if (!sp->arrive
                || !city_resolve_dest(sp->arrive, &sp->arrivelev))
                sp->arrivelev.dlevel = 0;
            /* a stage at some place is shown there on the city map */
            if (!sp->wherelev.dlevel && sp->arrivelev.dlevel)
                sp->wherelev = sp->arrivelev;
        }
}

/* read dat/story.lua; a missing or broken file means no story */
void
story_load(void)
{
    lua_State *L;
    nhl_sandbox_info sbi = { NHL_SB_SAFE, 4 * 1024 * 1024, 0,
                             4 * 1024 * 1024 };

    story_free();
    st.tracked = -1;
    if ((L = nhl_init(&sbi)) == 0)
        return;
    if (!nhl_loadlua(L, STORY_FILE)) {
        nhl_done(L);
        return;
    }
    lua_settop(L, 0);
    lua_getglobal(L, "story");
    if (lua_type(L, -1) == LUA_TTABLE) {
        st_read_named(L, "factions");
        st_read_named(L, "items");
        st_read_npcs(L);
        st_read_quests(L);
        st_read_nodes(L);
        st_read_gigs(L);
        st_read_districts(L);
        st_read_locks(L);
        st_read_endings(L);
        st_add_gig_slots();
        st.loaded = TRUE;
    } else {
        impossible("%s has no 'story' table.", STORY_FILE);
    }
    nhl_done(L);
    if (st.loaded)
        st_resolve();
}

void
story_free(void)
{
    int i, j;
    struct st_log *lp, *lnext;
    struct st_flag *fp, *fnext;

    for (i = 0; i < st.nquests; i++)
        st_free_quest(&st.quests[i]);
    st_free(st.quests);
    for (i = 0; i < st.ntpls; i++) {
        struct st_gigtpl *tp = &st.tpls[i];

        st_free_quest(&tp->q);
        st_free(tp->id), st_free(tp->place);
        for (j = 0; j < tp->nfixers; j++)
            st_free(tp->fixers[j]);
        for (j = 0; j < tp->nmonsters; j++)
            st_free(tp->monsters[j]);
        for (j = 0; j < tp->nnames; j++)
            st_free(tp->names[j]);
        for (j = 0; j < tp->ninames; j++)
            st_free(tp->inames[j]);
        st_free(tp->fixers), st_free(tp->monsters), st_free(tp->names);
        st_free(tp->inames);
    }
    st_free(st.tpls);
    for (i = 0; i < st.ndist; i++) {
        struct st_district *dp = &st.dist[i];

        st_free(dp->key), st_free(dp->mon), st_free(dp->monw);
        st_free(dp->people), st_free(dp->peoplen);
        for (j = 0; j < dp->nsight; j++)
            st_free(dp->sight[j]);
        for (j = 0; j < dp->nsound; j++)
            st_free(dp->sound[j]);
        st_free(dp->sight), st_free(dp->sound);
    }
    st_free(st.dist);
    for (i = 0; i < st.nnpcs; i++) {
        struct st_npc *np = &st.npcs[i];

        st_free(np->id), st_free(np->name), st_free(np->monster), st_free(np->home);
        st_free(np->faction), st_free(np->chat), st_free(np->nearnpc);
        for (j = 0; j < np->nidle; j++)
            st_free(np->idle[j]);
        st_free(np->idle);
        st_free_kvl(&np->died);
    }
    st_free(st.npcs);
    for (i = 0; i < st.nlocks; i++)
        st_free(st.locks[i].key), st_free(st.locks[i].flag),
            st_free(st.locks[i].text);
    st_free(st.locks);
    for (i = 0; i < st.nendings; i++) {
        st_free(st.endings[i].id), st_free(st.endings[i].title);
        st_free(st.endings[i].text);
        st_free_kvl(&st.endings[i].stay);
    }
    st_free(st.endings);
    for (i = 0; i < st.nnodes; i++) {
        struct st_node *np = &st.nodes[i];

        for (j = 0; j < np->nchoices; j++) {
            st_free(np->choices[j].text), st_free(np->choices[j].go);
            st_free_kvl(&np->choices[j].need);
            st_free_kvl(&np->choices[j].eff);
        }
        st_free(np->choices);
        st_free(np->id), st_free(np->text);
    }
    st_free(st.nodes);
    for (i = 0; i < st.nitems; i++)
        st_free(st.items[i].id), st_free(st.items[i].name);
    st_free(st.items);
    for (i = 0; i < st.nfactions; i++)
        st_free(st.factions[i].id), st_free(st.factions[i].name),
            st_free(st.factions[i].members);
    st_free(st.factions);
    for (lp = st.log; lp; lp = lnext) {
        lnext = lp->next;
        st_free(lp->text);
        free((genericptr_t) lp);
    }
    for (fp = st.flags; fp; fp = fnext) {
        fnext = fp->next;
        st_free(fp->name);
        free((genericptr_t) fp);
    }
    (void) memset((genericptr_t) &st, 0, sizeof st);
    st.tracked = -1;
}

staticfn void
st_free_quest(struct st_quest *qp)
{
    int j;

    for (j = 0; j < qp->nstages; j++) {
        struct st_stage *sp = &qp->stages[j];

        st_free(sp->goal), st_free(sp->where), st_free(sp->talk);
        st_free(sp->give), st_free(sp->carry), st_free(sp->kill);
        st_free(sp->arrive), st_free(sp->flag), st_free(sp->dialogue);
        st_free(sp->remind), st_free(sp->log);
        st_free_kvl(&sp->start), st_free_kvl(&sp->done);
    }
    st_free(qp->stages);
    st_free(qp->id), st_free(qp->title), st_free(qp->kind);
    st_free_kvl(&qp->done);
    (void) memset((genericptr_t) qp, 0, sizeof *qp);
}

/*
 * Lookups and small helpers
 */

staticfn int
st_quest_idx(const char *id)
{
    int i;

    for (i = 0; id && *id && i < st.nquests; i++)
        if (!strcmp(st.quests[i].id, id))
            return i;
    return -1;
}

staticfn int
st_npc_idx(const char *id)
{
    int i;

    for (i = 0; id && *id && i < st.nnpcs; i++)
        if (!strcmp(st.npcs[i].id, id))
            return i;
    return -1;
}

staticfn int
st_node_idx(const char *id)
{
    int i;

    for (i = 0; id && i < st.nnodes; i++)
        if (!strcmp(st.nodes[i].id, id))
            return i;
    return -1;
}

staticfn int
st_item_idx(const char *id)
{
    int i;

    for (i = 0; id && *id && i < st.nitems; i++)
        if (!strcmp(st.items[i].id, id))
            return i;
    return -1;
}

staticfn int
st_faction_idx(const char *id)
{
    int i;

    for (i = 0; id && i < st.nfactions; i++)
        if (!strcmp(st.factions[i].id, id))
            return i;
    return -1;
}

staticfn struct st_npc *
st_npc_by_mon(struct monst *mtmp)
{
    int i;

    if (!mtmp || !st.loaded)
        return (struct st_npc *) 0;
    for (i = 0; i < st.nnpcs; i++)
        if (st.npcs[i].m_id == mtmp->m_id
            && st.npcs[i].status == NPC_ALIVE)
            return &st.npcs[i];
    return (struct st_npc *) 0;
}

/* the NPC's monster if it is on this level */
staticfn struct monst *
st_npc_mon(struct st_npc *np)
{
    struct monst *mtmp;

    if (!np->m_id || np->status != NPC_ALIVE)
        return (struct monst *) 0;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp) && mtmp->m_id == np->m_id)
            return mtmp;
    return (struct monst *) 0;
}

staticfn struct st_flag *
st_flag_get(const char *name, boolean create)
{
    struct st_flag *fp;

    for (fp = st.flags; fp; fp = fp->next)
        if (!strcmp(fp->name, name))
            return fp;
    if (!create)
        return (struct st_flag *) 0;
    fp = (struct st_flag *) alloc(sizeof *fp);
    fp->name = dupstr(name);
    fp->val = 0L;
    fp->next = st.flags;
    st.flags = fp;
    return fp;
}

staticfn long
st_flag_val(const char *name)
{
    struct st_flag *fp = st_flag_get(name, FALSE);

    return fp ? fp->val : 0L;
}

staticfn void
st_flag_set(const char *name, long val)
{
    st_flag_get(name, TRUE)->val = val;
}

/* journal entries, oldest first */
staticfn void
st_log_add(int quest, long turn, const char *text)
{
    struct st_log *lp = (struct st_log *) alloc(sizeof *lp);

    lp->quest = quest;
    lp->turn = turn;
    lp->text = dupstr(text);
    lp->next = 0;
    if (st.logtail)
        st.logtail->next = lp;
    else
        st.log = lp;
    st.logtail = lp;
}

/* story text substitutions: %n your name, %r your role */
staticfn char *
st_subst(const char *in, char *out, int outsz)
{
    char *o = out;
    const char *add;

    while (*in && o < out + outsz - 1) {
        add = 0;
        if (*in == '%' && in[1]) {
            switch (in[1]) {
            case 'n':
                add = svp.plname;
                break;
            case 'r':
                add = (flags.female && gu.urole.name.f) ? gu.urole.name.f
                                                         : gu.urole.name.m;
                break;
            case '%':
                add = "%";
                break;
            }
        }
        if (add) {
            while (*add && o < out + outsz - 1)
                *o++ = *add++;
            in += 2;
        } else {
            *o++ = *in++;
        }
    }
    *o = '\0';
    return out;
}

staticfn const char *
st_place(d_level *lev, char *buf)
{
    if (lev->dlevel <= 0)
        return strcpy(buf, "somewhere");
    if (In_city(lev))
        return city_place_phrase(lev, buf);
    return strcpy(buf, svd.dungeons[lev->dnum].dname);
}

staticfn boolean
st_on(d_level *lev)
{
    return (boolean) (lev->dlevel > 0 && on_level(lev, &u.uz));
}

staticfn struct st_stage *
st_cur(int q)
{
    struct st_quest *qp = &st.quests[q];

    if (qp->status != Q_ACTIVE || qp->stage < 0 || qp->stage >= qp->nstages)
        return (struct st_stage *) 0;
    return &qp->stages[qp->stage];
}

/*
 * Story items
 */

staticfn struct obj *
st_carried(const char *id)
{
    struct obj *otmp;

    for (otmp = gi.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == STORY_ITEM && has_oname(otmp)
            && !strcmp(ONAME(otmp), id))
            return otmp;
    return (struct obj *) 0;
}

staticfn void
st_give_item(const char *id)
{
    struct obj *otmp;

    if (st_item_idx(id) < 0) {
        impossible("story: unknown item \"%s\"", id);
        return;
    }
    otmp = mksobj(STORY_ITEM, FALSE, FALSE);
    otmp = oname(otmp, id, ONAME_NO_FLAGS);
    otmp->known = otmp->dknown = otmp->bknown = 1;
    (void) hold_another_object(otmp, "You drop %s!", doname(otmp),
                               (const char *) 0);
}

staticfn void
st_take_item(const char *id)
{
    struct obj *otmp = st_carried(id);

    if (otmp) {
        if (otmp->owornmask)
            remove_worn_item(otmp, FALSE);
        useup(otmp);
    }
}

/* name of a story item for xname() */
const char *
story_item_name(struct obj *obj)
{
    int i;

    if (has_oname(obj) && (i = st_item_idx(ONAME(obj))) >= 0)
        return st.items[i].name;
    return "data shard";
}

/*
 * NPCs
 */

/* the monster type for a story NPC, for des.monster({ npc = "id" }) */
int
story_npc_montype(const char *id, boolean *female)
{
    int i = st_npc_idx(id), gend = 0, pm = NON_PM;

    if (i >= 0 && st.npcs[i].monster)
        pm = name_to_mon(st.npcs[i].monster, &gend);
    if (female)
        *female = (i >= 0) ? st.npcs[i].female : FALSE;
    if (pm < LOW_PM)
        pm = PM_HUMAN;
    return pm;
}

const char *
story_npc_name(const char *id)
{
    int i = st_npc_idx(id);

    return (i >= 0) ? st.npcs[i].name : (const char *) 0;
}

boolean
story_npc_hostile(const char *id)
{
    int i = st_npc_idx(id);

    return (i >= 0) ? (st.npcs[i].hostile || st.npcs[i].angry) : FALSE;
}

/* should a level file make this NPC here?  Not if it's alive elsewhere,
   dead, gone, or lives somewhere else */
boolean
story_npc_wanted(const char *id)
{
    int i = st_npc_idx(id);

    if (!st.loaded || i < 0)
        return TRUE;
    return (boolean) (st.npcs[i].status == NPC_UNMADE
                      && (st.npcs[i].homelev.dlevel <= 0
                          || on_level(&st.npcs[i].homelev, &u.uz)));
}

/* pets leave peaceful story NPCs alone */
boolean
story_npc_protected(struct monst *mtmp)
{
    return (boolean) (mtmp->mpeaceful && st_npc_by_mon(mtmp) != 0);
}

/* a level file or the story made an NPC's monster */
void
story_npc_made(const char *id, struct monst *mtmp)
{
    int i = st_npc_idx(id);
    struct st_npc *np;

    if (i < 0) {
        impossible("story: unknown NPC \"%s\"", id);
        return;
    }
    np = &st.npcs[i];
    if (np->status == NPC_ALIVE && np->m_id && np->m_id != mtmp->m_id) {
        /* already exists elsewhere; this copy isn't the NPC */
        return;
    }
    np->m_id = mtmp->m_id;
    np->status = NPC_ALIVE;
    np->pending = FALSE;
    if (np->hostile || np->angry) {
        mtmp->mpeaceful = 0;
    } else {
        mtmp->mpeaceful = 1;
        mtmp->mstrategy &= ~STRAT_WAITMASK;
    }
    if (!has_mgivenname(mtmp))
        mtmp = christen_monst(mtmp, np->name);
    if (np->level > 0 && np->level < 50) {
        mtmp->m_lev = (uchar) np->level;
        mtmp->mhpmax = mtmp->mhp = max(d(np->level, 8), np->level * 4);
    }
    set_malign(mtmp);
}

/* NPCs that keep their spot (fixers, ripperdocs) */
boolean
story_npc_stays(struct monst *mtmp)
{
    struct st_npc *np;

    if (!st.loaded || !mtmp->mpeaceful)
        return FALSE;
    np = st_npc_by_mon(mtmp);
    return (boolean) (np && np->stay && !mtmp->mtame);
}

/* make an NPC now if the hero is at its home, otherwise on arrival */
staticfn void
st_spawn(struct st_npc *np, boolean now)
{
    struct monst *mtmp;
    boolean fem = FALSE;
    int pm;

    if (np->status == NPC_ALIVE)
        return;
    if (np->homelev.dlevel > 0 && !st_on(&np->homelev)) {
        np->pending = TRUE;
        return;
    }
    if (!now) {
        np->pending = TRUE;
        return;
    }
    pm = story_npc_montype(np->id, &fem);
    {
        coordxy sx = 0, sy = 0;
        coord cc;
        stairway *stway;

        if (np->spot == SPOT_METRO) {
            for (stway = gs.stairs; stway; stway = stway->next)
                if (stway->kind == STWAY_METRO
                    && enexto(&cc, stway->sx, stway->sy, &mons[pm])) {
                    sx = cc.x, sy = cc.y;
                    break;
                }
        } else if (np->spot == SPOT_NEAR
                   && enexto(&cc, u.ux, u.uy, &mons[pm])) {
            sx = cc.x, sy = cc.y;
        }
        if (np->nearnpc) {
            int k = st_npc_idx(np->nearnpc);
            struct monst *other = (k >= 0) ? st_npc_mon(&st.npcs[k]) : 0;

            if (other && enexto(&cc, other->mx, other->my, &mons[pm]))
                sx = cc.x, sy = cc.y;
        }
        mtmp = makemon(&mons[pm], sx, sy,
                       MM_NOMSG | (fem ? MM_FEMALE : MM_MALE)
                       | ((np->hostile || np->angry) ? 0 : NO_MINVENT));
    }
    if (!mtmp) {
        np->pending = TRUE;
        return;
    }
    story_npc_made(np->id, mtmp);
}

staticfn void
st_despawn(struct st_npc *np)
{
    struct monst *mtmp = st_npc_mon(np);

    np->pending = FALSE;
    if (mtmp) {
        if (canseemon(mtmp))
            pline("%s leaves.", Monnam(mtmp));
        mongone(mtmp);
    }
    if (np->status != NPC_DEAD)
        np->status = NPC_GONE;
}

/* a monster died or left play (m_detach) */
void
story_mon_gone(struct monst *mtmp, boolean died)
{
    struct st_npc *np;
    int i, q;
    char buf[BUFSZ];

    if (!st.loaded || (np = st_npc_by_mon(mtmp)) == 0)
        return;
    np->status = died ? NPC_DEAD : NPC_GONE;
    if (!died)
        return;
    story_effects(&np->died);
    for (q = 0; q < st.nquests; q++) {
        struct st_stage *sp = st_cur(q);

        if (!sp)
            continue;
        if (sp->kill && !strcmp(sp->kill, np->id)) {
            st_finish_stage(q);
        } else if ((sp->talk && !strcmp(sp->talk, np->id))) {
            Sprintf(buf, "%s is dead.", np->name);
            st_end_quest(q, Q_FAILED, buf);
        }
    }
    i = (int) (np - st.npcs);
    nhUse(i);
}

/*
 * Factions
 */

/* change a faction's opinion of you; its people react when it tips */
staticfn void
st_rep_add(int f, long amt, boolean loud)
{
    struct st_faction *fp = &st.factions[f];
    int was = st_stance(fp->rep), now;

    fp->rep += amt;
    if (fp->rep > 100)
        fp->rep = 100;
    if (fp->rep < -100)
        fp->rep = -100;
    now = st_stance(fp->rep);
    if (now != was) {
        pline("Word gets around: %s %s.", fp->name,
              (now > 0) ? "counts you as a friend now"
              : (now < 0) ? "wants you dead"
              : (was > 0) ? "doesn't trust you anymore"
              : "is willing to let things go");
        st_faction_sync();
    } else if (loud && amt) {
        char buf[BUFSZ];

        pline("%s will remember that.", upstart(strcpy(buf, fp->name)));
    }
}

/* the faction a monster belongs to, or -1; story NPCs, pets and the
   like answer to nobody */
staticfn int
st_member_of(struct monst *mtmp)
{
    int f, k, mndx = monsndx(mtmp->data);

    if (mtmp->mtame || mtmp->isshk || mtmp->ispriest || mtmp->isgd
        || mtmp->isminion || st_npc_by_mon(mtmp))
        return -1;
    for (f = 0; f < st.nfactions; f++)
        for (k = 0; k < st.factions[f].nmembers; k++)
            if (st.factions[f].members[k] == mndx)
                return f;
    return -1;
}

/* a faction's people on this level act on its opinion of you */
staticfn void
st_faction_sync(void)
{
    struct monst *mtmp;
    int f, stance;

    if (!In_city(&u.uz)) /* the Undercity knows nothing of the factions */
        return;
    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (DEADMONSTER(mtmp) || (f = st_member_of(mtmp)) < 0)
            continue;
        stance = st_stance(st.factions[f].rep);
        if (stance > 0 && !mtmp->mpeaceful) {
            mtmp->mpeaceful = 1;
            set_malign(mtmp);
            newsym(mtmp->mx, mtmp->my);
        } else if (stance < 0 && mtmp->mpeaceful) {
            mtmp->mpeaceful = 0;
            mtmp->mstrategy &= ~STRAT_WAITMASK;
            set_malign(mtmp);
            newsym(mtmp->mx, mtmp->my);
        }
    }
}

/* for peace_minded(): 1 peaceful, 0 hostile, -1 up to NetHack */
int
story_faction_peace(struct permonst *ptr)
{
    int f, k, mndx = monsndx(ptr), stance;

    if (!st.loaded || !In_city(&u.uz))
        return -1;
    for (f = 0; f < st.nfactions; f++)
        for (k = 0; k < st.factions[f].nmembers; k++)
            if (st.factions[f].members[k] == mndx) {
                stance = st_stance(st.factions[f].rep);
                return (stance > 0) ? 1 : (stance < 0) ? 0 : -1;
            }
    return -1;
}

/* you attacked a peaceful monster, or killed one */
void
story_angered(struct monst *mtmp)
{
    int f;

    if (st.loaded && In_city(&u.uz) && (f = st_member_of(mtmp)) >= 0)
        st_rep_add(f, -3L, FALSE);
}

void
story_killed(struct monst *mtmp)
{
    int f;

    if (st.loaded && In_city(&u.uz) && (f = st_member_of(mtmp)) >= 0)
        st_rep_add(f, mtmp->mpeaceful ? -4L : -1L, FALSE);
}

/*
 * NPCs the story changes
 */

staticfn void
st_anger(struct st_npc *np)
{
    struct monst *mtmp = st_npc_mon(np);

    np->angry = TRUE;
    if (mtmp) {
        mtmp->mpeaceful = 0;
        mtmp->mtame = 0;
        mtmp->mstrategy &= ~STRAT_WAITMASK;
        set_malign(mtmp);
        newsym(mtmp->mx, mtmp->my);
    }
}

/* an old copy of a moved NPC, to remove when its level is visited */
staticfn void
st_stale_add(unsigned mid)
{
    if (mid && st.nstale < STALE_MAX)
        st.stale[st.nstale++] = mid;
}

staticfn void
st_stale_remove(void)
{
    struct monst *mtmp, *nmtmp;
    int k;

    for (mtmp = fmon; mtmp; mtmp = nmtmp) {
        nmtmp = mtmp->nmon;
        if (DEADMONSTER(mtmp))
            continue;
        for (k = 0; k < st.nstale; k++)
            if (st.stale[k] == mtmp->m_id) {
                mongone(mtmp);
                st.stale[k] = st.stale[--st.nstale];
                break;
            }
    }
}

/* the NPC lives somewhere else now; it turns up there */
staticfn void
st_move_npc(struct st_npc *np, const char *place)
{
    d_level lev;
    struct monst *mtmp;

    if (!city_resolve_dest(place, &lev)) {
        impossible("story: unknown place \"%s\"", place);
        return;
    }
    st_free(np->home);
    np->home = dupstr(place);
    np->homelev = lev;
    np->moved = TRUE;
    if (np->status == NPC_DEAD)
        return;
    if ((mtmp = st_npc_mon(np)) != 0) {
        if (on_level(&lev, &u.uz))
            return; /* already there */
        if (canseemon(mtmp))
            pline("%s is gone.", Monnam(mtmp));
        np->status = NPC_GONE; /* so that m_detach doesn't count it */
        mongone(mtmp);
    } else if (np->m_id) {
        st_stale_add(np->m_id);
    }
    np->status = NPC_UNMADE;
    np->m_id = 0;
    np->pending = FALSE;
    st_spawn(np, TRUE);
}

/* monsters that join you */
staticfn void
st_allies(const char *name, long n)
{
    int gend = 0, pm = name_to_mon(name, &gend);
    struct monst *mtmp;

    if (pm < LOW_PM) {
        impossible("story: unknown monster \"%s\"", name);
        return;
    }
    while (n-- > 0) {
        mtmp = makemon(&mons[pm], u.ux, u.uy,
                       MM_EDOG | MM_ADJACENTOK | MM_NOMSG);
        if (!mtmp)
            break;
        initedog(mtmp, TRUE);
    }
}

/*
 * Locks and endings
 */

/* stairs into a site or the Undercity that the story keeps shut */
boolean
story_stair_locked(d_level *dest)
{
    int i;
    char buf[BUFSZ * 4];

    if (!st.loaded || !dest || dest->dnum == u.uz.dnum)
        return FALSE;
    for (i = 0; i < st.nlocks; i++) {
        struct st_lock *lp = &st.locks[i];

        if (lp->lev.dnum != dest->dnum
            || (lp->lev.dlevel > 0 && lp->lev.dlevel != dest->dlevel))
            continue;
        if (lp->flag && st_flag_val(lp->flag))
            return FALSE;
        if (lp->text)
            pline("%s", st_subst(lp->text, buf, (int) sizeof buf));
        else
            pline("That way is closed.");
        return TRUE;
    }
    return FALSE;
}

/* is a site closed to gigs? */
staticfn boolean
st_site_locked(xint16 dnum)
{
    int i;

    for (i = 0; i < st.nlocks; i++)
        if (st.locks[i].lev.dnum == dnum && st.locks[i].lev.dlevel <= 0
            && !(st.locks[i].flag && st_flag_val(st.locks[i].flag)))
            return TRUE;
    return FALSE;
}

staticfn int
st_ending_idx(const char *id)
{
    int i;

    for (i = 0; id && i < st.nendings; i++)
        if (!strcmp(st.endings[i].id, id))
            return i;
    return -1;
}

/* the story ends: walk away (the game ends), or stay */
staticfn void
st_ending(int e)
{
    struct st_ending *ep = &st.endings[e];
    char buf[BUFSZ];

    st_show_text(ep->title, ep->text);
    livelog_printf(LL_ACHIEVE, "chose the %s ending of NeonHack 2",
                   ep->title);
    Sprintf(buf, "Ending: %s.", ep->title);
    st_log_add(st.tracked, svm.moves, buf);
    st_flag_set("ending", (long) e + 1);
    Sprintf(buf, "ending_%s", ep->id);
    st_flag_set(buf, 1L);
    if (!iflags.debug_fuzzer
        && y_n("Walk away from Neo Yendor?  (The game ends here.)") == 'y') {
        Snprintf(st.walkaway, sizeof st.walkaway,
                 "walked away from Neo Yendor (%s)", ep->title);
        done(ESCAPED);
        st.walkaway[0] = '\0'; /* still here: a debug-mode "no" */
        return;
    }
    pline("Neo Yendor goes on, and so do you.");
    story_effects(&ep->stay);
}

/* for done(): how the game ended, if you walked away */
const char *
story_walkaway(void)
{
    return st.walkaway[0] ? st.walkaway : (const char *) 0;
}

/*
 * Conditions and effects
 */

staticfn boolean
st_need(struct st_kvl *need)
{
    int i, f, q;

    for (i = 0; i < need->n; i++) {
        struct st_kv *kv = &need->kv[i];
        const char *s = kv->str ? kv->str : "";

        if (!strcmp(kv->key, "flag")) {
            if (!st_flag_val(s))
                return FALSE;
        } else if (!strcmp(kv->key, "noflag")) {
            if (st_flag_val(s))
                return FALSE;
        } else if (!strcmp(kv->key, "item")) {
            if (!st_carried(s))
                return FALSE;
        } else if (!strcmp(kv->key, "credits")) {
            if (money_cnt(gi.invent) < kv->num)
                return FALSE;
        } else if (!strcmp(kv->key, "rep")) {
            if ((f = st_faction_idx(s)) < 0 || st.factions[f].rep < kv->num)
                return FALSE;
        } else if (!strcmp(kv->key, "norep")) {
            if ((f = st_faction_idx(s)) >= 0 && st.factions[f].rep >= kv->num)
                return FALSE;
        } else if (!strcmp(kv->key, "active")) {
            if ((q = st_quest_idx(s)) < 0 || st.quests[q].status != Q_ACTIVE)
                return FALSE;
        } else if (!strcmp(kv->key, "done")) {
            if ((q = st_quest_idx(s)) < 0 || st.quests[q].status != Q_DONE)
                return FALSE;
        } else if (!strcmp(kv->key, "notdone")) {
            if ((q = st_quest_idx(s)) >= 0 && st.quests[q].status == Q_DONE)
                return FALSE;
        } else if (!strcmp(kv->key, "worst") || !strcmp(kv->key, "best")) {
            f = st_rank_faction(!strcmp(kv->key, "best"));
            if (f < 0 || strcmp(st.factions[f].id, s))
                return FALSE;
        } else if (!strcmp(kv->key, "alive") || !strcmp(kv->key, "dead")) {
            int x = st_npc_idx(s);
            boolean dead = (x >= 0 && st.npcs[x].status == NPC_DEAD);

            if (dead != !strcmp(kv->key, "dead"))
                return FALSE;
        }
    }
    return TRUE;
}

/* the faction you're on the worst (or best) terms with; ties go to the
   first in id order */
staticfn int
st_rank_faction(boolean best)
{
    int i, f = -1;

    for (i = 0; i < st.nfactions; i++)
        if (f < 0 || (best ? st.factions[i].rep > st.factions[f].rep
                           : st.factions[i].rep < st.factions[f].rep))
            f = i;
    return f;
}

/* a choice whose only unmet needs are ones the player can see (credits,
   an item) is shown greyed out with the reason in 'why'; any other unmet
   need (flags, reputation, quests) keeps it hidden */
staticfn boolean
st_need_shown(struct st_kvl *need, char *why)
{
    int i, x;
    long have;
    char tmp[BUFSZ];

    *why = '\0';
    for (i = 0; i < need->n; i++) {
        struct st_kv *kv = &need->kv[i];
        struct st_kvl one;

        one.n = 1, one.kv = kv;
        if (st_need(&one))
            continue;
        if (!strcmp(kv->key, "credits")) {
            have = money_cnt(gi.invent);
            Sprintf(tmp, "needs %ld %s; you have %ld", kv->num,
                    currency(kv->num), have);
        } else if (!strcmp(kv->key, "item")
                   && (x = st_item_idx(kv->str)) >= 0) {
            Sprintf(tmp, "needs %s", an(st.items[x].name));
        } else {
            return FALSE;
        }
        if (*why)
            Strcat(why, ", ");
        if (strlen(why) + strlen(tmp) < BUFSZ - 1)
            Strcat(why, tmp);
    }
    return (boolean) (*why != '\0');
}

/* show a block of story text in a window */
staticfn void
st_show_text(const char *title, const char *text)
{
    winid win;
    menu_item *nopick = 0;
    char buf[BUFSZ * 4];

    if (!text || !*text)
        return;
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    st_wrap_menu(win, st_subst(text, buf, (int) sizeof buf));
    end_menu(win, title);
    (void) select_menu(win, PICK_NONE, &nopick);
    destroy_nhwindow(win);
}

/* add text to a menu, word-wrapped; blank lines separate paragraphs */
staticfn void
st_wrap_menu(winid win, const char *text)
{
    char line[BUFSZ], word[BUFSZ];
    const char *p = text;
    int len = 0, wl, nsp = 0;
    boolean pending_blank = FALSE;

    line[0] = '\0';
    while (*p) {
        /* paragraph break */
        if (*p == '\n') {
            const char *q = p + 1;

            while (*q == ' ' || *q == '\t')
                q++;
            if (*q == '\n') {
                if (len) {
                    add_menu_str(win, line);
                    line[0] = '\0', len = 0;
                }
                pending_blank = TRUE;
                p = q + 1;
                continue;
            }
            nsp++;
            p++;
            continue;
        }
        if (*p == ' ' || *p == '\t') {
            nsp++;
            p++;
            continue;
        }
        for (wl = 0; *p && *p != ' ' && *p != '\t' && *p != '\n'
                     && wl < (int) sizeof word - 1; p++)
            word[wl++] = *p;
        word[wl] = '\0';
        if (pending_blank) {
            add_menu_str(win, "");
            pending_blank = FALSE;
        }
        if (len && len + 1 + wl > 72) {
            add_menu_str(win, line);
            line[0] = '\0', len = 0;
        }
        if (len) {
            /* keep the two spaces the writer put after a sentence */
            Strcat(line, " "), len++;
            if (nsp >= 2 && len + 1 + wl <= 72)
                Strcat(line, " "), len++;
        }
        nsp = 0;
        if (len + wl < (int) sizeof line - 1)
            Strcat(line, word), len += wl;
    }
    if (len)
        add_menu_str(win, line);
}

/* run effects, in a fixed order */
void
story_effects(void *vkvl)
{
    static const char *const order[] = {
        "say", "text", "log", "cases", "when", "credits", "object", "item",
        "take", "rep", "heat", "flag", "flags", "unflag", "anger", "move",
        "spawn", "despawn", "allies", "unlock", "advance", "fail", "finish",
        "start", "track", "xp", "gigs", "ending", 0
    };
    struct st_kvl *eff = (struct st_kvl *) vkvl;
    const char *calltitle = 0;
    char buf[BUFSZ * 4], tbuf[BUFSZ];
    int o, i, x;

    if (!eff || !eff->n)
        return;
    st.busy++;
    for (i = 0; i < eff->n; i++)
        if (!strcmp(eff->kv[i].key, "call") && eff->kv[i].str) {
            x = st_npc_idx(eff->kv[i].str);
            Sprintf(tbuf, "Holo-call: %s",
                    x >= 0 ? st.npcs[x].name : eff->kv[i].str);
            calltitle = tbuf;
        }
    for (o = 0; order[o]; o++) {
        for (i = 0; i < eff->n; i++) {
            struct st_kv *kv = &eff->kv[i];
            const char *s = kv->str ? kv->str : "";

            if (strcmp(kv->key, order[o]))
                continue;
            if (!strcmp(kv->key, "say")) {
                pline("%s", st_subst(s, buf, (int) sizeof buf));
            } else if (!strcmp(kv->key, "text")) {
                st_show_text(calltitle ? calltitle : "", s);
            } else if (!strcmp(kv->key, "log")) {
                /* attached to the tracked quest, else the first active */
                int q = st.tracked;

                if (q < 0 || st.quests[q].status != Q_ACTIVE)
                    for (q = 0; q < st.nquests; q++)
                        if (st.quests[q].status == Q_ACTIVE)
                            break;
                st_log_add(q < st.nquests ? q : -1, svm.moves,
                           st_subst(s, buf, (int) sizeof buf));
            } else if (!strcmp(kv->key, "credits")) {
                if (kv->num > 0) {
                    struct obj *gold = mksobj(GOLD_PIECE, FALSE, FALSE);

                    gold->quan = kv->num;
                    gold->owt = weight(gold);
                    You("get %ld %s.", kv->num, currency(kv->num));
                    (void) hold_another_object(gold, "You drop %s!",
                                               doname(gold),
                                               (const char *) 0);
                } else if (kv->num < 0) {
                    long amt = -kv->num, have = money_cnt(gi.invent);

                    if (amt > have)
                        amt = have;
                    if (amt > 0) {
                        (void) neon2_pay(amt);
                        You("hand over %ld %s.", amt, currency(amt));
                    }
                }
            } else if (!strcmp(kv->key, "object")) {
                struct obj *otmp = readobjnam(strcpy(buf, s),
                                              (struct obj *) 0);

                if (otmp && otmp != &hands_obj) {
                    otmp = hold_another_object(otmp, "You drop %s!",
                                               doname(otmp),
                                               (const char *) 0);
                    if (otmp)
                        prinv((char *) 0, otmp, 0L);
                }
            } else if (!strcmp(kv->key, "item")) {
                st_give_item(s);
            } else if (!strcmp(kv->key, "take")) {
                st_take_item(s);
            } else if (!strcmp(kv->key, "cases") || !strcmp(kv->key, "when")) {
                /* cases: the first that holds; when: every one that does */
                int c;

                for (c = 0; c < kv->ncases; c++)
                    if (st_need(&kv->cases[c].need)) {
                        story_effects(&kv->cases[c].eff);
                        if (!strcmp(kv->key, "cases"))
                            break;
                    }
            } else if (!strcmp(kv->key, "rep")) {
                if ((x = st_faction_idx(s)) >= 0)
                    st_rep_add(x, kv->num, TRUE);
                else
                    impossible("story: unknown faction \"%s\"", s);
            } else if (!strcmp(kv->key, "heat")) {
                heat_add((int) kv->num);
            } else if (!strcmp(kv->key, "anger")) {
                if ((x = st_npc_idx(s)) >= 0)
                    st_anger(&st.npcs[x]);
            } else if (!strcmp(kv->key, "move")) {
                if ((x = st_npc_idx(s)) >= 0 && kv->val)
                    st_move_npc(&st.npcs[x], kv->val);
            } else if (!strcmp(kv->key, "allies")) {
                st_allies(s, kv->num ? kv->num : 1L);
            } else if (!strcmp(kv->key, "gigs")) {
                if (st.talking && st.talking->fixer)
                    st_gig_board(st.talking);
            } else if (!strcmp(kv->key, "ending")) {
                if ((x = st_ending_idx(s)) >= 0)
                    st_ending(x);
                else
                    impossible("story: unknown ending \"%s\"", s);
            } else if (!strcmp(kv->key, "flag")) {
                st_flag_set(s, 1L);
            } else if (!strcmp(kv->key, "flags")) {
                st_flag_set(s, kv->num);
            } else if (!strcmp(kv->key, "unflag")) {
                st_flag_set(s, 0L);
            } else if (!strcmp(kv->key, "spawn")) {
                if ((x = st_npc_idx(s)) >= 0)
                    st_spawn(&st.npcs[x], TRUE);
            } else if (!strcmp(kv->key, "despawn")) {
                if ((x = st_npc_idx(s)) >= 0)
                    st_despawn(&st.npcs[x]);
            } else if (!strcmp(kv->key, "unlock")) {
                if ((x = city_district_num(s)) > 0)
                    city_unlock(x, TRUE);
            } else if (!strcmp(kv->key, "advance")) {
                if ((x = st_quest_idx(s)) >= 0 && st_cur(x))
                    st_finish_stage(x);
            } else if (!strcmp(kv->key, "fail")) {
                if ((x = st_quest_idx(s)) >= 0)
                    st_end_quest(x, Q_FAILED, (char *) 0);
            } else if (!strcmp(kv->key, "finish")) {
                if ((x = st_quest_idx(s)) >= 0)
                    st_end_quest(x, Q_DONE, (char *) 0);
            } else if (!strcmp(kv->key, "start")) {
                if ((x = st_quest_idx(s)) >= 0)
                    st_start_quest(x, TRUE);
                else
                    impossible("story: unknown quest \"%s\"", s);
            } else if (!strcmp(kv->key, "track")) {
                if ((x = st_quest_idx(s)) >= 0)
                    st.tracked = x;
            } else if (!strcmp(kv->key, "xp")) {
                more_experienced((int) kv->num, 0);
                newexplevel();
            }
        }
    }
    st.busy--;
    if (!st.busy)
        st_recheck();
}

/*
 * Gigs
 */

/* a place as story.lua would write it: "nc-stacks", "Block 7:2" */
staticfn void
st_place_spec(d_level *lev, char *buf)
{
    s_level *sp = Is_special(lev);

    if (In_district(lev) && sp)
        Strcpy(buf, sp->proto);
    else
        Sprintf(buf, "%s:%d", svd.dungeons[lev->dnum].dname, lev->dlevel);
}

/* fill a gig's holes: %T target, %P place, %I item, %F fixer, $target,
   $item, $place, $fixer, $fixerhome, $reward; returns a new string */
staticfn char *
st_gigsub(const char *in, struct st_gig *g)
{
    char out[BUFSZ * 4], tok[BUFSZ], *o = out;
    const char *add, *p = in;
    struct st_npc *fx = &st.npcs[g->fixer];
    int l;

    if (!in)
        return (char *) 0;
    while (*p && o < out + sizeof out - 1) {
        add = 0;
        tok[0] = '\0';
        if (*p == '%' && p[1] && strchr("TPIF", p[1])) {
            switch (p[1]) {
            case 'T':
                add = g->tname;
                break;
            case 'P':
                add = st_place(&g->place, tok);
                break;
            case 'I':
                add = g->iname;
                break;
            case 'F':
                add = fx->name;
                break;
            }
            p += 2;
        } else if (*p == '$') {
            static const char *const names[] = { "target", "item", "place",
                                                 "fixerhome", "fixer",
                                                 "reward", 0 };
            int i;

            for (i = 0; names[i]; i++)
                if (!strncmp(p + 1, names[i], strlen(names[i])))
                    break;
            if (!names[i]) {
                *o++ = *p++;
                continue;
            }
            p += 1 + strlen(names[i]);
            switch (i) {
            case 0:
                Sprintf(tok, "gig%ld_t", g->seq);
                break;
            case 1:
                Sprintf(tok, "gig%ld_i", g->seq);
                break;
            case 2:
                st_place_spec(&g->place, tok);
                break;
            case 3:
                Strcpy(tok, fx->home ? fx->home : "");
                break;
            case 4:
                Strcpy(tok, fx->id);
                break;
            case 5:
                Sprintf(tok, "%ld", g->reward);
                break;
            }
            add = tok;
        } else {
            *o++ = *p++;
            continue;
        }
        l = (int) strlen(add);
        if (o + l < out + sizeof out - 1)
            Strcpy(o, add), o += l;
    }
    *o = '\0';
    return dupstr(out);
}

/* copy effects with the holes filled; "credits = '$reward'" becomes a
   number */
staticfn void
st_gigsub_kvl(struct st_kvl *to, struct st_kvl *from, struct st_gig *g)
{
    int i;

    int j;

    to->n = from->n;
    to->kv = (struct st_kv *) alloc((from->n + 1) * sizeof *to->kv);
    for (i = 0; i < from->n; i++) {
        struct st_kv *f = &from->kv[i], *t = &to->kv[i];

        (void) memset((genericptr_t) t, 0, sizeof *t);
        t->key = dupstr(f->key);
        t->num = f->num;
        t->str = st_gigsub(f->str, g);
        t->val = st_gigsub(f->val, g);
        if (t->str && (!strcmp(t->key, "credits") || !strcmp(t->key, "xp"))
            && digit(*t->str)) {
            t->num = atol(t->str);
            free((genericptr_t) t->str), t->str = 0;
        }
        if (f->ncases) {
            t->ncases = f->ncases;
            t->cases = (struct st_case *) alloc(f->ncases * sizeof *t->cases);
            for (j = 0; j < f->ncases; j++) {
                st_gigsub_kvl(&t->cases[j].need, &f->cases[j].need, g);
                st_gigsub_kvl(&t->cases[j].eff, &f->cases[j].eff, g);
            }
        }
    }
}

/* put a gig into a slot: its quest, target NPC and item; returns the
   quest's index (it isn't started) */
staticfn int
st_gig_build(int slot, long seq, int tpl, int fixer, d_level *place,
             long reward, int pm, const char *tname, const char *iname,
             boolean dropped)
{
    struct st_gigtpl *tp = &st.tpls[tpl];
    struct st_gig *g = &st.gigs[slot];
    int q = st.nstatq + slot, nn = st.nstatn + slot, ii = st.nstati + slot,
        j;
    struct st_quest *qp = &st.quests[q];
    struct st_npc *np = &st.npcs[nn];
    struct st_item *ip = &st.items[ii];
    char buf[BUFSZ];

    g->used = TRUE;
    g->seq = seq;
    g->tpl = tpl;
    g->fixer = fixer;
    g->place = *place;
    g->reward = reward;
    g->pm = pm;
    copynchars(g->tname, tname, (int) sizeof g->tname - 1);
    copynchars(g->iname, iname, (int) sizeof g->iname - 1);

    /* the quest */
    st_free_quest(qp);
    Sprintf(buf, "gig%ld", seq);
    qp->id = dupstr(buf);
    qp->title = st_gigsub(tp->q.title, g);
    qp->kind = dupstr("gig");
    qp->nstages = tp->q.nstages;
    qp->stages = (struct st_stage *) alloc((qp->nstages + 1)
                                           * sizeof *qp->stages);
    (void) memset(qp->stages, 0, (qp->nstages + 1) * sizeof *qp->stages);
    for (j = 0; j < qp->nstages; j++) {
        struct st_stage *from = &tp->q.stages[j], *to = &qp->stages[j];

        to->goal = st_gigsub(from->goal, g);
        to->where = st_gigsub(from->where, g);
        to->talk = st_gigsub(from->talk, g);
        to->give = st_gigsub(from->give, g);
        to->carry = st_gigsub(from->carry, g);
        to->kill = st_gigsub(from->kill, g);
        to->arrive = st_gigsub(from->arrive, g);
        to->flag = st_gigsub(from->flag, g);
        to->dialogue = st_gigsub(from->dialogue, g);
        to->remind = st_gigsub(from->remind, g);
        to->log = st_gigsub(from->log, g);
        st_gigsub_kvl(&to->start, &from->start, g);
        st_gigsub_kvl(&to->done, &from->done, g);
        if (!to->where || !city_resolve_dest(to->where, &to->wherelev))
            to->wherelev.dlevel = 0;
        if (!to->arrive || !city_resolve_dest(to->arrive, &to->arrivelev))
            to->arrivelev.dlevel = 0;
        if (!to->wherelev.dlevel && to->arrivelev.dlevel)
            to->wherelev = to->arrivelev;
    }
    st_gigsub_kvl(&qp->done, &tp->q.done, g);
    qp->status = Q_INACTIVE;
    qp->stage = 0;
    qp->ended = 0L;

    /* the target */
    st_free(np->id), st_free(np->name), st_free(np->monster);
    st_free(np->home), st_free(np->faction), st_free(np->chat);
    (void) memset((genericptr_t) np, 0, sizeof *np);
    Sprintf(buf, "gig%ld_t", seq);
    np->id = dupstr(buf);
    np->name = dupstr(tname);
    np->monster = dupstr(mons[pm].pmnames[NEUTRAL]);
    st_place_spec(place, buf);
    np->home = dupstr(buf);
    np->homelev = *place;
    np->hostile = tp->hostile;
    np->stay = !tp->hostile;
    np->spot = tp->metro ? SPOT_METRO : SPOT_RANDOM;
    if (tp->hostile)
        np->level = 1 + city_difficulty(place) / 2;

    /* the item */
    st_free(ip->id), st_free(ip->name);
    (void) memset((genericptr_t) ip, 0, sizeof *ip);
    Sprintf(buf, "gig%ld_i", seq);
    ip->id = dupstr(buf);
    ip->name = dupstr(iname);
    if (tp->drop)
        ip->drop = *place;
    ip->dropped = dropped;
    return q;
}

/* somewhere for a gig: an open district, or a floor of a site in one */
staticfn boolean
st_gig_place(struct st_gigtpl *tp, d_level *lev)
{
    d_level cand[64], here;
    int n = 0, d, l;

    if (!strcmp(tp->place, "site")) {
        for (d = 0; d < svn.n_dgns && n < 60; d++) {
            dungeon *dp = &svd.dungeons[d];

            if (!dp->flags.city || d == city_dnum || dp->parent.dlevel <= 0
                || !city_district_isopen(dp->parent.dlevel)
                || st_site_locked((xint16) d))
                continue;
            for (l = 1; l <= dp->num_dunlevs && n < 60; l++) {
                here.dnum = d, here.dlevel = l;
                if (!Is_special(&here)) /* not the hand-made key floors */
                    cand[n++] = here;
            }
        }
    } else {
        for (d = 1; d <= city_district_total() && n < 60; d++) {
            if (!city_district_isopen(d))
                continue;
            here.dnum = city_dnum, here.dlevel = d;
            /* somewhere else than here, if there's anywhere else */
            if (on_level(&here, &u.uz) && city_district_total() > 1)
                continue;
            cand[n++] = here;
        }
    }
    if (!n)
        return FALSE;
    *lev = cand[rn2(n)];
    return TRUE;
}

/* a slot a new gig can use, or -1 */
staticfn int
st_gig_free_slot(void)
{
    int k;

    for (k = 0; k < GIG_SLOTS; k++) {
        struct st_quest *qp = &st.quests[st.nstatq + k];

        if (!st.gigs[k].used
            || ((qp->status == Q_DONE || qp->status == Q_FAILED
                 || qp->status == Q_INACTIVE)
                && qp->ended < svm.moves))
            return k;
    }
    return -1;
}

/* a fixer's offers */
staticfn void
st_gig_board(struct st_npc *fx)
{
    struct offer {
        int tpl, pm;
        d_level place;
        long reward;
        char tname[64], iname[64];
    } off[3];
    int nofr = 0, k, t, nactive = 0, fixer = (int) (fx - st.npcs), pick,
        tries;
    winid win;
    anything any;
    menu_item *sel = 0;
    char buf[BUFSZ], pbuf[BUFSZ];

    for (k = 0; k < GIG_SLOTS; k++)
        if (st.gigs[k].used && st.quests[st.nstatq + k].status == Q_ACTIVE)
            nactive++;
    if (nactive >= GIG_ACTIVE_MAX || st_gig_free_slot() < 0) {
        verbalize("Finish the jobs you've got, then come back.");
        return;
    }
    for (tries = 0; nofr < 3 && tries < 12 && st.ntpls; tries++) {
        struct st_gigtpl *tp;
        struct offer *o = &off[nofr];
        int gend = 0;
        boolean mine = FALSE;

        t = rn2(st.ntpls);
        tp = &st.tpls[t];
        for (k = 0; k < tp->nfixers; k++)
            if (!strcmp(tp->fixers[k], fx->id))
                mine = TRUE;
        if (tp->nfixers && !mine)
            continue;
        for (k = 0; k < nofr; k++)
            if (off[k].tpl == t)
                break;
        if (k < nofr && tries < 8) /* rather three kinds of job */
            continue;
        if (!st_gig_place(tp, &o->place))
            continue;
        o->tpl = t;
        o->pm = tp->nmonsters
                    ? name_to_mon(tp->monsters[rn2(tp->nmonsters)], &gend)
                    : PM_HUMAN;
        if (o->pm < LOW_PM)
            o->pm = PM_HUMAN;
        copynchars(o->tname, tp->nnames ? tp->names[rn2(tp->nnames)]
                                        : "a contact",
                   (int) sizeof o->tname - 1);
        copynchars(o->iname, tp->ninames ? tp->inames[rn2(tp->ninames)]
                                         : "package",
                   (int) sizeof o->iname - 1);
        o->reward = tp->reward + 10L * city_difficulty(&o->place);
        nofr++;
    }
    if (!nofr) {
        verbalize("Nothing for you today.");
        return;
    }
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    for (k = 0; k < nofr; k++) {
        struct st_gig tmp;
        char *title;

        (void) memset(&tmp, 0, sizeof tmp);
        tmp.seq = st.gigseq + 1, tmp.tpl = off[k].tpl, tmp.fixer = fixer;
        tmp.place = off[k].place, tmp.reward = off[k].reward;
        Strcpy(tmp.tname, off[k].tname), Strcpy(tmp.iname, off[k].iname);
        title = st_gigsub(st.tpls[off[k].tpl].q.title, &tmp);
        Snprintf(buf, sizeof buf, "%s (%s, %ld %s)", title,
                 st_place(&off[k].place, pbuf), off[k].reward,
                 currency(off[k].reward));
        free((genericptr_t) title);
        any = cg.zeroany;
        any.a_int = k + 1;
        add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, NO_COLOR, buf,
                 MENU_ITEMFLAGS_NONE);
    }
    add_menu_str(win, "");
    add_menu_str(win, "(pick a job; Escape for none)");
    Snprintf(buf, sizeof buf, "%s has work:", fx->name);
    end_menu(win, buf);
    pick = (select_menu(win, PICK_ONE, &sel) > 0 && sel)
               ? sel[0].item.a_int - 1 : -1;
    if (sel)
        free((genericptr_t) sel);
    destroy_nhwindow(win);
    if (pick < 0)
        return;
    if ((k = st_gig_free_slot()) < 0)
        return;
    t = st_gig_build(k, ++st.gigseq, off[pick].tpl, fixer, &off[pick].place,
                     off[pick].reward, off[pick].pm, off[pick].tname,
                     off[pick].iname, FALSE);
    st_start_quest(t, TRUE);
}

/* gig items waiting to be found on this level */
staticfn void
st_drop_items(void)
{
    int i, tries;
    coordxy x, y;
    struct obj *otmp;

    for (i = st.nstati; i < st.nitems; i++) {
        struct st_item *ip = &st.items[i];

        if (!*ip->id || ip->dropped || !st_on(&ip->drop))
            continue;
        for (tries = 0; tries < 1000; tries++) {
            x = rn1(COLNO - 3, 2), y = rn2(ROWNO);
            if ((levl[x][y].typ == ROOM || levl[x][y].typ == CORR)
                && !*in_rooms(x, y, SHOPBASE) && !On_stairs(x, y)
                && !t_at(x, y) && !closed_door(x, y))
                break;
        }
        if (tries == 1000)
            continue;
        otmp = mksobj(STORY_ITEM, FALSE, FALSE);
        otmp = oname(otmp, ip->id, ONAME_NO_FLAGS);
        otmp->known = otmp->dknown = otmp->bknown = 1;
        place_object(otmp, x, y);
        ip->dropped = TRUE;
    }
}

/*
 * City life: who walks the streets, what you see and hear
 */

/* this level's entry in the districts table, if any */
staticfn struct st_district *
st_here(void)
{
    int i;
    struct st_district *site = 0;

    if (!st.loaded || !In_city(&u.uz))
        return (struct st_district *) 0;
    for (i = 0; i < st.ndist; i++) {
        struct st_district *dp = &st.dist[i];

        if (dp->lev.dnum != u.uz.dnum)
            continue;
        if (dp->lev.dlevel == u.uz.dlevel)
            return dp;
        if (dp->lev.dlevel == 0)
            site = dp;
    }
    return site;
}

/* a random monster from the district's table, or 0 for the usual kind */
struct permonst *
story_city_monster(void)
{
    struct st_district *dp = st_here();
    int i, total = 0, r, maxd;

    if (!dp || !dp->nmon)
        return (struct permonst *) 0;
    maxd = monmax_difficulty(level_difficulty());
    for (i = 0; i < dp->nmon; i++)
        if (!(svm.mvitals[dp->mon[i]].mvflags & G_GONE)
            && !(mons[dp->mon[i]].geno & G_UNIQ)
            && mons[dp->mon[i]].difficulty <= maxd)
            total += dp->monw[i];
    if (!total)
        return (struct permonst *) 0;
    r = rn2(total);
    for (i = 0; i < dp->nmon; i++)
        if (!(svm.mvitals[dp->mon[i]].mvflags & G_GONE)
            && !(mons[dp->mon[i]].geno & G_UNIQ)
            && mons[dp->mon[i]].difficulty <= maxd) {
            if ((r -= dp->monw[i]) < 0)
                return &mons[dp->mon[i]];
        }
    return (struct permonst *) 0;
}

/* keep the streets peopled: top up the district's civilians, a few at a
   time and out of sight ('arrived': right away, up to half the number) */
void
story_city_people(boolean arrived)
{
    struct st_district *dp = st_here();
    struct monst *mtmp;
    int i, have;

    if (!dp || !dp->npeople || !In_district(&u.uz))
        return;
    for (i = 0; i < dp->npeople; i++) {
        int pm = dp->people[i], want = dp->peoplen[i];

        if (svm.mvitals[pm].mvflags & G_GONE)
            continue;
        have = 0;
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
            if (!DEADMONSTER(mtmp) && monsndx(mtmp->data) == pm)
                have++;
        if (arrived) {
            for (; have < (want + 1) / 2; have++)
                if (!makemon(&mons[pm], 0, 0, MM_NOMSG))
                    break;
        } else if (have < want && !rn2(30)) {
            (void) makemon(&mons[pm], 0, 0, MM_NOMSG);
        }
    }
}

/* now and then, something happens in the street */
void
story_city_ambience(void)
{
    struct st_district *dp = st_here();
    char buf[BUFSZ * 2];

    if (!dp || !In_district(&u.uz) || svm.moves - st.lastamb < 80
        || rn2(120))
        return;
    if (dp->nsound && (!dp->nsight || !rn2(2))) {
        if (Deaf || !flags.acoustics)
            return;
        pline("%s", st_subst(dp->sound[rn2(dp->nsound)], buf,
                             (int) sizeof buf));
    } else if (dp->nsight) {
        if (Blind)
            return;
        pline("%s", st_subst(dp->sight[rn2(dp->nsight)], buf,
                             (int) sizeof buf));
    } else {
        return;
    }
    st.lastamb = svm.moves;
}

/*
 * Quests and stages
 */

staticfn void
st_start_quest(int q, boolean announce)
{
    struct st_quest *qp = &st.quests[q];

    if (qp->status != Q_INACTIVE)
        return;
    qp->status = Q_ACTIVE;
    qp->stage = 0;
    if (announce)
        pline("New %s: %s.",
              !strcmp(qp->kind, "main") ? "story mission"
              : !strcmp(qp->kind, "gig") ? "gig" : "job",
              qp->title);
    if (st.tracked < 0 || st.quests[st.tracked].status != Q_ACTIVE
        || (!strcmp(qp->kind, "main")
            && strcmp(st.quests[st.tracked].kind, "main")))
        st.tracked = q;
    st_begin_stage(q);
}

staticfn void
st_begin_stage(int q)
{
    struct st_stage *sp = st_cur(q);
    char buf[BUFSZ * 4];

    if (!sp)
        return;
    if (sp->log)
        st_log_add(q, svm.moves, st_subst(sp->log, buf, (int) sizeof buf));
    story_effects(&sp->start);
}

staticfn void
st_finish_stage(int q)
{
    struct st_quest *qp = &st.quests[q];
    struct st_stage *sp = st_cur(q);

    if (!sp)
        return;
    if (sp->goal) {
        char buf[BUFSZ * 2], gbuf[BUFSZ];

        Snprintf(buf, sizeof buf, "Done: %s",
                 st_subst(sp->goal, gbuf, (int) sizeof gbuf));
        st_log_add(q, svm.moves, buf);
    }
    qp->stage++;
    story_effects(&sp->done);
    if (qp->status != Q_ACTIVE)
        return; /* the effects ended the quest */
    if (qp->stage >= qp->nstages)
        st_end_quest(q, Q_DONE, (char *) 0);
    else
        st_begin_stage(q);
}

staticfn void
st_end_quest(int q, xint8 how, const char *why)
{
    struct st_quest *qp = &st.quests[q];
    int i;

    if (qp->status == Q_DONE || qp->status == Q_FAILED)
        return;
    qp->status = how;
    qp->ended = svm.moves;
    if (why)
        st_log_add(q, svm.moves, why);
    if (how == Q_DONE) {
        pline("%s complete: %s.",
              !strcmp(qp->kind, "main") ? "Story mission"
              : !strcmp(qp->kind, "gig") ? "Gig" : "Job",
              qp->title);
        story_effects(&qp->done);
    } else {
        pline("%s failed: %s.",
              !strcmp(qp->kind, "main") ? "Story mission"
              : !strcmp(qp->kind, "gig") ? "Gig" : "Job",
              qp->title);
    }
    if (st.tracked == q) {
        st.tracked = -1;
        for (i = 0; i < st.nquests; i++)
            if (st.quests[i].status == Q_ACTIVE
                && (st.tracked < 0 || !strcmp(st.quests[i].kind, "main")))
                st.tracked = i;
    }
}

/* finish stages whose conditions already hold */
staticfn void
st_recheck(void)
{
    int q, rounds;
    boolean changed = TRUE;

    for (rounds = 0; changed && rounds < 20; rounds++) {
        changed = FALSE;
        for (q = 0; q < st.nquests; q++) {
            struct st_stage *sp = st_cur(q);

            if (!sp)
                continue;
            if ((sp->carry && st_carried(sp->carry))
                || (sp->flag && st_flag_val(sp->flag))
                || (sp->arrivelev.dlevel > 0 && st_on(&sp->arrivelev)
                    && !sp->talk)) {
                st_finish_stage(q);
                changed = TRUE;
            }
        }
    }
}

/*
 * Events
 */

/* a new game: start the quests marked "start" */
void
story_new_game(void)
{
    int q;

    if (!st.loaded)
        return;
    for (q = 0; q < st.nquests; q++)
        if (st.quests[q].autostart)
            st_start_quest(q, FALSE);
    story_arrived();
}

/* after every move */
void
story_turn(void)
{
    if (st.loaded && !st.busy)
        st_recheck();
}

/* after arriving on a level */
void
story_arrived(void)
{
    int i;
    struct st_stage *sp;
    char buf[BUFSZ], pbuf[BUFSZ];

    if (!st.loaded)
        return;
    for (i = 0; i < st.nnpcs; i++) {
        struct st_npc *np = &st.npcs[i];

        if (np->pending && st_on(&np->homelev))
            st_spawn(np, TRUE);
        else if (np->status == NPC_GONE && np->m_id) {
            struct monst *mtmp;

            for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
                if (!DEADMONSTER(mtmp) && mtmp->m_id == np->m_id) {
                    mongone(mtmp);
                    break;
                }
        }
    }
    st_stale_remove();
    st_faction_sync();
    st_drop_items();
    story_city_people(TRUE);
    st_recheck();
    /* remind the player what they're doing */
    if (In_district(&u.uz) && st.tracked >= 0
        && (sp = st_cur(st.tracked)) != 0 && sp->goal) {
        Sprintf(buf, "Objective: %s", st_subst(sp->goal, pbuf, BUFSZ));
        if (sp->wherelev.dlevel > 0 && !st_on(&sp->wherelev)) {
            Snprintf(eos(buf), (int) (sizeof buf - strlen(buf)), " (%s)",
                     st_place(&sp->wherelev, pbuf));
        }
        pline("%s", buf);
    }
}

/* talking to a monster; True if the story handled it */
boolean
story_chat(struct monst *mtmp)
{
    struct st_npc *np;
    struct st_stage *sp;
    int q, r;
    char buf[BUFSZ * 2];

    if (!st.loaded || (np = st_npc_by_mon(mtmp)) == 0 || !mtmp->mpeaceful)
        return FALSE;
    /* the tracked quest first, then the rest */
    for (r = -1; r < st.nquests; r++) {
        q = (r < 0) ? st.tracked : r;
        if (q < 0 || (r >= 0 && q == st.tracked))
            continue;
        if ((sp = st_cur(q)) == 0 || !sp->talk || strcmp(sp->talk, np->id))
            continue;
        if (sp->give && !st_carried(sp->give)) {
            if (sp->remind)
                pline("%s", st_subst(sp->remind, buf, (int) sizeof buf));
            else
                pline("%s is waiting for %s.", Monnam(mtmp),
                      st.items[st_item_idx(sp->give)].name);
            return TRUE;
        }
        if (sp->dialogue) {
            r = st_dialogue(np, sp->dialogue);
            if (r <= 0)
                return TRUE; /* walked away, or asked to wait */
        } else {
            You("talk with %s.", mon_nam(mtmp));
        }
        if (st_cur(q) == sp) { /* the dialogue didn't move on already */
            if (sp->give)
                st_take_item(sp->give);
            st_finish_stage(q);
        }
        return TRUE;
    }
    /* nothing urgent: small talk, which for a fixer can lead to work
       (a choice with gigs = true); a fixer without it gets to the point */
    if (np->chat && st_node_idx(np->chat) >= 0) {
        (void) st_dialogue(np, np->chat);
    } else if (np->fixer) {
        st_gig_board(np);
    } else if (np->nidle) {
        pline("%s", st_subst(np->idle[rn2(np->nidle)], buf,
                             (int) sizeof buf));
    } else {
        pline("%s has nothing to say to you right now.", Monnam(mtmp));
    }
    return TRUE;
}

/* run a conversation; 1: it ended normally, 0: walked away, -1: wait */
staticfn int
st_dialogue(struct st_npc *np, const char *start)
{
    int n = st_node_idx(start), i, k, res = 1, steps;
    winid win;
    anything any;
    menu_item *pick;
    char buf[BUFSZ * 4], tbuf[BUFSZ];

    st.talking = np;
    for (steps = 0; n >= 0 && steps < 50; steps++) {
        struct st_node *node = &st.nodes[n];
        int nshown = 0, chosen = -1, only = -1;
        char why[BUFSZ];

        /* how many answers can be picked? one is preselected, so that
           Enter (or Space) takes it */
        for (i = 0; i < node->nchoices; i++)
            if (st_need(&node->choices[i].need))
                nshown++, only = i;
        win = create_nhwindow(NHW_MENU);
        start_menu(win, MENU_BEHAVE_STANDARD);
        if (node->text)
            st_wrap_menu(win, st_subst(node->text, buf, (int) sizeof buf));
        if (node->nchoices)
            add_menu_str(win, "");
        for (i = 0; i < node->nchoices; i++) {
            struct st_choice *cp = &node->choices[i];

            if (!st_need(&cp->need)) {
                /* answers you can't afford yet are shown, but not offered */
                if (st_need_shown(&cp->need, why)) {
                    Snprintf(buf, sizeof buf, "    %s  (%s)",
                             st_subst(cp->text, tbuf, (int) sizeof tbuf),
                             why);
                    add_menu_str(win, buf);
                }
                continue;
            }
            any = cg.zeroany;
            any.a_int = i + 1;
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, NO_COLOR,
                     st_subst(cp->text, tbuf, (int) sizeof tbuf),
                     (nshown == 1) ? MENU_ITEMFLAGS_SELECTED
                                   : MENU_ITEMFLAGS_NONE);
        }
        if (nshown > 1) {
            add_menu_str(win, "");
            add_menu_str(win, "(pick an answer; Escape walks away)");
        }
        end_menu(win, np->name);
        pick = 0;
        k = select_menu(win, nshown ? PICK_ONE : PICK_NONE, &pick);
        destroy_nhwindow(win);
        if (k > 0 && pick) {
            chosen = pick[0].item.a_int - 1;
            free((genericptr_t) pick);
        }
        if (!nshown)
            break; /* just text: read and done */
        if (k == 0 && nshown == 1)
            chosen = only; /* Enter with the one answer deselected */
        if (k == 0 && chosen < 0)
            continue; /* Enter without an answer: ask again */
        if (chosen < 0) {
            You("walk away from %s.", np->name);
            res = 0;
            break;
        }
        {
            struct st_choice *cp = &node->choices[chosen];

            story_effects(&cp->eff);
            if (cp->wait)
                res = -1;
            n = cp->go ? st_node_idx(cp->go) : -1;
            if (cp->go && n < 0)
                impossible("story: unknown dialogue \"%s\"", cp->go);
        }
    }
    st.talking = 0;
    return res;
}

/*
 * The journal
 */

staticfn void
st_quest_details(int q)
{
    struct st_quest *qp = &st.quests[q];
    struct st_stage *sp = st_cur(q);
    struct st_log *lp;
    winid win;
    menu_item *nopick = 0;
    char buf[BUFSZ * 4], pbuf[BUFSZ];

    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    if (sp && sp->goal) {
        Sprintf(buf, "Objective: %s", st_subst(sp->goal, pbuf, BUFSZ));
        add_menu_str(win, buf);
        if (sp->wherelev.dlevel > 0) {
            Sprintf(buf, "Where: %s", upstart(strcpy(pbuf,
                                         st_place(&sp->wherelev, buf))));
            add_menu_str(win, buf);
        }
        add_menu_str(win, "");
    } else if (qp->status == Q_DONE) {
        add_menu_str(win, "Done.");
        add_menu_str(win, "");
    } else if (qp->status == Q_FAILED) {
        add_menu_str(win, "Failed.");
        add_menu_str(win, "");
    }
    for (lp = st.log; lp; lp = lp->next) {
        if (lp->quest != q)
            continue;
        Sprintf(buf, "T:%ld", lp->turn);
        add_menu_str(win, buf);
        st_wrap_menu(win, lp->text);
        add_menu_str(win, "");
    }
    end_menu(win, qp->title);
    (void) select_menu(win, PICK_NONE, &nopick);
    destroy_nhwindow(win);
}

/* the #journal command */
int
dojournal(void)
{
    static const struct {
        const char *kind, *heading;
    } groups[] = { { "main", "Story" }, { "side", "Jobs" }, { "gig", "Gigs" } };
    winid win;
    anything any;
    menu_item *pick = 0;
    char buf[BUFSZ], gbuf[BUFSZ], pbuf[BUFSZ];
    int g, q, n, shown = 0, ndone = 0;
    struct st_stage *sp;

    if (!st.loaded || !st.nquests) {
        pline("Your journal is empty.");
        return ECMD_OK;
    }
    win = create_nhwindow(NHW_MENU);
    start_menu(win, MENU_BEHAVE_STANDARD);
    for (g = 0; g < SIZE(groups); g++) {
        boolean head = FALSE;

        for (q = 0; q < st.nquests; q++) {
            struct st_quest *qp = &st.quests[q];

            if (qp->status != Q_ACTIVE)
                continue;
            if (strcmp(qp->kind, groups[g].kind)
                && !(g == 1 && strcmp(qp->kind, "main")
                     && strcmp(qp->kind, "gig")))
                continue;
            if (!head) {
                add_menu_heading(win, groups[g].heading);
                head = TRUE;
            }
            sp = st_cur(q);
            gbuf[0] = '\0';
            if (sp && sp->goal) {
                Snprintf(gbuf, sizeof gbuf, ": %s",
                         st_subst(sp->goal, pbuf, BUFSZ));
            }
            Snprintf(buf, sizeof buf, "%s %s%s",
                     (q == st.tracked) ? "*" : " ", qp->title, gbuf);
            any = cg.zeroany;
            any.a_int = q + 1;
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, NO_COLOR,
                     buf, MENU_ITEMFLAGS_NONE);
            shown++;
        }
    }
    for (q = 0; q < st.nquests; q++)
        if (st.quests[q].status == Q_DONE || st.quests[q].status == Q_FAILED)
            ndone++;
    if (ndone) {
        add_menu_heading(win, "Finished");
        for (q = 0; q < st.nquests; q++) {
            struct st_quest *qp = &st.quests[q];

            if (qp->status != Q_DONE && qp->status != Q_FAILED)
                continue;
            Sprintf(buf, "  %s%s", qp->title,
                    (qp->status == Q_FAILED) ? " (failed)" : "");
            any = cg.zeroany;
            any.a_int = q + 1;
            add_menu(win, &nul_glyphinfo, &any, 0, 0, ATR_NONE, NO_COLOR,
                     buf, MENU_ITEMFLAGS_NONE);
        }
    }
    if (!shown && !ndone)
        add_menu_str(win, "Nothing yet.");
    add_menu_str(win, "");
    add_menu_str(win, "(* is the tracked job; pick one to read it)");
    end_menu(win, "Journal");
    n = select_menu(win, PICK_ONE, &pick);
    destroy_nhwindow(win);
    if (n > 0 && pick) {
        q = pick[0].item.a_int - 1;
        free((genericptr_t) pick);
        st_quest_details(q);
        if (st.quests[q].status == Q_ACTIVE
            && !strcmp(st.quests[q].kind, "gig")) {
            char c;

            Snprintf(buf, sizeof buf, "\"%s\": track it, drop it, or neither?",
                     st.quests[q].title);
            c = yn_function(buf, "tdn", 'n', TRUE);
            if (c == 't')
                st.tracked = q;
            else if (c == 'd')
                st_end_quest(q, Q_FAILED, "Dropped.");
        } else if (st.quests[q].status == Q_ACTIVE && q != st.tracked) {
            Sprintf(buf, "Track \"%s\"?", st.quests[q].title);
            if (y_n(buf) == 'y')
                st.tracked = q;
        }
    }
    return ECMD_OK;
}

/*
 * City map hooks
 */

boolean
story_district_marker(int district)
{
    struct st_stage *sp;
    d_level d;

    if (!st.loaded || st.tracked < 0 || (sp = st_cur(st.tracked)) == 0)
        return FALSE;
    return (boolean) (sp->wherelev.dlevel > 0
                      && city_district_of(&sp->wherelev, &d)
                      && d.dlevel == district);
}

void
story_district_note(int district, char *buf, int bufsz)
{
    struct st_stage *sp;
    d_level d;
    int q;
    char gbuf[BUFSZ];

    if (!st.loaded || bufsz < 8)
        return;
    for (q = 0; q < st.nquests; q++) {
        if ((sp = st_cur(q)) == 0 || !sp->goal || sp->wherelev.dlevel <= 0
            || !city_district_of(&sp->wherelev, &d) || d.dlevel != district)
            continue;
        {
            int used = (int) strlen(buf);

            if (bufsz - used < 8)
                break;
            Snprintf(buf + used, bufsz - used, "%s%s",
                     used ? " - " : "", st_subst(sp->goal, gbuf, BUFSZ));
        }
    }
}

/*
 * Save file lines
 */

int
story_state_lines(char **lines, int n, int max)
{
    char buf[BUFSZ * 4 + 64];
    int i;
    struct st_flag *fp;
    struct st_log *lp;

    if (!st.loaded)
        return n;
    /* gigs before the quest and NPC lines that refer to them */
    Sprintf(buf, "gigseq %ld", st.gigseq);
    lines[n++] = dupstr(buf);
    for (i = 0; i < GIG_SLOTS && n < max; i++) {
        struct st_gig *g = &st.gigs[i];

        if (!g->used)
            continue;
        Snprintf(buf, sizeof buf, "gig %d %ld %s %s %d %d %ld %d %d|%s|%s", i,
                 g->seq, st.tpls[g->tpl].id, st.npcs[g->fixer].id,
                 g->place.dnum, g->place.dlevel, g->reward, g->pm,
                 st.items[st.nstati + i].dropped, g->tname, g->iname);
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nquests && n < max; i++) {
        struct st_quest *qp = &st.quests[i];

        if (qp->status == Q_INACTIVE)
            continue;
        Sprintf(buf, "quest %s %d %d %ld", qp->id, qp->status, qp->stage,
                qp->ended);
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nfactions && n < max; i++) {
        Sprintf(buf, "rep %s %ld", st.factions[i].id, st.factions[i].rep);
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nnpcs && n < max; i++) {
        struct st_npc *np = &st.npcs[i];

        if (np->status == NPC_UNMADE && !np->pending && !np->angry
            && !np->moved)
            continue;
        Snprintf(buf, sizeof buf, "npc %s %u %d %d %d %d %s", np->id,
                 np->m_id, np->status, np->pending, np->angry, np->moved,
                 (np->moved && np->home) ? np->home : "-");
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nstale && n < max; i++) {
        Sprintf(buf, "stale %u", st.stale[i]);
        lines[n++] = dupstr(buf);
    }
    for (fp = st.flags; fp && n < max; fp = fp->next) {
        Sprintf(buf, "flag %s %ld", fp->name, fp->val);
        lines[n++] = dupstr(buf);
    }
    if (st.tracked >= 0 && n < max) {
        Sprintf(buf, "track %s", st.quests[st.tracked].id);
        lines[n++] = dupstr(buf);
    }
    for (lp = st.log; lp && n < max; lp = lp->next) {
        Snprintf(buf, sizeof buf, "log %s %ld %s",
                 (lp->quest >= 0) ? st.quests[lp->quest].id : "-",
                 lp->turn, lp->text);
        lines[n++] = dupstr(buf);
    }
    return n;
}

boolean
story_state_line(const char *line)
{
    char id[BUFSZ];
    int a, b, i;
    unsigned mid;
    long l;

    if (!strncmp(line, "quest ", 6)) {
        l = 0L;
        if (sscanf(line, "quest %255s %d %d %ld", id, &a, &b, &l) >= 3
            && (i = st_quest_idx(id)) >= 0) {
            st.quests[i].status = (xint8) a;
            st.quests[i].stage = b;
            st.quests[i].ended = l;
        }
        return TRUE;
    }
    if (!strncmp(line, "gigseq ", 7)) {
        (void) sscanf(line, "gigseq %ld", &st.gigseq);
        return TRUE;
    }
    if (!strncmp(line, "gig ", 4)) {
        char tpl[BUFSZ], fixer[BUFSZ];
        const char *names;
        int slot, dn, dl, pm, dropped, t, f, k = 0;
        long seq, reward;
        d_level place;
        char tname[64], iname[64];

        if (sscanf(line, "gig %d %ld %255s %255s %d %d %ld %d %d|%n", &slot,
                   &seq, tpl, fixer, &dn, &dl, &reward, &pm, &dropped, &k)
                < 9 || !k || slot < 0 || slot >= GIG_SLOTS)
            return TRUE;
        for (t = 0; t < st.ntpls; t++)
            if (!strcmp(st.tpls[t].id, tpl))
                break;
        f = st_npc_idx(fixer);
        if (t == st.ntpls || f < 0 || pm < LOW_PM || pm >= NUMMONS)
            return TRUE; /* story.lua lost that kind of gig */
        names = line + k;
        copynchars(tname, names, (int) sizeof tname - 1);
        if (strchr(tname, '|'))
            *strchr(tname, '|') = '\0';
        names = strchr(names, '|');
        copynchars(iname, names ? names + 1 : "package",
                   (int) sizeof iname - 1);
        place.dnum = (xint16) dn, place.dlevel = (xint16) dl;
        (void) st_gig_build(slot, seq, t, f, &place, reward, pm, tname, iname,
                            (boolean) dropped);
        return TRUE;
    }
    if (!strncmp(line, "rep ", 4)) {
        if (sscanf(line, "rep %255s %ld", id, &l) == 2
            && (i = st_faction_idx(id)) >= 0)
            st.factions[i].rep = l;
        return TRUE;
    }
    if (!strncmp(line, "npc ", 4)) {
        int angry = 0, moved = 0, k = 0;

        if (sscanf(line, "npc %255s %u %d %d %d %d %n", id, &mid, &a, &b,
                   &angry, &moved, &k) >= 4
            && (i = st_npc_idx(id)) >= 0) {
            struct st_npc *np = &st.npcs[i];

            np->m_id = mid;
            np->status = (xint8) a;
            np->pending = (boolean) b;
            np->angry = (boolean) angry;
            if (moved && k > 0 && strcmp(line + k, "-")) {
                d_level lev;

                if (city_resolve_dest(line + k, &lev)) {
                    st_free(np->home);
                    np->home = dupstr(line + k);
                    np->homelev = lev;
                    np->moved = TRUE;
                }
            }
        }
        return TRUE;
    }
    if (!strncmp(line, "stale ", 6)) {
        if (sscanf(line, "stale %u", &mid) == 1)
            st_stale_add(mid);
        return TRUE;
    }
    if (!strncmp(line, "flag ", 5)) {
        if (sscanf(line, "flag %255s %ld", id, &l) == 2)
            st_flag_set(id, l);
        return TRUE;
    }
    if (!strncmp(line, "track ", 6)) {
        if (sscanf(line, "track %255s", id) == 1)
            st.tracked = st_quest_idx(id);
        return TRUE;
    }
    if (!strncmp(line, "log ", 4)) {
        const char *p = line + 4, *text;
        int n = 0;

        if (sscanf(p, "%255s %ld %n", id, &l, &n) >= 2 && n > 0) {
            text = p + n;
            st_log_add(st_quest_idx(id), l, text);
        }
        return TRUE;
    }
    return FALSE;
}

/*
 * Wizard mode
 */

/* #wizstory: "list", "start <quest>", "next <quest>", "finish <quest>",
   "fail <quest>", "flag <name> <n>", "rep <faction> <n>", "track <quest>",
   "spawn <npc>", "goto <npc>", "kill <npc>", "item <id>" */
int
wiz_story(void)
{
    char buf[BUFSZ], cmd[BUFSZ], arg[BUFSZ];
    long num = 0;
    int q, i, n;
    winid win;

    if (!wizard)
        return ECMD_OK;
    if (!st.loaded) {
        pline("No story loaded.");
        return ECMD_OK;
    }
    getlin("Story command? (list, start/next/finish/fail/track <quest>, "
           "flag/rep <name> <n>, spawn/goto/kill <npc>, item/fetch <id>, tp <x> <y>, "
           "gig <kind>, ending <id>, tostair <dungeon>, mons)",
           buf);
    if (*buf == '\033' || !*buf)
        return ECMD_OK;
    cmd[0] = arg[0] = '\0';
    n = sscanf(buf, "%255s %255s %ld", cmd, arg, &num);
    if (n < 1 || !strcmp(cmd, "list")) {
        struct st_flag *fp;

        win = create_nhwindow(NHW_TEXT);
        for (q = 0; q < st.nquests; q++) {
            struct st_quest *qp = &st.quests[q];
            static const char *const sts[] = { "-", "active", "done",
                                               "failed" };

            if (!*qp->id)
                continue; /* an empty gig slot */
            Sprintf(buf, "%c %-12s %-7s stage %d/%d  %s",
                    q == st.tracked ? '*' : ' ', qp->id, sts[qp->status],
                    qp->stage + 1, qp->nstages, qp->title);
            putstr(win, 0, buf);
        }
        putstr(win, 0, "");
        for (i = 0; i < st.nfactions; i++) {
            Sprintf(buf, "rep %s %ld", st.factions[i].id,
                    st.factions[i].rep);
            putstr(win, 0, buf);
        }
        for (fp = st.flags; fp; fp = fp->next) {
            Sprintf(buf, "flag %s %ld", fp->name, fp->val);
            putstr(win, 0, buf);
        }
        for (i = 0; i < st.nnpcs; i++) {
            static const char *const nst[] = { "unmade", "alive", "dead",
                                               "gone" };

            if (!*st.npcs[i].id)
                continue;
            Snprintf(buf, sizeof buf, "npc %-10s %-6s m_id %u%s%s%s%s",
                     st.npcs[i].id, nst[st.npcs[i].status], st.npcs[i].m_id,
                     st.npcs[i].pending ? " (pending)" : "",
                     st.npcs[i].angry ? " (angry)" : "",
                     st.npcs[i].moved ? " now at " : "",
                     st.npcs[i].moved ? st.npcs[i].home : "");
            putstr(win, 0, buf);
        }
        city_respawn_info(buf);
        putstr(win, 0, buf);
        display_nhwindow(win, TRUE);
        destroy_nhwindow(win);
        return ECMD_OK;
    }
    if (!strcmp(cmd, "tp")) { /* "tp <x> <y>": level coordinates */
        int tx = atoi(arg), ty = (int) num;

        if (n < 3 || !isok(tx, ty) || !goodpos(tx, ty, &gy.youmonst, 0)) {
            pline("Can't go to <%d,%d>.", tx, ty);
            return ECMD_OK;
        }
        teleds(tx, ty, TELEDS_TELEPORT);
        return ECMD_OK;
    }
    if (!strcmp(cmd, "gig")) { /* "gig <template>": from the first fixer */
        struct offer_tmp {
            d_level place;
            int pm, gend;
        } o;
        int t, f, k;
        char spec[BUFSZ];

        for (t = 0; t < st.ntpls; t++)
            if (!strcmp(st.tpls[t].id, arg))
                break;
        /* a fixer the template allows */
        for (f = 0; t < st.ntpls && f < st.nnpcs; f++) {
            if (!st.npcs[f].fixer)
                continue;
            for (k = 0; k < st.tpls[t].nfixers; k++)
                if (!strcmp(st.tpls[t].fixers[k], st.npcs[f].id))
                    break;
            if (!st.tpls[t].nfixers || k < st.tpls[t].nfixers)
                break;
        }
        if (t == st.ntpls || f == st.nnpcs || (k = st_gig_free_slot()) < 0
            || !st_gig_place(&st.tpls[t], &o.place)) {
            pline("Can't make a \"%s\" gig.", arg);
            return ECMD_OK;
        }
        o.gend = 0;
        o.pm = st.tpls[t].nmonsters
                   ? name_to_mon(st.tpls[t].monsters[0], &o.gend) : PM_HUMAN;
        if (o.pm < LOW_PM)
            o.pm = PM_HUMAN;
        q = st_gig_build(k, ++st.gigseq, t, f, &o.place,
                         st.tpls[t].reward + 10L * city_difficulty(&o.place),
                         o.pm,
                         st.tpls[t].nnames ? st.tpls[t].names[0] : "a contact",
                         st.tpls[t].ninames ? st.tpls[t].inames[0] : "package",
                         FALSE);
        st_start_quest(q, TRUE);
        Sprintf(spec, "%s:%d", svd.dungeons[o.place.dnum].dname,
                o.place.dlevel);
        pline("%s at %s.", st.quests[q].id, spec); /* for ^V */
        return ECMD_OK;
    }
    if (!strcmp(cmd, "fetch")) { /* "fetch <item>": stand on it */
        struct obj *otmp;

        for (otmp = fobj; otmp; otmp = otmp->nobj)
            if (otmp->otyp == STORY_ITEM && has_oname(otmp)
                && !strcmp(ONAME(otmp), arg))
                break;
        if (!otmp) {
            pline("No %s on this level.", arg);
            return ECMD_OK;
        }
        teleds(otmp->ox, otmp->oy, TELEDS_TELEPORT);
        return ECMD_OK;
    }
    if (!strcmp(cmd, "tostair")) { /* "tostair <dungeon>": stand on its stairs */
        const char *dname = buf + strlen("tostair");
        stairway *stway;
        xint16 dn;

        while (*dname == ' ')
            dname++;
        dn = dname_lookup(dname);
        for (stway = gs.stairs; stway; stway = stway->next)
            if (dn >= 0 && stway->tolev.dnum == dn)
                break;
        if (!stway) {
            pline("No stairs to \"%s\" here.", dname);
            return ECMD_OK;
        }
        teleds(stway->sx, stway->sy, TELEDS_TELEPORT);
        return ECMD_OK;
    }
    if (!strcmp(cmd, "mons")) { /* every monster on the level */
        struct monst *mtmp;

        win = create_nhwindow(NHW_TEXT);
        for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
            if (DEADMONSTER(mtmp))
                continue;
            Sprintf(buf, "<%d,%d> m_id %u %s%s%s", mtmp->mx, mtmp->my,
                    mtmp->m_id, noit_mon_nam(mtmp),
                    mtmp->isshk ? " (shopkeeper)" : "",
                    m_at(mtmp->mx, mtmp->my) == mtmp ? "" : " (NOT on map)");
            putstr(win, 0, buf);
        }
        Sprintf(buf, "you <%d,%d>", u.ux, u.uy);
        putstr(win, 0, buf);
        display_nhwindow(win, TRUE);
        destroy_nhwindow(win);
        return ECMD_OK;
    }
    if (!strcmp(cmd, "ending")) {
        if ((i = st_ending_idx(arg)) < 0) {
            pline("No ending \"%s\".", arg);
            return ECMD_OK;
        }
        st_ending(i);
        return ECMD_OK;
    }
    q = st_quest_idx(arg);
    if (!strcmp(cmd, "start") && q >= 0) {
        st_start_quest(q, TRUE);
    } else if (!strcmp(cmd, "next") && q >= 0) {
        if (st.quests[q].status == Q_INACTIVE)
            st_start_quest(q, TRUE);
        else
            st_finish_stage(q);
    } else if (!strcmp(cmd, "finish") && q >= 0) {
        st_end_quest(q, Q_DONE, (char *) 0);
    } else if (!strcmp(cmd, "fail") && q >= 0) {
        st_end_quest(q, Q_FAILED, (char *) 0);
    } else if (!strcmp(cmd, "track") && q >= 0) {
        st.tracked = q;
    } else if (!strcmp(cmd, "flag") && *arg) {
        st_flag_set(arg, n >= 3 ? num : 1L);
    } else if (!strcmp(cmd, "rep") && (i = st_faction_idx(arg)) >= 0) {
        st_rep_add(i, num - st.factions[i].rep, FALSE);
    } else if (!strcmp(cmd, "spawn") && (i = st_npc_idx(arg)) >= 0) {
        st_spawn(&st.npcs[i], TRUE);
    } else if (!strcmp(cmd, "item") && st_item_idx(arg) >= 0) {
        st_give_item(arg);
    } else if (!strcmp(cmd, "kill") && (i = st_npc_idx(arg)) >= 0) {
        struct monst *mtmp = st_npc_mon(&st.npcs[i]);

        if (!mtmp) {
            pline("%s isn't on this level.", st.npcs[i].name);
            return ECMD_OK;
        }
        xkilled(mtmp, XKILL_GIVEMSG); /* as if you had done it */
    } else if (!strcmp(cmd, "goto") && (i = st_npc_idx(arg)) >= 0) {
        struct monst *mtmp = st_npc_mon(&st.npcs[i]);
        coord cc;

        if (!mtmp) {
            pline("%s isn't on this level.", st.npcs[i].name);
            return ECMD_OK;
        }
        {
            /* prefer standing south of the NPC, then the other sides;
               say which way to #chat, for scripted tests */
            static const int dxy[8][2] = { { 0, 1 }, { 0, -1 }, { 1, 0 },
                                           { -1, 0 }, { 1, 1 }, { -1, 1 },
                                           { 1, -1 }, { -1, -1 } };
            int k;

            for (k = 0; k < 8; k++) {
                cc.x = mtmp->mx + dxy[k][0], cc.y = mtmp->my + dxy[k][1];
                if (isok(cc.x, cc.y) && goodpos(cc.x, cc.y, &gy.youmonst, 0)
                    && !m_at(cc.x, cc.y))
                    break;
            }
            if (k == 8) {
                pline("No room next to %s.", st.npcs[i].name);
                return ECMD_OK;
            }
            teleds(cc.x, cc.y, TELEDS_TELEPORT);
            pline("Chat direction: %c.",
                  xytodir(mtmp->mx - u.ux, mtmp->my - u.uy) >= 0
                      ? gc.Cmd.dirchars[xytodir(mtmp->mx - u.ux,
                                              mtmp->my - u.uy)]
                      : '.');
        }
    } else {
        pline("Story command not understood.");
        return ECMD_OK;
    }
    st_recheck();
    pline("Done.");
    return ECMD_OK;
}

/* check that every id story.lua mentions exists; returns problem count */
staticfn int
st_check_kvl(struct st_kvl *l, const char *where, winid win)
{
    int i, bad = 0;
    char buf[BUFSZ * 3];
    d_level lev;

    for (i = 0; i < l->n; i++) {
        struct st_kv *kv = &l->kv[i];
        const char *s = kv->str ? kv->str : "";
        boolean ok = TRUE;

        if (!strcmp(kv->key, "start") || !strcmp(kv->key, "finish")
            || !strcmp(kv->key, "fail") || !strcmp(kv->key, "track")
            || !strcmp(kv->key, "advance") || !strcmp(kv->key, "active")
            || !strcmp(kv->key, "done") || !strcmp(kv->key, "notdone"))
            ok = st_quest_idx(s) >= 0;
        else if (!strcmp(kv->key, "cases") || !strcmp(kv->key, "when")) {
            int c;

            for (c = 0; c < kv->ncases; c++) {
                Snprintf(buf, sizeof buf, "%s, %s %d", where, kv->key, c + 1);
                bad += st_check_kvl(&kv->cases[c].need, buf, win);
                bad += st_check_kvl(&kv->cases[c].eff, buf, win);
            }
            continue;
        } else if (!strcmp(kv->key, "spawn") || !strcmp(kv->key, "despawn")
                 || !strcmp(kv->key, "call") || !strcmp(kv->key, "anger")
                 || !strcmp(kv->key, "alive") || !strcmp(kv->key, "dead"))
            ok = st_npc_idx(s) >= 0;
        else if (!strcmp(kv->key, "move"))
            ok = st_npc_idx(s) >= 0 && kv->val
                 && city_resolve_dest(kv->val, &lev);
        else if (!strcmp(kv->key, "allies")) {
            int gend = 0;

            ok = name_to_mon(s, &gend) >= LOW_PM;
        } else if (!strcmp(kv->key, "ending"))
            ok = st_ending_idx(s) >= 0;
        else if (!strcmp(kv->key, "worst") || !strcmp(kv->key, "best"))
            ok = st_faction_idx(s) >= 0;
        else if (!strcmp(kv->key, "item") || !strcmp(kv->key, "take"))
            ok = st_item_idx(s) >= 0;
        else if (!strcmp(kv->key, "rep") || !strcmp(kv->key, "norep"))
            ok = st_faction_idx(s) >= 0;
        else if (!strcmp(kv->key, "unlock"))
            ok = city_district_num(s) > 0;
        else if (!strcmp(kv->key, "object")) {
            ok = TRUE; /* checked when used */
        } else if (strcmp(kv->key, "say") && strcmp(kv->key, "text")
                   && strcmp(kv->key, "log") && strcmp(kv->key, "credits")
                   && strcmp(kv->key, "flag") && strcmp(kv->key, "flags")
                   && strcmp(kv->key, "unflag") && strcmp(kv->key, "xp")
                   && strcmp(kv->key, "noflag") && strcmp(kv->key, "heat")
                   && strcmp(kv->key, "gigs")) {
            Snprintf(buf, sizeof buf, "%s: unknown key \"%s\"", where, kv->key);
            putstr(win, 0, buf);
            bad++;
            continue;
        }
        nhUse(lev);
        if (!ok) {
            Snprintf(buf, sizeof buf, "%s: %s \"%s\" doesn't exist", where, kv->key, s);
            putstr(win, 0, buf);
            bad++;
        }
    }
    return bad;
}

int
wiz_storycheck(void)
{
    winid win;
    int bad = 0, q, j, i, gend;
    char buf[BUFSZ * 3], where[BUFSZ];
    d_level lev;

    if (!wizard)
        return ECMD_OK;
    win = create_nhwindow(NHW_TEXT);
    if (!st.loaded) {
        putstr(win, 0, "story.lua didn't load.");
        bad++;
    }
    for (i = 0; i < st.nstatn; i++) { /* not the gig slots */
        struct st_npc *np = &st.npcs[i];

        if (!np->monster || name_to_mon(np->monster, &gend) < LOW_PM) {
            Snprintf(buf, sizeof buf, "npc %s: unknown monster \"%s\"", np->id,
                    np->monster ? np->monster : "(none)");
            putstr(win, 0, buf), bad++;
        }
        if (np->home && !city_resolve_dest(np->home, &lev)) {
            Snprintf(buf, sizeof buf, "npc %s: unknown home \"%s\"", np->id, np->home);
            putstr(win, 0, buf), bad++;
        }
        if (np->nearnpc && st_npc_idx(np->nearnpc) < 0) {
            Snprintf(buf, sizeof buf, "npc %s: unknown NPC \"%s\" (near)",
                     np->id, np->nearnpc);
            putstr(win, 0, buf), bad++;
        }
        if (np->chat && st_node_idx(np->chat) < 0) {
            Snprintf(buf, sizeof buf, "npc %s: unknown dialogue \"%s\"", np->id,
                    np->chat);
            putstr(win, 0, buf), bad++;
        }
        if (np->faction && st_faction_idx(np->faction) < 0) {
            Snprintf(buf, sizeof buf, "npc %s: unknown faction \"%s\"", np->id,
                    np->faction);
            putstr(win, 0, buf), bad++;
        }
        Snprintf(where, sizeof where, "npc %s died", np->id);
        bad += st_check_kvl(&np->died, where, win);
    }
    for (i = 0; i < st.nlocks; i++)
        if (st.locks[i].lev.dnum < 0 || !st.locks[i].flag) {
            Snprintf(buf, sizeof buf, "lock \"%s\": %s", st.locks[i].key,
                     st.locks[i].flag ? "no such place" : "no flag");
            putstr(win, 0, buf), bad++;
        }
    for (i = 0; i < st.nendings; i++) {
        Snprintf(where, sizeof where, "ending %s stay", st.endings[i].id);
        bad += st_check_kvl(&st.endings[i].stay, where, win);
    }
    for (q = 0; q < st.nstatq; q++) {
        struct st_quest *qp = &st.quests[q];

        for (j = 0; j < q; j++)
            if (!strcmp(st.quests[j].id, qp->id)) {
                Snprintf(buf, sizeof buf, "quest id \"%s\" used twice", qp->id);
                putstr(win, 0, buf), bad++;
            }
        Sprintf(where, "%s done", qp->id);
        bad += st_check_kvl(&qp->done, where, win);
        for (j = 0; j < qp->nstages; j++) {
            struct st_stage *sp = &qp->stages[j];

            Sprintf(where, "%s stage %d", qp->id, j + 1);
            if (sp->where && !city_resolve_dest(sp->where, &lev)) {
                Snprintf(buf, sizeof buf, "%s: unknown place \"%s\"", where, sp->where);
                putstr(win, 0, buf), bad++;
            }
            if (sp->arrive && !city_resolve_dest(sp->arrive, &lev)) {
                Snprintf(buf, sizeof buf, "%s: unknown place \"%s\"", where, sp->arrive);
                putstr(win, 0, buf), bad++;
            }
            if ((sp->talk && st_npc_idx(sp->talk) < 0)
                || (sp->kill && st_npc_idx(sp->kill) < 0)) {
                Snprintf(buf, sizeof buf, "%s: unknown NPC", where);
                putstr(win, 0, buf), bad++;
            }
            if ((sp->give && st_item_idx(sp->give) < 0)
                || (sp->carry && st_item_idx(sp->carry) < 0)) {
                Snprintf(buf, sizeof buf, "%s: unknown item", where);
                putstr(win, 0, buf), bad++;
            }
            if (sp->give && !sp->talk) {
                Snprintf(buf, sizeof buf, "%s: give without talk", where);
                putstr(win, 0, buf), bad++;
            }
            if (sp->dialogue && st_node_idx(sp->dialogue) < 0) {
                Snprintf(buf, sizeof buf, "%s: unknown dialogue \"%s\"", where,
                        sp->dialogue);
                putstr(win, 0, buf), bad++;
            }
            if (!sp->talk && !sp->arrive && !sp->kill && !sp->carry
                && !sp->flag) {
                Snprintf(buf, sizeof buf, "%s: no way to finish it (needs an advance)",
                        where);
                putstr(win, 0, buf); /* a note, not an error */
            }
            bad += st_check_kvl(&sp->start, where, win);
            bad += st_check_kvl(&sp->done, where, win);
        }
    }
    for (i = 0; i < st.nnodes; i++) {
        struct st_node *np = &st.nodes[i];

        for (j = 0; j < np->nchoices; j++) {
            struct st_choice *cp = &np->choices[j];

            Sprintf(where, "dialogue %s choice %d", np->id, j + 1);
            if (cp->go && st_node_idx(cp->go) < 0) {
                Snprintf(buf, sizeof buf, "%s: unknown dialogue \"%s\"", where, cp->go);
                putstr(win, 0, buf), bad++;
            }
            bad += st_check_kvl(&cp->need, where, win);
            bad += st_check_kvl(&cp->eff, where, win);
        }
    }
    for (i = 0; i < st.ntpls; i++) {
        struct st_gigtpl *tp = &st.tpls[i];

        for (j = 0; j < tp->nfixers; j++)
            if (st_npc_idx(tp->fixers[j]) < 0) {
                Snprintf(buf, sizeof buf, "gig %s: unknown fixer \"%s\"",
                         tp->id, tp->fixers[j]);
                putstr(win, 0, buf), bad++;
            }
        for (j = 0; j < tp->nmonsters; j++)
            if (name_to_mon(tp->monsters[j], &gend) < LOW_PM) {
                Snprintf(buf, sizeof buf, "gig %s: unknown monster \"%s\"",
                         tp->id, tp->monsters[j]);
                putstr(win, 0, buf), bad++;
            }
        if (strcmp(tp->place, "district") && strcmp(tp->place, "site")) {
            Snprintf(buf, sizeof buf, "gig %s: place must be district or site",
                     tp->id);
            putstr(win, 0, buf), bad++;
        }
    }
    Snprintf(buf, sizeof buf, "story check: %d quest%s, %d NPC%s, %d dialogue node%s, "
            "%d problem%s.",
            st.nstatq, plur(st.nstatq), st.nstatn, plur(st.nstatn),
            st.nnodes, plur(st.nnodes), bad, plur(bad));
    putstr(win, 0, buf);
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

/*story.c*/
