/*	Copyright Scott R. Turner, srt@ucla, 10/27/86 */
/* DynaHack may be freely redistributed.  See license for details. */

/* Code for drinking from fountains. */

#include "hack.h"

static void dowatersnakes(void);
static void dowaterdemon(void);
static void dowaternymph(void);
static void gush(int,int,void *);
static void dofindgem(void);

void floating_above(const char *what)
{
    pline("你飘浮于%s之上。", what);
}

static void dowatersnakes(void) /* Fountain of snakes! */
{
    int num = rn1(5,2);
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_MOCCASIN].mvflags & G_GONE)) {
	if (!Blind)
	    pline("一个无尽的%s之泉向外喷涌！",
		  Hallucination ? makeplural(rndmonnam()) : "毒蛇");
	else
	    You_hear("有东西发出嘶嘶声！");
	while (num-- > 0)
	    if ((mtmp = makemon(&mons[PM_WATER_MOCCASIN], level,
			u.ux, u.uy, NO_MM_FLAGS)) && t_at(level, mtmp->mx, mtmp->my))
		mintrap(mtmp);
    } else
	pline("泉水突然一阵猛烈翻腾，然后恢复平静。");
}


static void dowaterdemon(void) /* Water demon */
{
    struct monst *mtmp;

    if (!(mvitals[PM_WATER_DEMON].mvflags & G_GONE)) {
	if ((mtmp = makemon(&mons[PM_WATER_DEMON], level, u.ux, u.uy, NO_MM_FLAGS))) {
	    if (!Blind)
		pline("你放开%s！", a_monnam(mtmp));
	    else
		pline("你感到有邪恶的东西出现。");

	/* Give those on low levels a (slightly) better chance of survival */
	    if (rnd(100) > (80 + level_difficulty(&u.uz))) {
		pline("感谢你释放%s，%s满足你一个愿望！",
		      mhis(level, mtmp), mhe(level, mtmp));
		makewish(FALSE);
		mongone(mtmp);
	    } else if (t_at(level, mtmp->mx, mtmp->my))
		mintrap(mtmp);
	}
    } else
	pline("泉水突然一阵猛烈翻腾，然后恢复平静。");
}


static void dowaternymph(void) /* Water Nymph */
{
	struct monst *mtmp;

	if (!(mvitals[PM_WATER_NYMPH].mvflags & G_GONE) &&
	   (mtmp = makemon(&mons[PM_WATER_NYMPH], level, u.ux, u.uy, NO_MM_FLAGS))) {
		if (!Blind)
		   pline("你被%s吸引了！", a_monnam(mtmp));
		else
		   You_hear("一个魅惑的声音");
		mtmp->msleeping = 0;
		if (t_at(level, mtmp->mx, mtmp->my))
		    mintrap(mtmp);
	} else
		if (!Blind)
		   pline("一个大气泡浮到表面然后爆开。");
		else
		   You_hear("一阵气泡爆破声。");
}

void dogushforth(int drinking) /* Gushing forth along LOS from (u.ux, u.uy) */
{
	int madepool = 0;

	do_clear_area(u.ux, u.uy, 7, gush, &madepool);
	if (!madepool) {
	    if (drinking)
		pline("你不再感到口渴。");
	    else
		pline("泉水洒了你一身。");
	}
}

static void gush(int x, int y, void *poolcnt)
{
	struct monst *mtmp;
	struct trap *ttmp;

	if (((x+y)%2) || (x == u.ux && y == u.uy) ||
	    (rn2(1 + distmin(u.ux, u.uy, x, y)))  ||
	    (level->locations[x][y].typ != ROOM) ||
	    (sobj_at(BOULDER, level, x, y)) || nexttodoor(level, x, y))
		return;

	if ((ttmp = t_at(level, x, y)) != 0 && !delfloortrap(ttmp))
		return;

	if (!((*(int *)poolcnt)++))
	    pline("水从满满的泉中向外喷涌。");

	/* Put a pool at x, y */
	level->locations[x][y].typ = POOL;
	/* No kelp! */
	del_engr_at(level, x, y);
	water_damage(level->objects[x][y], FALSE, TRUE);

	if ((mtmp = m_at(level, x, y)) != 0)
		minliquid(mtmp);
	else
		newsym(x,y);
}

static void dofindgem(void) /* Find a gem in the sparkling waters. */
{
	if (!Blind) pline("你看见翻腾的水中有一个宝石！");
	else pline("你感觉到这有一个宝石！");
	mksobj_at(rnd_class(DILITHIUM_CRYSTAL, LUCKSTONE-1), level,
			 u.ux, u.uy, FALSE, FALSE);
	SET_FOUNTAIN_LOOTED(u.ux,u.uy);
	newsym(u.ux, u.uy);
	exercise(A_WIS, TRUE);			/* a discovery! */
}

