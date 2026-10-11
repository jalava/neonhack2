/* NeonHack neon.c  -- cyberpunk mechanics layered on NetHack 5.0 */
/* Copyright (c) 2026 NeonHack contributors. */
/* NetHack may be freely redistributed.  See license for details. */

/*
 * This file collects NeonHack's own game mechanics so that the changes
 * to the NetHack sources stay small hooks:
 *
 *   firearms   guns are NetHack launchers; this adds burst fire, noise
 *              and spent ammunition.
 *   grenades   thrown tools that go off where they land.
 *   cyberware  implants grant permanent intrinsics (as FROMCYBER bits),
 *              at the price of cyber load and, past your capacity,
 *              cyberpsychosis; EMPs and shocks short them out.
 *   hacking    a cyberdeck takes over robots and opens electronic locks.
 *   heat       crimes draw the attention of corporate security, which
 *              sends armed squads once you're wanted.
 */

#include "hack.h"

staticfn boolean utf8_locale(void);
staticfn int gunfire_range(struct obj *);
staticfn const char *gunfire_sound(struct obj *, int);
staticfn void flashbang(coordxy, coordxy);
staticfn const struct implant_info *implant_by_otyp(int);
staticfn void implant_props(int, boolean);
staticfn void cyber_glitch(void);
staticfn void cyberpsycho_episode(void);
staticfn int hack_chance(struct obj *, int);
staticfn boolean hack_feedback(struct obj *);
staticfn int hack_robot(struct obj *, struct monst *);
staticfn int hack_lock(struct obj *, coordxy, coordxy);
staticfn boolean squad_allowed_here(void);
staticfn void send_squad(void);

/* ------------------------------------------------------------------ */
/*  Emoji                                                             */
/* ------------------------------------------------------------------ */

/* does the terminal's locale look like it can show UTF-8? */
staticfn boolean
utf8_locale(void)
{
    static int is_utf8 = -1;

    if (is_utf8 < 0) {
        const char *loc = nh_getenv("LC_ALL");

        if (!loc || !*loc)
            loc = nh_getenv("LC_CTYPE");
        if (!loc || !*loc)
            loc = nh_getenv("LANG");
        is_utf8 = (loc && (strstri(loc, "UTF-8") || strstri(loc, "utf8")));
    }
    return (boolean) is_utf8;
}

/* can the terminal show NeonGrid's characters and emoji?  a UTF-8 locale,
   and not the Linux console, whose fonts have few of them */
boolean
neon_utf8_terminal(void)
{
    const char *term = nh_getenv("TERM");

    return (boolean) (utf8_locale() && !(term && !strcmp(term, "linux")));
}

/* 'e' (an emoji followed by a space) if the emoji option is on and the
   terminal can show it, otherwise "" */
const char *
neon_emoji(const char *e)
{
    if (!iflags.emoji || !neon_utf8_terminal()
        || !WINDOWPORT(tty) || iflags.debug_fuzzer)
        return "";
    return e;
}

/* ------------------------------------------------------------------ */
/*  Firearms                                                          */
/* ------------------------------------------------------------------ */

/* is obj a gun (a launcher using the firearms or shotgun skill)? */
boolean
is_firearm(struct obj *obj)
{
    return (obj && obj->oclass == WEAPON_CLASS && is_launcher(obj)
            && (objects[obj->otyp].oc_skill == P_BOW
                || objects[obj->otyp].oc_skill == P_CROSSBOW));
}

/* bullets and shotgun shells are spent when they hit something */
boolean
is_cartridge(struct obj *obj)
{
    return (obj && is_ammo(obj)
            && (objects[obj->otyp].oc_skill == -P_BOW
                || objects[obj->otyp].oc_skill == -P_CROSSBOW));
}

/* extra rounds in a volley from automatic weapons */
int
firearm_burst(struct obj *launcher)
{
    if (!launcher)
        return 0;
    switch (launcher->otyp) {
    case SUBMACHINE_GUN:
        return rnd(2);
    case ASSAULT_RIFLE:
        return 1;
    default:
        return 0;
    }
}

/* how far gunfire carries (squared distance, for wake_nearto()) */
staticfn int
gunfire_range(struct obj *launcher)
{
    switch (launcher->otyp) {
    case ASSAULT_RIFLE:
        return 15 * 15;
    case CROSSBOW: /* shotgun */
        return 13 * 13;
    case SUBMACHINE_GUN:
        return 12 * 12;
    case ORCISH_BOW: /* zip gun */
        return 8 * 8;
    case ELVEN_BOW: /* smart pistol: integral suppressor */
        return 5 * 5;
    default:
        return 10 * 10;
    }
}

