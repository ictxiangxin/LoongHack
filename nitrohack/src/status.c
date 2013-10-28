/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>

struct nh_player_info player;

/* for coloring status differences */
static struct nh_player_info saved_player, old_player;
static int statdiff_moves;	/* stop non-actions from blanking the colors */


/* Must be called before nh[net]_start_game(), nh[net]_restore_game() and
 * nh_view_replay_start(), which all call curses_update_status[_silent]() before
 * [replay_]commandloop() runs. */
void reset_old_status(void)
{
    /* Flag statdiff system for initialization. */
    statdiff_moves = -1;
}


void update_old_status(void)
{
    saved_player = player;
}


static int status_change_color(long old1, long old2, long new1, long new2)
{
    if (new1 > old1)
	return CLR_GREEN;
    else if (new1 < old1)
	return CLR_RED;
    else if (new2 > old2)
	return CLR_GREEN;
    else if (new2 < old2)
	return CLR_RED;
    else
	return 0;
}


static void status_change_color_on(int *col, attr_t *attr,
				   long old1, long old2, long new1, long new2)
{
    *col = status_change_color(old1, old2, new1, new2);
    if (*col) {
	*attr = curses_color_attr(*col, 0);
	wattron(statuswin, *attr);
    }
}


static void status_change_color_off(int col, attr_t attr)
{
    if (col)
	wattroff(statuswin, attr);
}


static void print_statdiff(const char *prefix, const char *fmt,
			   long oldv, long newv)
{
    int col;
    attr_t attr;

    if (prefix && prefix[0])
	waddstr(statuswin, prefix);
    status_change_color_on(&col, &attr, oldv, 0, newv, 0);
    wprintw(statuswin, fmt, newv);
    status_change_color_off(col, attr);
}


static void print_statdiffr(const char *prefix, const char *fmt,
			    long oldv, long newv)
{
    int col;
    attr_t attr;

    if (prefix && prefix[0])
	waddstr(statuswin, prefix);
    status_change_color_on(&col, &attr, -oldv, 0, -newv, 0);
    wprintw(statuswin, fmt, newv);
    status_change_color_off(col, attr);
}


/*
 * Rules: HP     Pw
 * max    white  white
 * >2/3   green  green
 * >1/3   brown  cyan
 * >1/7   orange magenta
 * <=1/7  red    blue
 */
static int percent_color(int val_cur, int val_max, nh_bool ishp)
{
    int color;

    if (val_cur == val_max)
	color = CLR_GRAY;
    else if (val_cur * 3 > val_max * 2)
	color = CLR_GREEN;
    else if (val_cur * 3 > val_max * 1)
	color = ishp ? CLR_BROWN : CLR_CYAN;
    else if (val_cur * 7 > val_max * 1)
	color = ishp ? CLR_ORANGE : CLR_MAGENTA;
    else
	color = ishp ? CLR_RED : CLR_BLUE;

    return color;
}


static void draw_name_hp_bar(const char *str, int val_cur, int val_max)
{
    int i, len, fill_len;
    attr_t colorattr;

    /* trim spaces to right */
    len = strlen(str);
    for (i = len - 1; i >= 0; i--) {
	if (str[i] == ' ')
	    len = i;
	else
	    break;
    }

    if (val_max <= 0 || val_cur <= 0)
	fill_len = 0;
    else
	fill_len = len * val_cur / val_max;
    if (fill_len > len)
	fill_len = len;

    colorattr = curses_color_attr(percent_color(val_cur, val_max, TRUE), 0);
    waddch(statuswin, '[');
    wattron(statuswin, colorattr);
    /* color the whole string and inverse the bar */
    wattron(statuswin, A_REVERSE);
    wprintw(statuswin, "%.*s", fill_len, str);
    wattroff(statuswin, A_REVERSE);
    wprintw(statuswin, "%.*s", len - fill_len, &str[fill_len]);
    wattroff(statuswin, colorattr);
    waddch(statuswin, ']');
}


