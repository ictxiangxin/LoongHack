/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#ifndef ESHK_H
#define ESHK_H

#define REPAIR_DELAY	5	/* minimum delay between shop damage & repair */

#define BILLSZ	200

struct bill_x {
	unsigned bo_id;
	boolean useup;
	int price;		/* price per unit */
	int bquan;		/* amount used up */
};

struct eshk {
	struct bill_x bill[BILLSZ];
	struct bill_x *bill_p;
	coord shk;		/* usual position shopkeeper */
	coord shd;		/* position shop door */
	int robbed;		/* amount stolen by most recent customer */
	int credit;		/* amount credited to customer */
	int debit;		/* amount of debt for using unpaid items */
	int loan;		/* shop-gold picked (part of debit) */
	short shoptype;		/* the value of rooms[shoproom].rtype */
	short billct;		/* no. of entries of bill[] in use */
	short visitct;		/* nr of visits by most recent customer */
	d_level shoplevel;	/* level (& dungeon) of his shop */
	schar shoproom;		/* index in rooms; set by inshop() */
	boolean following;	/* following customer since he owes us sth */
	boolean surcharge;	/* angry shk inflates prices */
	boolean cheapskate;	/* offers less than normal for sold goods */
	boolean pbanned;	/* player is banned from the shop */
	char customer[PL_NSIZ]; /* most recent customer */
	char shknam[PL_NSIZ];
};

#define ESHK(mon)	((struct eshk *)&(mon)->mextra[0])

#define NOTANGRY(mon)	((mon)->mpeaceful)
#define ANGRY(mon)	(!NOTANGRY(mon))

#endif /* ESHK_H */
