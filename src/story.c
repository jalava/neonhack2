/* NeonHack 2   story.c   quests, NPCs, dialogue and the journal */
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
   ("rep", "jade", 5); unlock = { "a", "b" } -> ("unlock", "a"), ... */
struct st_kv {
    char *key, *str;
    long num;
};
struct st_kvl {
    int n;
    struct st_kv *kv;
};

struct st_item {
    char *id, *name;
};
struct st_faction {
    char *id, *name;
    long rep; /* state */
};
struct st_npc {
    char *id, *name, *monster, *home, *faction, *chat;
    boolean female, stay, hostile;
    int nidle;
    char **idle;
    d_level homelev;
    /* state */
    unsigned m_id;
    xint8 status;    /* NPC_xxx */
    boolean pending; /* spawn when the hero reaches its home */
};
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

static struct story_data {
    boolean loaded;
    int nquests, nnpcs, nnodes, nitems, nfactions;
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
staticfn void st_read_kvl(lua_State *, struct st_kvl *, const char *const *);
staticfn void st_read_kvl_field(lua_State *, const char *, struct st_kvl *);
staticfn void st_free_kvl(struct st_kvl *);
staticfn void st_read_npcs(lua_State *);
staticfn void st_read_quests(lua_State *);
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

/* flatten the table at the top of the stack, skipping 'skip' keys */
staticfn void
st_read_kvl(lua_State *L, struct st_kvl *out, const char *const *skip)
{
    int cap = 8, i;
    const char *key;

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
        if (lua_type(L, -1) == LUA_TTABLE) {
            /* sub-table: array of strings, or name = number|string */
            lua_pushnil(L);
            while (lua_next(L, -2) != 0) {
                if (out->n >= cap) {
                    struct st_kv *bigger;

                    cap *= 2;
                    bigger = (struct st_kv *) alloc(cap * sizeof *bigger);
                    (void) memcpy(bigger, out->kv, out->n * sizeof *bigger);
                    free((genericptr_t) out->kv);
                    out->kv = bigger;
                }
                out->kv[out->n].key = dupstr(key);
                out->kv[out->n].str = 0;
                out->kv[out->n].num = 0;
                if (lua_type(L, -2) == LUA_TSTRING) {
                    out->kv[out->n].str = dupstr(lua_tostring(L, -2));
                    if (lua_type(L, -1) == LUA_TNUMBER)
                        out->kv[out->n].num = (long) lua_tointeger(L, -1);
                    else if (lua_type(L, -1) == LUA_TBOOLEAN)
                        out->kv[out->n].num = lua_toboolean(L, -1);
                } else if (lua_type(L, -1) == LUA_TSTRING) {
                    out->kv[out->n].str = dupstr(lua_tostring(L, -1));
                }
                out->n++;
                lua_pop(L, 1);
            }
        } else {
            if (out->n >= cap) {
                struct st_kv *bigger;

                cap *= 2;
                bigger = (struct st_kv *) alloc(cap * sizeof *bigger);
                (void) memcpy(bigger, out->kv, out->n * sizeof *bigger);
                free((genericptr_t) out->kv);
                out->kv = bigger;
            }
            out->kv[out->n].key = dupstr(key);
            out->kv[out->n].str = 0;
            out->kv[out->n].num = 0;
            if (lua_type(L, -1) == LUA_TSTRING)
                out->kv[out->n].str = dupstr(lua_tostring(L, -1));
            else if (lua_type(L, -1) == LUA_TNUMBER)
                out->kv[out->n].num = (long) lua_tointeger(L, -1);
            else if (lua_type(L, -1) == LUA_TBOOLEAN)
                out->kv[out->n].num = lua_toboolean(L, -1);
            out->n++;
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
    int i;

    for (i = 0; i < l->n; i++) {
        free((genericptr_t) l->kv[i].key);
        if (l->kv[i].str)
            free((genericptr_t) l->kv[i].str);
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
                st.factions[i].id = dupstr(id);
                st.factions[i].name = name;
            } else {
                st.items[i].id = dupstr(id);
                st.items[i].name = name;
            }
            i++;
        }
        lua_pop(L, 1);
    }
    if (factions)
        st.nfactions = i;
    else
        st.nitems = i;
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
            np->female = get_table_boolean_opt(L, "female", FALSE);
            np->stay = get_table_boolean_opt(L, "stay", FALSE);
            np->hostile = get_table_boolean_opt(L, "hostile", FALSE);
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

/* quests = { { id =, title =, stages = { {...}, ... } }, ... } */
staticfn void
st_read_quests(lua_State *L)
{
    int n, i, j;

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
        struct st_quest *qp = &st.quests[i];

        lua_pushinteger(L, i + 1);
        lua_gettable(L, -2);
        if (lua_type(L, -1) != LUA_TTABLE) {
            impossible("story.lua: quests[%d] is not a table", i + 1);
            lua_pop(L, 1);
            continue;
        }
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
            (void) memset(qp->stages, 0,
                          (qp->nstages + 1) * sizeof *qp->stages);
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
                    sp->dialogue = get_table_str_opt(L, "dialogue",
                                                     (char *) 0);
                    sp->remind = get_table_str_opt(L, "remind", (char *) 0);
                    sp->log = get_table_str_opt(L, "log", (char *) 0);
                    st_read_kvl_field(L, "start", &sp->start);
                    st_read_kvl_field(L, "done", &sp->done);
                } else {
                    impossible("story.lua: %s stage %d is not a table",
                               qp->id, j + 1);
                }
                lua_pop(L, 1);
            }
        } else {
            impossible("story.lua: quest %s has no stages", qp->id);
        }
        lua_pop(L, 1); /* stages */
        lua_pop(L, 1); /* quest */
    }
    st.nquests = n;
    lua_pop(L, 1);
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

    for (i = 0; i < st.nquests; i++) {
        struct st_quest *qp = &st.quests[i];

        for (j = 0; j < qp->nstages; j++) {
            struct st_stage *sp = &qp->stages[j];

            st_free(sp->goal), st_free(sp->where), st_free(sp->talk), st_free(sp->give);
            st_free(sp->carry), st_free(sp->kill), st_free(sp->arrive);
            st_free(sp->flag), st_free(sp->dialogue), st_free(sp->remind);
            st_free(sp->log);
            st_free_kvl(&sp->start), st_free_kvl(&sp->done);
        }
        st_free(qp->stages);
        st_free(qp->id), st_free(qp->title), st_free(qp->kind);
        st_free_kvl(&qp->done);
    }
    st_free(st.quests);
    for (i = 0; i < st.nnpcs; i++) {
        struct st_npc *np = &st.npcs[i];

        st_free(np->id), st_free(np->name), st_free(np->monster), st_free(np->home);
        st_free(np->faction), st_free(np->chat);
        for (j = 0; j < np->nidle; j++)
            st_free(np->idle[j]);
        st_free(np->idle);
    }
    st_free(st.npcs);
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
        st_free(st.factions[i].id), st_free(st.factions[i].name);
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