static void draw_dungeon_name(const struct nh_player_info *pi,
			      enum dgn_name_value default_verbose)
{
    if (settings.dungeon_name == DGN_NAME_DLVL ||
	(settings.dungeon_name == DGN_NAME_AUTO &&
	 default_verbose == DGN_NAME_DLVL)) {
	waddstr(statuswin, pi->levdesc_dlvl);

    } else if (settings.dungeon_name == DGN_NAME_SHORT ||
	       (settings.dungeon_name == DGN_NAME_AUTO &&
		default_verbose == DGN_NAME_SHORT)) {
	waddstr(statuswin, pi->levdesc_short);

    } else if (settings.dungeon_name == DGN_NAME_FULL ||
	       (settings.dungeon_name == DGN_NAME_AUTO &&
		default_verbose == DGN_NAME_FULL)) {
	waddstr(statuswin, pi->levdesc_full);

    } else {
	waddstr(statuswin, "？？？");
    }
}


static const struct {
    const char *name;
    int color;
} statuscolors[] = {
    /* encumberance */
    { "劳累", CLR_YELLOW },
    { "紧张", CLR_YELLOW },
    { "难受", CLR_YELLOW },
    { "痛苦", CLR_YELLOW },
    { "崩溃", CLR_YELLOW },
    /* hunger */
    { "饱腹", CLR_YELLOW },
    { "饥饿", CLR_YELLOW },
    { "虚弱", CLR_ORANGE },
    { "意识模糊", CLR_ORANGE },
    { "昏厥", CLR_ORANGE },
    { "饿死", CLR_ORANGE },
    /* misc */
    { "手无寸铁", CLR_YELLOW },
    { "飘浮", CLR_BRIGHT_CYAN },
    { "飞行", CLR_BRIGHT_GREEN },
    { "Elbereth", CLR_BRIGHT_GREEN },
    /* trapped */
    { "沼泽", CLR_YELLOW },
    { "陷阱", CLR_YELLOW },
    { "陷坑", CLR_YELLOW },
    { "针刺陷阱", CLR_YELLOW },
    { "网", CLR_YELLOW },
    { "陷地", CLR_YELLOW },
    { "岩浆", CLR_ORANGE },
    /* misc bad */
    { "滑腻", CLR_YELLOW },
    { "眼瞎", CLR_YELLOW },
    { "混乱", CLR_YELLOW },
    { "脚瘸", CLR_YELLOW },
    { "眩晕", CLR_YELLOW },
    { "幻觉", CLR_YELLOW },
    /* misc fatal */
    { "食物中毒", CLR_ORANGE },
    { "生病", CLR_ORANGE },
    { "窒息", CLR_ORANGE },
    { "粘液", CLR_ORANGE },
    { "石化", CLR_ORANGE },
    { NULL, 0 }
};

static void draw_statuses(const struct nh_player_info *pi, nh_bool threeline)
{
    int i, j, k;

    j = getmaxx(statuswin) + 1;
    for (i = 0; i < pi->nr_items; i++) {
	attr_t colorattr;
	int color = CLR_WHITE;
	j -= strlen(pi->statusitems[i]) + 1;
	for (k = 0; statuscolors[k].name; k++) {
	    if (!strcmp(pi->statusitems[i], statuscolors[k].name)) {
		color = statuscolors[k].color;
		break;
	    }
	}
	colorattr = curses_color_attr(color, 0);
	wmove(statuswin, (threeline ? 2 : 1), j);
	wattron(statuswin, colorattr);
	wprintw(statuswin, "%s", pi->statusitems[i]);
	wattroff(statuswin, colorattr);
    }
}


static void draw_time(const struct nh_player_info *pi)
{
    const struct nh_player_info *oldpi = &old_player;
    int movediff;
    attr_t colorattr;
    nh_bool colorchange;

    if (!settings.time)
	return;

    /* Give players a better idea of the passage of time. */
    movediff = pi->moves - oldpi->moves;
    if (movediff > 1) {
	colorattr = curses_color_attr(
			(movediff >= 100) ? CLR_RED :
			(movediff >=  50) ? CLR_YELLOW :
			(movediff >=  20) ? CLR_CYAN :
			(movediff >=  10) ? CLR_BLUE :
					    CLR_GREEN, 0);
	colorchange = TRUE;
    } else {
	colorchange = FALSE;
    }

    if (colorchange)
	wattron(statuswin, colorattr);
    wprintw(statuswin, "回合:%ld", pi->moves);
    if (colorchange)
	wattroff(statuswin, colorattr);
}


