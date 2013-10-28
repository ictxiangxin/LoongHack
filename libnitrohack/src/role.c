/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985-1999. */
/* DynaHack may be freely redistributed.  See license for details. */

#include "hack.h"

static boolean ok_race(int, int, int, int);
static boolean ok_gend(int, int, int, int);
static boolean ok_align(int, int, int, int);
static int randrace(int);
static int randalign(int, int);


/*** Table of all roles ***/
/* According to AD&D, HD for some classes (ex. Wizard) should be smaller
 * (4-sided for wizards).  But this is not AD&D, and using the AD&D
 * rule here produces an unplayable character.  Thus I have used a minimum
 * of an 10-sided hit die for everything.  Another AD&D change: wizards get
 * a minimum strength of 4 since without one you can't teleport or cast
 * spells. --KAA
 *
 * As the wizard has been updated (wizard patch 5 jun '96) their HD can be
 * brought closer into line with AD&D. This forces wizards to use magic more
 * and distance themselves from their attackers. --LSZ
 *
 * With the introduction of races, some hit points and energy
 * has been reallocated for each race.  The values assigned
 * to the roles has been reduced by the amount allocated to
 * humans.  --KMH
 *
 * God names use a leading underscore to flag goddesses.
 */
const struct Role roles[] = {
{	{"A-考古学家", 0}, {
	{"矿工",      0},
	{"实地工作员",0},
	{"勘探者",0},
	{"挖掘者",     0},
	{"挖掘家",   0},
	{"洞穴冒险者",   0},
	{"洞穴学者",0},
	{"收藏家",   0},
	{"博物馆长",     0} },
	"魁札尔科亚特尔", "卡马斯特利", "修提库特里", /* Central American */
	"Arc", "the College of Archeology", "the Tomb of the Toltec Kings",
	PM_ARCHEOLOGIST, NON_PM, NON_PM,
	PM_LORD_CARNARVON, PM_STUDENT, PM_MINION_OF_HUHETOTL,
	ART_ORB_OF_DETECTION,
	MH_HUMAN|MH_DWARF|MH_GNOME|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7, 10, 10,  7,  7,  7 },
	{  20, 20, 20, 10, 20, 10 },
	/* Init   Lower  Higher */
	{ 11, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	10, 5, 0, 2, 10, A_INT, SPE_MAGIC_MAPPING,   -4
},
{	{"B-野蛮人", 0}, {
	{"盗贼",   "女贼"},
	{"土匪",    0},
	{"强盗",      0},
	{"劫掠者",     0},
	{"入侵者",      0},
	{"掠夺者",      0},
	{"屠宰者",      0},
	{"酋长",   "女酋长"},
	{"征服者",   0} },
	"密特拉", "克罗姆", "赛特", /* Hyborian */
	"Bar", "the Camp of the Duali Tribe", "the Duali Oasis",
	PM_BARBARIAN, NON_PM, NON_PM,
	PM_PELIAS, PM_CHIEFTAIN, PM_THOTH_AMON,
	ART_HEART_OF_AHRIMAN,
	MH_HUMAN|MH_ORC|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  16,  7,  7, 15, 16,  6 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0,10,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	10, 14, 0, 0,  8, A_INT, SPE_HASTE_SELF,      -4
},
{	{"C-穴居人", 0}, {
	{"史前人",  0},
	{"土著人",   0},
	{"流浪人",    0},
	{"漂泊者",     0},
	{"步行者",    0},
	{"游民",      0},
	{"游牧民",       0},
	{"漫游者",       0},
	{"开拓者",     0} },
	"安努", "伊丝塔", "安莎尔", /* Babylonian */
	"Cav", "the Caves of the Ancestors", "the Dragon's Lair",
	PM_CAVEMAN, PM_CAVEWOMAN, NON_PM,
	PM_SHAMAN_KARNOV, PM_NEANDERTHAL, PM_TIAMAT,
	ART_SCEPTRE_OF_MIGHT,
	MH_HUMAN|MH_DWARF|MH_GNOME|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  7,  7,  8,  6 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 12, 0, 1,  8, A_INT, SPE_DIG,             -4
},
{	{"C-罪犯",   0}, {
	{"拘留者",  0},
	{"囚犯",    0},
	{"惯犯", 0},
	{"监狱罪犯",  0},
	{"歹徒",    0},
	{"恶霸",     0},
	{"暴徒", 0},
	{"重罪犯",     0},
	{"亡命之徒",  0} },
	"伊尔玛特", "古蓝巴", "泰摩拉", /* Faerunian */
	"Con", "Castle Waterdeep Dungeon", "the Warden's Level",
	PM_CONVICT, NON_PM, PM_SEWER_RAT,
	PM_ROBERT_THE_LIFER, PM_INMATE, PM_WARDEN_ARIANNA,
	ART_IRON_BALL_OF_LIBERATION,
	MH_HUMAN|MH_DWARF|MH_GNOME|MH_ORC|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  7,  7, 13,  6 },
	{  20, 20, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{  8, 0,  0, 8,  0, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	-10, 5, 0, 2, 10, A_INT, SPE_TELEPORT_AWAY,   -4
},
{	{"H-治疗者", 0}, {
	{"草药师",    0},
	{"江湖医者",        0},
	{"敛尸官",       0},
	{"包扎员",        0},
	{"医者", 0},
	{"药医",      0},
	{"助理医师",       0},
	{"医师",      0},
	{"神医",     0} },
	"雅典娜", "赫尔墨斯", "波塞冬", /* Greek */
	"Hea", "the Temple of Epidaurus", "the Temple of Coeus",
	PM_HEALER, NON_PM, NON_PM,
	PM_HIPPOCRATES, PM_ATTENDANT, PM_CYCLOPS,
	ART_STAFF_OF_AESCULAPIUS,
	MH_HUMAN|MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7,  7, 13,  7, 11, 16 },
	{  15, 20, 20, 15, 25, 5 },
	/* Init   Lower  Higher */
	{ 11, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },20,	/* Energy */
	10, 3,-3, 2, 10, A_WIS, SPE_CURE_SICKNESS,   -4
},
{	{"K-骑士", 0}, {
	{"豪客",     0},
	{"绅士",     0},
	{"无地骑士",    0},
	{"军士",    0},
	{"骑士",      0},
	{"方棋武士",    0},
	{"荣誉骑士",   0},
	{"诸侯",   0},
	{"圣骑士",     0} },
	"神卢", "布里吉特", "马南南麦克里尔", /* Celtic */
	"Kni", "Camelot Castle", "the Isle of Glass",
	PM_KNIGHT, NON_PM, PM_PONY,
	PM_KING_ARTHUR, PM_PAGE, PM_IXOTH,
	ART_MAGIC_MIRROR_OF_MERLIN,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  13,  7, 14,  8, 10, 17 },
	{  30, 15, 15, 10, 20, 10 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 4,  0, 1,  0, 2 },10,	/* Energy */
	10, 8,-2, 0,  9, A_WIS, SPE_TURN_UNDEAD,     -4
},
{	{"M-僧人", 0}, {
	{"小僧",         0},
	{"见习修士",            0},
	{"修士",          0},
	{"大地僧者", 0},
	{"水之僧者", 0},
	{"精神僧者", 0},
	{"风之僧者",  0},
	{"火之僧者",   0},
	{"大师",            0} },
	"善莱卿", "迟孙秋", "黄帝", /* Chinese */
	"Mon", "the Monastery of Chan-Sune",
	  "the Monastery of the Earth-Lord",
	PM_MONK, NON_PM, NON_PM,
	PM_GRAND_MASTER, PM_ABBOT, PM_MASTER_KAEN,
	ART_EYES_OF_THE_OVERWORLD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  8,  8,  7,  7 },
	{  25, 10, 20, 20, 15, 10 },
	/* Init   Lower  Higher */
	{ 12, 0,  0, 8,  1, 0 },	/* Hit points */
	{  2, 2,  0, 2,  0, 2 },10,	/* Energy */
	10, 8,-2, 2, 20, A_WIS, SPE_RESTORE_ABILITY, -4
},
{	{"P-牧师", 0}, {
	{"候补生",    0},
	{"侍僧",     0},
	{"术士",       0},
	{"修道士",      "女祭司"},
	{"副牧师",      0},
	{"牧师",       0},
	{"喇嘛",        0},
	{"教主",   0},
	{"教皇", 0} },
	0, 0, 0,	/* chosen randomly from among the other roles */
	"Pri", "the Great Temple", "the Temple of Nalzok",
	PM_PRIEST, PM_PRIESTESS, NON_PM,
	PM_ARCH_PRIEST, PM_ACOLYTE, PM_NALZOK,
	ART_MITRE_OF_HOLINESS,
	MH_HUMAN|MH_ELF | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7,  7, 10,  7,  7,  7 },
	{  15, 10, 30, 15, 20, 10 },
	/* Init   Lower  Higher */
	{ 12, 0,  0, 8,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 2 },10,	/* Energy */
	0, 3,-2, 2, 10, A_WIS, SPE_REMOVE_CURSE,    -4
},
  /* Note:  Rogue precedes Ranger so that use of `-R' on the command line
     retains its traditional meaning. */
{	{"R-流氓", 0}, {
	{"无赖",     0},
	{"小偷",    0},
	{"流氓",       0},
	{"惯偷",    0},
	{"强盗",      0},
	{"飞贼",     0},
	{"怪盗",     0},
	{"狂人",     0},
	{"神偷",       0} },
	"爱瑟科", "摩哥", "考斯", /* Nehwon */
	"Rog", "the Thieves' Guild Hall", "the Assassins' Guild Hall",
	PM_ROGUE, NON_PM, NON_PM,
	PM_MASTER_OF_THIEVES, PM_THUG, PM_MASTER_ASSASSIN,
	ART_MASTER_KEY_OF_THIEVERY,
	MH_HUMAN|MH_ORC|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7,  7,  7, 10,  7,  6 },
	{  20, 10, 10, 30, 20, 10 },
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 8, 0, 1,  9, A_INT, SPE_DETECT_TREASURE, -4
},
{	{"R-游侠", 0}, {
	{"新手",    0},
	{"瞭望者",       0},
	{"开拓者",   0},
	{"斥候", 0},
	{"侦察兵",         0},
	{"战弩手",    0},	/* One skilled at crossbows */
	{"弓箭手",        0},
	{"狙击手",  0},
	{"神射手",      0} },
	"墨丘利", "维纳斯", "玛尔斯", /* Roman/planets */
	"Ran", "Orion's camp", "the cave of the wumpus",
	PM_RANGER, NON_PM, NON_PM /* Orion & canis major */,
	PM_ORION, PM_HUNTER, PM_SCORPIUS,
	ART_LONGBOW_OF_DIANA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{  13, 13, 13,  9, 13,  7 },
	{  30, 10, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 6,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },12,	/* Energy */
	10, 9, 2, 1, 10, A_INT, SPE_INVISIBILITY,   -4
},
{	{"S-武士", 0}, {
	{"旗本",    0},  /* Banner Knight */
	{"浪人",       0},  /* no allegiance */
	{"忍者",       0},  /* secret society */
	{"禅师",       0},  /* heads a castle */
	{"武士",      0},  /* has a territory */
	{"国主",     0},  /* heads a province */
	{"大名主",      0},  /* a samurai lord */
	{"公家",        0},  /* Noble of the Court */
	{"幕府将军",      0} },/* supreme commander, warlord */
	"天照大神", "雷神", "须佐之男", /* Japanese */
	"Sam", "the Castle of the Taro Clan", "the Shogun's Castle",
	PM_SAMURAI, NON_PM, PM_LITTLE_DOG,
	PM_LORD_SATO, PM_ROSHI, PM_ASHIKAGA_TAKAUJI,
	ART_TSURUGI_OF_MURAMASA,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	/* Str Int Wis Dex Con Cha */
	{  10,  8,  7, 10, 17,  6 },
	{  30, 10,  8, 30, 14,  8 },
	/* Init   Lower  Higher */
	{ 13, 0,  0, 8,  1, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },11,	/* Energy */
	10, 10, 0, 0,  8, A_INT, SPE_CLAIRVOYANCE,    -4
},
{	{"T-旅行者", 0}, {
	{"漫步者",     0},
	{"观光者",   0},
	{"远足者",0},
	{"游客",0},
	{"旅行者",    0},
	{"远游者",   0},
	{"航海者",     0},
	{"探索者",    0},
	{"冒险家",  0} },
	"盲目卫一", "女王", "奥福尔", /* Discworld */
	"Tou", "Ankh-Morpork", "the Thieves' Guild Hall",
	PM_TOURIST, NON_PM, NON_PM,
	PM_TWOFLOWER, PM_GUIDE, PM_MASTER_OF_THIEVES,
	ART_YENDORIAN_EXPRESS_CARD,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{   7, 10,  6,  7,  7, 10 },
	{  15, 10, 10, 15, 30, 20 },
	/* Init   Lower  Higher */
	{  8, 0,  0, 8,  0, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },14,	/* Energy */
	0, 5, 1, 2, 10, A_INT, SPE_CHARM_MONSTER,   -4
},
{	{"V-女武神", 0}, {
	{"青年人",   0},
	{"散兵",  0},
	{"战士",     0},
	{"士兵", "女兵"},
	{"斗士",     0},
	{"剑客",0},
	{"英雄",        "女豪"},
	{"王者",    0},
	{"霸主",        "女王"} },
	"蒂尔", "奥丁", "洛基", /* Norse */
	"Val", "the Shrine of Destiny", "the cave of Surtur",
	PM_VALKYRIE, NON_PM, NON_PM /*PM_WINTER_WOLF_CUB*/,
	PM_NORN, PM_WARRIOR, PM_LORD_SURTUR,
	ART_ORB_OF_FATE,
	MH_HUMAN|MH_DWARF | ROLE_FEMALE | ROLE_LAWFUL|ROLE_NEUTRAL,
	/* Str Int Wis Dex Con Cha */
	{  10,  7,  7,  7, 10,  7 },
	{  30,  6,  7, 20, 30,  7 },
	/* Init   Lower  Higher */
	{ 14, 0,  0, 8,  2, 0 },	/* Hit points */
	{  1, 0,  0, 1,  0, 1 },10,	/* Energy */
	0, 10,-2, 0,  9, A_WIS, SPE_CONE_OF_COLD,    -4
},
{	{"W-巫师", 0}, {
	{"奇术师",      0},
	{"幻术师",    0},
	{"咒术师", 0},
	{"魔术师",    0},
	{"魔法师",   "女法师"},
	{"男巫士",    "女巫士"},
	{"亡灵巫士", 0},
	{"巫师",      0},
	{"大魔导师",        0} },
	"卜塔", "托特", "安赫", /* Egyptian */
	"Wiz", "the Lonely Tower", "the Tower of Darkness",
	PM_WIZARD, NON_PM, PM_KITTEN,
	PM_NEFERET_THE_GREEN, PM_APPRENTICE, PM_DARK_ONE,
	ART_EYE_OF_THE_AETHIOPICA,
	MH_HUMAN|MH_ELF|MH_GNOME|MH_ORC|MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE |
	  ROLE_NEUTRAL|ROLE_CHAOTIC,
	/* Str Int Wis Dex Con Cha */
	{   7, 10,  7,  7,  7,  7 },
	{  10, 30, 10, 20, 20, 10 },
	/* Init   Lower  Higher */
	{ 10, 0,  0, 8,  1, 0 },	/* Hit points */
	{  4, 3,  0, 2,  0, 3 },12,	/* Energy */
	0, 1, 0, 3, 10, A_INT, SPE_MAGIC_MISSILE,   -4
},
/* Array terminator */
{{0, 0}}
};


