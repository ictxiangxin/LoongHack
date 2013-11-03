/* Copyright (c) Daniel Thaler, 2011 */
/* DynaHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>



enum option_lists {
    NO_LIST,
    ACT_BIRTH_OPTS,
    CUR_BIRTH_OPTS,
    GAME_OPTS,
    UI_OPTS
};

enum optstyles {
    OPTSTYLE_DESC,
    OPTSTYLE_NAME,
    OPTSTYLE_FULL
};

enum extra_opttypes {
    OPTTYPE_KEYMAP = 1000
};

static void read_ui_config(void);
static void show_autopickup_menu(struct nh_option_desc *opt);
static void show_msgtype_menu(struct nh_option_desc *opt);


#if defined(NETCLIENT)
# define should_write_config() (!nhnet_active())
#else
# define should_write_config() (1)
#endif


/*----------------------------------------------------------------------------*/

#define listlen(list) (sizeof(list)/sizeof(list[0]))

static struct nh_listitem dungeon_name_list[] = {
    {DGN_NAME_AUTO, "auto"},
    {DGN_NAME_DLVL, "dungeon level"},
    {DGN_NAME_SHORT, "short dungeon name"},
    {DGN_NAME_FULL, "full dungeon name"},
};
static struct nh_enum_option dungeon_name_spec =
{dungeon_name_list, listlen(dungeon_name_list)};

static struct nh_listitem menu_headings_list[] = {
    {A_NORMAL, "none"},
    {A_REVERSE, "inverse"},
    {A_BOLD, "bold"},
    {A_UNDERLINE, "underline"}
};
static struct nh_enum_option menu_headings_spec =
{menu_headings_list, listlen(menu_headings_list)};

static struct nh_listitem graphics_list[] = {
    {ASCII_GRAPHICS, "plain"},
    {UNICODE_GRAPHICS, "Unicode graphics"}
};
static struct nh_enum_option graphics_spec = {graphics_list, listlen(graphics_list)};

static struct nh_listitem optstyle_list[] = {
    {OPTSTYLE_DESC, "description only"},
    {OPTSTYLE_NAME, "name only"},
    {OPTSTYLE_FULL, "name + description"}
};
static struct nh_enum_option optstyle_spec = {optstyle_list, listlen(optstyle_list)};


static const char *const bucnames[] = {"unknown", "blessed", "uncursed", "cursed", "all"};


#define VTRUE (void*)TRUE

struct nh_option_desc curses_options[] = {
    {"name", "命名新角色（空白=创建时询问）", OPTTYPE_STRING, {NULL}},
    {"classic_status", "使用经典的NetHack样式来显示状态栏", OPTTYPE_BOOL, { FALSE }},
    {"darkgray", "尝试使用黑色显示深灰色来替代深蓝色", OPTTYPE_BOOL,
#if defined(__linux__)
	{ VTRUE }
#else
	/* Windows and default OS X terminal config have problems with darkgray */
	{ FALSE }
#endif
    },
    {"darkmsg", "将过时信息显示为暗色", OPTTYPE_BOOL,
#if defined(__linux__)
	{ VTRUE }
#else
	/* default dark blue on Windows is far too dark to be legible */
	{ FALSE }
#endif
    },
    {"darkroom", "不在视野内的地方用暗色表示", OPTTYPE_BOOL,
#if defined(__linux__)
	{ VTRUE }
#else
	/* darkroom tends to look awful without working darkgray */
	{ FALSE }
#endif
    },
    {"dungeon_name", "如何显示地下城的名称和层数", OPTTYPE_ENUM, {(void*)DGN_NAME_AUTO}},
    {"extmenu", "使用目录来显示扩展命令（#）", OPTTYPE_BOOL, {FALSE}},
    {"frame", "绘制一个框架环绕窗体", OPTTYPE_BOOL, { VTRUE }},
    {"frame_hp_color", "框架颜色随生命值而改变", OPTTYPE_BOOL, { VTRUE }},
    {"graphics", "图像显示模式（LoongHack只能使用Unicode模式！）", OPTTYPE_ENUM, {(void*)UNICODE_GRAPHICS}},
    {"hilite_peaceful", "高亮处于和平状态的怪物", OPTTYPE_BOOL, { VTRUE }},
    {"hilite_pet", "高亮你的宠物", OPTTYPE_BOOL, { VTRUE }},
    {"invweight", "在物品栏中显示物品的重量", OPTTYPE_BOOL, { VTRUE }},
    {"keymap", "改变命令的键位", OPTTYPE_KEYMAP, {0}},
    {"mapcolors", "使用为特殊地图场景使用对应的环境着色", OPTTYPE_BOOL, { VTRUE }},
    {"menu_headings", "在目录顶端显示样式", OPTTYPE_ENUM, {(void*)A_REVERSE}},
    {"msgheight", "消息窗口的高度（0代表自动）", OPTTYPE_INT, {0}},
    {"msghistory", "“历史消息”保留的消息数", OPTTYPE_INT, {(void*)256}},
    {"optstyle", "“设置”目录使用样式", OPTTYPE_ENUM, {(void*)OPTSTYLE_FULL}},
    {"scores_own", "在列表中显示你自己的分数", OPTTYPE_BOOL, { FALSE }},
    {"scores_top", "在排行榜中显示的条目数", OPTTYPE_INT, {(void*)3}},
    {"scores_around", "在你的分数附近的条目数", OPTTYPE_INT, {(void*)2}},
    {"showexp", "显示经验值", OPTTYPE_BOOL, {VTRUE}},
    {"showscore", "现状态栏中显示分数", OPTTYPE_BOOL, {FALSE}},
    {"sidebar", "显示边侧物品栏", OPTTYPE_BOOL, { VTRUE }},
    {"standout", "用standout显示--More--", OPTTYPE_BOOL, { VTRUE }},
    {"status3", "3行状态栏显示模式", OPTTYPE_BOOL, { VTRUE }},
    {"time", "在状态栏中显示游戏回合数", OPTTYPE_BOOL, {VTRUE}},
#if defined(PDCURSES) && defined(WIN32)
    {"win_width", "窗口宽度", OPTTYPE_INT, {(void*)130}},
    {"win_height", "窗口高度", OPTTYPE_INT, {(void*)40}},
#endif
    {NULL, NULL, OPTTYPE_BOOL, { NULL }}
};

