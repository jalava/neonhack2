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
 */

#include "hack.h"

staticfn int gunfire_range(struct obj *);
staticfn const char *gunfire_sound(struct obj *, int);
staticfn void flashbang(coordxy, coordxy);

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
        pline("%s", gunfire_sound(launcher, shots));
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
            pline("%s", gunfire_sound(launcher, shots));
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
        pline("There is a blinding flash and a deafening bang!");
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
        pline("Blue sparks crackle through the air!");
    else if (!Deaf)
        You_hear("a sharp electrical crack.");

    if (dist2(u.ux, u.uy, x, y) <= r2) {
        if (u.uen > 0) {
            You_feel("your power drain away!");
            u.uen -= min(u.uen, d(4, 6));
            disp.botl = TRUE;
        }
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

/*neon.c*/