/* The player's role, created at runtime from initial
 * choices.  This will be munged in role_init().
 */
struct Role urole;



/* Table of all races */
const struct Race races[] = {
{	"h-人类", "人类", "人类", "Hum",
	{"男人", "女人"},
	PM_HUMAN, NON_PM, PM_HUMAN_MUMMY, PM_HUMAN_ZOMBIE,
	MH_HUMAN | ROLE_MALE|ROLE_FEMALE |
	  ROLE_LAWFUL|ROLE_NEUTRAL|ROLE_CHAOTIC,
	MH_HUMAN, 0, MH_GNOME|MH_ORC,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 18, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  2, 0,  0, 2,  1, 0 },	/* Hit points */
	{  1, 0,  2, 0,  2, 0 }		/* Energy */
},
{	"e-精灵", "精灵族", "精灵", "Elf",
	{0, 0},
	PM_ELF, NON_PM, PM_ELF_MUMMY, PM_ELF_ZOMBIE,
	MH_ELF | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ELF, MH_ELF, MH_ORC,
	/*  Str    Int Wis Dex Con Cha */
	{    3,     3,  3,  3,  3,  3 },
	{   18,    20, 20, 18, 16, 18 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  1, 0 },	/* Hit points */
	{  2, 0,  3, 0,  3, 0 }		/* Energy */
},
{	"d-矮人", "矮人族", "矮人", "Dwa",
	{0, 0},
	PM_DWARF, NON_PM, PM_DWARF_MUMMY, PM_DWARF_ZOMBIE,
	MH_DWARF | ROLE_MALE|ROLE_FEMALE | ROLE_LAWFUL,
	MH_DWARF, MH_DWARF|MH_GNOME, MH_ORC,
	/*    Str     Int Wis Dex Con Cha */
	{      3,      3,  3,  3,  3,  3 },
	{ STR18(100), 16, 16, 20, 20, 16 },
	/* Init   Lower  Higher */
	{  4, 0,  0, 3,  2, 0 },	/* Hit points */
	{  0, 0,  0, 0,  0, 0 }		/* Energy */
},
{	"g-侏儒", "侏儒族", "侏儒", "Gno",
	{0, 0},
	PM_GNOME, NON_PM, PM_GNOME_MUMMY, PM_GNOME_ZOMBIE,
	MH_GNOME | ROLE_MALE|ROLE_FEMALE | ROLE_NEUTRAL,
	MH_GNOME, MH_DWARF|MH_GNOME, MH_HUMAN,
	/*  Str    Int Wis Dex Con Cha */
	{    3,     3,  3,  3,  3,  3 },
	{STR18(50),19, 18, 18, 18, 18 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  2, 0,  2, 0,  2, 0 }		/* Energy */
},
{	"o-兽人", "兽族", "兽族", "Orc",
	{0, 0},
	PM_ORC, NON_PM, PM_ORC_MUMMY, PM_ORC_ZOMBIE,
	MH_ORC | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_ORC, 0, MH_HUMAN|MH_ELF|MH_DWARF,
	/*  Str    Int Wis Dex Con Cha */
	{   3,      3,  3,  3,  3,  3 },
	{STR18(50),16, 16, 18, 18, 16 },
	/* Init   Lower  Higher */
	{  1, 0,  0, 1,  0, 0 },	/* Hit points */
	{  1, 0,  1, 0,  1, 0 }		/* Energy */
},
{	"v-吸血鬼", "吸血鬼", "吸血鬼", "Vam",
	{0, 0},
	PM_VAMPIRE, NON_PM, PM_HUMAN_MUMMY, PM_HUMAN_ZOMBIE,
	MH_VAMPIRE | ROLE_MALE|ROLE_FEMALE | ROLE_CHAOTIC,
	MH_VAMPIRE, 0, MH_ELF|MH_GNOME|MH_DWARF|MH_ORC,
	/*    Str    Int Wis Dex Con Cha */
	{      3,     3,  3,  3,  3,  3 },
	{ STR19(19), 18, 18, 20, 20, 20 },
	/* Init   Lower  Higher */
	{  3, 0,  0, 3,  2, 0 },	/* Hit points */
	{  3, 0,  4, 0,  4, 0 }		/* Energy */
},
/* Array terminator */
{ 0, 0, 0, 0 }};