struct nh_boolopt_map boolopt_map[] = {
    {"classic_status", &settings.classic_status},
    {"darkgray", &settings.darkgray},
    {"darkmsg", &settings.darkmsg},
    {"darkroom", &settings.darkroom},
    {"extmenu", &settings.extmenu},
    {"frame", &settings.frame},
    {"frame_hp_color", &settings.frame_hp_color},
    {"hilite_peaceful", &settings.hilite_peaceful},
    {"hilite_pet", &settings.hilite_pet},
    {"invweight", &settings.invweight},
    {"mapcolors", &settings.mapcolors},
    {"scores_own", &settings.end_own},
    {"showexp", &settings.showexp},
    {"showscore", &settings.showscore},
    {"sidebar", &settings.sidebar},
    {"standout", &settings.standout},
    {"status3", &settings.status3},
    {"time", &settings.time},
    {NULL, NULL}
};


static nh_bool option_change_callback(struct nh_option_desc *option)
{
    if (!strcmp(option->name, "classic_status") ||
	!strcmp(option->name, "darkmsg") ||
	!strcmp(option->name, "frame") ||
	!strcmp(option->name, "frame_hp_color") ||
	!strcmp(option->name, "status3") ||
	!strcmp(option->name, "sidebar")) {
	rebuild_ui();
	return TRUE;
    }
    else if (!strcmp(option->name, "showexp") ||
	!strcmp(option->name, "showscore") ||
	!strcmp(option->name, "time")) {
	curses_update_status(NULL);
    }
    else if (!strcmp(option->name, "darkroom") ||
	!strcmp(option->name, "hilite_peaceful") ||
	!strcmp(option->name, "hilite_pet") ||
	!strcmp(option->name, "mapcolors")) {
	draw_map(player.x, player.y);
    }
    else if (!strcmp(option->name, "darkgray")) {
	set_darkgray();
	draw_map(player.x, player.y);
    }
    else if (!strcmp(option->name, "dungeon_name")) {
	settings.dungeon_name = option->value.e;
	rebuild_ui();
    }
    else if (!strcmp(option->name, "menu_headings")) {
	settings.menu_headings = option->value.e;
    }
    else if (!strcmp(option->name, "graphics")) {
	settings.graphics = option->value.e;
	switch_graphics(option->value.e);
	if (ui_flags.ingame) {
	    draw_map(player.x, player.y);
	    redraw_game_windows();
	}
    }
    else if (!strcmp(option->name, "scores_top")) {
	settings.end_top = option->value.i;
    }
    else if (!strcmp(option->name, "scores_around")) {
	settings.end_around = option->value.i;
    }
    else if (!strcmp(option->name, "optstyle")) {
	settings.optstyle = option->value.e;
    }
    else if (!strcmp(option->name, "msgheight")) {
	settings.msgheight = option->value.i;
	rebuild_ui();
    }
    else if (!strcmp(option->name, "msghistory")) {
	settings.msghistory = option->value.i;
	alloc_hist_array();
    }
#if defined(PDCURSES) && defined(WIN32)
    else if (!strcmp(option->name, "win_width")) {
	settings.win_width = option->value.i;
	resize_term(settings.win_height, settings.win_width);
	handle_resize();
    }
    else if (!strcmp(option->name, "win_height")) {
	settings.win_height = option->value.i;
	resize_term(settings.win_height, settings.win_width);
	handle_resize();
    }
#endif
    else if (!strcmp(option->name, "name")) {
	if (option->value.s)
	    strcpy(settings.plname, option->value.s);
	else
	    settings.plname[0] = '\0';
    }
    else
	return FALSE;
    
    return TRUE;
}


static struct nh_option_desc *find_option(const char *name)
{
    int i;
    for (i = 0; curses_options[i].name; i++)
	if (!strcmp(name, curses_options[i].name))
	    return &curses_options[i];
    
    return NULL;
}


void init_options(void)
{
    int i;

    find_option("name")->s.maxlen = PL_NSIZ;
    find_option("dungeon_name")->e = dungeon_name_spec;
    find_option("menu_headings")->e = menu_headings_spec;
    find_option("msgheight")->i.min = 0;
    find_option("msgheight")->i.max = MAX_MSGLINES;
    find_option("msghistory")->i.min = 20;   /* arbitrary min/max values */
    find_option("msghistory")->i.max = 20000;
    find_option("graphics")->e = graphics_spec;
    find_option("optstyle")->e = optstyle_spec;
    find_option("scores_top")->i.max = 10000;
    find_option("scores_around")->i.max = 100;
#if defined(PDCURSES) && defined(WIN32)
    find_option("win_width")->i.min = COLNO; /* can never be narrower than COLNO */
    find_option("win_width")->i.max = 100 + COLNO; /* 100 chars wide sidebar already looks pretty silly */
    find_option("win_height")->i.min = ROWNO + 3;
    find_option("win_height")->i.max = 70; /* ROWNO + max msgheight + extra for status and frame */
#endif
    nh_setup_ui_options(curses_options, boolopt_map, option_change_callback);
    
    /* set up option defaults; this is necessary for options that are not
     * specified in the config file */
    for (i = 0; curses_options[i].name; i++)
	nh_set_option(curses_options[i].name, curses_options[i].value, FALSE);
    
    read_ui_config();
}


static const char* get_display_string(struct nh_option_desc *option)
{
    switch ((int)option->type) {
	default:
	case OPTTYPE_BOOL:
	case OPTTYPE_ENUM:
	case OPTTYPE_INT:
	case OPTTYPE_STRING:
	    return nh_get_option_string(option);
	    
	case OPTTYPE_AUTOPICKUP_RULES:
	case OPTTYPE_MSGTYPE:
	case OPTTYPE_KEYMAP:
	    return "submenu";
    }
}