staticfn const char *
gunfire_sound(struct obj *launcher, int shots)
{
    if (objects[launcher->otyp].oc_skill == P_CROSSBOW)
        return (shots > 1) ? "BOOM!  BOOM!" : "BOOM!";
    if (launcher->otyp == ELVEN_BOW)
        return (shots > 1) ? "Pfft-pfft!" : "Pfft!";
    if (shots >= 3)
        return "Rat-a-tat-tat!";
    return (shots == 2) ? "BANG!  BANG!" : "BANG!";
}

/* the hero just fired 'shots' rounds from 'launcher' */
void
u_gunfire(struct obj *launcher, int shots)
{
    if (!is_firearm(launcher))
        return;
    if (!Deaf)
        pline("%s%s", neon_emoji("\U0001F4A5 "), gunfire_sound(launcher, shots));
    wake_nearto(u.ux, u.uy, gunfire_range(launcher));
}

/* monster 'mtmp' just fired 'shots' rounds from 'launcher' */
void
m_gunfire(struct monst *mtmp, struct obj *launcher, int shots)
{
    if (!is_firearm(launcher))
        return;
    if (!Deaf) {
        if (canseemon(mtmp))
            pline("%s%s", neon_emoji("\U0001F4A5 "),
                  gunfire_sound(launcher, shots));
        else
            You_hear("gunfire%s.",
                     (mdistu(mtmp) > 15 * 15) ? " in the distance" : "");
    }
    wake_nearto(mtmp->mx, mtmp->my, gunfire_range(launcher));
}

/* extra damage for a round fired by the hero from 'launcher' at 'mon' */
int
firearm_dmg_bonus(struct obj *launcher, struct monst *mon)
{
    if (!is_firearm(launcher))
        return 0;
    switch (launcher->otyp) {
    case CROSSBOW: /* shotgun: the spread hurts most up close */
        return (distmin(u.ux, u.uy, mon->mx, mon->my) <= 2) ? rnd(6) : 0;
    case ASSAULT_RIFLE:
    case YUMI: /* revolver */
        return 2;
    case BOW: /* pistol */
    case ELVEN_BOW: /* smart pistol */
        return 1;
    default: /* zip gun, submachine gun */
        return 0;
    }
}

/* the hero hit 'mon' with a stun baton; returns extra damage */
int
stun_baton_hit(struct monst *mon)
{
    if (resists_elec(mon)) {
        if (canseemon(mon))
            pline("%s doesn't seem to notice the shock.", Monnam(mon));
        return 0;
    }
    if (!rn2(3) && !mon->mstun) {
        mon->mstun = 1;
        if (canseemon(mon))
            pline("%s is jolted!", Monnam(mon));
    }
    return rnd(6);
}

/* missed with a monofilament whip; unskilled users may cut themselves */
void
monowhip_backlash(void)
{
    if (P_SKILL(P_WHIP) >= P_BASIC || rn2(Fumbling ? 3 : 8))
        return;
    You("lose control of the monofilament!");
    losehp(Maybe_Half_Phys(d(2, 6)), "own monofilament whip", KILLED_BY_AN);
}

/* ------------------------------------------------------------------ */
/*  Grenades                                                          */
/* ------------------------------------------------------------------ */

boolean
is_grenade(struct obj *obj)
{
    return (obj && (obj->otyp == FRAG_GRENADE || obj->otyp == FLASHBANG
                    || obj->otyp == EMP_GRENADE));
}

/* a grenade of type 'otyp' went off at <x,y>; the object is already gone;
   the hero threw (or kicked, or dropped) it */
void
grenade_explode(int otyp, coordxy x, coordxy y)
{
    switch (otyp) {
    case FRAG_GRENADE:
        Strcpy(svk.killer.name, "frag grenade explosion");
        svk.killer.format = KILLED_BY_AN;
        explode(x, y, PHYS_EXPL_TYPE, d(4, 6), MON_EXPLODE, EXPL_FIERY);
        break;
    case FLASHBANG:
        flashbang(x, y);
        break;
    case EMP_GRENADE:
        emp_burst(x, y, 3, TRUE);
        break;
    default:
        impossible("grenade_explode: not a grenade (%d)", otyp);
        break;
    }
}

