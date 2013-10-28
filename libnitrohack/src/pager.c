/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

/* This file contains the command routines dowhatis() and dohelp() and */
/* a few other help related facilities */

#include "hack.h"
#include "dlb.h"

extern const int monstr[];

static int append_str(char *buf, const char *new_str, int is_plur);
static void mon_vision_summary(const struct monst *mtmp, char *outbuf);
static void describe_bg(int x, int y, int bg, char *buf);
static int describe_object(int x, int y, int votyp, char *buf);
static void describe_mon(int x, int y, int monnum, char *buf);
static void checkfile(const char *inp, struct permonst *, boolean, boolean);
static int do_look(boolean);

/* same max width as data.base text */
#define MONDESC_MAX_WIDTH 72

/* The explanations below are also used when the user gives a string
 * for blessed genocide, so no text should wholly contain any later
 * text.  They should also always contain obvious names (eg. cat/feline).
 */
const char * const monexplain[MAXMCLASSES] = {
    0,
    "ant or other insect",	"blob",			"cockatrice",
    "dog or other canine",	"eye or sphere",	"cat or other feline",
    "gremlin",			"humanoid",		"imp or minor demon",
    "jelly",			"kobold",		"leprechaun",
    "mimic",			"nymph",		"orc",
    "piercer",			"quadruped",		"rodent",
    "arachnid or centipede",	"trapper or lurker above", "unicorn or horse",
    "vortex",		"worm", "xan or other mythical/fantastic insect",
    "light",			"zruty",

    "angelic being",		"bat or bird",		"centaur",
    "dragon",			"elemental",		"fungus or mold",
    "gnome",			"giant humanoid",	0,
    "jabberwock",		"Keystone Kop",		"lich",
    "mummy",			"naga",			"ogre",
    "pudding or ooze","quantum mechanic","rust monster, disenchanter or disintegrator",
    "snake",			"troll",		"umber hulk",
    "vampire",			"wraith",		"xorn",
    "apelike creature",		"zombie",

    "human or elf",		"ghost",		"golem",
    "major demon",		"sea monster",		"lizard",
    "long worm tail",		"mimic"
};

const char invisexplain[] = "remembered, unseen, creature";

/* Object descriptions.  Used in do_look(). */
const char * const objexplain[] = {	/* these match def_oc_syms */
/* 0*/	0,
	"strange object",
	"weapon",
	"suit or piece of armor",
	"ring",
/* 5*/	"amulet",
	"useful item (pick-axe, key, lamp...)",
	"piece of food",
	"potion",
	"scroll",
/*10*/	"spellbook",
	"wand",
	"pile of coins",
	"gem or rock",
	"boulder or statue",
/*15*/	"iron ball",
	"iron chain",
	"splash of venom"
};

/*
 * Append new_str to the end of buf if new_str doesn't already exist as
 * a substring of buf.  Return 1 if the string was appended, 0 otherwise.
 * It is expected that buf is of size BUFSZ.
 */
static int append_str(char *buf, const char *new_str, int is_plur)
{
    int space_left;	/* space remaining in buf */

    if (!new_str || !new_str[0])
	return 0;
    
    space_left = BUFSZ - strlen(buf) - 1;
    if (buf[0]) {
	strncat(buf, " on ", space_left);
	space_left -= 4;
    }
    
    if (is_plur)
	strncat(buf, new_str, space_left);
    else
	strncat(buf, an(new_str), space_left);
    
    return 1;
}