static void print_option_string(struct nh_option_desc *option, char *buf)
{
    char fmt[16];
    const char *opttxt;
    const char *valstr = get_display_string(option);
    
    switch (settings.optstyle) {
	case OPTSTYLE_DESC:
	    opttxt = option->helptxt;
	    if (!opttxt || strlen(opttxt) < 2)
		opttxt = option->name;
	    
	    sprintf(fmt, "%%.%ds\t[%%s]", COLS - 21);
	    snprintf(buf, BUFSZ, fmt, opttxt, valstr);
	    break;
	    
	case OPTSTYLE_NAME:
	    sprintf(fmt, "%%.%ds\t[%%s]", COLS - 21);
	    snprintf(buf, BUFSZ, fmt, option->name, valstr);
	    break;
	    
	default:
	case OPTSTYLE_FULL:
	    sprintf(fmt, "%%s\t[%%s]\t%%.%ds", COLS - 42);
	    snprintf(buf, BUFSZ, fmt, option->name, valstr, option->helptxt);
	    break;
    }
}


/* add a list of options to the given selection menu */
static int menu_add_options(struct nh_menuitem **items, int *size, int *icount,
	    int listid, struct nh_option_desc *options, nh_bool read_only)
{
    int i, id;
    char optbuf[256];
    
    for (i = 0; options[i].name; i++) {
	id = (listid << 10) | i;
	print_option_string(&options[i], optbuf);
	if (read_only)
	    add_menu_txt(*items, *size, *icount, optbuf, MI_TEXT);
	else
	    add_menu_item(*items, *size, *icount, id, optbuf, 0, 0);
    }
    
    /* add an empty line */
    add_menu_txt(*items, *size, *icount, "", MI_TEXT);
    
    return i;
}


/* display a selecton menu for boolean options */
static void select_boolean_value(union nh_optvalue *value, struct nh_option_desc *option)
{
    struct nh_menuitem *items;
    int icount, size;
    int n, pick_list[2];

    icount = 0; size = 4;
    items = malloc(sizeof(struct nh_menuitem) * size);

    add_menu_txt(items, size, icount, option->helptxt, MI_TEXT);
    add_menu_txt(items, size, icount, "", MI_TEXT);
    add_menu_item(items, size, icount, 1,
		  option->value.b ? "是（已设）" : "是", 't', 0);
    add_menu_item(items, size, icount, 2,
		  option->value.b ? "否" : "否（已设）", 'f', 0);

    n = curses_display_menu(items, icount, option->name, PICK_ONE, pick_list);
    free(items);

    value->b = option->value.b; /* in case of ESC */
    if (n == 1)
	value->b = pick_list[0] == 1;
}


/* display a selection menu for enum options */
static void select_enum_value(union nh_optvalue *value, struct nh_option_desc *option)
{
    struct nh_menuitem *items;
    int icount, size;
    int i, n, selectidx, *pick_list;

    icount = 0; size = 10;
    items = malloc(sizeof(struct nh_menuitem) * size);

    add_menu_txt(items, size, icount, option->helptxt, MI_TEXT);
    add_menu_txt(items, size, icount, "", MI_TEXT);

    for (i = 0; i < option->e.numchoices; i++) {
	char capbuf[QBUFSZ];
	char *cap;
	if (option->value.e == option->e.choices[i].id) {
	    snprintf(capbuf, QBUFSZ, "%s（已设）", option->e.choices[i].caption);
	    cap = capbuf;
	} else {
	    cap = option->e.choices[i].caption;
	}
	/* don't use the choice ids directly, 0 is a valid value for those */
	add_menu_item(items, size, icount, i+1, cap, 0, 0);
    }

    pick_list = malloc(sizeof(int) * icount);
    n = curses_display_menu(items, icount, option->name, PICK_ONE, pick_list);
    free(items);

    value->e = option->value.e; /* in case of ESC */
    if (n == 1) {
	selectidx = pick_list[0] - 1;
	value->e = option->e.choices[selectidx].id;
    }
    free(pick_list);
}


/* get a new value of the appropriate type for the given option */
static nh_bool get_option_value(struct win_menu *mdat, int idx)
{
    char buf[BUFSZ], query[BUFSZ];
    union nh_optvalue value;
    struct nh_option_desc *option, *optlist;
    int listid = mdat->items[idx].id >> 10;
    int id = mdat->items[idx].id & 0x1ff;
    char strbuf[BUFSZ];
    int prev_optstyle = settings.optstyle;
    
    switch (listid) {
	case ACT_BIRTH_OPTS:
	    optlist = nh_get_options(ACTIVE_BIRTH_OPTIONS); break;
	case CUR_BIRTH_OPTS:
	    optlist = nh_get_options(CURRENT_BIRTH_OPTIONS); break;
	case GAME_OPTS:
	    optlist = nh_get_options(GAME_OPTIONS); break;
	case UI_OPTS:
	    optlist = curses_options; break;
	    
	default:
	    return FALSE;
    }
    
    option = &optlist[id];
    value.s = strbuf;
    
    switch ((int)option->type) {
	case OPTTYPE_BOOL:
	    select_boolean_value(&value, option);
	    break;
	    
	case OPTTYPE_INT:
	    sprintf(query, "New value for %s (number from %d to %d)",
		    option->name, option->i.min, option->i.max);
	    sprintf(buf, "%d", value.i);
	    curses_getline(query, buf);
	    if (buf[0] == '\033')
		return FALSE;
	    sscanf(buf, "%d", &value.i);
	    break;
	    
	case OPTTYPE_ENUM:
	    select_enum_value(&value, option);
	    break;
	    
	case OPTTYPE_STRING:
	    sprintf(query, "New value for %s (text)", option->name);
	    curses_getline(query, value.s);
	    if (value.s[0] == '\033')
		return FALSE;
	    break;
	    
	case OPTTYPE_AUTOPICKUP_RULES:
	    show_autopickup_menu(option);
	    return FALSE;
	    
	case OPTTYPE_MSGTYPE:
	    show_msgtype_menu(option);
	    return FALSE;
	    
	case OPTTYPE_KEYMAP:
	    show_keymap_menu(FALSE);
	    return FALSE;
	    
	default:
	    return FALSE;
    }
    
    if (!nh_set_option(option->name, value, FALSE)) {
	sprintf(strbuf, "new value for %s rejected", option->name);
	curses_msgwin(strbuf);
    } else
	print_option_string(option, mdat->items[idx].caption);
    
    /* special case: directly redo option menu appearance */
    if (settings.optstyle != prev_optstyle)
	return TRUE;
    
    return FALSE;
}