/*
 * longest practical second status line at the moment is
 *	Astral Plane $:12345 HP:700(700) Pw:111(111) AC:-127 Xp:30/123456789
 *	T:123456 Satiated Conf FoodPois Ill Blind Stun Hallu Overloaded
 * -- or somewhat over 130 characters
 */

static void classic_status(struct nh_player_info *pi, nh_bool threeline)
{
    char buf[COLNO];
    int stat_ch_col;
    attr_t colorattr;
    const struct nh_player_info *oldpi = &old_player;

    /* line 1 */
    sprintf(buf, "%.10s the %-*s  ", pi->plname,
	    pi->max_rank_sz + 8 - (int)strlen(pi->plname), pi->rank);
    buf[0] = toupper((unsigned char)buf[0]);
    wmove(statuswin, 0, 0);
    draw_name_hp_bar(buf, pi->hp, pi->hpmax);

    waddstr(statuswin, " ");
    status_change_color_on(&stat_ch_col, &colorattr,
			   oldpi->st, oldpi->st_extra,
			   pi->st, pi->st_extra);
    waddstr(statuswin, "St:");
    if (pi->st == 18 && pi->st_extra) {
	if (pi->st_extra < 100)
	    wprintw(statuswin, "18/%02d", pi->st_extra);
	else
	    wprintw(statuswin,"18/**");
    } else
	wprintw(statuswin, "%-1d", pi->st);
    status_change_color_off(stat_ch_col, colorattr);

    print_statdiff(" ", "敏:%-1d", oldpi->dx, pi->dx);
    print_statdiff(" ", "体:%-1d", oldpi->co, pi->co);
    print_statdiff(" ", "智:%-1d", oldpi->in, pi->in);
    print_statdiff(" ", "感:%-1d", oldpi->wi, pi->wi);
    print_statdiff(" ", "魅:%-1d", oldpi->ch, pi->ch);

    wprintw(statuswin, (pi->align == A_CHAOTIC) ? "混乱的" :
		       (pi->align == A_NEUTRAL) ? "中立的" : "正义的");

    if (settings.showscore)
	print_statdiff(" ", "S:%ld", oldpi->score, pi->score);

    wclrtoeol(statuswin);

    /* line 2 */
    wmove(statuswin, 1, 0);
    draw_dungeon_name(pi, DGN_NAME_DLVL);

    waddstr(statuswin, " ");
    status_change_color_on(&stat_ch_col, &colorattr, oldpi->gold, 0, pi->gold, 0);
    wprintw(statuswin, "%c:%-2ld", pi->coinsym, pi->gold);
    status_change_color_off(stat_ch_col, colorattr);

    waddstr(statuswin, " HP:");
    if (pi->hp * 7 < pi->hpmax) wattron(statuswin, A_REVERSE);
    colorattr = curses_color_attr(percent_color(pi->hp, pi->hpmax, TRUE), 0);
    wattron(statuswin, colorattr);
    wprintw(statuswin, "%d(%d)", pi->hp, pi->hpmax);
    wattroff(statuswin, colorattr);
    if (pi->hp * 7 < pi->hpmax) wattroff(statuswin, A_REVERSE);

    waddstr(statuswin, " Pw:");
    colorattr = curses_color_attr(percent_color(pi->en, pi->enmax, FALSE), 0);
    wattron(statuswin, colorattr);
    wprintw(statuswin, "%d(%d)", pi->en, pi->enmax);
    wattroff(statuswin, colorattr);

    print_statdiffr(" ", "AC:%-2d", oldpi->ac, pi->ac);

    if (pi->monnum != pi->cur_monnum) {
	print_statdiff(" ", "HD:%d", oldpi->level, pi->level);
    } else if (settings.showexp) {
	print_statdiff(" ", "Xp:%u", oldpi->level, pi->level);
	print_statdiff("/", "%-1ld", oldpi->xp, pi->xp);
    } else {
	print_statdiff(" ", "Exp:%u", oldpi->level, pi->level);
    }

    waddstr(statuswin, " ");
    draw_time(pi);

    wclrtoeol(statuswin);

    if (threeline) {
	wmove(statuswin, 2, 0);
	wclrtoeol(statuswin);
    }

    draw_statuses(pi, threeline);

}