/* blinding flash and deafening bang within three squares of <x,y> */
staticfn void
flashbang(coordxy x, coordxy y)
{
    struct monst *mtmp, *nmon;
    boolean u_near = (dist2(u.ux, u.uy, x, y) <= 3 * 3 && couldsee(x, y));

    if (cansee(x, y) || u_near)
        pline("%sThere is a blinding flash and a deafening bang!",
              neon_emoji("\u2728 "));
    else if (!Deaf)
        You_hear("a loud bang!");

    if (u_near) {
        if (!resists_blnd(&gy.youmonst)) {
            You("are blinded by the flash!");
            make_blinded(BlindedTimeout + (long) rn1(10, 10), FALSE);
        }
        if (!Deaf) {
            Your("ears are ringing.");
            incr_itimeout(&HDeaf, rn1(20, 10));
            disp.botl = TRUE;
        }
        make_stunned((HStun & TIMEOUT) + (long) rnd(4), TRUE);
    }
    for (mtmp = fmon; mtmp; mtmp = nmon) {
        nmon = mtmp->nmon;
        if (DEADMONSTER(mtmp)
            || dist2(mtmp->mx, mtmp->my, x, y) > 3 * 3
            || !clear_path(x, y, mtmp->mx, mtmp->my))
            continue;
        if (haseyes(mtmp->data) && !resists_blnd(mtmp)) {
            mtmp->mcansee = 0;
            mtmp->mblinded = rn1(10, 10);
        }
        if (!mindless(mtmp->data))
            mtmp->mstun = 1;
        wakeup(mtmp, TRUE); /* angers peacefuls; it was an attack */
    }
    wake_nearto(x, y, 20 * 20);
}

/* electromagnetic pulse within 'radius' of <x,y>: shuts down robots,
   drains the hero's energy and shorts out cyberware */
void
emp_burst(coordxy x, coordxy y, int radius, boolean by_hero)
{
    struct monst *mtmp, *nmon;
    int r2 = radius * radius;

    if (cansee(x, y))
        pline("%sBlue sparks crackle through the air!", neon_emoji("\u26A1 "));
    else if (!Deaf)
        You_hear("a sharp electrical crack.");

    if (dist2(u.ux, u.uy, x, y) <= r2) {
        if (u.uen > 0) {
            You_feel("your power drain away!");
            u.uen -= min(u.uen, d(4, 6));
            disp.botl = TRUE;
        }
        cyber_short((long) rn1(30, 20));
        if (Upolyd && is_robot(gy.youmonst.data)) {
            You("shut down!");
            nomul(-rn1(5, 5));
            gm.multi_reason = "shut down by an EMP";
            gn.nomovemsg = "You reboot.";
        }
    }
    for (mtmp = fmon; mtmp; mtmp = nmon) {
        nmon = mtmp->nmon;
        if (DEADMONSTER(mtmp) || !is_robot(mtmp->data)
            || dist2(mtmp->mx, mtmp->my, x, y) > r2)
            continue;
        if (canseemon(mtmp))
            pline("%s shudders and powers down!", Monnam(mtmp));
        mtmp->mhp -= d(3, 6);
        if (DEADMONSTER(mtmp)) {
            if (by_hero)
                xkilled(mtmp, XKILL_GIVEMSG);
            else
                monkilled(mtmp, "", AD_ELEC);
            continue;
        }
        paralyze_monst(mtmp, rn1(10, 5));
        if (by_hero)
            setmangry(mtmp, TRUE);
    }
}

/* ------------------------------------------------------------------ */
/*  Cyberware                                                         */
/* ------------------------------------------------------------------ */

static const struct implant_info {
    short otyp;
    schar load;          /* strain on the psyche */
    short prop1, prop2;  /* intrinsics granted (0 if none) */
    const char *feel;    /* message when it comes online */
    const char *what;    /* enlightenment description */
} implant_list[] = {
    { OPTIC_IMPLANT, 2, INFRAVISION, SEE_INVIS,
      "Your vision snaps into razor-sharp focus.",
      "thermal and spectral optics" },
    { REFLEX_BOOSTER, 3, FAST, 0,
      "The world seems to slow down around you.", "boosted reflexes" },
    { DERMAL_PLATE, 2, 0, 0,
      "Your skin hardens into flexible armor.", "dermal armor plating" },
    { NEURAL_FIREWALL, 2, SLEEP_RES, 0,
      "Your thoughts feel walled off and secure.", "a neural firewall" },
    { TOXIN_FILTER, 1, POISON_RES, 0,
      "Your blood feels clean.", "a toxin filter" },
    { GROUNDING_MESH, 1, SHOCK_RES, 0,
      "A faint static tingle drains away into the floor.",
      "a grounding mesh" },
    { HEAT_SINK, 1, FIRE_RES, 0,
      "You feel pleasantly cool.", "an internal heat sink" },
    { CRYO_LINING, 1, COLD_RES, 0,
      "You feel pleasantly warm.", "cryo-insulated lining" },
    { THREAT_SENSOR, 1, WARNING, 0,
      "Your skin prickles with awareness.", "a threat sensor" },
    { NEURAL_UPLINK, 3, TELEPAT, 0,
      "Distant minds hum at the edge of your thoughts.",
      "a neural uplink" },
    { MUSCLE_GRAFT, 2, 0, 0,
      "Your muscles bulge with synthetic fibre!", "muscle grafts" },
    { SMARTLINK, 1, 0, 0,
      "A targeting reticle flickers into your vision.", "a smartlink" },
    { NANITE_REGENERATOR, 2, REGENERATION, 0,
      "Nanites swarm through your bloodstream.", "a nanite regenerator" },
    { SOUND_DAMPENER, 1, STEALTH, 0,
      "Your footsteps fall silent.", "a sound dampener" },
    { REBREATHER, 1, MAGICAL_BREATHING, 0,
      "You no longer need to breathe deeply.", "a rebreather" },
    { MANTIS_BLADES, 2, 0, 0,
      "Blades slide out of your forearms, then back in.",
      "retractable mantis blades" },
};

