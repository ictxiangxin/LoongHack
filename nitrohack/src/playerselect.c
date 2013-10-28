/* Copyright (c) Daniel Thaler, 2011.                             */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>
#include <time.h>


#define LISTSZ 32

static int is_valid_character(const struct nh_roles_info *ri, int rolenum,
			      int racenum, int gendnum, int alignnum)
{
    int i, j, k, l, ok;
    
    ok = 0;
    for (i = 0; i < ri->num_roles; i++) {
	if (rolenum > ROLE_NONE && i != rolenum)
	    continue;
	
	for (j = 0; j < ri->num_races; j++) {
	    if (racenum > ROLE_NONE && j != racenum)
		continue;
	    
	    for (k = 0; k < ri->num_genders; k++) {
		if (gendnum > ROLE_NONE && k != gendnum)
		    continue;
		
		for (l = 0; l < ri->num_aligns; l++) {
		    if (alignnum > ROLE_NONE && l != alignnum)
			continue;
		    
		    if (ri->matrix[nh_cm_idx((*ri), i, j, k, l)])
			ok++;
		}
	    }
	}
    }
    
    return ok > 0;
}


static int get_valid_roles(const struct nh_roles_info *ri, int racenum,
			   int gendnum, int alignnum, struct nh_listitem *list, int listlen)
{
    int i;
    int count = 0;
    char rolenamebuf[256];
    
    for (i = 0; i < ri->num_roles; i++) {
	if (!is_valid_character(ri, i, racenum, gendnum, alignnum))
	    continue;
	
	if (list && count < listlen) {
	    if (gendnum != ROLE_NONE && gendnum != ROLE_RANDOM) {
		if (gendnum == 1  && ri->rolenames_f[i])
		    strcpy(rolenamebuf, ri->rolenames_f[i]);
		else
		    strcpy(rolenamebuf, ri->rolenames_m[i]);
	    } else {
		if (ri->rolenames_f[i]) {
		    strcpy(rolenamebuf, ri->rolenames_m[i]);
		    strcat(rolenamebuf, "/");
		    strcat(rolenamebuf, ri->rolenames_f[i]);
		} else
		    strcpy(rolenamebuf, ri->rolenames_m[i]);
	    }
	    strcpy(list[count].caption, rolenamebuf);
	    list[count].id = i;
	}
	count++;
    }
    return count;
}


static int get_valid_races(const struct nh_roles_info *ri, int rolenum,
			   int gendnum, int alignnum, struct nh_listitem *list, int listlen)
{
    int i;
    int count = 0;
    
    for (i = 0; i < ri->num_races; i++) {
	if (!is_valid_character(ri, rolenum, i, gendnum, alignnum))
	    continue;
	
	if (list && count < listlen) {
	    strcpy(list[count].caption, ri->racenames[i]);
	    list[count].id = i;
	}
	count++;
    }
    return count;
}


static int get_valid_genders(const struct nh_roles_info *ri, int rolenum,
			   int racenum, int alignnum, struct nh_listitem *list, int listlen)
{
    int i;
    int count = 0;
    
    for (i = 0; i < ri->num_genders; i++) {
	if (!is_valid_character(ri, rolenum, racenum, i, alignnum))
	    continue;
	
	if (list && count < listlen) {
	    strcpy(list[count].caption, ri->gendnames[i]);
	    list[count].id = i;
	}
	count++;
    }
    return count;
}


static int get_valid_aligns(const struct nh_roles_info *ri, int rolenum,
			   int racenum, int gendnum, struct nh_listitem *list, int listlen)
{
    int i;
    int count = 0;
    
    for (i = 0; i < ri->num_aligns; i++) {
	if (!is_valid_character(ri, rolenum, racenum, gendnum, i))
	    continue;
	
	if (list && count < listlen) {
	    strcpy(list[count].caption, ri->alignnames[i]);
	    list[count].id = i;
	}
	count++;
    }
    return count;
}