static void draw_bar(int barlen, int val_cur, int val_max, nh_bool ishp)
{
    char str[COLNO];
    int fill_len = 0, bl;
    attr_t colorattr;

    bl = barlen - 2;
    if (val_max > 0 && val_cur > 0)
	fill_len = bl * val_cur / val_max;

    sprintf(str, "%*d / %-*d", (bl - 3) / 2, val_cur, (bl - 2) / 2, val_max);

    colorattr = curses_color_attr(percent_color(val_cur, val_max, ishp), 0);
    wattron(statuswin, colorattr);
    wprintw(statuswin, ishp ? "生命" : "魔法");
    wattroff(statuswin, colorattr);

    waddch(statuswin, '[');
    wattron(statuswin, colorattr);
    wattron(statuswin, A_REVERSE);
    wprintw(statuswin, "%.*s", fill_len, str);
    wattroff(statuswin, A_REVERSE);
    wprintw(statuswin, "%s", &str[fill_len]);
    wattroff(statuswin, colorattr);
    waddch(statuswin, ']');
}


/*

The status bar looks like this:

Two-line:
         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
HP:[    15 / 16    ] AC:-128 Xp:30 Astral Plane  Twelveletter, Student of Stones
Pw:[     5 / 5     ] $4294967295 S:4294967295 T:4294967295     Burdened Starving

Three-line:
         1         2         3         4         5         6         7         8
12345678901234567890123456789012345678901234567890123456789012345678901234567890
Twelveletter the Chaotic Gnomish Student of Stones          St:18/01 Dx:18 Co:18
HP:[    15 / 16    ] AC:-128 Xp:30(10000000) Astral Plane   In:18    Wi:18 Ch:18
Pw:[     5 / 5     ] $4294967295 S:4294967295 T:4294967295     Burdened Starving

*/

