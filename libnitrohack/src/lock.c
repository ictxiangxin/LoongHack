/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"

static int picklock(void);
static int forcelock(void);

/* at most one of `door' and `box' should be non-null at any given time */
static struct xlock_s {
	struct rm  *door;
	struct obj *box;
	int picktyp, chance, usedtime;
	boolean loot_unlocked;
	schar door_x, door_y; /* *only* for restoring, never valid otherwise */
} xlock;

static schar picklock_dx, picklock_dy;

static const char *lock_action(void);
static boolean obstructed(int,int);
static void chest_shatter_msg(struct obj *);

boolean picking_lock(int *x, int *y)
{
	if (occupation == picklock) {
	    *x = u.ux + picklock_dx;
	    *y = u.uy + picklock_dy;
	    return TRUE;
	} else {
	    *x = *y = 0;
	    return FALSE;
	}
}

boolean picking_at(int x, int y)
{
	return (occupation == picklock && xlock.door == &level->locations[x][y]);
}

/* produce an occupation string appropriate for the current activity */
static const char *lock_action(void)
{
	/* "unlocking"+2 == "locking" */
	static const char *const actions[] = {
		/* [0] */	"打开门",
		/* [1] */	"打开宝箱",
		/* [2] */	"打开箱子",
		/* [3] */	"安全破开",
		/* [4] */	"卸下锁"
	};

	/* if the target is currently unlocked, we're trying to lock it now */
	if (xlock.door && !(xlock.door->doormask & D_LOCKED))
		return actions[0]+2;	/* "locking the door" */
	else if (xlock.box && !xlock.box->olocked)
		return xlock.box->otyp == CHEST ? actions[1]+2 :
			xlock.box->otyp == IRON_SAFE ? actions[3]+2 : actions[2]+2;
	/* otherwise we're trying to unlock it */
	else if (xlock.picktyp == LOCK_PICK)
		return actions[4];	/* "picking the lock" */
	else if (xlock.picktyp == CREDIT_CARD)
		return actions[4];	/* same as lock_pick */
	else if (xlock.door)
		return actions[0];	/* "unlocking the door" */
	else
		return xlock.box->otyp == CHEST ? actions[1] :
			xlock.box->otyp == IRON_SAFE ? actions[3] : actions[2];
}

/* try to open/close a lock */
static int picklock(void)
{
	xchar x, y;
	if (xlock.box) {
	    if (((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy)) &&
		    (xlock.box->otyp != IRON_SAFE || abs(xlock.box->oy - u.uy) > 1 ||
		     abs(xlock.box->ox - u.ux) > 1)) {
		return (xlock.usedtime = 0);		/* you or it moved */
	    }
	} else {		/* door */
	    if (xlock.door != &(level->locations[u.ux+picklock_dx][u.uy+picklock_dy])) {
		return (xlock.usedtime = 0);		/* you moved */
	    }
	    switch (xlock.door->doormask) {
		case D_NODOOR:
		    pline("这个门廊没有门。");
		    return (xlock.usedtime = 0);
		case D_ISOPEN:
		    pline("你不能锁上一个开着的门。");
		    return (xlock.usedtime = 0);
		case D_BROKEN:
		    pline("这扇门坏了。");
		    return (xlock.usedtime = 0);
	    }
	}

	if (xlock.usedtime++ >= 50 || nohands(youmonst.data)) {
	    pline("你放弃尝试%s.", lock_action());
	    exercise(A_DEX, TRUE);	/* even if you don't succeed */
	    return (xlock.usedtime = 0);
	}

	if (rn2(100) >= xlock.chance) return 1;		/* still busy */

	pline("你成功进入%s.", lock_action());
	if (xlock.door) {
	    if (xlock.door->doormask & D_TRAPPED) {
		    b_trapped("门", FINGER);
		    xlock.door->doormask = D_NODOOR;
		    unblock_point(u.ux+picklock_dx, u.uy+picklock_dy);
		    if (*in_rooms(level, u.ux+picklock_dx, u.uy+picklock_dy, SHOPBASE))
			add_damage(u.ux+picklock_dx, u.uy+picklock_dy, 0L);
		    newsym(u.ux+picklock_dx, u.uy+picklock_dy);
	    } else if (xlock.door->doormask & D_LOCKED)
		xlock.door->doormask = D_CLOSED;
	    else xlock.door->doormask = D_LOCKED;
	    /*
	     * Player now knows the door's open/closed status, and its
	     * locked/unlocked status, and also that it isn't trapped
	     * (it would have exploded otherwise); we haven't recorded
	     * the location of the door being picked, so scan for it.
	     */
	    for (x = 1; x < COLNO; x++) {
		for (y = 0; y < ROWNO; y++) {
		    if (picking_at(x, y))
			magic_map_background(x, y, TRUE);
		}
	    }
	} else {
	    xlock.box->olocked = !xlock.box->olocked;
	    if (xlock.box->otrapped)
		chest_trap(xlock.box, FINGER, FALSE);
	    else if (!xlock.box->olocked && xlock.loot_unlocked)
		use_container(xlock.box, 0);
	}
	exercise(A_DEX, TRUE);
	return (xlock.usedtime = 0);
}