/* display the option dialog */
void display_options(nh_bool change_birth_opt)
{
    struct nh_menuitem *items;
    int icount, size;
    struct nh_option_desc *nhoptions = nh_get_options(GAME_OPTIONS);
    struct nh_option_desc *birthoptions = NULL;
    int n;
    
    size = 10;
    items = malloc(sizeof(struct nh_menuitem) * size);
    
    do {
	icount = 0;
	if (!change_birth_opt) {
	    birthoptions = nh_get_options(ACTIVE_BIRTH_OPTIONS);
	    /* add general game options */
	    add_menu_txt(items, size, icount, "游戏设置：", MI_HEADING);
	    menu_add_options(&items, &size, &icount, GAME_OPTS, nhoptions,
				FALSE);
	    
	    /* add or display birth options */
	    add_menu_txt(items, size, icount, "本次游戏人物设置：",
			    MI_HEADING);
	    menu_add_options(&items, &size, &icount, ACT_BIRTH_OPTS,
				birthoptions, TRUE);
	} else {
	    birthoptions = nh_get_options(CURRENT_BIRTH_OPTIONS);
	    /* add or display birth options */
	    add_menu_txt(items, size, icount, "创建人物设置：", MI_HEADING);
	    menu_add_options(&items, &size, &icount, CUR_BIRTH_OPTS,
				birthoptions, FALSE);
	    
	    add_menu_txt(items, size, icount, "游戏设置：", MI_HEADING);
	    menu_add_options(&items, &size, &icount, GAME_OPTS, nhoptions, FALSE);
	}
	
	/* add UI specific options */
	add_menu_txt(items, size, icount, "界面设置：", MI_HEADING);
	menu_add_options(&items, &size, &icount, UI_OPTS, curses_options, FALSE);
	
	n = curses_display_menu_core(items, icount, "选择哪个设置？", PICK_ONE,
				     NULL, 0, 0, -1, -1, get_option_value, FALSE);
    } while (n > 0);
    free(items);
    
    write_config();
}


void print_options(void)
{
    struct nh_menuitem *items;
    int i, icount, size;
    char buf[BUFSZ];
    struct nh_option_desc *options;

    icount = 0; size = 10;
    items = malloc(sizeof(struct nh_menuitem) * size);
    
    add_menu_txt(items, size, icount, "创建人物设置：", MI_HEADING);
    options = nh_get_options(CURRENT_BIRTH_OPTIONS);
    for (i = 0; options[i].name; i++) {
	snprintf(buf, BUFSZ, "%s\t%s", options[i].name, options[i].helptxt);
	add_menu_txt(items, size, icount, buf, MI_TEXT);
    }
    add_menu_txt(items, size, icount, "", MI_TEXT);
    
    add_menu_txt(items, size, icount, "游戏设置：", MI_HEADING);
    options = nh_get_options(GAME_OPTIONS);
    for (i = 0; options[i].name; i++) {
	snprintf(buf, BUFSZ, "%s\t%s", options[i].name, options[i].helptxt);
	add_menu_txt(items, size, icount, buf, MI_TEXT);
    }
    add_menu_txt(items, size, icount, "", MI_TEXT);

    /* add UI specific options */
    add_menu_txt(items, size, icount, "界面设置：", MI_HEADING);
    for (i = 0; curses_options[i].name; i++) {
	snprintf(buf, BUFSZ, "%s\t%s", curses_options[i].name, curses_options[i].helptxt);
	add_menu_txt(items, size, icount, buf, MI_TEXT);
    }

    curses_display_menu(items, icount, "可用选项：", PICK_NONE, NULL);
    free(items);
}

/*----------------------------------------------------------------------------*/

static void autopickup_rules_help(void)
{
    struct nh_menuitem items[] = {
	{0, MI_TEXT, "自动拾取规则只有当开启自动拾取功能时生效。"},
	{0, MI_TEXT, "当自动拾取功能开启时，你走到一件物品上方，这件物品"},
	{0, MI_TEXT, "将与每个规则相比较，直到有一个规则满足。"},
	{0, MI_TEXT, "如果存在一个满足的规则，与此规则相对应的动作将被执"},
	{0, MI_TEXT, "行，然后其他的规矩将被忽略。"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "每个规则都可以匹配物体名称、类型或祝福状态的任意组"},
	{0, MI_TEXT, "何（包括未知的情况）。"},
	{0, MI_TEXT, "可以设置一个规则来匹配任意的情况。"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "加入你的匹配规则像这样："},
	{0, MI_TEXT, " 1. IF name matches \"*lizard*\" AND type is \"food\": < GRAB"},
	{0, MI_TEXT, " 2. IF name matches \"*corpse*\" AND type is \"food\":   LEAVE >"},
	{0, MI_TEXT, " 3. IF type is \"food\":                             < GRAB"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "“newt corpse”将不满足规则1，但是会满足规则2。"},
	{0, MI_TEXT, "它将不会被拾取，规则3也将被忽略。"},
	{0, MI_TEXT, "（严格地说，\"type is food\"这条规则并不是必须"},
	{0, MI_TEXT, "的；它在这只是为了让例子更有趣一些。）"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "“dagger”将不会匹配上述任何一种情况，所有它将不会"},
	{0, MI_TEXT, "被自动拾取。"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "你可以在列表中选择一个已存在的规则来编辑它，修改它"},
	{0, MI_TEXT, "的条件，或者删除它。"},
    };
    curses_display_menu(items, listlen(items), "自动拾取规则帮助：", PICK_NONE, NULL);
}