static void draw_status_lines(struct nh_player_info *pi, nh_bool threeline)
{
    char buf[COLNO];
    int stat_ch_col;
    attr_t colorattr;
    const struct nh_player_info *oldpi = &old_player;

    /*
     * second-last line
     */
    wmove(statuswin, (threeline ? 1 : 0), 0);
    draw_bar(15, pi->hp, pi->hpmax, TRUE);
    print_statdiffr(" ", "AC:%d", oldpi->ac, pi->ac);

    /* experience level and points */
    if (pi->monnum == pi->cur_monnum)
	print_statdiff(" ", "Xp:%d", oldpi->level, pi->level);
    else
	print_statdiff(" ", "HD:%d", oldpi->level, pi->level);
    if (threeline && settings.showexp && pi->monnum == pi->cur_monnum) {
	waddstr(statuswin, "(");
	status_change_color_on(&stat_ch_col, &colorattr,
			       oldpi->level, oldpi->xp,
			       pi->level, pi->xp);
	wprintw(statuswin, "%ld", pi->xp_next - pi->xp);
	status_change_color_off(stat_ch_col, colorattr);
	waddstr(statuswin, ")");
    }

    /* dungeon level */
    waddstr(statuswin, " ");
    draw_dungeon_name(pi, (threeline ? DGN_NAME_FULL : DGN_NAME_SHORT));
    wclrtoeol(statuswin);

    /*
     * last line
     */
    wmove(statuswin, (threeline ? 2 : 1), 0);
    draw_bar(15, pi->en, pi->enmax, FALSE);

    /* gold */
    waddstr(statuswin, " ");
    status_change_color_on(&stat_ch_col, &colorattr, oldpi->gold, 0, pi->gold, 0);
    wprintw(statuswin, "%c%ld", pi->coinsym, pi->gold);
    status_change_color_off(stat_ch_col, colorattr);

    /* score and turns */
    if (settings.showscore)
	print_statdiff(" ", "分数:%ld", oldpi->score, pi->score);
    if (settings.time) {
	waddstr(statuswin, " ");
	draw_time(pi);
    }
    wclrtoeol(statuswin);

    /*
     * statuses (bottom right)
     */
    draw_statuses(pi, threeline);

    /*
     * name
     */
    if (threeline) {
	/* top left */
	sprintf(buf, "%.12s %s%s%s%s", pi->plname,
		(pi->align == A_CHAOTIC) ? "混乱的" :
		(pi->align == A_NEUTRAL) ? "中立的" : "正义的",
		(pi->monnum == pi->cur_monnum) ? pi->race_adj : "",
		(pi->monnum == pi->cur_monnum) ? " " : "",
		pi->rank);
	wmove(statuswin, 0, 0);
    } else {
	/* top right */
	sprintf(buf, "%.12s, %s", pi->plname, pi->rank);
	wmove(statuswin, 0, getmaxx(statuswin) - strlen(buf));
    }
    wprintw(statuswin, "%s", buf);
    wclrtoeol(statuswin);

    /*
     * attributes (in threeline mode) "In:18 Wi:18 Ch:18" = 17 chars
     */
    if (threeline) {
	/*
	 * first line
	 */
	/* strength */
	status_change_color_on(&stat_ch_col, &colorattr,
			       oldpi->st, oldpi->st_extra,
			       pi->st, pi->st_extra);
	if (pi->st == 18 && pi->st_extra) {
	    wmove(statuswin, 0, getmaxx(statuswin) - 20);
	    if (pi->st_extra == 100)
		waddstr(statuswin, "力:18/**");
	    else
		wprintw(statuswin, "力:18/%02d", pi->st_extra);
	} else {
	    wmove(statuswin, 0, getmaxx(statuswin) - 17);
	    wprintw(statuswin, "力:%-2d", pi->st);
	}
	status_change_color_off(stat_ch_col, colorattr);

	/* dexterity and constitution */
	print_statdiff(" ", "敏:%-2d", oldpi->dx, pi->dx);
	print_statdiff(" ", "体:%-2d", oldpi->co, pi->co);

	/*
	 * second line
	 */
	/* intelligence (align with strength) */
	if (pi->st == 18 && pi->st_extra)
	    wmove(statuswin, 1, getmaxx(statuswin) - 20);
	else
	    wmove(statuswin, 1, getmaxx(statuswin) - 17);
	print_statdiff(NULL, "智:%-2d", oldpi->in, pi->in);

	/* wisdom and charisma */
	wmove(statuswin, 1, getmaxx(statuswin) - 11);
	print_statdiff(NULL, "感:%-2d", oldpi->wi, pi->wi);
	print_statdiff(" ", "魅:%-2d", oldpi->ch, pi->ch);
    }
}


void curses_update_status_silent(struct nh_player_info *pi)
{
    if (!pi)
	return;

    player = *pi;

    if (statdiff_moves == -1) {
	saved_player = player;
	statdiff_moves = 0;
    }

    if (statdiff_moves != pi->moves) {
	old_player = saved_player;
	statdiff_moves = pi->moves;
    }
}


void curses_update_status(struct nh_player_info *pi)
{
    curses_update_status_silent(pi);

    if (player.x == 0)
	return; /* called before the game is running */

    if (settings.classic_status)
	classic_status(&player, ui_flags.status3);
    else
	draw_status_lines(&player, ui_flags.status3);

    /* prevent the cursor from flickering in the status line */
    wmove(mapwin, player.y, player.x - 1);

    /* if HP changes, frame color may change */
    if (ui_flags.draw_frame && settings.frame_hp_color)
	redraw_frame();

    wnoutrefresh(statuswin);
}