#define implant_bit(otyp) (1UL << ((otyp) - FIRST_IMPLANT))

staticfn const struct implant_info *
implant_by_otyp(int otyp)
{
    int i;

    for (i = 0; i < SIZE(implant_list); i++)
        if (implant_list[i].otyp == otyp)
            return &implant_list[i];
    return (const struct implant_info *) 0;
}

boolean
is_implant(struct obj *obj)
{
    return (obj && obj->otyp >= FIRST_IMPLANT && obj->otyp <= LAST_IMPLANT);
}

/* is the implant 'otyp' installed (regardless of being shorted out)? */
boolean
has_implant(int otyp)
{
    return (u.ucyber & implant_bit(otyp)) != 0;
}

/* is the implant 'otyp' installed and currently working? */
boolean
cyber_active(int otyp)
{
    return has_implant(otyp) && !u.ucybershort;
}

/* switch the FROMCYBER intrinsics of an installed implant on or off */
staticfn void
implant_props(int otyp, boolean on)
{
    const struct implant_info *ii = implant_by_otyp(otyp);
    short props[2];
    int i;

    if (!ii)
        return;
    props[0] = ii->prop1, props[1] = ii->prop2;
    for (i = 0; i < 2; i++) {
        if (!props[i])
            continue;
        if (on)
            u.uprops[props[i]].intrinsic |= FROMCYBER;
        else
            u.uprops[props[i]].intrinsic &= ~FROMCYBER;
    }
    if (otyp == OPTIC_IMPLANT || otyp == NEURAL_UPLINK
        || otyp == THREAT_SENSOR) {
        if (otyp == OPTIC_IMPLANT) {
            set_mimic_blocking();
            gv.vision_full_recalc = 1;
        }
        see_monsters();
    }
    if (otyp == DERMAL_PLATE)
        find_ac();
    disp.botl = TRUE;
}

/* total cyber load of installed implants */
int
cyber_load(void)
{
    int i, load = 0;

    for (i = 0; i < SIZE(implant_list); i++)
        if (has_implant(implant_list[i].otyp))
            load += implant_list[i].load;
    return load;
}

/* how much cyberware the hero's psyche can take */
int
cyber_capacity(void)
{
    int cap = 3 + ACURR(A_CON) / 4 + ACURR(A_WIS) / 6 + u.ulevel / 6;

    if (Role_if(PM_HEALER)) /* ripperdocs know their limits */
        cap += 2;
    return cap;
}

/* is the hero currently suffering from cyberpsychosis? */
boolean
cyberpsychotic(void)
{
    return (u.ucyber && cyber_load() > cyber_capacity() && !u.uneuroblock);
}