/* try to force a locked chest */
static int forcelock(void)
{

	struct obj *otmp;

	if ((xlock.box->ox != u.ux) || (xlock.box->oy != u.uy))
		return (xlock.usedtime = 0);		/* you or it moved */

	if (xlock.usedtime++ >= 50 || !uwep || nohands(youmonst.data)) {
	    pline("你放弃强行破开这把锁。");
	    if (xlock.usedtime >= 50)		/* you made the effort */
	      exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	    return (xlock.usedtime = 0);
	}

	if (xlock.picktyp) {	/* blade */

	    if (rn2(1000-(int)uwep->spe) > (992-greatest_erosion(uwep)*10) &&
	       !uwep->cursed && !obj_resists(uwep, 0, 99)) {
		/* for a +0 weapon, probability that it survives an unsuccessful
		 * attempt to force the lock is (.992)^50 = .67
		 */
		pline("你的%s%s坏了！",
		      (uwep->quan > 1L) ? "其中一个" : "", xname(uwep));
		useup(uwep);
		pline("你放弃强行破开这把锁。");
		exercise(A_DEX, TRUE);
		return (xlock.usedtime = 0);
	    }
	} else			/* blunt */
	    wake_nearby();	/* due to hammering on the container */

	if (rn2(100) >= xlock.chance) return 1;		/* still busy */

	pline("你成功破开这把锁。");
	xlock.box->olocked = 0;
	xlock.box->obroken = 1;
	if (!xlock.picktyp && !rn2(3)) {
	    struct monst *shkp;
	    boolean costly;
	    long loss = 0L;

	    costly = (*u.ushops && costly_spot(u.ux, u.uy));
	    shkp = costly ? shop_keeper(level, *u.ushops) : 0;

	    pline("实际上，你完全破坏了%s。",
		  the(xname(xlock.box)));

	    /* Put the contents on ground at the hero's feet. */
	    while ((otmp = xlock.box->cobj) != 0) {
		obj_extract_self(otmp);
		if (!rn2(3) || otmp->oclass == POTION_CLASS) {
		    chest_shatter_msg(otmp);
		    if (costly)
		        loss += stolen_value(otmp, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
		    if (otmp->quan == 1L) {
			obfree(otmp, NULL);
			continue;
		    }
		    useup(otmp);
		}
		if (xlock.box->otyp == ICE_BOX && otmp->otyp == CORPSE) {
		    otmp->age = moves - otmp->age; /* actual age */
		    start_corpse_timeout(otmp);
		}
		place_object(otmp, level, u.ux, u.uy);
		stackobj(otmp);
	    }

	    if (costly)
		loss += stolen_value(xlock.box, u.ux, u.uy,
					     (boolean)shkp->mpeaceful, TRUE);
	    if (loss) pline("你因损坏物品而欠下%ld%s。", loss, currency(loss));
	    delobj(xlock.box);
	}
	exercise((xlock.picktyp) ? A_DEX : A_STR, TRUE);
	return (xlock.usedtime = 0);
}

void reset_pick(void)
{
	xlock.usedtime = xlock.chance = xlock.picktyp = 0;
	xlock.loot_unlocked = FALSE;
	xlock.door = 0;
	xlock.box = 0;

	picklock_dx = picklock_dy = 0;
}

void save_pick(struct memfile *mf)
{
	schar door_x = 0;
	schar door_y = 0;
	unsigned int box_id = 0;

	/* avoid dereferencing dangling pointers here */
	if (xlock.usedtime) {
	    if (xlock.door) {
		/* make sure the door is on the current level */
		if (&level->locations[0][0] <= xlock.door &&
		    xlock.door < &level->locations[COLNO][ROWNO]) {
		    door_x = (&level->locations[0][0] - xlock.door) % COLNO;
		    door_y = (&level->locations[0][0] - xlock.door) / COLNO;
		}
	    } else {
		box_id = xlock.box->o_id;
	    }
	}

	mwrite32(mf, xlock.picktyp);
	mwrite32(mf, xlock.chance);
	mwrite32(mf, xlock.usedtime);
	mwrite8(mf, xlock.loot_unlocked);
	mwrite8(mf, door_x);
	mwrite8(mf, door_y);
	mwrite32(mf, box_id);

	mwrite8(mf, picklock_dx);
	mwrite8(mf, picklock_dy);
}

void restore_pick(struct memfile *mf)
{
	unsigned int box_id;

	xlock.picktyp = mread32(mf);
	xlock.chance = mread32(mf);
	xlock.usedtime = mread32(mf);
	xlock.loot_unlocked = mread8(mf);

	/* used by restore_pick_fix() to restore xlock.door */
	xlock.door_x = mread8(mf);
	xlock.door_y = mread8(mf);

	box_id = mread32(mf);
	xlock.box = box_id ? find_oid(box_id) : NULL;

	picklock_dx = mread8(mf);
	picklock_dy = mread8(mf);
}

/* restore xlock.door pointer from partial data gathered by restore_pick() */
/* must be called after 'level' has been set, since xlock.door points within it */
void restore_pick_fix(void)
{
	if (xlock.usedtime && !xlock.box)
	    xlock.door = &level->locations[xlock.door_x][xlock.door_y];
}

/* pick a lock with a given object */
/* explicit = mention tool being used explicitly */
/* loot_after = #loot after successfully unlocking a box */
int pick_lock(struct obj *pick, int rx, int ry, boolean explicit, boolean loot_after)
{
	/* rx and ry are passed only from the use-stethoscope stuff */
	int picktyp, c, ch;
	coord cc;
	struct rm	*door;
	struct obj	*otmp;
	char qbuf[QBUFSZ];
	schar dx, dy, dz = 0;

	picktyp = pick->otyp;

	/* check whether we're resuming an interrupted previous attempt */
	if (xlock.usedtime && picktyp == xlock.picktyp) {
	    static const char no_longer[] = "不幸的是，你不能一直%s%s。";

	    if (nohands(youmonst.data)) {
		const char *what = (picktyp == LOCK_PICK) ? "开锁器" : "钥匙";
		if (picktyp == CREDIT_CARD) what = "信用卡";
		if (picktyp == STETHOSCOPE) what = "听诊器";
		pline(no_longer, "拿着", what);
		reset_pick();
		return 0;
	    } else if (xlock.box && !can_reach_floor()) {
		pline(no_longer, "触碰这把", "锁");
		reset_pick();
		return 0;
	    } else {
		const char *action = lock_action();
		pline("你重新尝试%s。", action);
		set_occupation(picklock, action, 0);
		return 1;
	    }
	}

	if (nohands(youmonst.data)) {
		pline("你不能握住%s————你没有手！", doname(pick));
		return 0;
	}

	if (picktyp != LOCK_PICK &&
		picktyp != STETHOSCOPE &&
		picktyp != CREDIT_CARD &&
		picktyp != SKELETON_KEY) {
		warning("用%d取下锁？", picktyp);
		return 0;
	}
	ch = 0;		/* lint suppression */

	/* If this is a stethoscope, we know where we came from. */
	if (rx != 0 && ry != 0) {
	    cc.x = rx;
	    cc.y = ry;
	} else {
	    if (!get_adjacent_loc(NULL, "无效的位置！", u.ux, u.uy, &cc, &dz))
		return 0;
	}

	dx = cc.x - u.ux;
	dy = cc.y - u.uy;

	/* Very clumsy special case for this, but forcing the player to
	 * a)pply > just to open a safe, when a)pply . works in all other cases? */
	if ((cc.x == u.ux && cc.y == u.uy) || picktyp == STETHOSCOPE) {	/* pick lock on a container */
	    const char *verb;
	    boolean it;
	    int count;

	    if (dz < 0) {
		pline("%s没有任何一种锁。",
		      Levitation ? "在这" : "在那");
		return 0;
	    } else if (is_lava(level, u.ux, u.uy)) {
		pline("这样做会使你的%s熔化。",
		      xname(pick));
		return 0;
	    } else if (is_pool(level, u.ux, u.uy) && !Underwater) {
		pline("水没有锁。");
		return 0;
	    }

	    count = 0;
	    c = 'n';			/* in case there are no boxes here */
	    for (otmp = level->objects[cc.x][cc.y]; otmp; otmp = otmp->nexthere)
		if (Is_box(otmp)) {
		    ++count;
		    if (!can_reach_floor()) {
			pline("你不能从这里碰到%s。", the(xname(otmp)));
			return 0;
		    }
		    it = 0;
		    if (otmp->obroken) verb = "修复";
		    else if (otmp->otyp == IRON_SAFE) verb = "破开", it = 1;
		    else if (!otmp->olocked) verb = "锁上", it = 1;
		    else if (picktyp != LOCK_PICK) verb = "解开", it = 1;
		    else verb = "pick";
		    sprintf(qbuf, "这有一个%s，%s%s？",
			    safe_qbuf("", sizeof("这有一个，打开它的锁？"),
				      doname(otmp), an(simple_typename(otmp->otyp)), "一个箱子"),
			    verb, it ? "它" : "它上面的锁");

		    c = ynq(qbuf);
		    if (c == 'q') return 0;
		    if (c == 'n') continue;

		    if (otmp->otyp == IRON_SAFE && picktyp != STETHOSCOPE) {
			pline("你不知道如何去打开保险箱。");
			return 0;
		    }
		    if (!otmp->olocked && otmp->otyp == IRON_SAFE) {
			pline("你不能改变它的组合方式。");
			return 0;
		    }

		    if (otmp->obroken) {
			pline("你不能用%s修好这把破锁。", doname(pick));
			return 0;
		    }
		    else if (picktyp == CREDIT_CARD && !otmp->olocked) {
			/* credit cards are only good for unlocking */
			pline("你不能用%s那样做。", doname(pick));
			return 0;
		    }
		    switch(picktyp) {
			case CREDIT_CARD:
			    ch = ACURR(A_DEX) + 20*Role_if (PM_ROGUE);
			    break;
			case LOCK_PICK:
			    ch = 4*ACURR(A_DEX) + 25*Role_if (PM_ROGUE);
			    break;
			case SKELETON_KEY:
			    ch = 75 + ACURR(A_DEX);
			    break;
			case STETHOSCOPE:
			    ch = 5 + 2*ACURR(A_DEX)*Role_if(PM_ROGUE);
			    break;
			default:	ch = 0;
		    }
		    if (otmp->cursed) ch /= 2;

		    xlock.picktyp = picktyp;
		    xlock.box = otmp;
		    xlock.door = 0;
		    xlock.loot_unlocked = loot_after;
		    break;
		}
	    if (c != 'y') {
		if (!count)
		    pline("There doesn't seem to be any sort of pickable lock here.");
		return 0;		/* decided against all boxes */
	    }
	} else {			/* pick the lock in a door */
	    struct monst *mtmp;

	    if (u.utrap && u.utraptype == TT_PIT) {
		pline("你不能越过这个深坑的边缘。");
		return 0;
	    }

	    door = &level->locations[cc.x][cc.y];
	    if ((mtmp = m_at(level, cc.x, cc.y)) && canseemon(level, mtmp)
			&& mtmp->m_ap_type != M_AP_FURNITURE
			&& mtmp->m_ap_type != M_AP_OBJECT) {
		if (picktyp == CREDIT_CARD &&
		    (mtmp->isshk || mtmp->data == &mons[PM_ORACLE]))
		    verbalize("不收支票，不赊帐，就没问题。");
		else
		    pline("你不认为%s会欣赏那样。", mon_nam(mtmp));
		return 0;
	    }
	    if (!IS_DOOR(door->typ)) {
		if (is_drawbridge_wall(cc.x,cc.y) >= 0)
		    pline("你%s这吊桥没有锁。",
				Blind ? "感觉" : "看到");
		else
		    pline("你%s不到这有门。",
				Blind ? "感觉" : "看");
		return 0;
	    }
	    switch (door->doormask) {
		case D_NODOOR:
		    pline("这个门廊没有门。");
		    return 0;
		case D_ISOPEN:
		    pline("你不能解锁然后打开这扇门。");
		    return 0;
		case D_BROKEN:
		    pline("这扇门坏了。");
		    return 0;
		default:
		    /* credit cards are only good for unlocking */
		    if (picktyp == CREDIT_CARD && !(door->doormask & D_LOCKED)) {
			pline("你不能用信用卡去锁门。");
			return 0;
		    }

		    /*
		     * At this point, the player knows that the door
		     * is a door, and whether it's locked, but not
		     * whether it's trapped; to do this, we set the
		     * mem_door_l flag and call map_background(),
		     * which will clear it if necessary (i.e. not a door
		     * after all).
		     */
		    level->locations[cc.x][cc.y].mem_door_l = 1;
		    map_background(cc.x, cc.y, TRUE);

		    sprintf(qbuf,"%s%s%s？",
		    explicit ? "用" : "",
			explicit ? doname(pick) : "",
			(door->doormask & D_LOCKED) ? "解锁" : "锁上");

		    c = yn(qbuf);
		    if (c == 'n') return 0;

		    switch(picktyp) {
			case CREDIT_CARD:
			    ch = 2*ACURR(A_DEX) + 20*Role_if (PM_ROGUE);
			    break;
			case LOCK_PICK:
			    ch = 3*ACURR(A_DEX) + 30*Role_if (PM_ROGUE);
			    break;
			case SKELETON_KEY:
			    ch = 70 + ACURR(A_DEX);
			    break;
			default:    ch = 0;
		    }
		    xlock.door = door;
		    xlock.box = 0;
		    xlock.loot_unlocked = FALSE;

		    /* ALI - Artifact doors */
		    if (artifact_door(level, cc.x, cc.y)) {
			if (picktyp == SKELETON_KEY) {
			    pline("你的钥匙似乎不对。");
			    return 0;
			} else {
			    /* -1 == 0% chance */
			    ch = -1;
			}
		    }
	    }
	}
	flags.move = 0;
	xlock.chance = ch;
	xlock.picktyp = picktyp;
	xlock.usedtime = 0;
	picklock_dx = dx;
	picklock_dy = dy;
	set_occupation(picklock, lock_action(), 0);
	return 1;
}

/* try to force a chest with your weapon */
int doforce(void)
{
	struct obj *otmp;
	int c, picktyp;
	char qbuf[QBUFSZ];

	if (!uwep ||	/* proper type test */
	    (uwep->oclass != WEAPON_CLASS && !is_weptool(uwep) &&
	     uwep->oclass != ROCK_CLASS) ||
	    (objects[uwep->otyp].oc_skill < P_DAGGER) ||
	    (objects[uwep->otyp].oc_skill > P_LANCE) ||
	    uwep->otyp == FLAIL || uwep->otyp == AKLYS ||
	    uwep->otyp == RUBBER_HOSE) {
	    pline("你不能在没有%s武器时强开任何东西。", (uwep) ? "适当的" : "");
	    return 0;
	}

	picktyp = is_blade(uwep);
	if (xlock.usedtime && xlock.box && picktyp == xlock.picktyp) {
	    pline("你再次尝试强行开锁。");
	    set_occupation(forcelock, "强行开锁", 0);
	    return 1;
	}

	/* A lock is made only for the honest man, the thief will break it. */
	xlock.box = NULL;
	for (otmp = level->objects[u.ux][u.uy]; otmp; otmp = otmp->nexthere)
	    if (Is_box(otmp)) {
		if (otmp->otyp == IRON_SAFE) {
		    pline("你没办法强行打开%s", the(xname(otmp)));
		    continue;
		}
		if (otmp->obroken || !otmp->olocked) {
		    pline("这有一个%s，但是它的锁已经%s。",
			  doname(otmp), otmp->obroken ? "坏了" : "打开了");
		    continue;
		}
		sprintf(qbuf,"这有一个%s，强行打开它的锁？",
			safe_qbuf("", sizeof("这有一个，强行打开它的锁？"),
				doname(otmp), an(simple_typename(otmp->otyp)),
				"一个箱子"));

		c = ynq(qbuf);
		if (c == 'q') return 0;
		if (c == 'n') continue;

		if (picktyp)
		    pline("你强行撬开%s。", xname(uwep));
		else
		    pline("你开始用你的%s猛击它。", xname(uwep));
		xlock.box = otmp;
		xlock.chance = objects[uwep->otyp].oc_wldam * 2;
		xlock.picktyp = picktyp;
		xlock.usedtime = 0;
		break;
	    }

	if (xlock.box)	set_occupation(forcelock, "强行开锁", 0);
	else		pline("你决定不去强行解决。");
	return 1;
}

/* try to open a door */
int doopen(int dx, int dy, int dz)
{
	coord cc;
	struct rm *door;
	struct monst *mtmp;
	boolean got_dir = FALSE;
	schar unused;

	if (nohands(youmonst.data)) {
	    pline("你不能打开、关闭或者解锁任何东西————你没有手！");
	    return 0;
	}

	if (u.utrap && u.utraptype == TT_PIT) {
	    pline("你不能越过这个深坑的边缘。");
	    return 0;
	}
	
	if (dx != -2 && dy != -2 && dz != -2) { /* -2 signals no direction given  */
	    cc.x = u.ux + dx;
	    cc.y = u.uy + dy;
	    if (isok(cc.x, cc.y))
		got_dir = TRUE;
	}

	if (!got_dir && !get_adjacent_loc(NULL, NULL, u.ux, u.uy, &cc, &unused))
	    return 0;

        if (!got_dir) {
            dx = cc.x-u.ux;
            dy = cc.y-u.uy;
            dz = 0;
        }

	if ((cc.x == u.ux) && (cc.y == u.uy))
	    return 0;

	if ((mtmp = m_at(level, cc.x,cc.y))		&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp, cc.x-u.ux, cc.y-u.uy);
	    return 1;
	}

	door = &level->locations[cc.x][cc.y];

	if (!IS_DOOR(door->typ)) {
		if (is_db_wall(cc.x,cc.y)) {
		    pline("没有方法来打开这个吊桥。");
		    return 0;
		}
		pline("你%s这有门。",
				Blind ? "感觉不到" : "看不到");
		return 0;
	}

        if (door->doormask == D_ISOPEN)
            return doclose(dx, dy, dz);

	if (!(door->doormask & D_CLOSED)) {
	    const char *mesg;
	    int locked = FALSE;

	    switch (door->doormask) {
	    case D_BROKEN: mesg = "被破坏"; break;
	    case D_NODOOR: mesg = "没有门"; break;
	    case D_ISOPEN: mesg = "已经打开了"; break;
	    default:
		door->mem_door_l = 1;
		map_background(cc.x, cc.y, TRUE);
		mesg = "锁住了";
		locked = TRUE;
		break;
	    }
	    pline("这扇门%s。", mesg);
	    if (Blind) feel_location(cc.x,cc.y);
	    if (locked && !flags.run) {
		struct obj *otmp = NULL;
		if (flags.autounlock &&
		    ((otmp = carrying(SKELETON_KEY)) ||
		     (otmp = carrying(LOCK_PICK)) ||
		     (otmp = carrying(CREDIT_CARD))))
		    return pick_lock(otmp, cc.x, cc.y, TRUE, FALSE);
	    }
	    return 0;
	}

	if (verysmall(youmonst.data)) {
	    pline("你太弱了，不能拉开这扇门。");
	    return 0;
	}

	/* door is known to be CLOSED */
	pline("门打开了。");
	if (door->doormask & D_TRAPPED) {
	    b_trapped("门", FINGER);
	    door->doormask = D_NODOOR;
	    if (*in_rooms(level, cc.x, cc.y, SHOPBASE)) add_damage(cc.x, cc.y, 0L);
	} else
	    door->doormask = D_ISOPEN;
	if (Blind)
	    feel_location(cc.x,cc.y);	/* the hero knows she opened it  */
	else
	    newsym(cc.x,cc.y);
	unblock_point(cc.x,cc.y);	/* vision: new see through there */

	return 1;
}