static enum nh_bucstatus get_autopickup_buc(enum nh_bucstatus cur)
{
    struct nh_menuitem items[] = {
	{B_DONT_CARE + 1,MI_NORMAL, "all", 'a'},
	{B_BLESSED + 1,	MI_NORMAL, "blessed", 'b'},
	{B_CURSED + 1,	MI_NORMAL, "cursed", 'c'},
	{B_UNCURSED + 1,MI_NORMAL, "uncursed", 'u'},
	{B_UNKNOWN + 1,	MI_NORMAL, "unknown", 'U'}
    };
    int n, selected[1];
    n = curses_display_menu(items, 5, "Beatitude match:", PICK_ONE, selected);
    if (n <= 0)
	return cur;
    return selected[0]-1;
}


static int get_autopickup_oclass(struct nh_autopick_option *desc, int cur)
{
    int i, n, size, icount, selected[1];
    struct nh_menuitem *items;
    
    size = desc->numclasses;
    items = malloc(sizeof(struct nh_menuitem) * size);
    icount = 0;
    
    for (i = 0; i < desc->numclasses; i++)
	add_menu_item(items, size, icount, desc->classes[i].id,
		      desc->classes[i].caption, (char)desc->classes[i].id, 0);
	
    n = curses_display_menu(items, icount, "Object class match:", PICK_ONE, selected);
    free(items);
    if (n <= 0)
	return cur;
    return selected[0];
}


static void edit_ap_rule(struct nh_autopick_option *desc,
			 struct nh_autopickup_rules *ar, int ruleno)
{
    struct nh_autopickup_rule *r = &ar->rules[ruleno];
    struct nh_autopickup_rule tmprule;
    struct nh_menuitem *items;
    int i, icount, size = 7, n, selected[1], newpos;
    char query[BUFSZ], buf[BUFSZ], *classname;
    
    items = malloc(sizeof(struct nh_menuitem) * size);
    
    do {
	icount = 0;
	sprintf(buf, "rule position:\t[%d]", ruleno + 1);
	add_menu_item(items, size, icount, 1, buf, 0, 0);
	
	sprintf(buf, "name pattern:\t[%s]", r->pattern);
	add_menu_item(items, size, icount, 2, buf, 0, 0);
	
	classname = NULL;
	for (i = 0; i < desc->numclasses && !classname; i++)
	    if (desc->classes[i].id == r->oclass)
		classname = desc->classes[i].caption;
	sprintf(buf, "object type:\t[%s]", classname);
	add_menu_item(items, size, icount, 3, buf, 0, 0);
	
	sprintf(buf, "beatitude:\t[%s]", bucnames[r->buc]);
	add_menu_item(items, size, icount, 4, buf, 0, 0);
	
	sprintf(buf, "action:\t[%s]", r->action == AP_GRAB ? "GRAB" : "LEAVE");
	add_menu_item(items, size, icount, 5, buf, 0, 0);
	add_menu_txt(items, size, icount, "", MI_TEXT);
	add_menu_item(items, size, icount, 6, "delete this rule", 'x', 0);
	
	n = curses_display_menu(items, icount, "Edit rule:", PICK_ONE, selected);
	if (n <= 0)
	    break;
	
	switch (selected[0]) {
	    /* move this rule */
	    case 1:
		sprintf(query, "New rule position: (1 - %d), currently: %d",
			ar->num_rules, ruleno + 1);
		buf[0] = '\0';
		curses_getline(query, buf);
		if (!*buf || *buf == '\033')
		    break;
		newpos = atoi(buf);
		if (newpos <= 0 || newpos > ar->num_rules) {
		    curses_msgwin("Invalid rule position.");
		    break;
		}
		newpos--;
		if (newpos == ruleno)
		    break;
		
		tmprule = ar->rules[ruleno];
		/* shift the rules around */
		if (newpos > ruleno) {
		    for (i = ruleno; i < newpos; i++)
			ar->rules[i] = ar->rules[i+1];
		} else {
		    for (i = ruleno; i > newpos; i--)
			ar->rules[i] = ar->rules[i-1];
		}
		ar->rules[newpos] = tmprule;
		goto out;
		
	    /* edit the pattern */
	    case 2:
		sprintf(query, "New name pattern (empty matches everything):");
		buf[0] = '\0';
		curses_getline(query, buf);
		if (*buf != '\033')
		    strncpy(r->pattern, buf, sizeof(r->pattern));
		r->pattern[sizeof(r->pattern)-1] = '\0';
		break;
	    
	    /* edit object class match */
	    case 3:
		r->oclass = get_autopickup_oclass(desc, r->oclass);
		break;
	    
	    /* edit beatitude match */
	    case 4:
		r->buc = get_autopickup_buc(r->buc);
		break;
		
	    /* toggle action */
	    case 5:
		if (r->action == AP_GRAB)
		    r->action = AP_LEAVE;
		else
		    r->action = AP_GRAB;
		break;
		
	    /* delete */
	    case 6:
		for (i = ruleno; i < ar->num_rules - 1; i++)
		    ar->rules[i] = ar->rules[i+1];
		ar->num_rules--;
		ar->rules = realloc(ar->rules, ar->num_rules * sizeof(struct nh_autopickup_rule));
		goto out; /* break just beaks the switch .. doh */
	}
	
    } while (n > 0);
out:
    free(items);
}


