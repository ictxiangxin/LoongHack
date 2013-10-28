/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"

static void topten_add_score(struct nh_topten_entry *entry,
			     struct nh_menuitem **items, int *size, int *icount,
			     int maxwidth)
{
    char line[BUFSZ], txt[BUFSZ], *txt2;
    char fmt[32], hpbuf[16], maxhpbuf[16];
    int txtfw, initialfw, hpfw; /* field widths */
    enum nh_menuitem_role role = entry->highlight ? MI_HEADING : MI_TEXT;
    
    initialfw = strlen(" No     Points   ");
    hpfw = strlen(" Hp [max] ");
    
    /* build padded hp strings */
    if (entry->hp <= 0) {
	hpbuf[0] = '-';
	hpbuf[1] = '\0';
    } else
	sprintf(hpbuf, "%d", entry->hp);
    sprintf(maxhpbuf, "[%d]", entry->maxhp);
    
    /* calc maximum text field width for the current terminal.
     * maxwidth already accounts for window borders and spacing. */
    txtfw = maxwidth - initialfw - hpfw - 1;
    
    if (strlen(entry->entrytxt) > txtfw) { /* text needs to be split */
	strncpy(txt, entry->entrytxt, BUFSZ);
	txt2 = &txt[txtfw];
	while (*txt2 != ' ' && txt2 > txt)
	    txt2--;
	/* special case: if about to wrap in the middle of maximum
	    dungeon depth reached, wrap in front of it instead */
	if (txt2 > txt + 5 && !strncmp(txt2 - 5, " [max", 5))
	    txt2 -= 5;
	*txt2++ = '\0';
	
	sprintf(fmt, "%%4d %%10d  %%-%ds", maxwidth - initialfw);
	sprintf(line, fmt, entry->rank, entry->points, txt);
	add_menu_txt(*items, *size, *icount, line, role);
	
	sprintf(fmt, "%%%ds%%-%ds %%3s %%5s ", initialfw, txtfw);
	sprintf(line, fmt, "", txt2, hpbuf, maxhpbuf);
	add_menu_txt(*items, *size, *icount, line, role);
    } else {
	sprintf(fmt, "%%4d %%10d  %%-%ds %%3s %%5s ", txtfw);
	sprintf(line, fmt, entry->rank, entry->points, entry->entrytxt,
		hpbuf, maxhpbuf);
	add_menu_txt(*items, *size, *icount, line, role);
    }
}


static void makeheader(char *linebuf)
{
    size_t i;

    strcpy(linebuf, " No     Points   Name");
    for (i = strlen(linebuf); i < COLS - strlen(" Hp [max] ") - 4; i++)
	linebuf[i] = ' ';
    strcpy(&linebuf[i], " Hp [max] ");
}


void show_topten(char *player, int top, int around, nh_bool own)
{
    struct nh_topten_entry *scores;
    char buf[BUFSZ];
    int i, listlen = 0;
    struct nh_menuitem *items;
    int icount, size;
    
    scores = nh_get_topten(&listlen, buf, player, top, around, own);
    
    if (listlen == 0) {
	curses_msgwin("排行榜没有任何记录。");
	return;
    }
    
    /* show the score list on a blank screen */
    clear();
    refresh();

    icount = 0;
    size = 4 + listlen * 2; /* maximum length, only required if every item wraps */
    items = malloc(size * sizeof(struct nh_menuitem));
    
    /* buf has the topten status if there is one, eg:
     *  "you did not beat your previous score" */
    if (buf[0]) { 
	add_menu_txt(items, size, icount, buf, MI_TEXT);
	add_menu_txt(items, size, icount, "", MI_TEXT);
    }
    
    makeheader(buf);
    add_menu_txt(items, size, icount, buf, MI_HEADING);
    
    for (i = 0; i < listlen; i++)
	topten_add_score(&scores[i], &items, &size, &icount, COLS - 4);
    add_menu_txt(items, size, icount, "", MI_TEXT);
    
    curses_display_menu(items, icount, "排行榜：", PICK_NONE, NULL);
    free(items);
}