static void validate_character_presets(const struct nh_roles_info *ri,
				       int *initrole, int *initrace,
				       int *initgend, int *initalign)
{
    struct nh_listitem list[1];
    char listbuffer[256];
    
    list[0].caption = listbuffer;
    
    /* throw out obviously invalid values */
    if (*initrole < ROLE_RANDOM || *initrole >= ri->num_roles)
	*initrole = ROLE_NONE;
    if (*initrace < ROLE_RANDOM || *initrace >= ri->num_races)
	*initrace = ROLE_NONE;
    if (*initgend < ROLE_RANDOM || *initgend >= ri->num_genders)
	*initgend = ROLE_NONE;
    if (*initalign < ROLE_RANDOM || *initalign >= ri->num_aligns)
	*initalign = ROLE_NONE;
    
    /* catch mutually incompatible character options (male valkyrie) */
    if (!is_valid_character(ri, *initrole, *initrace, *initgend, *initalign)) {
	/* always keep the role */
	if (is_valid_character(ri, *initrole, *initrace, *initgend, ROLE_NONE)) {
	    curses_msgwin("Incompatible alignment!");
	    *initalign = ROLE_NONE;
	} else if (is_valid_character(ri, *initrole, *initrace, ROLE_NONE, *initalign)) {
	    curses_msgwin("Incompatible gender!");
	    *initgend = ROLE_NONE;
	} else if (is_valid_character(ri, *initrole, ROLE_NONE, *initgend, *initalign)) {
	    curses_msgwin("Incompatible race!");
	    *initrace = ROLE_NONE;
	} else {
	    curses_msgwin("Incompatible character presets!");
	    *initrace = ROLE_NONE;
	    *initgend = ROLE_NONE;
	    *initalign = ROLE_NONE;
	}
    }
    
    /* if there is only one possible option for any attribute, choose it here */
    if ((*initrole == ROLE_NONE || *initrole == ROLE_RANDOM) &&
	get_valid_roles(ri, *initrace, *initgend, *initalign, list, 1) == 1)
	*initrole = list[0].id;
    
    if ((*initrace == ROLE_NONE || *initrace == ROLE_RANDOM) &&
	get_valid_races(ri, *initrole, *initgend, *initalign, list, 1) == 1)
	*initrace = list[0].id;
    
    if ((*initgend == ROLE_NONE || *initgend == ROLE_RANDOM) &&
	get_valid_genders(ri, *initrole, *initrace, *initalign, list, 1) == 1)
	*initgend = list[0].id;
    
    if ((*initalign == ROLE_NONE || *initalign == ROLE_RANDOM) &&
	get_valid_aligns(ri, *initrole, *initrace, *initgend, list, 1) == 1)
	*initalign = list[0].id;
}


static nh_bool find_role(const struct nh_roles_info *ri,
			 const char *role_str, int *out_role)
{
    int i;

    for (i = 0; i < ri->num_roles; i++) {
	if (!strcmp(role_str, ri->rolenames_m[i]) ||
	    (ri->rolenames_f[i] && !strcmp(role_str, ri->rolenames_f[i]))) {
	    *out_role = i;
	    return TRUE;
	}
    }
    return FALSE;
}


static nh_bool find_race(const struct nh_roles_info *ri,
			 const char *race_str, int *out_race)
{
    int i;

    for (i = 0; i < ri->num_races; i++) {
	if (!strcmp(race_str, ri->racenames[i])) {
	    *out_race = i;
	    return TRUE;
	}
    }
    return FALSE;
}


static nh_bool find_gend(const struct nh_roles_info *ri,
			 const char *gend_str, int *out_gend)
{
    int i;

    for (i = 0; i < ri->num_genders; i++) {
	if (!strcmp(gend_str, ri->gendnames[i])) {
	    *out_gend = i;
	    return TRUE;
	}
    }
    return FALSE;
}


static nh_bool find_align(const struct nh_roles_info *ri,
			  const char *align_str, int *out_align)
{
    int i;

    for (i = 0; i < ri->num_aligns; i++) {
	if (!strcmp(align_str, ri->alignnames[i])) {
	    *out_align = i;
	    return TRUE;
	}
    }
    return FALSE;
}