/*
 * Lookups and small helpers
 */

staticfn int
st_quest_idx(const char *id)
{
    int i;

    for (i = 0; id && i < st.nquests; i++)
        if (!strcmp(st.quests[i].id, id))
            return i;
    return -1;
}

staticfn int
st_npc_idx(const char *id)
{
    int i;

    for (i = 0; id && i < st.nnpcs; i++)
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

    for (i = 0; id && i < st.nitems; i++)
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

    return (i >= 0) ? st.npcs[i].hostile : FALSE;
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
    if (np->hostile) {
        mtmp->mpeaceful = 0;
    } else {
        mtmp->mpeaceful = 1;
        mtmp->mstrategy &= ~STRAT_WAITMASK;
    }
    if (!has_mgivenname(mtmp))
        mtmp = christen_monst(mtmp, np->name);
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
    return (boolean) (np && np->stay);
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
    mtmp = makemon(&mons[pm], 0, 0,
                   MM_NOMSG | (fem ? MM_FEMALE : MM_MALE)
                   | (np->hostile ? 0 : NO_MINVENT));
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
        }
    }
    return TRUE;
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
        "say", "text", "log", "credits", "object", "item", "take", "rep",
        "flag", "flags", "unflag", "spawn", "despawn", "unlock", "advance",
        "fail", "finish", "start", "track", "xp", 0
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
            } else if (!strcmp(kv->key, "rep")) {
                if ((x = st_faction_idx(s)) >= 0) {
                    st.factions[x].rep += kv->num;
                    if (st.factions[x].rep > 100)
                        st.factions[x].rep = 100;
                    if (st.factions[x].rep < -100)
                        st.factions[x].rep = -100;
                    if (kv->num)
                        pline("%s will remember that.",
                              upstart(strcpy(buf, st.factions[x].name)));
                } else {
                    impossible("story: unknown faction \"%s\"", s);
                }
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
    /* nothing urgent: small talk */
    if (np->chat && st_node_idx(np->chat) >= 0) {
        (void) st_dialogue(np, np->chat);
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
            return 1; /* just text: read and done */
        if (k == 0 && nshown == 1)
            chosen = only; /* Enter with the one answer deselected */
        if (k == 0 && chosen < 0)
            continue; /* Enter without an answer: ask again */
        if (chosen < 0) {
            You("walk away from %s.", np->name);
            return 0;
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
        if (st.quests[q].status == Q_ACTIVE && q != st.tracked) {
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
    for (i = 0; i < st.nquests && n < max; i++) {
        struct st_quest *qp = &st.quests[i];

        if (qp->status == Q_INACTIVE)
            continue;
        Sprintf(buf, "quest %s %d %d", qp->id, qp->status, qp->stage);
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nfactions && n < max; i++) {
        Sprintf(buf, "rep %s %ld", st.factions[i].id, st.factions[i].rep);
        lines[n++] = dupstr(buf);
    }
    for (i = 0; i < st.nnpcs && n < max; i++) {
        struct st_npc *np = &st.npcs[i];

        if (np->status == NPC_UNMADE && !np->pending)
            continue;
        Sprintf(buf, "npc %s %u %d %d", np->id, np->m_id, np->status,
                np->pending);
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
        if (sscanf(line, "quest %255s %d %d", id, &a, &b) == 3
            && (i = st_quest_idx(id)) >= 0) {
            st.quests[i].status = (xint8) a;
            st.quests[i].stage = b;
        }
        return TRUE;
    }
    if (!strncmp(line, "rep ", 4)) {
        if (sscanf(line, "rep %255s %ld", id, &l) == 2
            && (i = st_faction_idx(id)) >= 0)
            st.factions[i].rep = l;
        return TRUE;
    }
    if (!strncmp(line, "npc ", 4)) {
        if (sscanf(line, "npc %255s %u %d %d", id, &mid, &a, &b) == 4
            && (i = st_npc_idx(id)) >= 0) {
            st.npcs[i].m_id = mid;
            st.npcs[i].status = (xint8) a;
            st.npcs[i].pending = (boolean) b;
        }
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
           "flag/rep <name> <n>, spawn/goto/kill <npc>, item <id>)", buf);
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

            Sprintf(buf, "npc %-10s %-6s m_id %u%s", st.npcs[i].id,
                    nst[st.npcs[i].status], st.npcs[i].m_id,
                    st.npcs[i].pending ? " (pending)" : "");
            putstr(win, 0, buf);
        }
        city_respawn_info(buf);
        putstr(win, 0, buf);
        display_nhwindow(win, TRUE);
        destroy_nhwindow(win);
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
        st.factions[i].rep = num;
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
        else if (!strcmp(kv->key, "spawn") || !strcmp(kv->key, "despawn")
                 || !strcmp(kv->key, "call"))
            ok = st_npc_idx(s) >= 0;
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
                   && strcmp(kv->key, "noflag")) {
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
    for (i = 0; i < st.nnpcs; i++) {
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
    }
    for (q = 0; q < st.nquests; q++) {
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
    Snprintf(buf, sizeof buf, "story check: %d quest%s, %d NPC%s, %d dialogue node%s, "
            "%d problem%s.",
            st.nquests, plur(st.nquests), st.nnpcs, plur(st.nnpcs),
            st.nnodes, plur(st.nnodes), bad, plur(bad));
    putstr(win, 0, buf);
    display_nhwindow(win, TRUE);
    destroy_nhwindow(win);
    return ECMD_OK;
}

/*story.c*/