/* install an implant (applying it); obj is used up unless refused */
int
use_implant(struct obj *obj)
{
    const struct implant_info *ii = implant_by_otyp(obj->otyp);
    struct obj *kit = (struct obj *) 0, *otmp;
    boolean expert = Role_if(PM_HEALER), wasover;
    int otyp = obj->otyp, dmg;
    char qbuf[QBUFSZ];

    if (!ii) {
        impossible("use_implant: not an implant (%d)", obj->otyp);
        return ECMD_OK;
    }
    if (has_implant(otyp)) {
        /* only the type can be known for sure if it's already in you */
        You("already have %s installed.",
            an(OBJ_NAME(objects[otyp])));
        if (!objects[otyp].oc_name_known) {
            makeknown(otyp);
            update_inventory();
        }
        return ECMD_OK;
    }
    if (Upolyd && !humanoid(gy.youmonst.data)) {
        You("can't find anywhere to install %s in this form.",
            the(xname(obj)));
        return ECMD_OK;
    }
    for (otmp = gi.invent; otmp; otmp = otmp->nobj)
        if (otmp->otyp == MEDKIT && otmp->spe > 0) {
            kit = otmp;
            break;
        }
    Snprintf(qbuf, sizeof qbuf, "Perform surgery on yourself%s?",
             kit ? " (using your medkit)" : "");
    if (y_n(qbuf) != 'y')
        return ECMD_OK;

    wasover = cyberpsychotic();
    if (kit)
        consume_obj_charge(kit, TRUE);
    if (expert) {
        You("deftly install %s.", the(xname(obj)));
        dmg = rnd(3);
    } else {
        You("grit your teeth and cut %s into yourself.", the(xname(obj)));
        dmg = obj->blessed ? d(2, 4) : d(3, 6);
    }
    if (kit)
        dmg = (dmg + 1) / 2;
    losehp(Maybe_Half_Phys(dmg), "botched self-surgery", KILLED_BY);
    if (u.uhp < 1) /* shouldn't happen; losehp() handles death */
        return ECMD_TIME;

    /* rejection: a careless install may be torn out by the body */
    if (!expert && !kit && !obj->blessed && rn2(20) >= ACURR(A_CON)) {
        pline("Your body rejects %s!", the(xname(obj)));
        make_vomiting((long) rn1(10, 10), TRUE);
        useup(obj);
        return ECMD_TIME;
    }

    u.ucyber |= implant_bit(otyp);
    if (obj->cursed)
        u.ucyberglitch |= implant_bit(otyp);
    u.uconduct.cyberware++;
    if (!objects[otyp].oc_name_known)
        makeknown(otyp);
    useup(obj);
    obj = 0;

    if (!u.ucybershort) {
        implant_props(otyp, TRUE);
        pline("%s%s", neon_emoji("\U0001F9BE "), ii->feel);
    } else {
        pline("It will come online once your cyberware recovers.");
    }
    if (otyp == MUSCLE_GRAFT)
        (void) adjattrib(A_STR, rn1(2, 2), 0); /* +2..3, with message */
    if (u.ucyberglitch & implant_bit(otyp))
        pline("Something about the installation doesn't feel right...");
    livelog_printf(LL_MINORAC, "installed %s", an(OBJ_NAME(objects[otyp])));

    if (!wasover && cyberpsychotic())
        You_feel("a cold, distant detachment from your own %s.",
                 body_part(BLOOD));
    else if (cyber_load() > cyber_capacity() - 2 && !cyberpsychotic())
        You_feel("strangely detached from your body.");
    disp.botl = TRUE;
    return ECMD_TIME;
}

/* apply a medkit: patch yourself up */
int
use_medkit(struct obj *obj)
{
    int heal;

    if (obj->spe <= 0) {
        pline("%s is empty.", The(xname(obj)));
        return ECMD_OK;
    }
    consume_obj_charge(obj, TRUE);
    heal = d(4, 4) + (Role_if(PM_HEALER) ? u.ulevel : 0);
    if (obj->cursed)
        heal /= 2;
    You("patch yourself up with %s.", the(xname(obj)));
    /* uncursed kits also cure sickness, blessed ones blindness too */
    healup(heal, 0, (boolean) !obj->cursed, (boolean) obj->blessed);
    return ECMD_TIME;
}

/* drank a neuroblocker: suppress cyberpsychosis for a while */
void
neuroblocker_effect(struct obj *otmp)
{
    if (otmp->cursed) {
        pline("The world melts into neon static!");
        (void) make_hallucinated((HHallucination & TIMEOUT) + (long) rn1(100, 50),
                                 TRUE, 0L);
        return;
    }
    if (cyber_load() > cyber_capacity())
        You_feel("your thoughts settle into a calm, steady hum.");
    else
        You_feel("very calm.");
    u.uneuroblock += (long) (otmp->blessed ? rn1(1000, 1000)
                                           : rn1(500, 500));
    disp.botl = TRUE;
}

/* shocks and EMPs short out cyberware for a while */
void
cyber_short(long turns)
{
    int i;

    if (!u.ucyber || turns <= 0)
        return;
    if (!u.ucybershort) {
        for (i = 0; i < SIZE(implant_list); i++)
            if (has_implant(implant_list[i].otyp))
                implant_props(implant_list[i].otyp, FALSE);
        Your("cyberware sputters and goes dark!");
    }
    if (u.ucybershort < turns)
        u.ucybershort = turns;
    disp.botl = TRUE;
}

/* the hero took electrical damage */
void
cyber_shock(void)
{
    if (u.ucyber && !Shock_resistance && !rn2(3))
        cyber_short((long) rn1(20, 10));
}

