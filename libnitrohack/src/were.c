/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"

void were_change(struct monst *mon)
{
	if (!is_were(mon->data))
	    return;

	if (is_human(mon->data)) {
	    if (!Protection_from_shape_changers &&
		!rn2(night() ? (flags.moonphase == FULL_MOON ?  3 : 30)
			     : (flags.moonphase == FULL_MOON ? 10 : 50))) {
		new_were(mon);		/* change into animal form */
		if (flags.soundok && !canseemon(level, mon)) {
		    const char *howler;

		    switch (mon->mnum) {
		    case PM_WEREWOLF:	howler = "wolf";    break;
		    case PM_WEREJACKAL: howler = "jackal";  break;
		    default:		howler = NULL; break;
		    }
		    if (howler)
			You_hear("a %s howling at the moon.", howler);
		}
	    }
	} else if (!rn2(30) || Protection_from_shape_changers) {
	    new_were(mon);		/* change back into human form */
	}
}


static int counter_were(int);

static int counter_were(int pm)
{
	switch(pm) {
	    case PM_WEREWOLF:	      return PM_HUMAN_WEREWOLF;
	    case PM_HUMAN_WEREWOLF:   return PM_WEREWOLF;
	    case PM_WEREJACKAL:	      return PM_HUMAN_WEREJACKAL;
	    case PM_HUMAN_WEREJACKAL: return PM_WEREJACKAL;
	    case PM_WERERAT:	      return PM_HUMAN_WERERAT;
	    case PM_HUMAN_WERERAT:    return PM_WERERAT;
	    default:		      return 0;
	}
}

void new_were(struct monst *mon)
{
	int pm;

	pm = counter_were(monsndx(mon->data));
	if (!pm) {
	    warning("unknown lycanthrope %s.", mons_mname(mon->data));
	    return;
	}

	if (canseemon(level, mon) && !Hallucination)
	    pline("%s changes into a %s.", Monnam(mon),
			is_human(&mons[pm]) ? "human" :
			mons_mname(&mons[pm])+4);

	set_mon_data(mon, &mons[pm], 0);
	if (mon->msleeping || !mon->mcanmove) {
	    /* transformation wakens and/or revitalizes */
	    mon->msleeping = 0;
	    mon->mfrozen = 0;	/* not asleep or paralyzed */
	    mon->mcanmove = 1;
	}
	/* regenerate by 1/4 of the lost hit points */
	mon->mhp += (mon->mhpmax - mon->mhp) / 4;
	newsym(mon->mx,mon->my);
	mon_break_armor(level, mon, FALSE);
	possibly_unwield(mon, FALSE);
}

/* were-creature (even you) summons a horde */
int were_summon(const struct permonst *ptr, boolean yours,
		int *visible,	/* number of visible helpers created */
		char *genbuf)
{
	int i, typ, pm = monsndx(ptr);
	struct monst *mtmp;
	int total = 0;

	*visible = 0;
	if (Protection_from_shape_changers && !yours)
		return 0;
	for (i = rnd(5); i > 0; i--) {
	   switch(pm) {

		case PM_WERERAT:
		case PM_HUMAN_WERERAT:
			typ = rn2(3) ? PM_SEWER_RAT : rn2(3) ? PM_GIANT_RAT : PM_RABID_RAT ;
			if (genbuf) strcpy(genbuf, "rat");
			break;
		case PM_WEREJACKAL:
		case PM_HUMAN_WEREJACKAL:
			typ = PM_JACKAL;
			if (genbuf) strcpy(genbuf, "jackal");
			break;
		case PM_WEREWOLF:
		case PM_HUMAN_WEREWOLF:
			typ = rn2(5) ? PM_WOLF : PM_WINTER_WOLF ;
			if (genbuf) strcpy(genbuf, "wolf");
			break;
		default:
			continue;
	    }
	    mtmp = makemon(&mons[typ], level, u.ux, u.uy, NO_MM_FLAGS);
	    if (mtmp) {
		total++;
		if (canseemon(level, mtmp)) *visible += 1;
	    }
	    if (yours && mtmp)
		tamedog(mtmp, NULL);
	}
	return total;
}

void you_were(void)
{
	char qbuf[QBUFSZ];

	if (Unchanging || (u.umonnum == u.ulycn)) return;
	if (Polymorph_control) {
	    /* `+4' => skip "were" prefix to get name of beast */
	    sprintf(qbuf, "Do you want to change into %s? ",
		    an(mons_mname(&mons[u.ulycn])+4));
	    if (yn(qbuf) == 'n') return;
	}
	polymon(u.ulycn);
}

void you_unwere(boolean purify)
{
	if (purify) {
	    pline("You feel purified.");
	    u.ulycn = NON_PM;	/* cure lycanthropy */
	}
	if (!Unchanging && is_were(youmonst.data) &&
		(!Polymorph_control || yn("Remain in beast form?") == 'n'))
	    rehumanize();
}

/*were.c*/
