/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"
#include "epri.h"
#include "edog.h"
#include "eshk.h"

static boolean no_repeat = FALSE;

static char *You_buf(int);


/*VARARGS1*/
/* Note that these declarations rely on knowledge of the internals
 * of the variable argument handling stuff in "tradstdc.h"
 */

static void vpline(const char *, va_list);

void pline(const char * line, ...)
{
	va_list the_args;
	
	va_start(the_args, line);
	vpline(line, the_args);
	va_end(the_args);
}

static enum msgtype_action msgtype_match_line(const char *line)
{
	const struct nh_msgtype_rules *mtrs = iflags.mt_rules;
	if (mtrs) {
	    int i;
	    for (i = 0; i < mtrs->num_rules; i++) {
		const struct nh_msgtype_rule *r = &mtrs->rules[i];
		if (pmatch(r->pattern, line))
		    return r->action;
	    }
	}
	return MSGTYPE_DEFAULT;
}

static void vpline(const char *line, va_list the_args)
{
	char pbuf[BUFSZ], *c;
	boolean repeated;
	int lastline;
	enum msgtype_action mtact;

	lastline = curline - 1;
	if (lastline < 0)
	    lastline += MSGCOUNT;

	if (!line || !*line) return;

	vsnprintf(pbuf, BUFSZ, line, the_args);

	/* Sanitize, otherwise the line can mess up
	 * the message window and message history. */
	for (c = pbuf; *c && c < pbuf + BUFSZ; c++) {
	    if (*c == '\n' || *c == '\t')
		*c = ' ';
	}

	line = pbuf;

	mtact = msgtype_match_line(line);
	if (mtact == MSGTYPE_HIDE) return;

	repeated = !strcmp(line, toplines[lastline]);
	if (repeated && (no_repeat || mtact == MSGTYPE_NO_REPEAT))
	    return;

	if (vision_full_recalc)
	    vision_recalc(0);
	if (u.ux)
	    flush_screen();

	if (repeated) {
	    toplines_count[lastline]++;
	} else {
	    strcpy(toplines[curline], line);
	    toplines_count[curline] = 1;
	    curline++;
	    curline %= MSGCOUNT;
	}

	if (iflags.next_msg_nonblocking) {
	    (*windowprocs.win_print_message_nonblocking)(moves, line);
	    iflags.next_msg_nonblocking = FALSE;
	} else {
	    print_message(moves, line);
	}

	if (mtact == MSGTYPE_MORE)
	    win_pause_output(P_MESSAGE);	/* --more-- */
}

/*
 * Allow the next printed message to push previous messages out of view
 * if needed without a --More--.
 */
void suppress_more(void)
{
	iflags.next_msg_nonblocking = TRUE;
}

/*VARARGS1*/
void Norep (const char * line, ...)
{
	va_list the_args;
	
	va_start(the_args, line);
	no_repeat = TRUE;
	vpline(line, the_args);
	no_repeat = FALSE;
	va_end(the_args);
	return;
}

/* work buffer for You(), &c and verbalize() */
static char *you_buf = 0;
static int you_buf_siz = 0;

static char * You_buf(int siz)
{
	if (siz > you_buf_siz) {
		if (you_buf) free(you_buf);
		you_buf_siz = siz + 10;
		you_buf = malloc((unsigned) you_buf_siz);
	}
	return you_buf;
}

void free_youbuf(void)
{
	if (you_buf) free(you_buf),  you_buf = NULL;
	you_buf_siz = 0;
}

/* `prefix' must be a string literal, not a pointer */
#define YouPrefix(pointer,prefix,text) \
 strcpy((pointer = You_buf((int)(strlen(text) + sizeof prefix))), prefix)

#define YouMessage(pointer,prefix,text) \
 strcat((YouPrefix(pointer, prefix, text), pointer), text)

/*VARARGS1*/
void You_hear (const char *line, ...)
{
	va_list the_args;
	char *tmp;
	va_start(the_args, line);
	if (Underwater)
		YouPrefix(tmp, "你隐约听到", line);
	else if (u.usleep)
		YouPrefix(tmp, "你仿佛听到", line);
	else
		YouPrefix(tmp, "你听到", line);
	vpline(strcat(tmp, line), the_args);
	va_end(the_args);
}