staticfn void
cyber_glitch(void)
{
    int i, n = 0, pick = -1;

    for (i = 0; i < SIZE(implant_list); i++)
        if (u.ucyberglitch & implant_bit(implant_list[i].otyp))
            if (!rn2(++n))
                pick = i;
    if (pick < 0)
        return;
    Your("%s glitches!", OBJ_NAME(objects[implant_list[pick].otyp]));
    switch (rn2(3)) {
    case 0:
        make_stunned((HStun & TIMEOUT) + (long) rnd(3), FALSE);
        break;
    case 1:
        make_confused((HConfusion & TIMEOUT) + (long) rnd(5), FALSE);
        break;
    default:
        losehp(rnd(4), "malfunctioning cyberware", KILLED_BY);
        break;
    }
}

staticfn void
cyberpsycho_episode(void)
{
    switch (rn2(5)) {
    case 0:
        pline("Reality glitches around you.");
        (void) make_hallucinated((HHallucination & TIMEOUT) + (long) rn1(20, 10),
                                 TRUE, 0L);
        break;
    case 1:
        You("can't tell which thoughts are yours.");
        make_confused((HConfusion & TIMEOUT) + (long) rn1(6, 4), FALSE);
        break;
    case 2:
        You("scream in sudden, chrome-plated rage!%s",
            *neon_emoji("x") ? " \U0001F92F" : "");
        wake_nearby(FALSE);
        break;
    case 3:
        if (ACURR(A_CHA) > 3) {
            You_feel("less human.");
            (void) adjattrib(A_CHA, -1, 1); /* no message */
        } else {
            You("stare at your hands.  Whose are they?");
        }
        break;
    default:
        You("twitch uncontrollably.");
        make_stunned((HStun & TIMEOUT) + (long) rnd(3), FALSE);
        break;
    }
}

/* once per turn */
void
cyber_turn(void)
{
    int i, over;

    if (u.uneuroblock && !--u.uneuroblock) {
        if (cyberpsychotic())
            You_feel("the edges of your mind start to fray again.");
        disp.botl = TRUE;
    }
    if (!u.ucyber)
        return;
    if (u.ucybershort && !--u.ucybershort) {
        for (i = 0; i < SIZE(implant_list); i++)
            if (has_implant(implant_list[i].otyp))
                implant_props(implant_list[i].otyp, TRUE);
        Your("cyberware reboots.");
        disp.botl = TRUE;
    }
    if (u.ucyberglitch && !u.ucybershort && !rn2(400))
        cyber_glitch();
    over = cyber_load() - cyber_capacity();
    if (over > 0 && !u.uneuroblock && rn2(400) < over)
        cyberpsycho_episode();
}

/* for ^X enlightenment and the end-of-game dump */
void
cyber_enlightenment(void (*out)(const char *))
{
    char buf[BUFSZ];
    int i, load, cap;

    if (!u.ucyber)
        return;
    for (i = 0; i < SIZE(implant_list); i++) {
        if (!has_implant(implant_list[i].otyp))
            continue;
        Sprintf(buf, " You have %s%s%s.", implant_list[i].what,
                (u.ucyberglitch & implant_bit(implant_list[i].otyp))
                    ? " (glitchy)" : "",
                u.ucybershort ? " (offline)" : "");
        (*out)(buf);
    }
    load = cyber_load(), cap = cyber_capacity();
    Sprintf(buf, " Your cyber load is %d of %d%s.", load, cap,
            (load > cap) ? (u.uneuroblock ? ", held in check by neuroblockers"
                                          : ", and you are cyberpsychotic")
                         : "");
    (*out)(buf);
}

/* ------------------------------------------------------------------ */
/*  Hacking                                                           */
/* ------------------------------------------------------------------ */

/* percent chance of a hack succeeding against a target of 'difficulty' */
staticfn int
hack_chance(struct obj *deck, int difficulty)
{
    int chance = 20 + 2 * ACURR(A_INT) + 2 * u.ulevel + 5 * deck->spe
                 - difficulty;

    if (Role_if(PM_WIZARD)) /* netrunners live for this */
        chance += 25;
    if (cyber_active(NEURAL_UPLINK))
        chance += 10;
    if (Confusion || Stunned)
        chance -= 30;
    if (deck->cursed)
        chance -= 20;
    else if (deck->blessed)
        chance += 10;
    return max(5, min(chance, 95));
}

/* a hack failed and the target's countermeasures hit back;
   returns TRUE if the hero survived (always, unless killed) */
staticfn boolean
hack_feedback(struct obj *deck)
{
    pline("ICE!  Feedback surges back through %s!", the(xname(deck)));
    if (Shock_resistance) {
        shieldeff(u.ux, u.uy);
        You("are unharmed.");
    } else {
        losehp(Maybe_Half_Phys(d(2, 6)), "black ICE feedback", KILLED_BY);
        cyber_shock();
    }
    return TRUE;
}