/* The player's race, created at runtime from initial
 * choices.  This will be munged in role_init().
 */
struct Race urace;


/* Table of all genders */
const struct Gender genders[] = {
	{"m-男性",	"他",	"他",	"他的",	"Mal",	ROLE_MALE},
	{"f-女性",	"他",	"她",	"她的",	"Fem",	ROLE_FEMALE},
	{"n-中性",	"它",	"它",	"它的",	"Ntr",	ROLE_NEUTER}
};


/* Table of all alignments */
const struct Align aligns[] = {
	{"正义",	"l-正义的",	"Law",	ROLE_LAWFUL,	A_LAWFUL},
	{"平衡",	"b-中立的",	"Neu",	ROLE_NEUTRAL,	A_NEUTRAL},
	{"混沌",	"c-混沌的",	"Cha",	ROLE_CHAOTIC,	A_CHAOTIC},
	{"邪恶",	"e-不结盟的",	"Una",	0,		A_NONE}
};


/* Table of roleplay-conducts */
const struct Conduct conducts[] = {
	{"pacifism",	"pacifist",	"peaceful",	TRUE,
	 "You ","have been ","were ","a pacifist",
	 "pretended to be a pacifist"},

	{"sadism",	"sadist",	"sadistic",	TRUE,
	 "You ","have been ","were ","a sadist",
	 "pretended to be a sadist"},

	{"atheism",	"atheist",	"atheistic",	TRUE,
	 "You ","have been ","were ","an atheist",
	 "pretended to be an atheist"},

	{"nudism",	"nudist",	"nude",		TRUE,
	 "You ","have been ","were ","a nudist",
	 "pretended to be a nudist"},

	{"zen",		"zen master",	"blindfolded",	TRUE,
	 "You ","have followed ","followed ","the true Path of Zen",
	 "left the true Path of Zen"},

	{"asceticism",	"ascetic",	"hungry",	TRUE,
	 "You ","have gone ","went ","without food",
	 "pretended to be an ascet"},

	{"vegan",	"vegan",	"vegan",	TRUE,
	 "You ","have followed ","followed ","a strict vegan diet",
	 "pretended to be a vegan"},

	{"vegetarian",	"vegetarian",	"vegetarian",	TRUE,
	 "You ","have been ","were ","vegetarian",
	 "pretended to be a vegetarian"},

	{"illiteracy",	"illiterate",	"illiterate",	TRUE,
	 "You ","have been ","were ","illiterate",
	 "became literate"},

	{"thievery",	"master thief",	"tricky",	TRUE,
	 "You ","have been ","were ","very tricky",
	 "pretended to be a master thief"}
};