static nh_bool tutorial_player_selection(int *out_role, int *out_race,
					 int *out_gend, int *out_align)
{
    const struct nh_roles_info *ri;
    struct nh_menuitem items[4];	/* 3 options + quit */
    int pick_list[2];
    int result;

    ri = nh_get_roles();
    if (!ri)
	return FALSE;

    set_menuitem(&items[0], 1, MI_NORMAL,
		 "lawful female dwarf Valkyrie (uses melee and thrown weapons)",
		 'v', 0);
    set_menuitem(&items[1], 2, MI_NORMAL,
		 "chaotic male elf Wizard      (relies mostly on spells)",
		 'w', 0);
    set_menuitem(&items[2], 3, MI_NORMAL,
		 "neutral female human Ranger  (good with ranged combat)",
		 'R', 0);
    set_menuitem(&items[3], 4, MI_NORMAL,
		 "quit",
		 'q', 0);

    result = curses_display_menu(items, 4, "Choose a character", PICK_ONE, pick_list);
    if (result == -1 || pick_list[0] == -1)
	return FALSE;

    switch (pick_list[0]) {
    case 1:
	if (!find_role(ri, "Valkyrie", out_role) ||
	    !find_race(ri, "dwarf", out_race) ||
	    !find_gend(ri, "female", out_gend) ||
	    !find_align(ri, "lawful", out_align))
	    return FALSE;
	break;
    case 2:
	if (!find_role(ri, "Wizard", out_role) ||
	    !find_race(ri, "elf", out_race) ||
	    !find_gend(ri, "male", out_gend) ||
	    !find_align(ri, "chaotic", out_align))
	    return FALSE;
	break;
    case 3:
	if (!find_role(ri, "Ranger", out_role) ||
	    !find_race(ri, "human", out_race) ||
	    !find_gend(ri, "female", out_gend) ||
	    !find_align(ri, "neutral", out_align))
	    return FALSE;
	break;
    case 4:
    default:
	return FALSE;
	break;
    }

    if (!is_valid_character(ri, *out_role, *out_race, *out_gend, *out_align))
	return FALSE;

    return TRUE;
}