void dryup(xchar x, xchar y, boolean isyou)
{
	if (IS_FOUNTAIN(level->locations[x][y].typ) &&
	    (!rn2(3) || FOUNTAIN_IS_WARNED(x,y))) {
		if (isyou && in_town(x, y) && !FOUNTAIN_IS_WARNED(x,y)) {
			struct monst *mtmp;
			SET_FOUNTAIN_WARNED(x,y);
			/* Warn about future fountain use. */
			for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon) {
			    if (DEADMONSTER(mtmp)) continue;
			    if ((mtmp->data == &mons[PM_WATCHMAN] ||
				mtmp->data == &mons[PM_WATCH_CAPTAIN]) &&
			       couldsee(mtmp->mx, mtmp->my) &&
			       mtmp->mpeaceful) {
				pline("%s大叫：", Amonnam(mtmp));
				verbalize("停下，不要再使用这个泉水！");
				break;
			    }
			}
			/* You can see or hear this effect */
			if (!mtmp) pline("流水慢慢地减弱。");
			return;
		}
		
		if (isyou && wizard) {
			if (yn("堵住泉水？") == 'n')
				return;
		}
		
		/* replace the fountain with ordinary floor */
		level->locations[x][y].typ = ROOM;
		level->locations[x][y].looted = 0;
		level->locations[x][y].blessedftn = 0;
		if (cansee(x,y)) pline("泉水干涸了！");
		/* The location is seen if the hero/monster is invisible */
		/* or felt if the hero is blind.			 */
		newsym(x, y);
		level->flags.nfountains--;
		if (isyou && in_town(x, y))
		    angry_guards(FALSE);
	}
}

void drinkfountain(void)
{
	/* What happens when you drink from a fountain? */
	boolean mgkftn = (level->locations[u.ux][u.uy].blessedftn == 1);
	int fate = rnd(30);

	if (Levitation) {
		floating_above("泉水");
		return;
	}

	if (mgkftn && u.uluck >= 0 && fate >= 10) {
		int i, ii, littleluck = (u.uluck < 4);

		pline("哈哈！  这让你感觉棒极了！");
		/* blessed restore ability */
		for (ii = 0; ii < A_MAX; ii++)
		    if (ABASE(ii) < AMAX(ii)) {
			ABASE(ii) = AMAX(ii);
			iflags.botl = 1;
		    }
		/* gain ability, blessed if "natural" luck is high */
		i = rn2(A_MAX);		/* start at a random attribute */
		for (ii = 0; ii < A_MAX; ii++) {
		    if (adjattrib(i, 1, littleluck ? -1 : 0) && littleluck)
			break;
		    if (++i >= A_MAX) i = 0;
		}
		win_pause_output(P_MESSAGE);
		pline("一缕蒸汽从泉水中升起...");
		exercise(A_WIS, TRUE);
		level->locations[u.ux][u.uy].blessedftn = 0;
		return;
	}

	if (fate < 10) {
		pline("一阵干冷使你突然清晰。");
		u.uhunger += rnd(10); /* don't choke on water */
		newuhs(FALSE);
		if (mgkftn) return;
	} else {
	    switch (fate) {

		case 19: /* Self-knowledge */

			pline("你感到自己知识渊博...");
			win_pause_output(P_MESSAGE);
			enlightenment(0);
			exercise(A_WIS, TRUE);
			pline("你感到慢慢消退。");
			break;

		case 20: /* Foul water */

			pline("这水很肮脏！  你感到窒息和恶心。");
			morehungry(rn1(20, 11));
			vomit();
			break;

		case 21: /* Poisonous */

			pline("这水被污染了！");
			if (Poison_resistance) {
			   pline(
			      "也许是从临近的%s农场流出来的。",
				 fruitname(FALSE));
			   losehp(rnd(4),"未被冷藏的一小口果汁",
				KILLED_BY_AN);
			   break;
			}
			losestr(rn1(4,3));
			losehp(rnd(10),"被污染的水", KILLED_BY);
			exercise(A_CON, FALSE);
			break;

		case 22: /* Fountain of snakes! */

			dowatersnakes();
			break;

		case 23: /* Water demon */
			dowaterdemon();
			break;

		case 24: /* Curse an item */ {
			struct obj *obj;

			pline("这水不好！");
			morehungry(rn1(20, 11));
			exercise(A_CON, FALSE);
			for (obj = invent; obj ; obj = obj->nobj)
				if (!rn2(5))	curse(obj);
			break;
			}

		case 25: /* See invisible */

			if (Blind) {
			    if (Invisible) {
				pline("你感觉自己透明了。");
			    } else {
			    	pline("你强烈感知到自我。");
			    	pline("这感觉立即消失了。");
			    }
			} else {
			   pline("你看到一副画面，有人在和你说话。");
			   pline("但是马上又消失了。");
			}
			HSee_invisible |= FROMOUTSIDE;
			newsym(u.ux,u.uy);
			exercise(A_WIS, TRUE);
			break;

		case 26: /* See Monsters */

			monster_detect(NULL, 0);
			exercise(A_WIS, TRUE);
			break;

		case 27: /* Find a gem in the sparkling waters. */

			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}

		case 28: /* Water Nymph */

			dowaternymph();
			break;

		case 29: /* Scare */ {
			struct monst *mtmp;

			pline("这水让你有了口臭！");
			for (mtmp = level->monlist; mtmp; mtmp = mtmp->nmon)
			    if (!DEADMONSTER(mtmp))
				monflee(mtmp, 0, FALSE, FALSE);
			}
			break;

		case 30: /* Gushing forth in this room */

			dogushforth(TRUE);
			break;

		default:

			pline("这温水没有味道。");
			break;
	    }
	}
	dryup(u.ux, u.uy, TRUE);
}