staticfn int
hack_robot(struct obj *deck, struct monst *mtmp)
{
    int difficulty = 4 * mtmp->m_lev + (mtmp->data->geno & G_UNIQ ? 50 : 0);
    boolean seen = canspotmon(mtmp);

    if (mtmp->mtame) {
        pline("%s is already running your code.", Monnam(mtmp));
        return ECMD_OK;
    }
    You("%sjack into %s...", neon_emoji("\U0001F4BB "),
        seen ? mon_nam(mtmp) : "something");
    if (rn2(100) < hack_chance(deck, difficulty)) {
        exercise(A_INT, TRUE);
        if (mtmp->m_lev <= u.ulevel / 2 + 2 && !(mtmp->data->geno & G_UNIQ)
            && tamedog(mtmp, (struct obj *) 0, FALSE)) {
            pline("Access granted.  %s is yours now.",
                  seen ? Monnam(mtmp) : "It");
        } else if (!mtmp->mpeaceful && !rn2(2)) {
            pline("Access granted.  %s stands down.",
                  seen ? Monnam(mtmp) : "It");
            mtmp->mpeaceful = 1;
            set_malign(mtmp);
        } else {
            pline("Access granted.  %s powers down.",
                  seen ? Monnam(mtmp) : "It");
            paralyze_monst(mtmp, rn1(50, 25));
        }
        newsym(mtmp->mx, mtmp->my);
    } else {
        pline("%sAccess denied.", neon_emoji("\U0001F6AB "));
        if (mtmp->mpeaceful)
            setmangry(mtmp, TRUE);
        else
            wakeup(mtmp, FALSE);
        if (!rn2(2))
            (void) hack_feedback(deck);
    }
    return ECMD_TIME;
}

staticfn int
hack_lock(struct obj *deck, coordxy x, coordxy y)
{
    struct rm *door = &levl[x][y];
    struct obj *box;

    if (IS_DOOR(door->typ) && (door->doormask & D_LOCKED)) {
        You("splice into the door's maglock...");
        if (rn2(100) < hack_chance(deck, 10)) {
            door->doormask &= ~D_LOCKED;
            door->doormask |= D_CLOSED;
            pline("Click!  The door unlocks.");
            exercise(A_INT, TRUE);
            feel_newsym(x, y);
        } else {
            pline("The lock refuses your credentials.");
            if (!rn2(4))
                (void) hack_feedback(deck);
        }
        return ECMD_TIME;
    }
    for (box = svl.level.objects[x][y]; box; box = box->nexthere)
        if (Is_box(box) && box->olocked)
            break;
    if (box) {
        You("splice into %s's lock...", the(xname(box)));
        if (rn2(100) < hack_chance(deck, 5)) {
            box->olocked = 0;
            pline("Click!  %s unlocks.", The(xname(box)));
            exercise(A_INT, TRUE);
        } else {
            pline("The lock refuses your credentials.");
        }
        return ECMD_TIME;
    }
    if (IS_DOOR(door->typ) && door->doormask != D_NODOOR
        && door->doormask != D_BROKEN)
        pline("This door's lock isn't engaged.");
    else
        pline("There's nothing there to hack.");
    return ECMD_OK;
}

/* apply a cyberdeck */
int
use_cyberdeck(struct obj *deck)
{
    struct monst *mtmp;
    coordxy x, y;
    int res;

    if (deck->spe <= 0) {
        pline("%s battery is dead.", s_suffix(The(xname(deck))));
        return ECMD_OK;
    }
    if (!getdir("Hack in what direction?"))
        return ECMD_CANCEL;
    if (u.dz) {
        pline("There's no jack %s you.", u.dz > 0 ? "below" : "above");
        return ECMD_OK;
    }
    x = u.ux + u.dx, y = u.uy + u.dy;
    if (!isok(x, y))
        return ECMD_OK;

    if ((mtmp = m_at(x, y)) != 0 && is_robot(mtmp->data)) {
        consume_obj_charge(deck, TRUE);
        res = hack_robot(deck, mtmp);
    } else if (mtmp && !u.dx && !u.dy) {
        res = ECMD_OK; /* can't happen: hero's own spot */
    } else if (mtmp && canspotmon(mtmp)) {
        pline("%s has no ports to jack into.", Monnam(mtmp));
        res = ECMD_OK;
    } else {
        res = hack_lock(deck, x, y);
        if (res == ECMD_TIME)
            consume_obj_charge(deck, TRUE);
    }
    return res;
}

/* the #hack command: use the first cyberdeck in inventory */
int
dohack(void)
{
    struct obj *deck;

    if (nohands(gy.youmonst.data)) {
        You("can't work a cyberdeck in your current form.");
        return ECMD_OK;
    }
    for (deck = gi.invent; deck; deck = deck->nobj)
        if (deck->otyp == CYBERDECK)
            break;
    if (!deck) {
        You("need a cyberdeck to hack anything.");
        return ECMD_OK;
    }
    return use_cyberdeck(deck);
}