nh_bool player_selection(int *out_role, int *out_race, int *out_gend,
			 int *out_align, int randomall, nh_bool tutorial)
{
    struct nh_menuitem *items;
    int icount, size;
    int i, k, n, listlen, id;
    char pick4u = 'n', thisch, lastch = 0;
    char pbuf[QBUFSZ], plbuf[QBUFSZ];
    struct nh_listitem list[LISTSZ]; /* need enough space for lists of roles or races */
    char listbuffers[LISTSZ][256];
    int pick_list[2];
    int initrole, initrace, initgend, initalign;
    const struct nh_roles_info *ri = nh_get_roles();
    
    if (tutorial)
	return tutorial_player_selection(out_role, out_race, out_gend, out_align);
    
    initrole = *out_role; initrace = *out_race;
    initalign = *out_align; initgend = *out_gend;
    if (initrole == ROLE_NONE) initrole = ri->def_role;
    if (initrace == ROLE_NONE) initrace = ri->def_race;
    if (initgend == ROLE_NONE) initgend = ri->def_gend;
    if (initalign == ROLE_NONE) initalign = ri->def_align;
    
    validate_character_presets(ri, &initrole, &initrace, &initgend, &initalign);
    
    for (i = 0; i < LISTSZ; i++) {
	listbuffers[i][0] = '\0';
	list[i].caption = listbuffers[i];
    }
    
    srandom(time(NULL));
    
    /* Should we randomly pick for the player? */
    if (!randomall &&
	(initrole == ROLE_NONE || initrace == ROLE_NONE ||
	    initgend == ROLE_NONE || initalign == ROLE_NONE)) {
	char *prompt = nh_build_plselection_prompt(pbuf, QBUFSZ, initrole,
			    initrace, initgend, initalign);

	pick4u = curses_yn_function(prompt, "ynq", 'y');	
	if (pick4u != 'y' && pick4u != 'n')
	    return FALSE;
    }

    nh_root_plselection_prompt(plbuf, QBUFSZ - 1,
		    initrole, initrace, initgend, initalign);
    icount = 0; size = 10;
    items = malloc(sizeof(struct nh_menuitem) * size);

    /* Select a role, if necessary */
    /* if pre-selected race/gender/alignment are still set after
     * validate_character_presets we know they're OK */
    if (initrole < 0) {
	listlen = get_valid_roles(ri, initrace, initgend, initalign, list, LISTSZ);
	
	/* Process the choice */
	if (pick4u == 'y' || initrole == ROLE_RANDOM || randomall) {
	    /* Pick a random role */
	    initrole = list[random() % listlen].id;
	} else {
	    /* Prompt for a role */
	    for (i = 0; i < listlen; i++) {
		id = list[i].id + 1; /* list[i].id starts at 0 */
		thisch = tolower((unsigned char)*list[i].caption);
		if (thisch == lastch)
		    thisch = toupper((unsigned char)thisch);
		add_menu_item(items, size, icount, id, list[i].caption + 2, thisch, 0);
		lastch = thisch;
	    }
	    pick_list[0] = id = list[random() % listlen].id+1;
	    add_menu_item(items, size, icount, id, "随机", '*', 0);
	    add_menu_item(items, size, icount, -1, "退出", 'q', 0);
	    
	    sprintf(pbuf, "为你的%s选择一个角色", plbuf);
	    n = curses_display_menu(items, icount, pbuf, PICK_ONE, pick_list);

	    /* Process the choice */
	    if (n == -1 || pick_list[0] == -1)
		goto give_up;		/* Selected quit */

	    initrole = pick_list[0] - 1;
	    icount = 0;
	}
	nh_root_plselection_prompt(plbuf, QBUFSZ - 1,
		    initrole, initrace, initgend, initalign);
    }
    
    /* Select a race, if necessary */
    if (initrace < 0) {
	listlen = get_valid_races(ri, initrole, initgend, initalign, list, LISTSZ);
	
	if (pick4u == 'y' || initrace == ROLE_RANDOM || randomall) {
	    initrace = list[random() % listlen].id;
	} else {	/* pick4u == 'n' */
	    /* Count the number of valid races */
	    k = list[0].id;	/* valid race */

	    /* Permit the user to pick, if there is more than one */
	    if (listlen > 1) {
		for (i = 0; i < listlen; i++) {
		    id = list[i].id + 1;
		    add_menu_item(items, size, icount, id, list[i].caption + 2,
				  list[i].caption[0], 0);
		}
		pick_list[0] = id = list[random() % listlen].id+1;
		add_menu_item(items, size, icount, id, "随机", '*', 0);
		add_menu_item(items, size, icount, -1, "退出", 'q', 0);

		sprintf(pbuf, "为你的%s选择一个种族", plbuf);
		n = curses_display_menu(items, icount, pbuf, PICK_ONE, pick_list);
		
		if (n == -1 || pick_list[0] == -1)
		    goto give_up;		/* Selected quit */

		k = pick_list[0] - 1;
		icount = 0;
	    }
	    initrace = k;
	}
	nh_root_plselection_prompt(plbuf, QBUFSZ - 1,
		    initrole, initrace, initgend, initalign);
    }

    /* Select a gender, if necessary */
    if (initgend < 0) {
	listlen = get_valid_genders(ri, initrole, initrace, initalign, list, LISTSZ);

	if (pick4u == 'y' || initgend == ROLE_RANDOM || randomall) {
	    initgend = list[random() % listlen].id;
	} else {	/* pick4u == 'n' */
	    /* Count the number of valid genders */
	    k = list[0].id;	/* valid gender */

	    /* Permit the user to pick, if there is more than one */
	    if (listlen > 1) {
		for (i = 0; i < listlen; i++) {
		    id = list[i].id + 1;
		    add_menu_item(items, size, icount, id, list[i].caption + 2,
				  list[i].caption[0], 0);
		}
		pick_list[0] = id = list[random() % listlen].id+1;
		add_menu_item(items, size, icount, id, "随机", '*', 0);
		add_menu_item(items, size, icount, -1, "退出", 'q', 0);

		sprintf(pbuf, "为你的%s选择一个性别", plbuf);
		n = curses_display_menu(items, icount, pbuf, PICK_ONE, pick_list);
		
		if (n == -1 || pick_list[0] == -1)
		    goto give_up;		/* Selected quit */

		k = pick_list[0] - 1;
		icount = 0;
	    }
	    initgend = k;
	}
	nh_root_plselection_prompt(plbuf, QBUFSZ - 1,
		    initrole, initrace, initgend, initalign);
    }

    /* Select an alignment, if necessary */
    if (initalign < 0) {
	listlen = get_valid_aligns(ri, initrole, initrace, initgend, list, LISTSZ);
	
	if (pick4u == 'y' || initalign == ROLE_RANDOM || randomall) {
	    initalign = list[random() % listlen].id;
	} else {	/* pick4u == 'n' */
	    /* Count the number of valid alignments */
	    k = list[0].id;	/* valid alignment */

	    /* Permit the user to pick, if there is more than one */
	    if (listlen > 1) {
		for (i = 0; i < listlen; i++) {
		    id = list[i].id + 1;
		    add_menu_item(items, size, icount, id, list[i].caption + 2,
				  list[i].caption[0], 0);
		}
		pick_list[0] = id = list[random() % listlen].id+1;
		add_menu_item(items, size, icount, id, "随机", '*', 0);
		add_menu_item(items, size, icount, -1, "退出", 'q', 0);
		
		sprintf(pbuf, "为你的%s选择一个阵营", plbuf);
		n = curses_display_menu(items, icount, pbuf, PICK_ONE, pick_list);
		
		if (n == -1 || pick_list[0] == -1)
		    goto give_up;		/* Selected quit */

		k = pick_list[0] - 1;
	    }
	    initalign = k;
	}
    }
    
    *out_role = initrole;
    *out_race = initrace;
    *out_gend = initgend;
    *out_align = initalign;
    
    free(items);
    return TRUE;
    
give_up:
    free(items);
    return FALSE;
}