void dipfountain(struct obj *obj)
{
	if (Levitation) {
		floating_above("泉水");
		return;
	}

	/* Don't grant Excalibur when there's more than one object.  */
	/* (quantity could be > 1 if merged daggers got polymorphed) */
	if (obj->otyp == LONG_SWORD && obj->quan == 1L
	    && u.ulevel >= 5 && !rn2(6)
	    && !obj->oartifact
	    && !exist_artifact(LONG_SWORD, artiname(ART_EXCALIBUR))) {

		if (u.ualign.type != A_LAWFUL) {
			/* Ha!  Trying to cheat her. */
			pline("一阵寒冷的雾气从水中升起将剑包裹。");
			pline("泉水消失了！");
			curse(obj);
			if (obj->spe > -6 && !rn2(3)) obj->spe--;
			obj->oerodeproof = FALSE;
			exercise(A_WIS, FALSE);
		} else {
			/* The lady of the lake acts! - Eric Backus */
			/* Be *REAL* nice */
	  pline("从黑暗中伸出一只手为这把剑附上祝福。");
			pline("随着这只手收回，泉水消失了！");
			obj = oname(obj, artiname(ART_EXCALIBUR));
			discover_artifact(ART_EXCALIBUR);
			bless(obj);
			obj->oeroded = obj->oeroded2 = 0;
			obj->oerodeproof = TRUE;
			exercise(A_WIS, TRUE);
		}
		update_inventory();
		level->locations[u.ux][u.uy].typ = ROOM;
		level->locations[u.ux][u.uy].looted = 0;
		newsym(u.ux, u.uy);
		level->flags.nfountains--;
		if (in_town(u.ux, u.uy))
		    angry_guards(FALSE);
		return;
	} else if (get_wet(obj) && !rn2(2))
		return;

	/* Acid and water don't mix */
	if (obj->otyp == POT_ACID) {
	    useup(obj);
	    return;
	}

	switch (rnd(30)) {
		case 16: /* Curse the item */
			curse(obj);
			break;
		case 17:
		case 18:
		case 19:
		case 20: /* Uncurse the item */
			if (obj->cursed) {
			    if (!Blind)
				pline("水在片刻间发光。");
			    uncurse(obj);
			} else {
			    pline("你感到一阵失落。");
			}
			break;
		case 21: /* Water Demon */
			dowaterdemon();
			break;
		case 22: /* Water Nymph */
			dowaternymph();
			break;
		case 23: /* an Endless Stream of Snakes */
			dowatersnakes();
			break;
		case 24: /* Find a gem */
			if (!FOUNTAIN_IS_LOOTED(u.ux,u.uy)) {
				dofindgem();
				break;
			}
		case 25: /* Water gushes forth */
			dogushforth(FALSE);
			break;
		case 26: /* Strange feeling */
			pline("你的%s感到一阵刺痛。",
							body_part(ARM));
			break;
		case 27: /* Strange feeling */
			pline("你感到一阵寒冷。");
			break;
		case 28: /* Strange feeling */
			pline("你突然有了想要洗澡的强烈欲望。");
			{
			    long money = money_cnt(invent);
			    struct obj *otmp;
                            if (money > 10) {
				/* Amount to loose.  Might get rounded up as fountains don't pay change... */
			        money = somegold(money) / 10; 
			        for (otmp = invent; otmp && money > 0; otmp = otmp->nobj) if (otmp->oclass == COIN_CLASS) {
				    int denomination = objects[otmp->otyp].oc_cost;
				    long coin_loss = (money + denomination - 1) / denomination;
                                    coin_loss = min(coin_loss, otmp->quan);
				    otmp->quan -= coin_loss;
				    money -= coin_loss * denomination;				  
				    if (!otmp->quan) delobj(otmp);
				}
			        pline("你在泉水中丢失了一些金钱！");
				CLEAR_FOUNTAIN_LOOTED(u.ux,u.uy);
			        exercise(A_WIS, FALSE);
                            }
			}
			break;
		case 29: /* You see coins */

		/* We make fountains have more coins the closer you are to the
		 * surface.  After all, there will have been more people going
		 * by.	Just like a shopping mall!  Chris Woodbury  */

		    if (FOUNTAIN_IS_LOOTED(u.ux,u.uy)) break;
		    SET_FOUNTAIN_LOOTED(u.ux,u.uy);
		    mkgold((long)
			(rnd((dunlevs_in_dungeon(&u.uz)-dunlev(&u.uz)+1)*2)+5),
			level, u.ux, u.uy);
		    if (!Blind)
		pline("在你下方的深处，你发现有一些金币在水中发光。");
		    exercise(A_WIS, TRUE);
		    newsym(u.ux,u.uy);
		    break;
	}
	update_inventory();
	dryup(u.ux, u.uy, TRUE);
}