static void show_autopickup_menu(struct nh_option_desc *opt)
{
    struct nh_menuitem *items;
    int i, j, n, icount, size, menusize, parts, selected[1], id;
    struct nh_autopickup_rule *r;
    char buf[BUFSZ];
    struct nh_autopickup_rule *rule;
    union nh_optvalue value;
    
    /* clone autopickup rules */
    value.ar = malloc(sizeof(struct nh_autopickup_rules));
    value.ar->num_rules = 0;
    value.ar->rules = NULL;
    if (opt->value.ar){
	value.ar->num_rules = opt->value.ar->num_rules;
	size = value.ar->num_rules * sizeof(struct nh_autopickup_rule);
	value.ar->rules = malloc(size);
	memcpy(value.ar->rules, opt->value.ar->rules, size);
    }
    
    menusize = value.ar->num_rules + 4;
    items = malloc(sizeof(struct nh_menuitem) * menusize);

    selected[0] = 0;
    do {
	icount = 0;
	
	add_menu_txt(items, menusize, icount, "Pos\tRule\tAction", MI_HEADING);
	
	/* list the rules in human-readable form */
	for (i = 0; i < value.ar->num_rules; i++) {
	    r = &value.ar->rules[i];
	    parts = 0;
	    sprintf(buf, "%2d.\tIF ", i+1);
	    
	    if (strlen(r->pattern)) {
		parts++;
		sprintf(buf + strlen(buf), "name matches \"%s\"", r->pattern);
	    }
	    
	    if (r->oclass != OCLASS_ANY) {
		char *classname = NULL;
		for (j = 0; j < opt->a.numclasses && !classname; j++)
		    if (opt->a.classes[j].id == r->oclass)
			classname = opt->a.classes[j].caption;
		
		if (parts++)
		    strcat(buf, " AND ");
		sprintf(buf + strlen(buf), "type is \"%s\"", classname);
	    }
	    
	    if (r->buc != B_DONT_CARE) {
		if (parts++)
		    strcat(buf, " AND ");
		sprintf(buf + strlen(buf), "beatitude is %s", bucnames[r->buc]);
	    }
	    
	    if (!parts)
		sprintf(buf, "%2d.\teverything", i+1);
	    
	    if (r->action == AP_GRAB)
		sprintf(buf + strlen(buf), ":\t< GRAB");
	    else
		sprintf(buf + strlen(buf), ":\t  LEAVE >");
	    
	    add_menu_item(items, menusize, icount, i+1, buf, 0, 0);
	}
	
	add_menu_txt(items, menusize, icount, "", MI_TEXT);
	add_menu_item(items, menusize, icount, -1, "add a new rule", '!', 0);
	add_menu_item(items, menusize, icount, -2, "help", '?', 0);
	
	/* If the previous selection was to add a rule, scroll to the bottom now
	 * so that the player can see it. */
	if (selected[0] == -1) {
	    n = curses_display_menu_bottom(items, icount, "Autopickup rules:",
					   PICK_ONE, selected);
	} else {
	    n = curses_display_menu(items, icount, "Autopickup rules:",
				    PICK_ONE, selected);
	}
	if (n <= 0)
	    break;
	
	/* add or edit a rule */
	id = selected[0];
	if (id == -1) {
	    /* create a new rule */
	    id = value.ar->num_rules;
	    value.ar->num_rules++;
	    size = value.ar->num_rules * sizeof(struct nh_autopickup_rule);
	    value.ar->rules = realloc(value.ar->rules, size);
	    
	    rule = &value.ar->rules[id];
	    rule->pattern[0] = '\0';
	    rule->oclass = OCLASS_ANY;
	    rule->buc = B_DONT_CARE;
	    rule->action = AP_GRAB;
	} else if (id == -2) {
	    autopickup_rules_help();
	    continue;
	} else
	    id--;
	
	edit_ap_rule(&opt->a, value.ar, id);
    } while (n > 0);
    
    nh_set_option(opt->name, value, FALSE);
    
    free(value.ar->rules);
    free(value.ar);
    free(items);
}

/*----------------------------------------------------------------------------*/

static void msgtype_help(void)
{
    struct nh_menuitem items[] = {
	{0, MI_TEXT, "消息类型能让你将某些消息在游戏中以不同的方式"},
	{0, MI_TEXT, "显示出来，像暂停等待输入 --More--，"},
	{0, MI_TEXT, "重复崩溃的消息或者隐藏的消息入口。"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "一个匹配的消息可能的动作："},
	{0, MI_TEXT, "    DEFAULT    视为一条普通的消息"},
	{0, MI_TEXT, "    MORE       在这条消息之后用 --More-- 暂停"},
	{0, MI_TEXT, "    NO REPEAT  隐藏这条消息的重复部分"},
	{0, MI_TEXT, "    HIDE       完全隐藏这条消息"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "被匹配的消息的排序模式："},
	{0, MI_TEXT, "    ?                    匹配任何单个字符"},
	{0, MI_TEXT, "    *                    匹配任何长度的字符串"},
	{0, MI_TEXT, "    所有其他的字符匹配它自己"},
	{0, MI_TEXT, ""},
	{0, MI_TEXT, "选择一个已有的项来编辑，重排或者删除它。"},
    };
    curses_display_menu(items, listlen(items), "消息类型帮助：",
			PICK_NONE, NULL);
}

static const char *msgtype_action_string(enum msgtype_action action)
{
    return action == MSGTYPE_DEFAULT ? "DEFAULT" :
	   action == MSGTYPE_MORE ? "MORE" :
	   action == MSGTYPE_NO_REPEAT ? "NO REPEAT" :
	   action == MSGTYPE_HIDE ? "HIDE" : "(invalid)";
}

static enum msgtype_action get_msgtype_action(enum msgtype_action current)
{
    struct nh_menuitem items[] = {
	{MSGTYPE_DEFAULT + 1,	MI_NORMAL, "DEFAULT", 'd'},
	{MSGTYPE_MORE + 1,	MI_NORMAL, "MORE", 'm'},
	{MSGTYPE_NO_REPEAT + 1,	MI_NORMAL, "NO REPEAT", 'r'},
	{MSGTYPE_HIDE + 1,	MI_NORMAL, "HIDE", 'h'},
    };
    int n, selected[1];
    char query[QBUFSZ];
    sprintf(query, "Message type action: (currently %s)",
	    msgtype_action_string(current));
    n = curses_display_menu(items, listlen(items), query, PICK_ONE, selected);
    if (n <= 0)
	return current;
    return selected[0] - 1;
}