static boolean obstructed(int x, int y)
{
	struct monst *mtmp = m_at(level, x, y);

	if (mtmp && mtmp->m_ap_type != M_AP_FURNITURE) {
		if (mtmp->m_ap_type == M_AP_OBJECT) goto objhere;
		pline("%s挡在路上！", !canspotmon(level, mtmp) ?
			"不明生物" : Monnam(mtmp));
		if (!canspotmon(level, mtmp))
		    map_invisible(mtmp->mx, mtmp->my);
		return TRUE;
	}
	if (OBJ_AT(x, y)) {
objhere:	pline("有什么东西挡住路了。");
		return TRUE;
	}
	return FALSE;
}

/* try to close a door */
int doclose(int dx, int dy, int dz)
{
        boolean got_dir = FALSE;
	struct rm *door;
	struct monst *mtmp;
        coord cc;
        schar unused;

	if (nohands(youmonst.data)) {
	    pline("你不能关闭任何东西————你没有手！");
	    return 0;
	}

	if (u.utrap && u.utraptype == TT_PIT) {
	    pline("你不能越过这个深坑的边缘。");
	    return 0;
	}

	if (dx != -2 && dy != -2 && dz != -2) { /* -2 signals no direction given  */
	    cc.x = u.ux + dx;
	    cc.y = u.uy + dy;
	    if (isok(cc.x, cc.y))
		got_dir = TRUE;
	}

	if (!got_dir && !get_adjacent_loc(NULL, NULL, u.ux, u.uy, &cc, &unused))
	    return 0;

        if (!got_dir) {
          dx = cc.x-u.ux;
          dy = cc.y-u.uy;
          dz = 0;
        }

	if ((cc.x == u.ux) && (cc.y == u.uy)) {
		pline("你挡住路了！");
		return 1;
	}

	if ((mtmp = m_at(level, cc.x, cc.y))		&&
		mtmp->m_ap_type == M_AP_FURNITURE	&&
		(mtmp->mappearance == S_hcdoor ||
			mtmp->mappearance == S_vcdoor)	&&
		!Protection_from_shape_changers)	 {

	    stumble_onto_mimic(mtmp, dx, dy);
	    return 1;
	}

	door = &level->locations[cc.x][cc.y];

	if (!IS_DOOR(door->typ)) {
		if (door->typ == DRAWBRIDGE_DOWN)
		    pline("没有方法去锁上这个吊桥。");
		else
		    pline("你%s这没门。",
				Blind ? "感觉到" : "看见");
		return 0;
	}

	if (door->doormask == D_NODOOR) {
	    pline("这个门廊没有门。");
	    return 0;
	}

	if (obstructed(cc.x, cc.y)) return 0;

	if (door->doormask == D_BROKEN) {
	    pline("这扇门坏了。");
	    return 0;
	}

	if (door->doormask & (D_CLOSED | D_LOCKED)) {
	    pline("这扇门已经关上了。");
	    return 0;
	}

	if (door->doormask == D_ISOPEN) {
	    if (verysmall(youmonst.data) && !u.usteed) {
		 pline("你太弱了，不能推动门板关上它。");
		 return 0;
	    }
	    if (u.usteed ||
		rn2(25) < (ACURRSTR+ACURR(A_DEX)+ACURR(A_CON))/3) {
		pline("门关上了。");
		door->doormask = D_CLOSED;
		door->mem_door_l = 1;
		/*
		 * map_background() here sets the mem_door flags correctly;
		 * and it's redundant to both feel_location() and newsym()
		 * with a door.
		 *
		 * Exception: if we remember an invisible monster on the
		 * door square, but in this case, we want to set the
		 * memory of a door there anyway because we know there's a
		 * door there because we just closed it, and with layered
		 * memory this doesn't clash with keeping the I there.
		 */
		map_background(cc.x, cc.y, TRUE);
		if (Blind)
		    feel_location(cc.x,cc.y);	/* the hero knows she closed it */
		else
		    newsym(cc.x,cc.y);
		block_point(cc.x,cc.y);	/* vision:  no longer see there */
	    }
	    else {
	        exercise(A_STR, TRUE);
	        pline("这门很坚固！");
	    }
	}

	return 1;
}