/*VARARGS1*/
void verbalize (const char *line, ...)
{
	va_list the_args;
	char *tmp;
	if (!flags.soundok) return;
	va_start(the_args, line);
	tmp = You_buf((int)strlen(line) + sizeof "\"\"");
	strcpy(tmp, "\"");
	strcat(tmp, line);
	strcat(tmp, "\"");
	vpline(tmp, the_args);
	va_end(the_args);
}

static void vraw_printf(const char *,va_list);

void raw_printf (const char *line, ...)
{
	va_list the_args;
	va_start(the_args, line);
	vraw_printf(line, the_args);
	va_end(the_args);
}

static void vraw_printf(const char *line, va_list the_args)
{
	if (!strchr(line, '%'))
	    raw_print(line);
	else {
	    char pbuf[BUFSZ];
	    vsprintf(pbuf,line,the_args);
	    raw_print(pbuf);
	}
}


/*VARARGS1*/
void impossible (const char *s, ...)
{
	va_list the_args;
	va_start(the_args, s);
	if (program_state.in_impossible)
		panic("impossible called impossible");
	program_state.in_impossible = 1;
	{
	    char pbuf[BUFSZ];
	    vsprintf(pbuf,s,the_args);
	    paniclog("impossible", pbuf);
	}
	vpline(s,the_args);
	pline("Program in disorder - perhaps you'd better save.");
	program_state.in_impossible = 0;
	va_end(the_args);
}

void warning(const char *s, ...)
{
	char str[BUFSZ];
	va_list the_args;
	va_start(the_args, s);
	{
	    char pbuf[BUFSZ];
	    vsprintf(pbuf, s, the_args);
	    paniclog("warning", pbuf);
	}
	vsprintf(str, s, the_args);
	pline("警告：%s\n", str);
	va_end(the_args);
}

const char * align_str(aligntyp alignment)
{
    switch ((int)alignment) {
	case A_CHAOTIC: return "混沌的";
	case A_NEUTRAL: return "中立的";
	case A_LAWFUL:	return "正义的";
	case A_NONE:	return "不结盟的";
    }
    return "未知的";
}

void mstatusline(struct monst *mtmp)
{
	aligntyp alignment;
	char info[BUFSZ], monnambuf[BUFSZ];

	if (mtmp->ispriest || mtmp->data == &mons[PM_ALIGNED_PRIEST]
				|| mtmp->data == &mons[PM_ANGEL])
		alignment = EPRI(mtmp)->shralign;
	else
		alignment = mtmp->data->maligntyp;
	alignment = (alignment > 0) ? A_LAWFUL :
		(alignment < 0) ? A_CHAOTIC :
		A_NEUTRAL;

	info[0] = 0;
	if (mtmp->mtame) {	  strcat(info, "， 顺从的");
	    if (wizard) {
		sprintf(eos(info), " （%d", mtmp->mtame);
		if (!mtmp->isminion)
		    sprintf(eos(info), "；饥饿%u；显形%d",
			EDOG(mtmp)->hungrytime, EDOG(mtmp)->apport);
		strcat(info, "）");
	    }
	}
	else if (mtmp->mpeaceful) strcat(info, "，平静");
	if (mtmp->meating)	  strcat(info, "，可食用");
	if (mtmp->mcan)		  strcat(info, "，已作废");
	if (mtmp->mconf)	  strcat(info, "，迷茫");
	if (mtmp->mblinded || !mtmp->mcansee)
				  strcat(info, "，致盲");
	if (mtmp->mstun)	  strcat(info, "，眩晕");
	if (mtmp->msleeping)	  strcat(info, "，沉睡");
	else if (mtmp->mfrozen || !mtmp->mcanmove)
				  strcat(info, "，不能移动");
				  /* [arbitrary reason why it isn't moving] */
	else if (mtmp->mstrategy & STRAT_WAITMASK)
				  strcat(info, "，冥想");
	else if (mtmp->mflee)	  strcat(info, "，害怕");
	if (mtmp->mtrapped)	  strcat(info, "，中陷阱");
	if (mtmp->mspeed)	  strcat(info,
					mtmp->mspeed == MFAST ? "，快速" :
					mtmp->mspeed == MSLOW ? "，缓慢" :
					"，速度？？？");
	if (mtmp->mundetected)	  strcat(info, "，隐藏");
	if (mtmp->minvis)	  strcat(info, "，隐身");
	if (mtmp == u.ustuck)	  strcat(info,
			(sticks(youmonst.data)) ? "，被你抓住" :
				u.uswallow ? (is_animal(u.ustuck->data) ?
				"，生吞了你" :
				"，吞没了你") :
				"，握住你");
	if (mtmp == u.usteed)	  strcat(info, "，抓住你");
	if (wizard && mtmp->isshk && ESHK(mtmp)->cheapskate)
	    strcat(info, "，吝啬鬼");

	/* avoid "Status of the invisible newt ..., invisible" */
	/* and unlike a normal mon_nam, use "saddled" even if it has a name */
	strcpy(monnambuf, x_monnam(mtmp, ARTICLE_THE, NULL,
	    (SUPPRESS_IT|SUPPRESS_INVISIBLE), FALSE));

	pline("%s （%s）的状态：  Level %d  HP %d(%d)  AC %d%s.",
		monnambuf,
		align_str(alignment),
		mtmp->m_lev,
		mtmp->mhp,
		mtmp->mhpmax,
		find_mac(mtmp),
		info);
}