/* ------------------------------------------------------------------ */
/*  Heat                                                              */
/* ------------------------------------------------------------------ */

#define HEAT_MAX 100

/* crimes add heat; good behavior (or payment) takes it away */
void
heat_add(int amount)
{
    long old = u.uheat;

    u.uheat += amount;
    if (u.uheat < 0)
        u.uheat = 0;
    if (u.uheat > HEAT_MAX)
        u.uheat = HEAT_MAX;
    if (old < HEAT_WANTED && u.uheat >= HEAT_WANTED) {
        You_feel("the cold attention of a corporate algorithm.");
        pline("%sYou are now wanted by corporate security.",
              neon_emoji("\U0001F6A8 "));
        u.uheatsquad = svm.moves; /* a little grace period */
    } else if (old >= HEAT_WANTED && u.uheat < HEAT_WANTED) {
        You_feel("that corporate security has lost interest in you.");
    }
    if (old != u.uheat)
        disp.botl = TRUE;
}

staticfn boolean
squad_allowed_here(void)
{
    /* corporate security doesn't go everywhere */
    return !(In_sokoban(&u.uz) || In_endgame(&u.uz) || In_V_tower(&u.uz)
             || In_tutorial(&u.uz)
             || Is_knox(&u.uz) || u.uswallow || u.uinvulnerable
             || In_hell(&u.uz));
}

/* a squad of corporate security deploys onto the level */
staticfn void
send_squad(void)
{
    int i, n, heat = (int) u.uheat;
    struct monst *mtmp;
    int leader = (heat >= 80) ? PM_LIEUTENANT
                 : (heat >= 50) ? PM_SERGEANT : NON_PM;

    if (!Deaf)
        You_hear("the whine of a corpsec dropship overhead!%s",
                 *neon_emoji("x") ? " \U0001F6A8" : "");
    else
        You_feel("the floor vibrate as something heavy lands nearby.");
    stop_occupation();
    n = 1 + heat / 25; /* 1..5 troopers */
    for (i = 0; i < n; i++) {
        mtmp = makemon(&mons[PM_SOLDIER], 0, 0, MM_NOMSG);
        if (mtmp)
            mtmp->mpeaceful = 0, mtmp->mstrategy &= ~STRAT_WAITMASK;
    }
    if (leader != NON_PM && (mtmp = makemon(&mons[leader], 0, 0, MM_NOMSG)))
        mtmp->mpeaceful = 0, mtmp->mstrategy &= ~STRAT_WAITMASK;
    if (heat >= 60)
        for (i = 0; i < 2; i++)
            (void) makemon(&mons[PM_SECURITY_DRONE], 0, 0, MM_NOMSG);
    if (heat >= 90)
        (void) makemon(&mons[PM_SENTRY_BOT], 0, 0, MM_NOMSG);
    u.uheatsquad = svm.moves;
}

/* once per turn */
void
heat_turn(void)
{
    if (!u.uheat)
        return;
    /* heat cools off slowly: -1 every 30 turns */
    if (!(svm.moves % 30))
        heat_add(-1);
    if (u.uheat >= HEAT_WANTED && svm.moves - u.uheatsquad > 300
        && !rn2(150 - (int) min(u.uheat, (long) HEAT_MAX))
        && squad_allowed_here())
        send_squad();
}

/* #wizheat: set heat; if wanted, deploy a squad right away */
int
wiz_heat(void)
{
    char buf[BUFSZ];
    int val;

    if (!wizard)
        return ECMD_OK;
    Sprintf(buf, "Set heat to what? (now %ld, wanted at %d)", u.uheat,
            HEAT_WANTED);
    getlin(buf, buf);
    if (*buf == '\033' || !*buf)
        return ECMD_OK;
    val = atoi(buf);
    heat_add(val - (int) u.uheat);
    if (u.uheat >= HEAT_WANTED && squad_allowed_here())
        send_squad();
    return ECMD_OK;
}

void
heat_enlightenment(void (*out)(const char *))
{
    char buf[BUFSZ];
    const char *status;

    if (!u.uheat)
        return;
    status = (u.uheat >= 80) ? "on the corporate kill list"
             : (u.uheat >= 50) ? "most wanted by corporate security"
               : (u.uheat >= HEAT_WANTED) ? "wanted by corporate security"
                 : (u.uheat >= 10) ? "a person of interest to corpsec"
                   : "on corpsec's radar";
    Sprintf(buf, " You are %s (heat %ld).", status, u.uheat);
    (*out)(buf);
}

/*neon.c*/