static void msgtype_edit_rule(struct nh_msgtype_rules *mt_rules, int ruleno)
{
    struct nh_msgtype_rule *rule;
    /* Menu variables. */
    struct nh_menuitem *items;
    int icount, menusize, selected[1], n;

    rule = &mt_rules->rules[ruleno];

    menusize = 5;
    items = malloc(sizeof(struct nh_menuitem) * menusize);

    do {
	char buf[BUFSZ], query[BUFSZ];
	int i;
	icount = 0;

	sprintf(buf, "Position:\t[%d]", ruleno + 1);
	add_menu_item(items, menusize, icount, 1, buf, 0, 0);

	sprintf(buf, "Action:\t[%s]", msgtype_action_string(rule->action));
	add_menu_item(items, menusize, icount, 2, buf, 0, 0);

	sprintf(buf, "Pattern:\t[%s]", rule->pattern);
	add_menu_item(items, menusize, icount, 3, buf, 0, 0);

	add_menu_txt(items, menusize, icount, "", MI_TEXT);
	add_menu_item(items, menusize, icount, 4, "Delete this match", '!', 0);

	n = curses_display_menu(items, icount, "Edit match:", PICK_ONE, selected);
	if (n > 0) {
	    switch (selected[0]) {
	    case 1:	/* change position */
		{
		    int newpos;
		    struct nh_msgtype_rule tmprule;

		    sprintf(query, "New match position: (1 - %d, currently %d)",
			    mt_rules->num_rules, ruleno + 1);
		    buf[0] = '\0';
		    curses_getline(query, buf);
		    if (!*buf || *buf == '\033')
			break;

		    newpos = atoi(buf);
		    if (newpos <= 0 || newpos > mt_rules->num_rules) {
			curses_msgwin("Invalid match position.");
			break;
		    }

		    newpos--;
		    if (newpos == ruleno)
			break;

		    tmprule = mt_rules->rules[ruleno];
		    /* Shuffle rules in-between. */
		    if (newpos > ruleno) {
			for (i = ruleno; i < newpos; i++)
			    mt_rules->rules[i] = mt_rules->rules[i + 1];
		    } else {
			for (i = ruleno; i > newpos; i--)
			    mt_rules->rules[i] = mt_rules->rules[i - 1];
		    }
		    mt_rules->rules[newpos] = tmprule;

		    /* Exit menu do-while (n > 0) */
		    n = 0;
		}
		break;

	    case 2:	/* change action */
		rule->action = get_msgtype_action(rule->action);
		break;

	    case 3:	/* change pattern */
		snprintf(query, BUFSZ, "New match pattern: (currently \"%s\")",
			 rule->pattern);
		buf[0] = '\0';
		curses_getline(query, buf);
		if (*buf != '\033') {
		    /* Replace the same chars as msgtype_to_string to reduce
		     * player surprise when loading a game and viewing msgtypes. */
		    for (i = 0; i < sizeof(buf); i++) {
			if (buf[i] == '"' || buf[i] == '|' || buf[i] == ';')
			    buf[i] = '?';
		    }
		    strncpy(rule->pattern, buf, sizeof(rule->pattern));
		}
		rule->pattern[sizeof(rule->pattern) - 1] = '\0';
		break;

	    case 4:	/* delete match */
		/* Shuffle rules down. */
		for (i = ruleno; i < mt_rules->num_rules - 1; i++)
		    mt_rules->rules[i] = mt_rules->rules[i + 1];
		mt_rules->num_rules--;
		mt_rules->rules = realloc(mt_rules->rules, mt_rules->num_rules *
					  sizeof(struct nh_msgtype_rule));
		/* Exit menu do-while (n > 0) */
		n = 0;
		break;

	    default:
		curses_msgwin("无效的消息类型匹配目录选择。");
	    }
	}
    } while (n > 0);

    free(items);
}

static void show_msgtype_menu(struct nh_option_desc *opt)
{
    /* msgtype option variables. */
    union nh_optvalue value;
    unsigned int size;
    /* Menu variables. */
    struct nh_menuitem *items;
    int icount, menusize, selected[1], n;

    /* Clone msgtype rules. */
    value.mt = malloc(sizeof(struct nh_msgtype_rules));
    if (opt->value.mt) {
	value.mt->num_rules = opt->value.mt->num_rules;
	size = value.mt->num_rules * sizeof(struct nh_msgtype_rule);
	value.mt->rules = malloc(size);
	memcpy(value.mt->rules, opt->value.mt->rules, size);
    } else {
	value.mt->num_rules = 0;
	value.mt->rules = NULL;
    }

    menusize = value.mt->num_rules + 4;
    items = malloc(sizeof(struct nh_menuitem) * menusize);

    selected[0] = 0;
    do {
	int i, id;
	icount = 0;

	add_menu_txt(items, menusize, icount, "Pos\tAction\tPattern", MI_HEADING);

	for (i = 0; i < value.mt->num_rules; i++) {
	    /* position (3) + '.' (1) + '\t' (1) + pattern (119) + '\t' (1) +
	     * "NO REPEAT" (9) + null (1) */
	    char buf[134];
	    struct nh_msgtype_rule *r = &value.mt->rules[i];
	    sprintf(buf, "%2d.\t%s\t%s", i + 1, msgtype_action_string(r->action),
		    r->pattern);
	    add_menu_item(items, menusize, icount, i + 1, buf, 0, 0);
	}

	add_menu_txt(items, menusize, icount, "", MI_TEXT);
	add_menu_item(items, menusize, icount, -1, "add new match", '+', 0);
	add_menu_item(items, menusize, icount, -2, "help", '?', 0);

	/* If the previous selection was to add a rule, scroll to the bottom now
	 * so that the player can see it. */
	if (selected[0] == -1) {
	    n = curses_display_menu_bottom(items, icount, "Message types:",
					   PICK_ONE, selected);
	} else {
	    n = curses_display_menu(items, icount, "Message types:",
				    PICK_ONE, selected);
	}
	if (n > 0) {
	    id = selected[0];
	    if (id == -2) {
		msgtype_help();
	    } else if (id == -1) {
		/* add new match */
		if (value.mt->num_rules >= MSGTYPE_MAX_RULES) {
		    curses_msgwin("Maximum number of rules reached.");
		} else {
		    struct nh_msgtype_rule *r;
		    id = value.mt->num_rules;

		    value.mt->num_rules++;
		    size = value.mt->num_rules * sizeof(struct nh_msgtype_rule);
		    value.mt->rules = realloc(value.mt->rules, size);

		    r = &value.mt->rules[id];
		    r->pattern[0] = '\0';
		    r->action = MSGTYPE_DEFAULT;
		}
	    } else {
		/* edit existing match */
		msgtype_edit_rule(value.mt, id - 1);
	    }
	}
    } while (n > 0);

    nh_set_option(opt->name, value, FALSE);

    if (value.mt->rules)
	free(value.mt->rules);
    free(value.mt);
    free(items);
}