/* box obj was hit with spell effect otmp */
/* returns true if something happened */
boolean boxlock(struct obj *obj, struct obj *otmp)
{
	boolean res = 0;

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    if (!obj->olocked) {	/* lock it; fix if broken */
		pline("咚！");
		obj->olocked = 1;
		obj->obroken = 0;
		res = 1;
	    } /* else already closed and locked */
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (obj->olocked) {		/* unlock; couldn't be broken */
		pline("喀！");
		obj->olocked = 0;
		res = 1;
	    } else			/* silently fix if broken */
		obj->obroken = 0;
	    break;
	case WAN_POLYMORPH:
	case SPE_POLYMORPH:
	    /* maybe start unlocking chest, get interrupted, then zap it;
	       we must avoid any attempt to resume unlocking it */
	    if (xlock.box == obj)
		reset_pick();
	    break;
	}
	return res;
}

/* Door/secret door was hit with spell effect otmp */
/* returns true if something happened */
boolean doorlock(struct obj *otmp, int x, int y)
{
	struct rm *door = &level->locations[x][y];
	boolean res = TRUE;
	int loudness = 0;
	const char *msg = NULL;
	const char *dustcloud = "一团尘埃";
	const char *quickly_dissipates = "快速消散";
	int key = artifact_door(level, x, y);	/* ALI - Artifact doors */

	if (door->typ == SDOOR) {
	    switch (otmp->otyp) {
	    case WAN_OPENING:
	    case SPE_KNOCK:
	    case WAN_STRIKING:
	    case SPE_FORCE_BOLT:
		if (key) {
		    /* Artifact doors are revealed only */
		    cvt_sdoor_to_door(door, &u.uz);
		} else {
		    door->typ = DOOR;
		    door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
		}
		newsym(x,y);
		if (cansee(x,y)) pline("一扇门在墙中出现！");
		if (otmp->otyp == WAN_OPENING || otmp->otyp == SPE_KNOCK)
		    return TRUE;
		break;		/* striking: continue door handling below */
	    case WAN_LOCKING:
	    case SPE_WIZARD_LOCK:
	    default:
		return FALSE;
	    }
	}

	switch(otmp->otyp) {
	case WAN_LOCKING:
	case SPE_WIZARD_LOCK:
	    if (Is_rogue_level(&u.uz)) {
	    	boolean vis = cansee(x,y);
		/* Can't have real locking in Rogue, so just hide doorway */
		if (vis) pline("%s在陈旧古老的门廊中升起。",
			dustcloud);
		else
			You_hear("哗哗声。");
		if (obstructed(x,y)) {
			if (vis) pline("这团云%s.",quickly_dissipates);
			return FALSE;
		}
		block_point(x, y);
		door->typ = SDOOR;
		if (vis) pline("门廊消失了！");
		newsym(x,y);
		return TRUE;
	    }
	    if (obstructed(x,y)) return FALSE;
	    /* Don't allow doors to close over traps.  This is for pits */
	    /* & trap doors, but is it ever OK for anything else? */
	    if (t_at(level, x, y)) {
		/* maketrap() clears doormask, so it should be NODOOR */
		pline("%s在门廊中升起，但是%s.",
		      dustcloud, quickly_dissipates);
		return FALSE;
	    }

	    switch (door->doormask & ~D_TRAPPED) {
	    case D_CLOSED:
		msg = key ? "这扇门关上了！" :
			    "这扇门锁上了！";
		break;
	    case D_ISOPEN:
		msg = key ? "这扇门突然关上了！" :
			    "这扇门突然关上锁住了！";
		break;
	    case D_BROKEN:
		msg = key ? "这扇破门重组恢复了！" :
			    "这扇破门重组恢复锁上了！";
		break;
	    case D_NODOOR:
		msg =
		"一团尘埃升起钻进了一扇门！";
		break;
	    default:
		res = FALSE;
		break;
	    }
	    block_point(x, y);
	    door->doormask = (key ? D_CLOSED : D_LOCKED) |
			     (door->doormask & D_TRAPPED);
	    newsym(x,y);
	    break;
	case WAN_OPENING:
	case SPE_KNOCK:
	    if (!key && door->doormask & D_LOCKED) {
		msg = "门解锁了！";
		door->doormask = D_CLOSED | (door->doormask & D_TRAPPED);
	    } else res = FALSE;
	    break;
	case WAN_STRIKING:
	case SPE_FORCE_BOLT:
	    if (!key && door->doormask & (D_LOCKED | D_CLOSED)) {
		if (door->doormask & D_TRAPPED) {
		    if (MON_AT(level, x, y))
			mb_trapped(m_at(level, x,y));
		    else if (flags.verbose) {
			if (cansee(x,y))
			    pline("轰！！  你看见门爆炸了。");
			else if (flags.soundok)
			    You_hear("远处的爆炸声。");
		    }
		    door->doormask = D_NODOOR;
		    unblock_point(x,y);
		    newsym(x,y);
		    loudness = 40;
		    break;
		}
		door->doormask = D_BROKEN;
		if (flags.verbose) {
		    if (cansee(x,y))
			pline("门被摧毁了！");
		    else if (flags.soundok)
			You_hear("损坏的声音。");
		}
		unblock_point(x,y);
		newsym(x,y);
		/* force vision recalc before printing more messages */
		if (vision_full_recalc) vision_recalc(0);
		loudness = 20;
	    } else res = FALSE;
	    break;
	default: warning("在门上施放法术（%d）。", otmp->otyp);
	    break;
	}
	if (msg && cansee(x,y)) {
	    pline(msg);
	    /* we know whether it's locked now */
	    level->locations[x][y].mem_door_l = 1;
	    map_background(x, y, TRUE);
	}
	if (loudness > 0) {
	    /* door was destroyed */
	    wake_nearto(x, y, loudness);
	    if (*in_rooms(level, x, y, SHOPBASE)) add_damage(x, y, 0L);
	}

	if (res && picking_at(x, y)) {
	    /* maybe unseen monster zaps door you're unlocking */
	    stop_occupation();
	    reset_pick();
	}
	return res;
}