void breaksink(int x, int y)
{
    if (cansee(x,y) || (x == u.ux && y == u.uy))
	pline("管道破裂了！  水喷涌出来！");
    level->flags.nsinks--;
    level->locations[x][y].doormask = 0;
    level->locations[x][y].typ = FOUNTAIN;
    level->flags.nfountains++;
    newsym(x,y);
}

void drinksink(void)
{
	struct obj *otmp;
	struct monst *mtmp;

	if (Levitation) {
		floating_above("水槽");
		return;
	}
	switch(rn2(20)) {
		case 0: pline("你喝了一小口非常寒冷的水。");
			break;
		case 1: pline("你喝了一小口非常温暖的水");
			break;
		case 2: pline("你喝了一小口滚烫的水");
			if (Fire_resistance)
				pline("尝起来似乎非常美味。");
			else losehp(rnd(6), "啜饮沸水", KILLED_BY);
			break;
		case 3: if (mvitals[PM_SEWER_RAT].mvflags & G_GONE)
				pline("水槽看起来有点脏。");
			else {
				mtmp = makemon(&mons[PM_SEWER_RAT], level,
						u.ux, u.uy, NO_MM_FLAGS);
				if (mtmp) pline("呀！  有%s在水槽中！",
					(Blind || !canspotmon(level, mtmp)) ?
					"蠕动的东西" :
					a_monnam(mtmp));
			}
			break;
		case 4: do {
				otmp = mkobj(level, POTION_CLASS,FALSE);
				if (otmp->otyp == POT_WATER) {
					obfree(otmp, NULL);
					otmp = NULL;
				}
			} while (!otmp);
			otmp->cursed = otmp->blessed = 0;
			pline("一些%s从水龙头中流出。",
			      Blind ? "奇怪的" :
			      hcolor(OBJ_DESCR(objects[otmp->otyp])));
			otmp->dknown = !(Blind || Hallucination);
			otmp->quan++; /* Avoid panic upon useup() */
			otmp->fromsink = 1; /* kludge for docall() */
			dopotion(otmp);
			obfree(otmp, NULL);
			break;
		case 5: if (!(level->locations[u.ux][u.uy].looted & S_LRING)) {
			    pline("你在水槽中找到一枚戒指！");
			    mkobj_at(RING_CLASS, level, u.ux, u.uy, TRUE);
			    level->locations[u.ux][u.uy].looted |= S_LRING;
			    exercise(A_WIS, TRUE);
			    newsym(u.ux,u.uy);
			} else pline("一些污水流入了下水道中。");
			break;
		case 6: breaksink(u.ux,u.uy);
			break;
		case 7: pline("水开始自主地流动起来！");
			if ((mvitals[PM_WATER_ELEMENTAL].mvflags & G_GONE)
			    || !makemon(&mons[PM_WATER_ELEMENTAL], level,
					u.ux, u.uy, NO_MM_FLAGS))
				pline("但是马上恢复了正常。");
			break;
		case 8: pline("呕，这水真难喝。");
			more_experienced(1, 1, 0);
			newexplevel();
			break;
		case 9: pline("哇... 这尝起来像污水！  你呕吐了。");
			morehungry(rn1(30-ACURR(A_CON), 11));
			vomit();
			break;
		case 10: pline("这水里含有毒物！");
			if (!Unchanging) {
				pline("你忍受者有毒的污水！");
				polyself(FALSE);
			}
			break;
		/* more odd messages --JJB */
		case 11: You_hear("管道中发出叮叮咚咚的声音...");
			break;
		case 12: You_hear("下水道中传来一阵歌声...");
			break;
		case 19: if (Hallucination) {
		   pline("从黑暗的管道中伸出一只手... 啊！！！");
				break;
			}
		default: pline("你喝了一小口%s水。",
			rn2(3) ? (rn2(2) ? "冷" : "温") : "热");
	}
}

/*fountain.c*/