static char * promptsep(char *, int);
static int role_gendercount(int);
static int race_alignmentcount(int);

/* used by nh_str2XXX() */
static const char randomstr[] = "随机";


boolean validrole(int rolenum)
{
	return rolenum >= 0 && rolenum < SIZE(roles)-1;
}


int randrole(void)
{
	return rn2(SIZE(roles)-1);
}


int str2role(char *str)
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; roles[i].name.m; i++) {
	    /* Does it match the male name? */
	    if (!strncmpi(str, roles[i].name.m, len))
		return i;
	    /* Or the female name? */
	    if (roles[i].name.f && !strncmpi(str, roles[i].name.f, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, roles[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean validrace(int rolenum, int racenum)
{
	/* Assumes nh_validrole */
	return (racenum >= 0 && racenum < SIZE(races)-1 &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_RACEMASK) &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_GENDMASK) &&
		(roles[rolenum].allow & races[racenum].allow & ROLE_ALIGNMASK));
}


int randrace(int rolenum)
{
	int i, n = 0;

	/* Count the number of valid races */
	for (i = 0; races[i].noun; i++) {
	    if (validrace(rolenum, i))
		n++;
	}

	/* Pick a random race */
	/* Use a factor of 100 in case of bad random number generators */
	if (n) n = rn2(n*100)/100;
	for (i = 0; races[i].noun; i++) {
	    if (validrace(rolenum, i)) {
		if (n) n--;
		else return i;
	    }
	}

	/* This role has no permitted races? */
	return rn2(SIZE(races)-1);
}


int str2race(char *str)
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; races[i].noun; i++) {
	    /* Does it match the noun? */
	    if (!strncmpi(str, races[i].noun, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, races[i].filecode))
		return i;
	}

	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean validgend(int rolenum, int racenum, int gendnum)
{
	/* Assumes nh_validrole and nh_validrace */
	return (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		(roles[rolenum].allow & races[racenum].allow &
		 genders[gendnum].allow & ROLE_GENDMASK));
}


int str2gend(char *str)
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_GENDERS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, genders[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, genders[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


boolean validalign(int rolenum, int racenum, int alignnum)
{
	/* Assumes nh_validrole and nh_validrace */
	return (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		(roles[rolenum].allow & races[racenum].allow &
		 aligns[alignnum].allow & ROLE_ALIGNMASK));
}


int randalign(int rolenum, int racenum)
{
	int i, n = 0;

	/* Count the number of valid alignments */
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK)
	    	n++;

	/* Pick a random alignment */
	if (n) n = rn2(n);
	for (i = 0; i < ROLE_ALIGNS; i++)
	    if (roles[rolenum].allow & races[racenum].allow &
	    		aligns[i].allow & ROLE_ALIGNMASK) {
	    	if (n) n--;
	    	else return i;
	    }

	/* This role/race has no permitted alignments? */
	return rn2(ROLE_ALIGNS);
}


int str2align(char *str)
{
	int i, len;

	/* Is str valid? */
	if (!str || !str[0])
	    return ROLE_NONE;

	/* Match as much of str as is provided */
	len = strlen(str);
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    /* Does it match the adjective? */
	    if (!strncmpi(str, aligns[i].adj, len))
		return i;
	    /* Or the filecode? */
	    if (!strcmpi(str, aligns[i].filecode))
		return i;
	}
	if ((len == 1 && (*str == '*' || *str == '@')) ||
		!strncmpi(str, randomstr, len))
	    return ROLE_RANDOM;

	/* Couldn't find anything appropriate */
	return ROLE_NONE;
}


/* is racenum compatible with any rolenum/gendnum/alignnum constraints? */
boolean ok_race(int rolenum, int racenum, int gendnum, int alignnum)
{
    int i;
    short allow;

    if (racenum >= 0 && racenum < SIZE(races)-1) {
	allow = races[racenum].allow;

	if (rolenum >= 0 && rolenum < SIZE(roles)-1)
	    allow &= roles[rolenum].allow;
	if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		!(allow & genders[gendnum].allow & ROLE_GENDMASK))
	    return FALSE;
	if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		!(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
	    return FALSE;

	if (!(allow & ROLE_RACEMASK) || !(allow & ROLE_GENDMASK) ||
		!(allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < SIZE(races)-1; i++) {
	    allow = races[i].allow;

	    if (rolenum >= 0 && rolenum < SIZE(roles)-1)
		allow &= roles[rolenum].allow;
	    if (gendnum >= 0 && gendnum < ROLE_GENDERS &&
		    !(allow & genders[gendnum].allow & ROLE_GENDMASK))
		continue;
	    if (alignnum >= 0 && alignnum < ROLE_ALIGNS &&
		    !(allow & aligns[alignnum].allow & ROLE_ALIGNMASK))
		continue;

	    if (!(allow & ROLE_RACEMASK) || !(allow & ROLE_GENDMASK) ||
		    !(allow & ROLE_ALIGNMASK))
		continue;
	    return TRUE;
	}
	return FALSE;
    }
}


/* is gendnum compatible with any rolenum/racenum/alignnum constraints? */
/* gender and alignment are not comparable (and also not constrainable) */
boolean ok_gend(int rolenum, int racenum, int gendnum, int alignnum)
{
    int i;
    short allow;

    if (gendnum >= 0 && gendnum < ROLE_GENDERS) {
	allow = genders[gendnum].allow;

	if (rolenum >= 0 && rolenum < SIZE(roles)-1)
	    allow &= roles[rolenum].allow;
	if (racenum >= 0 && racenum < SIZE(races)-1)
	    allow &= races[racenum].allow;

	if (!(allow & ROLE_GENDMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_GENDERS; i++) {
	    allow = genders[i].allow;

	    if (rolenum >= 0 && rolenum < SIZE(roles)-1)
		allow &= roles[rolenum].allow;
	    if (racenum >= 0 && racenum < SIZE(races)-1)
		allow &= races[racenum].allow;
		continue;

	    if (allow & ROLE_GENDMASK)
		return TRUE;
	}
	return FALSE;
    }
}


/* is alignnum compatible with any rolenum/racenum/gendnum constraints? */
/* alignment and gender are not comparable (and also not constrainable) */
boolean ok_align(int rolenum, int racenum, int gendnum, int alignnum)
{
    int i;
    short allow;

    if (alignnum >= 0 && alignnum < ROLE_ALIGNS) {
	allow = aligns[alignnum].allow;

	if (rolenum >= 0 && rolenum < SIZE(roles)-1)
	    allow &= roles[rolenum].allow;
	if (racenum >= 0 && racenum < SIZE(races)-1)
	    allow &= races[racenum].allow;

	if (!(allow & ROLE_ALIGNMASK))
	    return FALSE;
	return TRUE;
    } else {
	for (i = 0; i < ROLE_ALIGNS; i++) {
	    allow = races[i].allow;

	    if (rolenum >= 0 && rolenum < SIZE(roles)-1)
		allow &= roles[rolenum].allow;
	    if (racenum >= 0 && racenum < SIZE(races)-1)
		allow &= races[racenum].allow;

	    if (allow & ROLE_ALIGNMASK)
		return TRUE;
	}
	return FALSE;
    }
}


struct nh_roles_info *nh_get_roles(void)
{
	int i, rolenum, racenum, gendnum, alignnum, arrsize;
	struct nh_roles_info *info = xmalloc(sizeof(struct nh_roles_info));
	const char **names, **names2;
	nh_bool *tmpmatrix;
	
	/* number of choices */
	for (i = 0; roles[i].name.m; i++);
	info->num_roles = i;
	
	for (i = 0; races[i].noun; i++);
	info->num_races = i;
	
	info->num_genders = ROLE_GENDERS;
	info->num_aligns = ROLE_ALIGNS;
	
	/* names of choices */
	names = xmalloc(info->num_roles * sizeof(char*));
	names2 = xmalloc(info->num_roles * sizeof(char*));
	for (i = 0; i < info->num_roles; i++) {
	    names[i]  = roles[i].name.m;
	    names2[i] = roles[i].name.f;
	}
	info->rolenames_m = names;
	info->rolenames_f = names2;

	names = xmalloc(info->num_races * sizeof(char*));
	for (i = 0; i < info->num_races; i++)
	    names[i] = races[i].noun;
	info->racenames = names;
	
	names = xmalloc(info->num_genders * sizeof(char*));
	for (i = 0; i < info->num_genders; i++)
	    names[i] = genders[i].adj;
	info->gendnames = names;
	
	names = xmalloc(info->num_aligns * sizeof(char*));
	for (i = 0; i < info->num_aligns; i++)
	    names[i] = aligns[i].adj;
	info->alignnames = names;
	
	/* default choices */
	info->def_role = flags.init_role;
	info->def_race = flags.init_race;
	info->def_gend = flags.init_gend;
	info->def_align = flags.init_align;
	
	/* valid combinations of choices */
	arrsize = info->num_roles * info->num_races * info->num_genders * info->num_aligns;
	tmpmatrix = xmalloc(arrsize * sizeof(nh_bool));
	memset(tmpmatrix, FALSE, arrsize * sizeof(nh_bool));
	for (rolenum = 0; rolenum < info->num_roles; rolenum++) {
	    for (racenum = 0; racenum < info->num_races; racenum++) {
		if (!ok_race(rolenum, racenum, ROLE_NONE, ROLE_NONE))
		    continue;
		for (gendnum = 0; gendnum < info->num_genders; gendnum++) {
		    if (!ok_gend(rolenum, racenum, gendnum, ROLE_NONE))
			continue;
		    for (alignnum = 0; alignnum < info->num_aligns; alignnum++) {
			tmpmatrix[nh_cm_idx((*info), rolenum, racenum, gendnum, alignnum)] =
			    ok_align(rolenum, racenum, gendnum, alignnum);
		    }
		}
	    }
	}
	info->matrix = tmpmatrix;
	
	return info;
}


#define BP_ALIGN	0
#define BP_GEND		1
#define BP_RACE		2
#define BP_ROLE		3
#define NUM_BP		4

static char pa[NUM_BP], post_attribs;

static char *promptsep(char *buf, int num_post_attribs)
{
	const char *conj = "和";
	if (num_post_attribs > 1
	    && post_attribs < num_post_attribs && post_attribs > 1)
		strcat(buf, "、");
	--post_attribs;
	if (!post_attribs && num_post_attribs > 1) strcat(buf, conj);
	return buf;
}

static int role_gendercount(int rolenum)
{
	int gendcount = 0;
	if (validrole(rolenum)) {
		if (roles[rolenum].allow & ROLE_MALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_FEMALE) ++gendcount;
		if (roles[rolenum].allow & ROLE_NEUTER) ++gendcount;
	}
	return gendcount;
}

static int race_alignmentcount(int racenum)
{
	int aligncount = 0;
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (races[racenum].allow & ROLE_CHAOTIC) ++aligncount;
		if (races[racenum].allow & ROLE_LAWFUL) ++aligncount;
		if (races[racenum].allow & ROLE_NEUTRAL) ++aligncount;
	}
	return aligncount;
}


const char *nh_root_plselection_prompt(char *suppliedbuf, int buflen, int rolenum,
			      int racenum, int gendnum, int alignnum)
{
	int k, gendercount = 0, aligncount = 0;
	char buf[BUFSZ];
	static const char err_ret[] = "人物的";
	boolean donefirst = FALSE;

	if (!suppliedbuf || buflen < 1) return err_ret;

	/* initialize these static variables each time this is called */
	post_attribs = 0;
	for (k=0; k < NUM_BP; ++k)
		pa[k] = 0;
	buf[0] = '\0';
	*suppliedbuf = '\0';
	
	/* How many alignments are allowed for the desired race? */
	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM)
		aligncount = race_alignmentcount(racenum);

	if (alignnum != ROLE_NONE && alignnum != ROLE_RANDOM) {
		/* if race specified, and multiple choice of alignments for it */
		strcat(buf, aligns[alignnum].adj);
		donefirst = TRUE;
	} else {
		/* if alignment not specified, but race is specified
			and only one choice of alignment for that race then
			don't include it in the later list */
		if ((((racenum != ROLE_NONE && racenum != ROLE_RANDOM) &&
			ok_race(rolenum, racenum, gendnum, alignnum))
		      && (aligncount > 1))
		     || (racenum == ROLE_NONE || racenum == ROLE_RANDOM)) {
			pa[BP_ALIGN] = 1;
			post_attribs++;
		}
	}
	/* <your lawful> */

	/* How many genders are allowed for the desired role? */
	if (validrole(rolenum))
		gendercount = role_gendercount(rolenum);

	if (gendnum != ROLE_NONE  && gendnum != ROLE_RANDOM) {
		if (validrole(rolenum)) {
		     /* if role specified, and multiple choice of genders for it,
			and name of role itself does not distinguish gender */
			if ((rolenum != ROLE_NONE) && (gendercount > 1)
						&& !roles[rolenum].name.f) {
				strcat(buf, genders[gendnum].adj + 2);
				donefirst = TRUE;
			}
	        } else {
	        	strcat(buf, genders[gendnum].adj + 2);
			donefirst = TRUE;
	        }
	} else {
		/* if gender not specified, but role is specified
			and only one choice of gender then
			don't include it in the later list */
		if ((validrole(rolenum) && (gendercount > 1)) || !validrole(rolenum)) {
			pa[BP_GEND] = 1;
			post_attribs++;
		}
	}
	/* <your lawful female> */

	if (racenum != ROLE_NONE && racenum != ROLE_RANDOM) {
		if (validrole(rolenum) && ok_race(rolenum, racenum, gendnum, alignnum)) {
			strcat(buf, (rolenum == ROLE_NONE) ?
				races[racenum].noun :
				races[racenum].adj);
			donefirst = TRUE;
		} else if (!validrole(rolenum)) {
			strcat(buf, races[racenum].noun);
			donefirst = TRUE;
		} else {
			pa[BP_RACE] = 1;
			post_attribs++;
		}
	} else {
		pa[BP_RACE] = 1;
		post_attribs++;
	}
	/* <your lawful female gnomish> || <your lawful female gnome> */

	if (validrole(rolenum)) {
		if (gendnum != ROLE_NONE) {
		    if (gendnum == 1  && roles[rolenum].name.f)
			strcat(buf, roles[rolenum].name.f);
		    else
  			strcat(buf, roles[rolenum].name.m + 2);
		} else {
			if (roles[rolenum].name.f) {
				strcat(buf, roles[rolenum].name.m + 2);
				strcat(buf, "/");
				strcat(buf, roles[rolenum].name.f);
			} else 
				strcat(buf, roles[rolenum].name.m + 2);
		}
		donefirst = TRUE;
	} else if (rolenum == ROLE_NONE) {
		pa[BP_ROLE] = 1;
		post_attribs++;
	}
	
	if ((racenum == ROLE_NONE || racenum == ROLE_RANDOM) && !validrole(rolenum)) {
		strcat(buf, "人物");
	}
	/* <your lawful female gnomish cavewoman> || <your lawful female gnome>
	 *    || <your lawful female character>
	 */
	if (buflen > (int) (strlen(buf) + 1)) {
		strcpy(suppliedbuf, buf);
		return suppliedbuf;
	} else
		return err_ret;
}

char *nh_build_plselection_prompt(char *buf, int buflen, int rolenum, int racenum,
			       int gendnum, int alignnum)
{
	const char *defprompt = "需要我帮你选一个角色吗？";
	int num_post_attribs = 0;
	char tmpbuf[BUFSZ];
	
	if (buflen < QBUFSZ)
		return (char *)defprompt;

	strcpy(tmpbuf, "需要我选择");
	if (racenum != ROLE_NONE || validrole(rolenum))
		strcat(tmpbuf, "your ");
	else {
		strcat(tmpbuf, "一个");
	}
	/* <your> */

	nh_root_plselection_prompt(eos(tmpbuf), buflen - strlen(tmpbuf),
					rolenum, racenum, gendnum, alignnum);
	sprintf(buf, "%s", s_suffix(tmpbuf));

	/* buf should now be:
	 * < your lawful female gnomish cavewoman's> || <your lawful female gnome's>
	 *    || <your lawful female character's>
	 *
         * Now append the post attributes to it
	 */

	num_post_attribs = post_attribs;
	if (post_attribs) {
		if (pa[BP_RACE]) {
			promptsep(eos(buf), num_post_attribs);
			strcat(buf, "种族");
		}
		if (pa[BP_ROLE]) {
			promptsep(eos(buf), num_post_attribs);
			strcat(buf, "角色");
		}
		if (pa[BP_GEND]) {
			promptsep(eos(buf), num_post_attribs);
			strcat(buf, "性别");
		}
		if (pa[BP_ALIGN]) {
			promptsep(eos(buf), num_post_attribs);
			strcat(buf, "阵营");
		}
	}
	strcat(buf, "吗？");
	return buf;
}

#undef BP_ALIGN
#undef BP_GEND
#undef BP_RACE
#undef BP_ROLE
#undef NUM_BP


/*
 *	Special setup modifications here:
 *
 *	Unfortunately, this is going to have to be done
 *	on each newgame or restore, because you lose the permonst mods
 *	across a save/restore.  :-)
 *
 *	1 - The Rogue Leader is the Tourist Nemesis.
 *	2 - Priests start with a random alignment - convert the leader and
 *	    guardians here.
 *	3 - Elves can have one of two different leaders, but can't work it
 *	    out here because it requires hacking the level file data (see
 *	    sp_lev.c).
 *
 * This code also replaces quest_init().
 */
void role_init(void)
{
	int alignmnt;

	/* Check for a valid role.  Try u.initrole first. */
	if (!validrole(u.initrole)) {
	    /* Try the player letter second */
	    if ((u.initrole = str2role(pl_character)) < 0)
	    	/* None specified; pick a random role */
	    	u.initrole = randrole();
	}

	/* We now have a valid role index.  Copy the role name back. */
	/* This should become OBSOLETE */
	strcpy(pl_character, roles[u.initrole].name.m);
	pl_character[PL_CSIZ-1] = '\0';

	/* Check for a valid race */
	if (!validrace(u.initrole, u.initrace))
	    u.initrace = randrace(u.initrole);

	/* Check for a valid gender.  If new game, check both initgend
	 * and female.  On restore, assume flags.female is correct. */
	if (flags.pantheon == -1) {	/* new game */
	    if (!validgend(u.initrole, u.initrace, flags.female))
		flags.female = !flags.female;
	}
	if (!validgend(u.initrole, u.initrace, u.initgend))
	    /* Note that there is no way to check for an unspecified gender. */
	    u.initgend = flags.female;

	/* Check for a valid alignment */
	if (!validalign(u.initrole, u.initrace, u.initalign))
	    /* Pick a random alignment */
	    u.initalign = randalign(u.initrole, u.initrace);
	alignmnt = aligns[u.initalign].value;

	/* Initialize urole and urace */
	urole = roles[u.initrole];
	urace = races[u.initrace];

	/* Fix up the quest leader */
	pm_leader = mons[urole.ldrnum];
	if (urole.ldrnum != NON_PM) {
	    pm_leader.msound = MS_LEADER;
	    pm_leader.mflags2 |= (M2_PEACEFUL);
	    pm_leader.mflags3 |= M3_CLOSE;
	    pm_leader.maligntyp = alignmnt * 3;
	}

	/* Fix up the quest guardians */
	pm_guardian = mons[urole.guardnum];
	if (urole.guardnum != NON_PM) {
	    pm_guardian.mflags2 |= (M2_PEACEFUL);
	    pm_guardian.maligntyp = alignmnt * 3;
	}

	/* Fix up the quest nemesis */
	pm_nemesis = mons[urole.neminum];
	if (urole.neminum != NON_PM) {
	    pm_nemesis.msound = MS_NEMESIS;
	    pm_nemesis.mflags2 &= ~(M2_PEACEFUL);
	    pm_nemesis.mflags2 |= (M2_NASTY|M2_STALK|M2_HOSTILE);
	    pm_nemesis.mflags3 |= M3_WANTSARTI | M3_WAITFORU;
	}

	/* Fix up the god names */
	if (flags.pantheon == -1) {		/* new game */
	    flags.pantheon = u.initrole;	/* use own gods */
	    while (!roles[flags.pantheon].lgod)	/* unless they're missing */
		flags.pantheon = randrole();
	}
	if (!urole.lgod) {
	    urole.lgod = roles[flags.pantheon].lgod;
	    urole.ngod = roles[flags.pantheon].ngod;
	    urole.cgod = roles[flags.pantheon].cgod;
	}

	/* Fix up initial roleplay flags */
	if (Role_if(PM_MONK))
	    flags.vegetarian = TRUE;
	flags.vegan |= flags.ascet;
	flags.vegetarian |= flags.vegan;

	/* Artifacts are fixed in hack_artifacts() */

	/* Success! */
	return;
}

const char *Hello(struct monst *mtmp)
{
	switch (Role_switch) {
	case PM_KNIGHT:
	    return "愿圣光与你同在"; /* Olde English */
	case PM_MONK:
	    return "阿弥陀佛";     /* Sanskrit */
	case PM_SAMURAI:
	    return (mtmp && mtmp->data == &mons[PM_SHOPKEEPER] ?
	    		"欢迎到来" : "效忠天皇"); /* Japanese */
	case PM_TOURIST:
	    return "你好";       /* Hawaiian */
	case PM_VALKYRIE:
	    return "祈求众神";   /* Norse */
	default:
	    return "你好";
	}
}

const char *Goodbye(void)
{
	switch (Role_switch) {
	case PM_KNIGHT:
	    return "圣光永存";  /* Olde English */
	case PM_MONK:
	    return "贫僧告辞";  /* Sanskrit */
	case PM_SAMURAI:
	    return "再见";        /* Japanese */
	case PM_TOURIST:
	    return "再见";           /* Hawaiian */
	case PM_VALKYRIE:
	    return "众神归位";          /* Norse */
	default:
	    return "再见";
	}
}

/* Record the breaking of a roleplay-conduct. */
void violated(int cdt)
{
	switch (cdt) {
	case CONDUCT_PACIFISM:
	    u.uconduct.killer++;
	    if (u.roleplay.pacifist) {
		pline("你感到狂暴！");
		if (yn("你确定要退出吗？") == 'y') {
		    killer_format = NO_KILLER_PREFIX;
		    killer = "在一阵狂暴之后退出";
		    done(QUIT);
		}
		if (u.uconduct.killer >= 10) u.roleplay.pacifist = FALSE;
	    }
	    break;

	case CONDUCT_NUDISM:
	    u.uconduct.armoruses++;
	    if (u.roleplay.nudist) {
		pline("你意识到你光着身子。");
		makemon(&mons[PM_COBRA], level, u.ux, u.uy, NO_MM_FLAGS);
		mksobj_at(APPLE, level, u.ux, u.uy, FALSE, FALSE);
		u.roleplay.nudist = FALSE;
	    }
	    break;

	case CONDUCT_BLINDFOLDED:
	    u.uconduct.unblinded++;
	    if (u.roleplay.blindfolded) {
		pline("禅之精神，离开你的身体。");
		makemon(mkclass(&level->z, S_ZOMBIE, 0),
			level, u.ux, u.uy, NO_MM_FLAGS);	/* Z */
		makemon(mkclass(&level->z, S_EYE, 0),
			level, u.ux, u.uy, NO_MM_FLAGS);	/* e */
		makemon(mkclass(&level->z, S_NYMPH, 0),
			level, u.ux, u.uy, NO_MM_FLAGS);	/* n */
		u.roleplay.blindfolded = FALSE;
	    }
	    break;

	case CONDUCT_VEGETARIAN:	/* replaces violated_vegetarian() */
	    if (u.roleplay.vegetarian)
		pline("你感到内疚。");
	    if (Role_if(PM_MONK))
		adjalign(-1);
	    u.uconduct.unvegetarian++;
	    u.uconduct.unvegan++;
	    u.uconduct.food++;
	    if (u.uconduct.unvegetarian >= 30) u.roleplay.vegetarian = FALSE;
	    if (u.uconduct.unvegan >= 20) u.roleplay.vegan = FALSE;
	    if (u.uconduct.food >= 10) u.roleplay.ascet = FALSE;
	    break;

	case CONDUCT_VEGAN:
	    if (u.roleplay.vegan)
		pline("你感到有点内疚。");
	    u.uconduct.unvegan++;
	    u.uconduct.food++;
	    if (u.uconduct.unvegan >= 20) u.roleplay.vegan = FALSE;
	    if (u.uconduct.food >= 10) u.roleplay.ascet = FALSE;
	    break;

	case CONDUCT_FOODLESS:
	    if (u.roleplay.ascet)
		pline("你略微感到内疚。");
	    u.uconduct.food++;
	    if (u.uconduct.food >= 10) u.roleplay.ascet = FALSE;
	    break;

	case CONDUCT_ILLITERACY:
	    u.uconduct.literate++;
	    if (u.roleplay.illiterate) {
		/* should be impossible */
		pline("Literatally literature for literate illiterates!");
		exercise(A_WIS, TRUE);
	    }
	    break;

	case CONDUCT_THIEVERY:
	    u.uconduct.robbed++;
	    if (Role_if(PM_ROGUE))
		pline("你感觉自己像一个强盗。");
	    break;

	default:
	    impossible("violated: unknown conduct");
	}
}

/* Check if a conduct has been adhered to, return FALSE if broken. */
boolean successful_cdt(int cdt)
{
	if (cdt == CONDUCT_PACIFISM && !u.uconduct.killer &&
		!num_genocides() && u.uconduct.weaphit <= 100)
	    return TRUE;
	if (cdt == CONDUCT_SADISM && !u.uconduct.killer &&
		(num_genocides() || u.uconduct.weaphit > 100))
	    return TRUE;
	if (cdt == CONDUCT_ATHEISM && !u.uconduct.gnostic) return TRUE;
	if (cdt == CONDUCT_NUDISM && !u.uconduct.armoruses) return TRUE;
	if (cdt == CONDUCT_BLINDFOLDED && !u.uconduct.unblinded) return TRUE;
	if (cdt == CONDUCT_VEGETARIAN && !u.uconduct.unvegetarian) return TRUE;
	if (cdt == CONDUCT_VEGAN && !u.uconduct.unvegan) return TRUE;
	if (cdt == CONDUCT_FOODLESS && !u.uconduct.food) return TRUE;
	if (cdt == CONDUCT_ILLITERACY && !u.uconduct.literate) return TRUE;
	if (cdt == CONDUCT_THIEVERY && !u.uconduct.robbed) return TRUE;

	return FALSE;
}

/* Check if a specific conduct was selected at character creation. */
boolean intended_cdt(int cdt)
{
	if (cdt == CONDUCT_PACIFISM && flags.pacifist) return TRUE;
	if (cdt == CONDUCT_ATHEISM && flags.atheist) return TRUE;
	if (cdt == CONDUCT_NUDISM && flags.nudist) return TRUE;
	if (cdt == CONDUCT_BLINDFOLDED && flags.blindfolded) return TRUE;
	if (cdt == CONDUCT_FOODLESS && flags.ascet) return TRUE;
	if (cdt == CONDUCT_VEGAN && flags.vegan) return TRUE;
	if (cdt == CONDUCT_VEGETARIAN && flags.vegetarian) return TRUE;
	if (cdt == CONDUCT_ILLITERACY && flags.illiterate) return TRUE;

	return FALSE;
}

/* Check if a conduct is superfluous to list. */
boolean superfluous_cdt(int cdt)
{
	if (cdt == CONDUCT_VEGAN && successful_cdt(CONDUCT_FOODLESS)) return TRUE;
	if (cdt == CONDUCT_VEGETARIAN && successful_cdt(CONDUCT_VEGAN)) return TRUE;
	if (cdt == CONDUCT_THIEVERY && !u.uevent.invoked) return TRUE;

	return FALSE;
}

/* Check if an conduct was intended, but broken. */
boolean failed_cdt(int cdt)
{
	return intended_cdt(cdt) && !successful_cdt(cdt);
}

/* role.c */