void ustatusline(void)
{
	char info[BUFSZ];

	info[0] = '\0';
	if (Sick) {
		strcat(info, "，死于");
		if (u.usick_type & SICK_VOMITABLE)
			strcat(info, "食物中毒");
		if (u.usick_type & SICK_NONVOMITABLE) {
			if (u.usick_type & SICK_VOMITABLE)
				strcat(info, "导致");
			strcat(info, "生病");
		}
	}
	if (Stoned)		strcat(info, "，凝固");
	if (Slimed)		strcat(info, "，变得泥泞");
	if (Strangled)		strcat(info, "，被卡住");
	if (Vomiting)		strcat(info, "，恶心"); /* !"nauseous" */
	if (Confusion)		strcat(info, "，迷茫");
	if (Blind) {
	    strcat(info, "，致盲");
	    if (u.ucreamed) {
	    strcat(info, "由于有粘稠物");
		if ((long)u.ucreamed < Blinded || Blindfolded
						|| !haseyes(youmonst.data))
		    strcat(info, "覆盖");
	    }	/* note: "goop" == "glop"; variation is intentional */
	}
	if (Stunned)		strcat(info, "，眩晕");
	if (!u.usteed && Wounded_legs) {
	    const char *what = body_part(LEG);
	    if ((Wounded_legs & BOTH_SIDES) == BOTH_SIDES)
		what = makeplural(what);
				sprintf(eos(info), "，受伤的%s", what);
	}
	if (Glib)		sprintf(eos(info), "，狡猾的%s",
					makeplural(body_part(HAND)));
	if (u.utrap)		strcat(info, "，中陷阱");
	if (Fast)		strcat(info, Very_fast ?
						"，非常快" : "，快");
	if (u.uundetected)	strcat(info, "，隐藏");
	if (Invis)		strcat(info, "，隐身");
	if (u.ustuck) {
	    if (sticks(youmonst.data))
		strcat(info, "，抓住");
	    else
		strcat(info, "，被抓住");
	    strcat(info, mon_nam(u.ustuck));
	}

	pline("%s （%s%s）的状态：  Level %d  HP %d(%d)  AC %d%s。",
		plname,
		    (u.ualign.record >= 20) ? "虔诚地 " :
		    (u.ualign.record > 13) ? "虔敬地 " :
		    (u.ualign.record > 8) ? "热诚地 " :
		    (u.ualign.record > 3) ? "刺耳地 " :
		    (u.ualign.record == 3) ? "" :
		    (u.ualign.record >= 1) ? "犹豫地 " :
		    (u.ualign.record == 0) ? "名义上地 " :
					    "不够格地 ",
		align_str(u.ualign.type),
		Upolyd ? mons[u.umonnum].mlevel : u.ulevel,
		Upolyd ? u.mh : u.uhp,
		Upolyd ? u.mhmax : u.uhpmax,
		u.uac,
		info);
}

void self_invis_message(void)
{
	pline("%s %s.",
	    Hallucination ? "太棒了！  你" : "哔！  一刹那间，你",
	    See_invisible ? "发现身体变得透明了" :
		"看不见自己的身体了");
}

/*pline.c*/