static void chest_shatter_msg(struct obj *otmp)
{
	const char *disposition;
	const char *thing;
	long save_Blinded;

	if (otmp->oclass == POTION_CLASS) {
		pline("你%s%s碎了！", Blind ? "听见" : "看见", an(bottlename()));
		if (!breathless(youmonst.data) || haseyes(youmonst.data))
			potionbreathe(otmp);
		return;
	}
	/* We have functions for distant and singular names, but not one */
	/* which does _both_... */
	save_Blinded = Blinded;
	Blinded = 1;
	thing = singular(otmp, xname);
	Blinded = save_Blinded;
	switch (objects[otmp->otyp].oc_material) {
	case PAPER:	disposition = "被撕成碎片";
		break;
	case WAX:	disposition = "变形了";
		break;
	case VEGGY:	disposition = "被打成浆";
		break;
	case FLESH:	disposition = "捣碎了";
		break;
	case GLASS:	disposition = "碎了";
		break;
	case WOOD:	disposition = "裂成小块";
		break;
	default:	disposition = "被摧毁了";
		break;
	}
	pline("%s%s！", An(thing), disposition);
}

/*
 * ALI - Kevin Hugo's artifact doors.
 *
 * Originally returned the artifact number (alignment?) unlocking a door at (x, y).
 * Seems to come from SLASH'EM, with its alignment keys that open such doors to
 * Vlad's Tower.
 *
 * For the Advent Calendar branch, it's effectively used to create unbreakable
 * and impassible doors, so instead returns A_NONE in advcal and 0 otherwise.
 */
int artifact_door(struct level *lev, int x, int y)
{
	if (Is_advent_calendar(&lev->z))
	    return A_NONE;

	return 0;
}

/*lock.c*/