/*----------------------------------------------------------------------------*/

/* parse a single line from the config file and set the option */
static void read_config_line(char* line)
{
    char *comment, *delim, *name, *value;
    union nh_optvalue optval;

    comment = strchr(line, '#');
    if (comment)
	comment = '\0';
    delim = strchr(line, '=');
    if (!delim)
	return; /* could whine about junk chars in the config, but why bother */
    
    name = line;
    value = delim + 1;
    *delim-- = '\0';
    
    /* remove space around name */
    while (isspace((unsigned char)*name))
	name++;
    while (isspace((unsigned char)*delim))
	*delim-- = '\0';
    
    /* remove spaces around value */
    delim = value;
    while (*delim)
	delim++;
    delim--;
    
    while (isspace((unsigned char)*value))
	value++;
    while (isspace((unsigned char)*delim))
	*delim-- = '\0';
    
    /* value may be enclosed with double quotes (") */
    if (*value == '"' && *delim == '"') {
	value++;
	*delim ='\0';
    }
    
    optval.s = value;
    nh_set_option(name, optval, TRUE);
}


/* open a config file and separate it into lines for read_config_line() */
static void read_config_file(const fnchar *filename)
{
    FILE *fp;
    int fsize;
    char *buf, *line;
    
    fp = fopen(filename, "rb");
    if (!fp)
	return;

    /* obtain file size. */
    fseek(fp , 0 , SEEK_END);
    fsize = ftell(fp);
    rewind(fp);
    
    if (!fsize) {/* truncated config file */
	fclose(fp);
	return;
    }

    buf = malloc(fsize+1);
    if (!buf) {
	fclose(fp);
	return;
    }

    fread(buf, fsize, 1, fp);
    fclose(fp);
    
    buf[fsize] = '\0';
    
    /* each option is expected to have the following format:
	* name=value\n
	*/
    line = strtok(buf, "\n");
    do {
	read_config_line(line);
	
	line = strtok(NULL, "\n");
    } while (line);
    
    free(buf);
}


/* determine the correct filename for the config file */
static void get_config_name(fnchar *buf, nh_bool ui)
{
    buf[0] = '\0';

#if defined(UNIX)
    char *envval;
    if (!ui) {
	/* check for env override first */
	envval = getenv("DYNAHACKOPTIONS");
	if (envval) {
	    strncpy(buf, envval, BUFSZ);
	    return;
	}
    }
#endif
    
    /* look in regular location */
    if (!get_gamedir(CONFIG_DIR, buf))
	return;

    fnncat(buf, ui ? FN("curses.conf") : FN("DynaHack.conf"), BUFSZ);
}


void read_nh_config(void)
{
    fnchar filename[BUFSZ];
    get_config_name(filename, FALSE);
    read_config_file(filename);
}

void read_ui_config(void)
{
    fnchar uiconfname[BUFSZ];
    get_config_name(uiconfname, TRUE);
    read_config_file(uiconfname);    
}


static FILE *open_config_file(fnchar *filename)
{
    FILE *fp;
    
    fp = fopen(filename, "w");
    if (!fp && (errno == ENOTDIR || errno == ENOENT)) {
	fp = fopen(filename, "w");
    }
    
    if (!fp) {
	fprintf(stderr, "could not open " FN_FMT ": %s", filename, strerror(errno));
	return NULL;
    }
    
    fprintf(fp, "# note: this file is rewritten whenever options are changed ingame\n");
    
    return fp;
}


static void write_config_options(FILE *fp, struct nh_option_desc *options)
{
    int i;
    const char *optval;
    
    for (i = 0; options[i].name; i++) {
	optval = nh_get_option_string(&options[i]);
	if (options[i].type == OPTTYPE_STRING ||
	    options[i].type == OPTTYPE_ENUM)
	    fprintf(fp, "%s=\"%s\"\n", options[i].name, optval);
	else
	    fprintf(fp, "%s=%s\n", options[i].name, optval);
    }
}


void write_config(void)
{
    FILE *fp;
    fnchar filename[BUFSZ];
    fnchar uiconfname[BUFSZ];
    
    get_config_name(filename, FALSE);
    get_config_name(uiconfname, TRUE);
    
    fp = open_config_file(filename);
    if (fp && should_write_config()) {
	write_config_options(fp, nh_get_options(GAME_OPTIONS));
	write_config_options(fp, nh_get_options(CURRENT_BIRTH_OPTIONS));
	fclose(fp);
    }
    
    fp = open_config_file(uiconfname);
    if (fp) {
	write_config_options(fp, curses_options);
	fclose(fp);
    }
}