static void mon_vision_summary(const struct monst *mtmp, char *outbuf)
{
    int ways_seen = 0, normal = 0, xraydist;
    boolean useemon = (boolean) canseemon(level, mtmp);
    
    outbuf[0] = '\0';

    xraydist = (u.xray_range<0) ? -1 : u.xray_range * u.xray_range;
    /* normal vision */
    if ((mtmp->wormno ? worm_known(level, mtmp) : cansee(mtmp->mx, mtmp->my)) &&
	    mon_visible(mtmp) && !mtmp->minvis) {
	ways_seen++;
	normal++;
    }
    /* see invisible */
    if (useemon && mtmp->minvis)
	ways_seen++;
    /* infravision */
    if ((!mtmp->minvis || See_invisible) && see_with_infrared(mtmp))
	ways_seen++;
    /* telepathy */
    if (tp_sensemon(mtmp))
	ways_seen++;
    /* xray */
    if (useemon && xraydist > 0 &&
	    distu(mtmp->mx, mtmp->my) <= xraydist)
	ways_seen++;
    if (Detect_monsters)
	ways_seen++;
    if (match_warn_of_mon(mtmp))
	ways_seen++;

    if (ways_seen > 1 || !normal) {
	if (normal) {
	    strcat(outbuf, "normal vision");
	    /* can't actually be 1 yet here */
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if (useemon && mtmp->minvis) {
	    strcat(outbuf, "see invisible");
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if ((!mtmp->minvis || See_invisible) &&
		see_with_infrared(mtmp)) {
	    strcat(outbuf, "infravision");
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if (tp_sensemon(mtmp)) {
	    strcat(outbuf, "telepathy");
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if (useemon && xraydist > 0 &&
		distu(mtmp->mx, mtmp->my) <= xraydist) {
	    /* Eyes of the Overworld */
	    strcat(outbuf, "astral vision");
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if (Detect_monsters) {
	    strcat(outbuf, "monster detection");
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
	if (match_warn_of_mon(mtmp)) {
	    char wbuf[BUFSZ];
	    if (Hallucination)
		    strcat(outbuf, "paranoid delusion");
	    else {
		    sprintf(wbuf, "warned of %s",
			    makeplural(mons_mname(mtmp->data)));
		    strcat(outbuf, wbuf);
	    }
	    if (ways_seen-- > 1) strcat(outbuf, ", ");
	}
    }
}


static void describe_bg(int x, int y, int bg, char *buf)
{
    if (!bg)
	return;
    
    switch(bg) {
	case S_altar:
	    if (!In_endgame(&u.uz))
		sprintf(buf, "%s altar",
		    align_str(Amask2align(level->locations[x][y].altarmask & ~AM_SHRINE)));
	    else
		sprintf(buf, "aligned altar");
	    break;
	    
	case S_ndoor:
	    if (is_drawbridge_wall(x, y) >= 0)
		strcpy(buf,"open drawbridge portcullis");
	    else if ((level->locations[x][y].doormask & ~D_TRAPPED) == D_BROKEN)
		strcpy(buf,"broken door");
	    else
		strcpy(buf,"doorway");
	    break;
	    
	case S_cloud:
	    strcpy(buf, Is_airlevel(&u.uz) ? "cloudy area" : "fog/vapor cloud");
	    break;
	    
	default:
	    strcpy(buf, defexplain[bg]);
	    break;
    }
}


static int describe_object(int x, int y, int votyp, char *buf)
{
    int num_objs = 0;
    struct obj *otmp;
    boolean unfelt_ball, unfelt_chain;

    if (votyp == -1)
	return -1;

    /* If we're blind and punished and the ball/chain are on top of an object,
     * vobj_at() may mismatch with votyp and describe the remembered object
     * incorrectly. */
    if (Blind && Punished) {
	unfelt_ball = ((u.bc_felt & BC_BALL) == 0) && !carried(uball) &&
		      x == uball->ox && y == uball->oy;
	unfelt_chain = ((u.bc_felt & BC_CHAIN) == 0) &&
		       x == uchain->ox && y == uchain->oy;
    } else {
	/* Even if the ball/chain exist, we're not blind so it doesn't matter. */
	unfelt_ball = unfelt_chain = FALSE;
    }

    otmp = vobj_at(x,y);

    /* Skip unfelt ball/chain if needed. */
    while (otmp && ((unfelt_ball  && otmp == uball) ||
		    (unfelt_chain && otmp == uchain)))
	otmp = otmp->nexthere;

    if (!otmp || otmp->otyp != votyp) {
	if (votyp == STRANGE_OBJECT) {
	    strcpy(buf, "strange object");
	} else {
	    otmp = mksobj(level, votyp, FALSE, FALSE);
	    if (otmp->oclass == COIN_CLASS)
		otmp->quan = 1L; /* to force pluralization off */
	    else if (otmp->otyp == SLIME_MOLD)
		otmp->spe = current_fruit;	/* give the fruit a type */
	    strcpy(buf, distant_name(otmp, xname));
	    dealloc_obj(otmp);
	    otmp = vobj_at(x,y); /* make sure we don't point to the temp obj any more */
	}
    } else
	strcpy(buf, distant_name(otmp, xname));
    
    if (level->locations[x][y].mem_obj_stacks)
	strcat(buf, " and more");
    
    if (level->locations[x][y].typ == STONE || level->locations[x][y].typ == SCORR)
	strcat(buf, " embedded in stone");
    else if (IS_WALL(level->locations[x][y].typ) || level->locations[x][y].typ == SDOOR)
	strcat(buf, " embedded in a wall");
    else if (closed_door(level, x,y))
	strcat(buf, " embedded in a door");
    else if (is_pool(level, x,y))
	strcat(buf, " in water");
    else if (is_lava(level, x,y))
	strcat(buf, " in molten lava");	/* [can this ever happen?] */
    
    if (!cansee(x, y))
	return -1; /* don't disclose the number of objects for location out of LOS */
    
    if (!otmp)
	/* There is no object here. Since the player sees one it must be a mimic */
	return 1;
    
    if (otmp->otyp != votyp)
	/* Hero sees something other than the actual top object. Probably a mimic */
	num_objs++;

    /* Don't count unfelt ball/chain. */
    for ( ; otmp; otmp = otmp->nexthere)
	if ((!unfelt_ball || otmp != uball) && (!unfelt_chain || otmp != uchain))
	    num_objs++;

    return num_objs;
}


static void describe_mon(int x, int y, int monnum, char *buf)
{
    char race[QBUFSZ];
    char *name, monnambuf[BUFSZ];
    boolean accurate = !Hallucination;
    char steedbuf[BUFSZ];
    struct monst *mtmp;
    char visionbuf[BUFSZ], temp_buf[BUFSZ];
    
    if (monnum == -1)
	return;
    
    if (u.ux == x && u.uy == y && senseself()) {
	/* if not polymorphed, show both the role and the race */
	race[0] = 0;
	if (!Upolyd)
	    sprintf(race, "%s ", urace.adj);

	sprintf(buf, "%s%s%s called %s",
		Invis ? "invisible " : "",
		race,
		mons_mname(&mons[u.umonnum]),
		plname);
	
	if (u.usteed) {
	    sprintf(steedbuf, ", mounted on %s", y_monnam(u.usteed));
	    /* assert((sizeof buf >= strlen(buf)+strlen(steedbuf)+1); */
	    strcat(buf, steedbuf);
	}
	/* When you see yourself normally, no explanation is appended
	(even if you could also see yourself via other means).
	Sensing self while blind or swallowed is treated as if it
	were by normal vision (cf canseeself()). */
	if ((Invisible || u.uundetected) && !Blind && !u.uswallow) {
	    unsigned how = 0;

	    if (Infravision)	 how |= 1;
	    if (Unblind_telepat) how |= 2;
	    if (Detect_monsters) how |= 4;

	    if (how)
		sprintf(eos(buf), " [seen: %s%s%s%s%s]",
			(how & 1) ? "infravision" : "",
			/* add comma if telep and infrav */
			((how & 3) > 2) ? ", " : "",
			(how & 2) ? "telepathy" : "",
			/* add comma if detect and (infrav or telep or both) */
			((how & 7) > 4) ? ", " : "",
			(how & 4) ? "monster detection" : "");
	}
	
    } else if (monnum >= NUMMONS) {
	monnum -= NUMMONS;
	if (monnum < WARNCOUNT)
	    strcat(buf, warnexplain[monnum]);
	
    } else if ( (mtmp = m_at(level, x,y)) ) {
	bhitpos.x = x;
	bhitpos.y = y;

	if (mtmp->data == &mons[PM_COYOTE] && accurate)
	    name = coyotename(mtmp, monnambuf);
	else
	    name = distant_monnam(mtmp, ARTICLE_NONE, monnambuf);

	sprintf(buf, "%s%s%s",
		(mtmp->mx != x || mtmp->my != y) ?
		    ((mtmp->isshk && accurate)
			    ? "tail of " : "tail of a ") : "",
		(mtmp->mtame && accurate) ? "tame " :
		(mtmp->mpeaceful && accurate) ? "peaceful " : "",
		name);
	if (u.ustuck == mtmp)
	    strcat(buf, (Upolyd && sticks(youmonst.data)) ?
		    ", being held" : ", holding you");
	if (mtmp->mleashed)
	    strcat(buf, ", leashed to you");

	if (mtmp->mtrapped && cansee(mtmp->mx, mtmp->my)) {
	    struct trap *t = t_at(level, mtmp->mx, mtmp->my);
	    int tt = t ? t->ttyp : NO_TRAP;

	    /* newsym lets you know of the trap, so mention it here */
	    if (tt == BEAR_TRAP || tt == PIT ||
		    tt == SPIKED_PIT || tt == WEB)
		sprintf(eos(buf), ", trapped in %s", an(trapexplain[tt-1]));
	}

	mon_vision_summary(mtmp, visionbuf);
	if (visionbuf[0]) {
	    sprintf(temp_buf, " [seen: %s]", visionbuf);
	    strncat(buf, temp_buf, BUFSZ-strlen(buf)-1);
	}
    }
}


void nh_describe_pos(int x, int y, struct nh_desc_buf *bufs)
{
    int monid = dbuf_get_mon(x, y);
    
    bufs->bgdesc[0] = '\0';
    bufs->trapdesc[0] = '\0';
    bufs->objdesc[0] = '\0';
    bufs->mondesc[0] = '\0';
    bufs->invisdesc[0] = '\0';
    bufs->effectdesc[0] = '\0';
    bufs->objcount = -1;
    
    if (!program_state.game_running || !api_entry_checkpoint())
	return;
    
    describe_bg(x, y, level->locations[x][y].mem_bg, bufs->bgdesc);
    
    if (level->locations[x][y].mem_trap) {
	/* Avoid "monster trapped in a web on a web" from describe_mon(). */
	const struct monst *mtmp;
	const struct trap *t;
	if (!(monid &&					/* monster seen */
		(monid - 1) < NUMMONS &&		/* not a monster warning */
		!(u.ux == x && u.uy == y) &&		/* not the hero */
		(mtmp = m_at(level, x, y)) &&		/* monster at location */
		(mtmp->mtrapped && cansee(x, y)) &&	/* mon seen in trap */
		(t = t_at(level, x, y)) &&		/* trap at location */
		(t->ttyp == BEAR_TRAP || t->ttyp == PIT ||
		 t->ttyp == SPIKED_PIT || t->ttyp == WEB)))
	    strcpy(bufs->trapdesc, trapexplain[level->locations[x][y].mem_trap - 1]);
    }
    
    bufs->objcount = describe_object(x, y, level->locations[x][y].mem_obj - 1,
				     bufs->objdesc);
    
    describe_mon(x, y, monid - 1, bufs->mondesc);
    
    if (level->locations[x][y].mem_invis)
	strcpy(bufs->invisdesc, invisexplain);
    
    if (u.uswallow && (x != u.ux || y != u.uy)) {
	/* all locations when swallowed other than the hero are the monster */
	sprintf(bufs->effectdesc, "interior of %s", Blind ? "a monster" : a_monnam(u.ustuck));
    }
    
    api_exit();
}


static void add_menutext_wrapped(struct menulist *menu, int width, const char *text)
{
    char **output;
    int i, output_count;

    wrap_text(width, text, &output_count, &output);
    for (i = 0; i < output_count; i++)
	add_menutext(menu, output[i]);
    free_wrap(output);
}


static int appendc(char *buf, boolean cond, char *text, int num)
{
    if (!cond) return num;
    sprintf(eos(buf), "%s%s", (num ? ", " : ""), text);
    return num + 1;
}


static int appendp(char *buf, boolean cond, char *text, int num)
{
    if (!cond) return num;
    sprintf(eos(buf), "%s%s", (num ? ".  " : ""), text);
    return num + 1;
}


static void mondesc_speed(struct menulist *menu, int speed)
{
    char buf[BUFSZ];
    sprintf(buf, "Speed %d (", speed);
    strcat(buf, speed >= 36 ? "extremely fast" :
		speed >= 20 ? "very fast" :
		speed >  12 ? "fast" :
		speed == 12 ? "normal speed" :
		speed >=  9 ? "slow" :
		speed >=  3 ? "very slow" :
		speed >=  1 ? "extremely slow" :
			      "sessile");
    strcat(buf, ").");
    add_menutext(menu, buf);
}


static void mondesc_generation(struct menulist *menu, unsigned short geno)
{
    char buf[BUFSZ] = "";
    int num = 0;

    if (geno & G_NOGEN) {
	num = appendc(buf, !!(geno & G_NOGEN), "Specially generated", num);
    } else {
	strcpy(buf, "Normally appears ");
	num = appendc(buf, !(geno & G_NOHELL) && !(geno & G_HELL),
		      "everywhere", num);
	num = appendc(buf, !!(geno & G_NOHELL), "outside of Gehennom", num);
	num = appendc(buf, !!(geno & G_HELL), "in Gehennom", num);
    }

    num = appendc(buf, !!(geno & G_UNIQ), "unique", num);
    if (geno & (G_SGROUP|G_LGROUP)) {
	num = 0;
	num = appendc(buf, !!(geno & G_SGROUP), " in groups", num);
	num = appendc(buf, !!(geno & G_LGROUP), " in large groups", num);
    }

    if (!(geno & G_NOGEN)) {
	if (num) strcat(buf, ", ");
	switch (geno & G_FREQ) {
	case 1: strcat(buf, "very rare"); break;
	case 2: strcat(buf, "quite rare"); break;
	case 3: strcat(buf, "rare"); break;
	case 4: strcat(buf, "uncommon"); break;
	case 5: strcat(buf, "common"); break;
	case 6: strcat(buf, "very common"); break;
	case 7: strcat(buf, "prolific"); break;
	default: sprintf(eos(buf), " frequency %d", geno & G_FREQ);
	}
    }

    strcat(buf, ".");

    add_menutext_wrapped(menu, MONDESC_MAX_WIDTH, buf);
}


static int mondesc_resist_flags_to_str(char *buf, uchar flags)
{
    int num = 0;
    num = appendc(buf, !!(flags & MR_FIRE), "fire", num);
    num = appendc(buf, !!(flags & MR_COLD), "cold", num);
    num = appendc(buf, !!(flags & MR_SLEEP), "sleep", num);
    num = appendc(buf, !!(flags & MR_DISINT), "disintegration", num);
    num = appendc(buf, !!(flags & MR_ELEC), "shock", num);
    num = appendc(buf, !!(flags & MR_POISON), "poison", num);
    num = appendc(buf, !!(flags & MR_ACID), "acid", num);
    num = appendc(buf, !!(flags & MR_STONE), "petrification", num);
    return num;
}


static void mondesc_resistances(struct menulist *menu, const struct permonst *pm)
{
    char buf[BUFSZ];

    if (is_unknown_dragon(pm)) {
	add_menutext(menu, "Resistances unknown.");
	add_menutext(menu, "Corpse conveys unknown resistances.");
	return;
    }

    strcpy(buf, "Resists ");
    if (mondesc_resist_flags_to_str(buf, pm->mresists)) {
	strcat(buf, ".");
	add_menutext_wrapped(menu, MONDESC_MAX_WIDTH, buf);
    } else {
	add_menutext(menu, "Has no resistances.");
    }

    if (pm->geno & G_NOCORPSE) {
	add_menutext(menu, "Leaves no corpse.");
    } else {
	strcpy(buf, "Corpse conveys ");
	if (mondesc_resist_flags_to_str(buf, pm->mconveys)) {
	    strcat(buf, " resistance.");
	    add_menutext_wrapped(menu, MONDESC_MAX_WIDTH, buf);
	} else {
	    add_menutext(menu, "Corpse conveys no resistances.");
	}
    }
}


static void mondesc_flags(struct menulist *menu, const struct permonst *pm)
{
    char buf[BUFSZ] = "";
    char size[BUFSZ] = "";
    char adjbuf[BUFSZ] = "";
    char specialadj[BUFSZ] = "";
    char noun[BUFSZ] = "";
    int num;
    int adjnum;
    int nounnum;

    strcpy(size, pm->msize == MZ_TINY ? "tiny" :
		 pm->msize == MZ_SMALL ? "small" :
		 pm->msize == MZ_LARGE ? "large" :
		 pm->msize == MZ_HUGE ? "huge" :
		 pm->msize == MZ_GIGANTIC ? "gigantic" : "");
    if (!*size) {
	/* monster may be of a non-standard size */
	if (verysmall(pm)) strcpy(size, "small");
	else if (hugemonst(pm)) strcpy(size, "huge");
	else if (bigmonst(pm)) strcpy(size, "big");
    }

    adjnum = 0;
    adjnum = appendc(adjbuf, !(pm->geno & G_GENO), "ungenocidable", adjnum);
    adjnum = appendc(adjbuf, breathless(pm), "breathless", adjnum);
    adjnum = appendc(adjbuf, amphibious(pm), "amphibious", adjnum);
    adjnum = appendc(adjbuf, passes_walls(pm), "phasing", adjnum);
    adjnum = appendc(adjbuf, amorphous(pm), "amorphous", adjnum);
    adjnum = appendc(adjbuf, noncorporeal(pm), "noncorporeal", adjnum);
    adjnum = appendc(adjbuf, unsolid(pm), "unsolid", adjnum);
    adjnum = appendc(adjbuf, acidic(pm), "acidic", adjnum);
    if (!is_unknown_dragon(pm)) /* don't reveal poison dragons */
	adjnum = appendc(adjbuf, poisonous(pm), "poisonous", adjnum);
    adjnum = appendc(adjbuf, regenerates(pm), "regenerating", adjnum);
    adjnum = appendc(adjbuf, can_teleport(pm), "teleporting", adjnum);
    adjnum = appendc(adjbuf, is_reviver(pm), "reviving", adjnum);
    adjnum = appendc(adjbuf, pm_invisible(pm), "invisible", adjnum);
    adjnum = appendc(adjbuf, nonliving(pm) && !is_undead(pm),
		     "nonliving", adjnum);

    appendc(specialadj, is_undead(pm), "undead", 0);

    nounnum = 0;
    nounnum = appendc(noun, is_hider(pm), "hider", nounnum);
    nounnum = appendc(noun, is_swimmer(pm), "swimmer", nounnum);
    nounnum = appendc(noun, is_flyer(pm), "flyer", nounnum);
    nounnum = appendc(noun, is_floater(pm), "floater", nounnum);
    nounnum = appendc(noun, is_clinger(pm), "clinger", nounnum);
    if (tunnels(pm)) {
	nounnum = appendc(noun, TRUE, (needspick(pm) ? "miner" : "digger"),
			  nounnum);
    }

    /* <size><adjectives><special adjectives><noun> */
    if (*size) {
	if (adjnum <= 1 && (*specialadj || *noun)) {
	    /* huge undead */
	    /* small noncorporeal miner */
	    strcat(buf, size);
	    strcat(buf, " ");
	} else if (adjnum >= 1) {
	    /* small, genocideable, amphibious swimmer */
	    /* big, poisonous, invisible miner */
	    /* big, poisonous, invisible hider, swimmer, flyer */
	    /* huge */
	    /* small, noncorporeal */
	    /* big, poisonous, invisible */
	    strcat(buf, size);
	    strcat(buf, ", ");
	} else if (adjnum == 0) {
	    /* small swimmer */
	    /* big miner */
	    /* big swimmer, flyer */
	    /* huge */
	    /* small undead digger */
	    strcat(buf, size);
	    strcat(buf, " ");
	} else {
	    impossible("mondesc_flags(): impossible adjnum (%d)", adjnum);
	}
    }
    if (*adjbuf) {
	strcat(buf, adjbuf);
	strcat(buf, " ");
    }
    if (*specialadj) {
	strcat(buf, specialadj);
	strcat(buf, " ");
    }
    if (*noun) {
	strcat(buf, noun);
	strcat(buf, " ");
    }

    if (*buf) {
	upstart(buf);
	*(eos(buf) - 1) = '.'; /* replaces last space */
	strcat(buf, "  ");
    }

    num = 0;
    num = appendp(buf, perceives(pm), "Sees invisible", num);
    num = appendp(buf, control_teleport(pm), "Has teleport control", num);
    num = appendp(buf, your_race(pm), "Is the same race as you", num);
    num = appendp(buf, touch_petrifies(pm), "Petrifies by touch", num);
    num = appendp(buf, touch_disintegrates(pm), "Disintegrates by touch", num);
    if (!(pm->geno & G_NOCORPSE)) {
	if (vegan(pm))
	    num = appendp(buf, TRUE, "May be eaten by vegans", num);
	else if (vegetarian(pm))
	    num = appendp(buf, TRUE, "May be eaten by vegetarians", num);
    }
    /*
     * Unfortunately, keepdogs() is quite mysterious:
     * - Cthulhu and Orcus never follow (M2_STALK and STRAT_WAITFORU)
     * - Vlad follows (also M2_STALK and STRAT_WAITFORU)
     */
    /*
    num = appendp(buf, !!(pm->mflags2 & M2_STALK),
		  "Follows you across levels", num);
    */
    if (polyok(pm))
	num = appendp(buf, TRUE, "Is a valid polymorph form", num);
    else
	num = appendp(buf, TRUE, "Is not a valid polymorph form", num);
    num = appendp(buf, ignores_scary(pm),
		  "Ignores Elbereth engravings and dropped scare monster scrolls",
		  num);

    if (*buf) {
	strcat(buf, ".");
	add_menutext_wrapped(menu, MONDESC_MAX_WIDTH, buf);
    }
}


static const char *mondesc_attack_type(uchar atype)
{
    const char *str = "???";

    switch (atype) {
    case AT_NONE: str = "Passive"; break;
    case AT_CLAW: str = "Claw"; break;
    case AT_BITE: str = "Bite"; break;
    case AT_KICK: str = "Kick"; break;
    case AT_BUTT: str = "Butt"; break;
    case AT_TUCH: str = "Touch"; break;
    case AT_STNG: str = "Sting"; break;
    case AT_HUGS: str = "Hug"; break;
    case AT_SPIT: str = "Spit"; break;
    case AT_ENGL: str = "Engulf"; break;
    case AT_BREA: str = "Breath"; break;
    case AT_EXPL: str = "Explode"; break;
    case AT_BOOM: str = "Explodes when killed"; break;
    case AT_GAZE: str = "Gaze"; break;
    case AT_TENT: str = "Tentacle"; break;

    case AT_WEAP: str = "Weapon"; break;
    case AT_MAGC: str = "Spell-casting"; break;

    default:
	impossible("mondesc_attack_type(): invalid attack type (%d)", atype);
    }

    return str;
}


static const char *mondesc_damage_type(uchar dtype)
{
    const char *str = "???";

    switch (dtype) {
    case AD_PHYS: str = ""; break; /* physical */
    case AD_MAGM: str = "magic missile"; break;
    case AD_FIRE: str = "fire"; break;
    case AD_COLD: str = "cold"; break;
    case AD_SLEE: str = "sleep"; break;
    case AD_DISN: str = "disintegration"; break;
    case AD_ELEC: str = "shock"; break;
    case AD_DRST: str = "poison"; break;
    case AD_ACID: str = "acid"; break;
    case AD_SPC1: str = "buzz1"; break;
    case AD_SPC2: str = "buzz2"; break;
    case AD_BLND: str = "blind"; break;
    case AD_STUN: str = "stun"; break;
    case AD_SLOW: str = "slow"; break;
    case AD_PLYS: str = "paralysis"; break;
    case AD_DRLI: str = "drain life"; break;
    case AD_DREN: str = "drain energy"; break;
    case AD_LEGS: str = "wound legs"; break;
    case AD_STON: str = "petrification"; break;
    case AD_STCK: str = "sticky"; break;
    case AD_SGLD: str = "steal gold"; break;
    case AD_SITM: str = "steal item"; break;
    case AD_SEDU: str = "seduce (steal items)"; break;
    case AD_TLPT: str = "teleport"; break;
    case AD_RUST: str = "erosion"; break;
    case AD_CONF: str = "confusion"; break;
    case AD_DGST: str = "digest"; break;
    case AD_HEAL: str = "heal"; break;
    case AD_WRAP: str = "drowning"; break;
    case AD_WERE: str = "lycanthropy"; break;
    case AD_DRDX: str = "poison (dexterity)"; break;
    case AD_DRCO: str = "poison (constitution)"; break;
    case AD_DRIN: str = "drain intelligence"; break;
    case AD_DISE: str = "disease"; break;
    case AD_DCAY: str = "decays organic items"; break;
    case AD_SSEX: str = "seduce"; break;
    case AD_HALU: str = "hallucinate"; break;
    case AD_DETH: str = "death"; break;
    case AD_PEST: str = "plus disease"; break;
    case AD_FAMN: str = "plus hunger"; break;
    case AD_SLIM: str = "sliming"; break;
    case AD_ENCH: str = "disenchant"; break;
    case AD_CORR: str = "corrosion"; break;
    case AD_HEAD: str = "beheading"; break;

    case AD_CLRC: str = "(clerical)"; break;
    case AD_SPEL: str = ""; break; /* (magical) */
    case AD_RBRE: str = "random"; break;

    case AD_SAMU: str = "artifact stealing"; break;
    case AD_CURS: str = "steal intrinsic"; break;
    default:
	impossible("mondesc_damage_type(): invalid damage type (%d)", dtype);
    }

    return str;
}


static const char *mondesc_one_attack(const struct attack *mattk,
				      const struct permonst *pm)
{
    static char buf[BUFSZ];

    buf[0] = '\0';

    if (!mattk->damn && !mattk->damd && !mattk->aatyp && !mattk->adtyp)
	return buf;

    strcpy(buf, mondesc_attack_type(mattk->aatyp));
    if (mattk->damn || mattk->damd) {
	strcat(buf, " ");
	if (mattk->damn)
	    sprintf(eos(buf), "%d", mattk->damn);
	else
	    strcat(buf, "(level+1)");
	sprintf(eos(buf), "d%d", mattk->damd);
    }

    /* hide breaths of unknown dragons */
    if (mattk->aatyp == AT_BREA && is_unknown_dragon(pm)) {
	strcat(buf, " unknown");
    } else {
	const char *dtmp = mondesc_damage_type(mattk->adtyp);
	if (*dtmp) {
	    strcat(buf, " ");
	    strcat(buf, dtmp);
	}
    }

    return buf;
}


static void mondesc_attacks(struct menulist *menu, const struct permonst *pm)
{
    char buf[BUFSZ] = "";
    const char *tmp;
    int sum[NATTK];
    struct attack mattk, alt_attk;
    int i;

    strcpy(buf, "Attacks: ");
    for (i = 0; i < NATTK; i++) {
	sum[i] = 1; /* show "stun" for e.g. Demogorgon */
	mattk = *getmattk(pm, i, sum, &alt_attk);
	tmp = mondesc_one_attack(&mattk, pm);
	if (!*tmp) {
	    if (!i) strcat(buf, "none");
	    break;
	}
	if (i) strcat(buf, ", ");
	strcat(buf, tmp);
    }
    strcat(buf, ".");

    add_menutext_wrapped(menu, MONDESC_MAX_WIDTH, buf);
}


static void mondesc_all(struct menulist *menu, const struct permonst *pm)
{
    char buf[BUFSZ];

    sprintf(buf, "Difficulty %d, AC %d, magic resistance %d.",
	    monstr[monsndx(pm)], pm->ac, pm->mr);
    add_menutext(menu, buf);
    mondesc_speed(menu, pm->mmove);
    mondesc_generation(menu, pm->geno);
    mondesc_resistances(menu, pm);
    mondesc_flags(menu, pm);
    mondesc_attacks(menu, pm);
}


/*
 * Look in the "data" file for more info.  Called if the user typed in the
 * whole name (user_typed_name == TRUE), or we've found a possible match
 * with a character/glyph.
 */
static void checkfile(const char *inp, struct permonst *pm, boolean user_typed_name,
		      boolean without_asking)
{
    dlb *fp;
    char buf[BUFSZ], newstr[BUFSZ];
    char *ep, *dbase_str;
    long txt_offset;
    int chk_skip;
    boolean found_in_file = FALSE, skipping_entry = FALSE;
    int mntmp;
    char mnname[BUFSZ];
    struct menulist menu;

    fp = dlb_fopen(DATAFILE, "r");
    if (!fp) {
	pline("Cannot open data file!");
	return;
    }

    if (user_typed_name)
	pline("Looking up \"%s\"...", inp);

    /* To prevent the need for entries in data.base like *ngel to account
     * for Angel and angel, make the lookup string the same for both
     * user_typed_name and picked name.
     */
    if (pm != NULL && !user_typed_name)
	dbase_str = strcpy(newstr, mons_mname(pm));
    else dbase_str = strcpy(newstr, inp);
    lcase(dbase_str);

    if (!strncmp(dbase_str, "interior of ", 12))
	dbase_str += 12;
    if (!strncmp(dbase_str, "a ", 2))
	dbase_str += 2;
    else if (!strncmp(dbase_str, "an ", 3))
	dbase_str += 3;
    else if (!strncmp(dbase_str, "the ", 4))
	dbase_str += 4;
    if (!strncmp(dbase_str, "tame ", 5))
	dbase_str += 5;
    else if (!strncmp(dbase_str, "peaceful ", 9))
	dbase_str += 9;
    if (!strncmp(dbase_str, "invisible ", 10))
	dbase_str += 10;
    if (!strncmp(dbase_str, "statue of ", 10))
	dbase_str[6] = '\0';
    else if (!strncmp(dbase_str, "figurine of ", 12))
	dbase_str[8] = '\0';

    /* Make sure the name is non-empty. */
    if (*dbase_str) {
	/* adjust the input to remove " [seen" and "named " and convert to lower case */
	char *alt = 0;	/* alternate description */

	if ((ep = strstri(dbase_str, " [seen")) != 0 ||
	    (ep = strstri(dbase_str, " and more")) != 0)
	    *ep = '\0';

	if ((ep = strstri(dbase_str, " named ")) != 0)
	    alt = ep + 7;
	else
	    ep = strstri(dbase_str, " called ");
	if (!ep) ep = strstri(dbase_str, ", ");
	if (ep && ep > dbase_str) *ep = '\0';

	/*
	 * If the object is named, then the name is the alternate description;
	 * otherwise, the result of makesingular() applied to the name is. This
	 * isn't strictly optimal, but named objects of interest to the user
	 * will usually be found under their name, rather than under their
	 * object type, so looking for a singular form is pointless.
	 */

	if (!alt)
	    alt = makesingular(dbase_str);
	else
	    if (user_typed_name)
		lcase(alt);

	/* skip first record; read second */
	txt_offset = 0L;
	if (!dlb_fgets(buf, BUFSZ, fp) || !dlb_fgets(buf, BUFSZ, fp)) {
	    impossible("can't read 'data' file");
	    dlb_fclose(fp);
	    return;
	} else if (sscanf(buf, "%8lx\n", &txt_offset) < 1 || txt_offset <= 0)
	    goto bad_data_file;

	/* look for the appropriate entry */
	while (dlb_fgets(buf,BUFSZ,fp)) {
	    if (*buf == '.') break;  /* we passed last entry without success */

	    if (digit(*buf)) {
		/* a number indicates the end of current entry */
		skipping_entry = FALSE;
	    } else if (!skipping_entry) {
		if (!(ep = strchr(buf, '\n'))) goto bad_data_file;
		*ep = 0;
		/* if we match a key that begins with "~", skip this entry */
		chk_skip = (*buf == '~') ? 1 : 0;
		if (pmatch(&buf[chk_skip], dbase_str) ||
			(alt && pmatch(&buf[chk_skip], alt))) {
		    if (chk_skip) {
			skipping_entry = TRUE;
			continue;
		    } else {
			found_in_file = TRUE;
			break;
		    }
		}
	    }
	}
    }

    init_menulist(&menu);

    mntmp = name_to_mon(dbase_str);
    if (mntmp >= LOW_PM) {
	strcpy(mnname, mons_mname(&mons[mntmp]));
	lcase(mnname);
	if (!strcmp(dbase_str, mnname)) {
	    dbase_str = strcpy(newstr, mons_mname(&mons[mntmp]));
	    mondesc_all(&menu, &mons[mntmp]);
	}
    }

    if (found_in_file) {
	long entry_offset;
	int  entry_count;
	int  i;

	/* skip over other possible matches for the info */
	do {
	    if (!dlb_fgets(buf, BUFSZ, fp))
		goto bad_data_file;
	} while (!digit(*buf));
	
	if (sscanf(buf, "%ld,%d\n", &entry_offset, &entry_count) < 2) {
bad_data_file:	impossible("'data' file in wrong format");
		free(menu.items);
		dlb_fclose(fp);
		return;
	}

	if (user_typed_name || without_asking || yn("More info?") == 'y') {

	    if (dlb_fseek(fp, txt_offset + entry_offset, SEEK_SET) < 0) {
		pline("? Seek error on 'data' file!");
		free(menu.items);
		dlb_fclose(fp);
		return;
	    }

	    if (menu.icount)
		add_menutext(&menu, "");

	    for (i = 0; i < entry_count; i++) {
		if (!dlb_fgets(buf, BUFSZ, fp))
		    goto bad_data_file;
		if ((ep = strchr(buf, '\n')) != 0)
		    *ep = 0;
		if (strchr(buf+1, '\t') != 0)
		    tabexpand(buf+1);
		add_menutext(&menu, buf+1);
	    }
	}
    } else if (user_typed_name && !menu.icount) {
	pline("I don't have any information on those things.");
    }

    if (menu.icount)
	display_menu(menu.items, menu.icount, upstart(dbase_str), FALSE, NULL);
    free(menu.items);

    dlb_fclose(fp);
}

/*
 * Extract a data.base-friendly word or phrase from an object
 * for use with e.g. checkfile().
 */
static const char *database_oname(struct obj *obj)
{
	const char *dbterm;
	boolean has_name = FALSE;

	if (!obj)
	    return NULL;

	/* Heuristic check for a given name. */
	if (obj->onamelth && (obj->known || obj->dknown)) {
	    const char *oname = ONAME(obj);
	    char c;

	    has_name = TRUE;

	    /* Given names begin with a capital letter. */
	    if ((c = *oname) && !(c >= 'A' && c <= 'Z'))
		has_name = FALSE;

	    /* Given names consist of letters, spaces, "hyphens" (i.e. '-')
	     * and single quotes. */
	    while ((c = *oname) && has_name) {
		if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
		      c == ' ' || c == '-' || c == '\''))
		    has_name = FALSE;
		oname++;
	    }
	}

	if (has_name) {
	    /* Given name may be an artifact, the corpse/tin/statue of
	     * a named pet/monster or named object from a special level. */
	    dbterm = ONAME(obj);
	} else if (obj->otyp == CORPSE ||
		   obj->otyp == STATUE ||
		   obj->otyp == FIGURINE ||
		   (obj->otyp == TIN && obj->spe <= 0 && obj->corpsenm >= LOW_PM &&
		    obj->known) ||
		   (obj->otyp == EGG && obj->corpsenm >= LOW_PM &&
		    (obj->known || mvitals[obj->corpsenm].mvflags & MV_KNOWS_EGG))) {
	    /* Monster-based objects with no given name just go by monster name. */
	    dbterm = mons_mname(&mons[obj->corpsenm]);
	} else {
	    /* Use xname(), but suppress player-provided name(s). */
	    struct objclass *ocl = &objects[obj->otyp];
	    uchar save_onamelth = obj->onamelth;
	    char *save_uname = ocl->oc_uname;

	    obj->onamelth = 0;
	    ocl->oc_uname = NULL;
	    dbterm = xname_single(obj);
	    obj->onamelth = save_onamelth;
	    ocl->oc_uname = save_uname;
	}

	return dbterm;
}

/* getpos() return values */
#define LOOK_TRADITIONAL	0	/* '.' -- ask about "more info?" */
#define LOOK_QUICK		1	/* ',' -- skip "more info?" */
#define LOOK_ONCE		2	/* ';' -- skip and stop looping */
#define LOOK_VERBOSE		3	/* ':' -- show more info w/o asking */

/* also used by getpos hack in do_name.c */
const char what_is_an_unknown_object[] = "an unknown object";

/* quick: use cursor && don't search for "more info" */
static int do_look(boolean quick)
{
    char out_str[BUFSZ];
    char firstmatch[BUFSZ];
    int i, ans = 0, objplur = 0;
    int found;		/* count of matching syms found */
    coord cc;		/* screen pos of unknown glyph */
    boolean save_verbose;	/* saved value of flags.verbose */
    boolean from_screen;	/* question from the screen */
    struct nh_desc_buf descbuf;
    struct obj *otmp;
    struct menulist menu;
    int n, selected[1];

    if (quick) {
	from_screen = TRUE;	/* yes, we want to use the cursor */
    } else {
	i = ynq("Specify unknown object by cursor?");
	if (i == 'q') return 0;
	from_screen = (i == 'y');
    }

    if (from_screen) {
	cc.x = u.ux;
	cc.y = u.uy;
    } else {
	getlin("Specify what? (type the word)", out_str);
	if (out_str[0] == '\0' || out_str[0] == '\033')
	    return 0;

	/* the ability to specify symbols is gone: it is simply impossible to
	 * know how the window port is displaying things (tiles?) and even if
	 * charaters are used it may not be possible to type them (utf8)
	 */
	
	checkfile(out_str, NULL, TRUE, TRUE);
	return 0;
    }
    /* Save the verbose flag, we change it later. */
    save_verbose = flags.verbose;
    flags.verbose = flags.verbose && !quick;
    
    /*
     * we're identifying from the screen.
     */
    do {
	/* Reset some variables. */
	found = 0;
	out_str[0] = '\0';
	objplur = 0;

	if (flags.verbose)
	    pline("Please move the cursor to %s.",
		    what_is_an_unknown_object);
	else
	    pline("Pick an object.");

	ans = getpos(&cc, FALSE, what_is_an_unknown_object);
	if (ans < 0 || cc.x < 0) {
	    flags.verbose = save_verbose;
	    if (flags.verbose)
		pline(quick ? "Never mind." : "Done.");
	    return 0;	/* done */
	}
	flags.verbose = FALSE;	/* only print long question once */

	nh_describe_pos(cc.x, cc.y, &descbuf);
	
	otmp = vobj_at(cc.x, cc.y);
	if (otmp && is_plural(otmp))
	    objplur = 1;

	init_menulist(&menu);
	out_str[0] = '\0';
	if (append_str(out_str, descbuf.effectdesc, 0)) {
	    add_menuitem(&menu, 'e', descbuf.effectdesc, 0, FALSE);
	    if (++found == 1)
		strcpy (firstmatch, descbuf.effectdesc);
	}
	if (append_str(out_str, descbuf.invisdesc, 0)) {
	    add_menuitem(&menu, 'i', descbuf.invisdesc, 0, FALSE);
	    if (++found == 1)
		strcpy (firstmatch, descbuf.invisdesc);
	}
	if (append_str(out_str, descbuf.mondesc, 0)) {
	    add_menuitem(&menu, 'm', descbuf.mondesc, 0, FALSE);
	    if (++found == 1)
		strcpy (firstmatch, descbuf.mondesc);
	}
	if (append_str(out_str, descbuf.objdesc, objplur)) {
	    add_menuitem(&menu, 'o', descbuf.objdesc, 0, FALSE);
	    if (++found == 1)
		strcpy (firstmatch, descbuf.objdesc);
	}
	if (append_str(out_str, descbuf.trapdesc, 0)) {
	    add_menuitem(&menu, 't', descbuf.trapdesc, 0, FALSE);
	    if (++found == 1)
		strcpy (firstmatch, descbuf.trapdesc);
	}
	if (append_str(out_str, descbuf.bgdesc, 0)) {
	    if (!found) {
		add_menuitem(&menu, 'b', descbuf.bgdesc, 0, FALSE);
		found++; /* only increment found if nothing else was seen,
		so that checkfile can be called below */
		strcpy (firstmatch, descbuf.bgdesc);
	    }
	}

	/* Finally, print out our explanation. */
	if (found) {
	    out_str[0] = highc(out_str[0]);
	    pline("%s.", out_str);
	    /* check the data file for information about this thing */
	    if (found > 0 && ans != LOOK_QUICK && ans != LOOK_ONCE &&
			(ans == LOOK_VERBOSE || !quick)) {
		if (found > 1) {
		    n = display_menu(menu.items, menu.icount, "More info?",
				     PICK_ONE, selected);
		    if (n == 1) {
			switch (selected[0]) {
			case 'e': strcpy(firstmatch, descbuf.effectdesc); break;
			case 'i': strcpy(firstmatch, descbuf.invisdesc); break;
			case 'm': strcpy(firstmatch, descbuf.mondesc); break;
			case 'o': strcpy(firstmatch, descbuf.objdesc); break;
			case 't': strcpy(firstmatch, descbuf.trapdesc); break;
			case 'b': strcpy(firstmatch, descbuf.bgdesc); break;
			}
		    }
		} else {
		    n = 1;
		}
		if (n == 1) {
		    /* Fake user_typed_name here when choosing from a menu above
		     * so players get feedback for missing database entries. */
		    checkfile(firstmatch, NULL, found > 1,
			      (ans == LOOK_VERBOSE || found > 1));
		}
	    }
	} else {
	    pline("I've never heard of such things.");
	}

	if (menu.icount)
	    free(menu.items);

	if (quick) check_tutorial_farlook(cc.x, cc.y);
    } while (!quick && ans != LOOK_ONCE);

    flags.verbose = save_verbose;
    if (!quick && flags.verbose)
	pline("Done.");

    return 0;
}


int dowhatis(void)
{
	return do_look(FALSE);
}

int doquickwhatis(void)
{
	check_tutorial_message(QT_T_CURSOR_NUMPAD);
	return do_look(TRUE);
}

int dowhatisinv(struct obj *obj)
{
	static const char allowall[] = { ALL_CLASSES, 0 };

	if (!obj)
	    obj = getobj(allowall, "examine", NULL);
	if (!obj || obj == &zeroobj)
	    return 0;

	/* Fake user_typed_name for feedback on failure to find an entry. */
	checkfile(database_oname(obj), NULL, TRUE, TRUE);

	return 0;
}

int doidtrap(void)
{
	struct trap *trap;
	int x, y, tt;
	schar dx, dy, dz;

	if (!getdir(NULL, &dx, &dy, &dz))
	    return 0;
	
	x = u.ux + dx;
	y = u.uy + dy;
	for (trap = level->lev_traps; trap; trap = trap->ntrap)
	    if (trap->tx == x && trap->ty == y) {
		if (!trap->tseen) break;
		tt = trap->ttyp;
		if (dz) {
		    if (dz < 0 ? (tt == TRAPDOOR || tt == HOLE) :
			    tt == ROCKTRAP) break;
		}
		tt = what_trap(tt);
		pline("That is %s%s%s.",
		      an(trapexplain[tt - 1]),
		      !trap->madeby_u ? "" : (tt == WEB) ? " woven" :
			  /* trap doors & spiked pits can't be made by
			     player, and should be considered at least
			     as much "set" as "dug" anyway */
			  (tt == HOLE || tt == PIT) ? " dug" : " set",
		      !trap->madeby_u ? "" : " by you");
		return 0;
	    }
	pline("I can't see a trap there.");
	return 0;
}


int dolicense(void)
{
	display_file(LICENSE, TRUE);
	return 0;
}


int doverhistory(void)
{
	display_file(HISTORY, TRUE);
	return 0;
}

/*pager.c*/
