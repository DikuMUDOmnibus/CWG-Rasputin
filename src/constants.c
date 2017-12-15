/* ************************************************************************
*   File: constants.c                                   Part of CircleMUD *
*  Usage: Numeric and string contants used by the MUD                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/constants.c,v 1.26 2005/02/11 02:30:05 zizazat Exp $");

#include "structs.h"
#include "interpreter.h"	/* alias_data */

cpp_extern const char *circlemud_version =
	"CircleMUD, version 3.1";

cpp_extern const char *oasisolc_version =
	"OasisOLC 2.0.6";

cpp_extern const char *ascii_pfiles_version =
        "ASCII Player Files 3.0.1";

const char *patch_list[] = 
{
  "Oasis OLC                            version 2.0.6	(2003/07/28)",
  "DG Scripts                           version 1.0.13	(2004/02/27)",
  "ASCII Player Files 			version 3.0.1   (2004/06/10)",
  "Races Support                        version 3.1",
  "Xapobjs                              version 1.2",
  "EZColor                              version 2.2",
  "Spoken Language Code                 version 2.1",
  "Copyover                             version 1.2",
  "128bit Support                       version 1.4",
  "Assembly Edit Code                   version 1.1",
  "Whois Command                        version 1.0",
  "Weapon Skill/Skill Progression       version 1.3",
  "Race/Class Restriction               version 1.0",
  "Vehicle Code                         version 2.0	(2003/10/01)",
  "Compare Object Code                  version 1.1",
  "Object Damage/Material Types         version 1.3",
  "Container Patch                      version 1.1",
  "Percentage Zone Loads                version 1.2",
  "Patch list                           version 1.3	(2003/08/27)",
  "Portal Object/Spell                  version 1.3	(2003/09/13)",
  "Mobile stacking                      version 3.1	(2003/08/30)",
  "Object stacking                      version 3.1	(2003/08/30)",
  "Stacking in cedit                    version 1.1	(2003/08/30)",
  "Fixtake in extra descs               version 1.1	(2003/06/21)",
  "Exits                                version 3.2	(2003/10/22)",
  "Seelight                             version 1.1	(2003/06/22)",
  "Manual Color 			version 3.1	(2003/04/15)",
  "Command Disable 			version 1.1 	(2003/12/09)",
  "Reroll Player Stats w/ cedit		version 1.1 	(2004/05/23)",
  "Spell Memorization Patch 		version 2.0 	(2003/07/15)",
  "Dynamic Boards 			version 2.4 	(2004/04/23)",
  "MCCP2				version cwg1.0  (2004/09/05)",
  "Dupecheck				version 1.0     (2003/12/07)",
  "Auto-Assist				version 1.0     (2004/9/06)",
  "Remove color question		version 1.0     (2004/9/27)",
  "Room currents			version 1.0     (2004/9/27)",
  "Timed Deathtraps			version 1.0     (2004/9/27)",
  "\n"
};

/* strings corresponding to ordinals/bitvectors in structs.h ***********/


/* (Note: strings for class definitions in class.c instead of here) */

/* Alignments */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
const char *alignments[] = {
  "Lawful Good",
  "Neutral Good",
  "Chaotic Good",
  "Lawful Neutral",
  "True Neutral",
  "Chaotic Neutral",
  "Lawful Evil",
  "Neutral Evil",
  "Chaotic Evil",
  "\n",
};

/* Armor Types */
const char *armor_type[] = {
  "Undefined",
  "Light",
  "Medium",
  "Heavy",
  "Shield",
  "\n"
};

/* Weapon Types */
/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *weapon_type[] = {
  "undefined",
  "unarmed",
  "dagger",
  "mace",
  "sickle",
  "spear",
  "staff",
  "crossbow",
  "longbow",
  "shortbow",
  "sling",
  "thrown",
  "hammer",
  "lance",
  "flail",
  "longsword",
  "shortsword",
  "greatsword",
  "rapier",
  "scimitar",
  "polearm",
  "club",
  "bastard sword",
  "monk weapon",
  "double weapon",
  "axe",
  "whip",
  "\n"
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *crit_type[] =
{
  "x2",
  "x3",
  "x4",
  "\n"
};

/* cardinal directions */
const char *dirs[] =
{
  "north",
  "east",
  "south",
  "west",
  "up",
  "down",
  "northwest",
  "northeast",
  "southeast",
  "southwest",
  "inside",
  "outside",
  "\n"
};
const char *abbr_dirs[] = 
{
  "n",
  "e",
  "s",
  "w",
  "u",
  "d",
  "nw",
  "ne",
  "se",
  "sw",
  "in",
  "out",
  "\n"
};

/* ROOM_x */
const char *room_bits[] = {
  "DARK",
  "DEATH",
  "NO_MOB",
  "INDOORS",
  "PEACEFUL",
  "SOUNDPROOF",
  "NO_TRACK",
  "NO_MAGIC",
  "TUNNEL",
  "PRIVATE",
  "GODROOM",
  "HOUSE",
  "HCRSH",
  "ATRIUM",
  "OLC",
  "*",				/* BFS MARK */
  "VEHICLE",
  "UNDERGROUND",
  "CURRENT",
  "TIMED_DT",
  "\n"
};


/* EX_x */
const char *exit_bits[] = {
  "DOOR",
  "CLOSED",
  "LOCKED",
  "PICKPROOF",
  "SECRET",
  "\n"
};


/* SECT_ */
const char *sector_types[] = {
  "Inside",
  "City",
  "Field",
  "Forest",
  "Hills",
  "Mountains",
  "Water (Swim)",
  "Water (No Swim)",
  "In Flight",
  "Underwater",
  "\n"
};


/*
 * SEX_x
 * Not used in sprinttype() so no \n.
 */
const char *genders[] =
{
  "neutral",
  "male",
  "female",
  "\n"
};


/* POS_x */
const char *position_types[] = {
  "Dead",
  "Mortally wounded",
  "Incapacitated",
  "Stunned",
  "Sleeping",
  "Resting",
  "Sitting",
  "Fighting",
  "Standing",
  "\n"
};


/* PLR_x */
const char *player_bits[] = {
  "KILLER",
  "ROGUE",
  "FROZEN",
  "DONTSET",
  "WRITING",
  "MAILING",
  "CSH",
  "SITEOK",
  "NOSHOUT",
  "NOTITLE",
  "DELETED",
  "LOADRM",
  "NO_WIZL",
  "NO_DEL",
  "INVST",
  "CRYO",
  "DEAD",    /* You should never see this. */
  "MID_AGE_POS",
  "MID_AGE_NEG",
  "OLD_AGE_POS",
  "OLD_AGE_NEG",
  "VEN_AGE_POS",
  "VEN_AGE_NEG",
  "\n"
};


/* MOB_x */
const char *action_bits[] = {
  "SPEC",
  "SENTINEL",
  "SCAVENGER",
  "ISNPC",
  "AWARE",
  "AGGR",
  "STAY-ZONE",
  "WIMPY",
  "AGGR_EVIL",
  "AGGR_GOOD",
  "AGGR_NEUTRAL",
  "MEMORY",
  "HELPER",
  "NO_CHARM",
  "NO_SUMMN",
  "NO_SLEEP",
  "SPARE17",
  "NO_BLIND",
  "NO_KILL",
  "DEAD",    /* You should never see this. */
  "SPARE1",
  "SPARE2",
  "SPARE3",
  "SPARE4",
  "SPARE5",
  "SPARE6",
  "SPARE7",
  "SPARE8",
  "SPARE9",
  "SPARE10",
  "SPARE11",
  "SPARE12",
  "SPARE13",
  "SPARE14",
  "SPARE15",
  "SPARE16",
  "\n"
};


/* PRF_x */
const char *preference_bits[] = {
  "BRIEF",
  "COMPACT",
  "DEAF",
  "NO_TELL",
  "D_HP",
  "D_MANA",
  "D_MOVE",
  "AUTOEX",
  "NO_HASS",
  "QUEST",
  "SUMN",
  "NO_REP",
  "LIGHT",
  "COLOR",
  "SPARE",
  "NO_WIZ",
  "L1",
  "L2",
  "NO_AUC",
  "NO_GOS",
  "NO_GTZ",
  "RMFLG",
  "D_AUTO",
  "CLS",
  "BLDWLK",
  "AFK",
  "AUTOLOOT",
  "AUTOGOLD",
  "AUTOSPLIT",
  "FULL_AUTOEX",
  "AUTOSAC",
  "AUTOMEM",
  "VIEWORDER",
  "NO_COMPRESS",
  "AUTOASSIST",
  "D_KI",
  "\n"
};


/* AFF_x */
/* Many are taken from the SRD under OGL, see ../doc/srd.txt for information */
const char *affected_bits[] =
{
  "\0", /* DO NOT REMOVE!! */
  "BLIND",
  "INVIS",
  "DET-ALIGN",
  "DET-INVIS",
  "DET-MAGIC",
  "SENSE-LIFE",
  "WATWALK",
  "SANCT",
  "GROUP",
  "CURSE",
  "INFRA",
  "POISON",
  "PROT-EVIL",
  "PROT-GOOD",
  "SLEEP",
  "NO_TRACK",
  "UNDEAD",
  "PARALYZED",
  "SNEAK",
  "HIDE",
  "UNUSED",
  "CHARM",
  "FLYING",
  "WATERB",
  "ANGELIC",
  "ETHEREAL",
  "MAGICONLY",
  "NEXTPARTIAL",
  "NEXTNOACT",
  "STUNNED",
  "UNUSED",
  "CDEATH",
  "SPIRIT",
  "STONESKIN",
  "SUMMONED",
  "CELESTIAL",
  "FIENDISH",
  "\n"
};


/* CON_x */
const char *connected_types[] = {
  "Playing",
  "Disconnecting",
  "Get name",
  "Confirm name",
  "Get password",
  "Get new PW",
  "Confirm new PW",
  "Select sex",
  "Select class",
  "Reading MOTD",
  "Main Menu",
  "Get descript.",
  "Changing PW 1",
  "Changing PW 2",
  "Changing PW 3",
  "Self-Delete 1",
  "Self-Delete 2",
  "Disconnecting",
  "Object edit",
  "Room edit",
  "Zone edit",
  "Mobile edit",
  "Shop edit",
  "Text edit",
  "Config edit",
  "Select race",
  "Assembly Edit",
  "Social edit",
  "Trigger Edit",
  "Race help",
  "Class help",
  "Query ANSI",
  "Guild edit",
  "Reroll stats",
  "iObject Edit",
  "Level Up",
  "\n"
};


/*
 * WEAR_x - for eq list
 * Not use in sprinttype() so no \n.
 */
const char *wear_where[] = {
  "<used somewhere>     ",
  "<worn on finger>     ",
  "<worn on finger>     ",
  "<worn around neck>   ",
  "<worn around neck>   ",
  "<worn on body>       ",
  "<worn on head>       ",
  "<worn on legs>       ",
  "<worn on feet>       ",
  "<worn on hands>      ",
  "<worn on arms>       ",
  "<worn somewhere>     ",
  "<worn about body>    ",
  "<worn about waist>   ",
  "<worn around wrist>  ",
  "<worn around wrist>  ",
  "<wielded>            ",
  "<offhand>            ",
  "<worn on back>       ",
  "<worn in ear>        ",
  "<worn in ear>        ",
  "<worn as wings>      ",
  "<worn as mask>       ",
  "\n"
};


/* WEAR_x - for stat */
const char *equipment_types[] = {
  "Used as light",
  "Worn on right finger",
  "Worn on left finger",
  "First worn around Neck",
  "Second worn around Neck",
  "Worn on body",
  "Worn on head",
  "Worn on legs",
  "Worn on feet",
  "Worn on hands",
  "Worn on arms",
  "Worn as shield",
  "Worn about body",
  "Worn around waist",
  "Worn around right wrist",
  "Worn around left wrist",
  "Wielded",
  "Held",
  "Worn on back",
  "Worn in ear",
  "Worn in ear",
  "Worn as wings",
  "Worn as mask",
  "\n"
};


/* ITEM_x (ordinal object types) */
const char *item_types[] = {
  "UNDEFINED",
  "LIGHT",
  "SCROLL",
  "WAND",
  "STAFF",
  "WEAPON",
  "FIRE WEAPON",
  "MISSILE",
  "TREASURE",
  "ARMOR",
  "POTION",
  "WORN",
  "OTHER",
  "TRASH",
  "TRAP",
  "CONTAINER",
  "NOTE",
  "LIQ CONTAINER",
  "KEY",
  "FOOD",
  "MONEY",
  "PEN",
  "BOAT",
  "FOUNTAIN",
  "VEHICLE",
  "HATCH",
  "WINDOW",
  "CONTROL",
  "PORTAL",
  "SPELLBOOK",
  "BOARD",
  "\n"
};


/* ITEM_WEAR_ (wear bitvector) */
const char *wear_bits[] = {
  "TAKE",
  "FINGER",
  "NECK",
  "BODY",
  "HEAD",
  "LEGS",
  "FEET",
  "HANDS",
  "ARMS",
  "SHIELD",
  "ABOUT",
  "WAIST",
  "WRIST",
  "WIELD",
  "HOLD",
  "PACK",
  "EAR",
  "WINGS",
  "MASK",
  "\n"
};


/* ITEM_x (extra bits) */
const char *extra_bits[] = {
  "GLOW",
  "HUM",
  "NO_RENT",
  "NO_DONATE",
  "NO_INVIS",
  "INVISIBLE",
  "MAGIC",
  "NO_DROP",
  "BLESS",
  "ANTI_GOOD",
  "ANTI_EVIL",
  "ANTI_NEUTRAL",
  "ANTI_MAGE",
  "ANTI_CLERIC",
  "ANTI_ROGUE",
  "ANTI_FIGHTER",
  "NO_SELL",
  "ANTI_DRUID",
  "2H",
  "ANTI_BARD",
  "ANTI_RANGER",
  "ANTI_PALADIN",
  "ANTI_HUMAN",
  "ANTI_DWARF",
  "ANTI_ELF",
  "ANTI_GNOME",
  "UNIQUE",
  "BROKEN",
  "UNBREAKABLE",
  "ANTI_MONK",
  "ANTI_BARBARIAN",
  "ANTI_SORCERER",
  "DOUBLE",
  "ONLY_MAGE",
  "ONLY_CLERIC",
  "ONLY_ROGUE",
  "ONLY_FIGHTER",
  "ONLY_DRUID",
  "ONLY_BARD",
  "ONLY_RANGER",
  "ONLY_PALADIN",
  "ONLY_HUMAN",
  "ONLY_DWARF",
  "ONLY_ELF",
  "ONLY_GNOME",
  "ONLY_MONK",
  "ONLY_BARBARIAN",
  "ONLY_SORCERER",
  "\n"
};


/* APPLY_x */
const char *apply_types[] = {
  "NONE",
  "STR",
  "DEX",
  "INT",
  "WIS",
  "CON",
  "CHA",
  "CLASS",
  "LEVEL",
  "AGE",
  "CHAR_WEIGHT",
  "CHAR_HEIGHT",
  "MAXMANA",
  "MAXHIT",
  "MAXMOVE",
  "GOLD",
  "EXP",
  "ARMOR",
  "ACCURACY",
  "DAMAGE",
  "SAVING_PARA",
  "SAVING_ROD",
  "SAVING_PETRI",
  "SAVING_BREATH",
  "SAVING_SPELL",
  "RACE",
  "TURN_LEVEL",
  "SPELL_LEVEL_0",
  "SPELL_LEVEL_1",
  "SPELL_LEVEL_2",
  "SPELL_LEVEL_3",
  "SPELL_LEVEL_4",
  "SPELL_LEVEL_5",
  "SPELL_LEVEL_6",
  "SPELL_LEVEL_7",
  "SPELL_LEVEL_8",
  "SPELL_LEVEL_9",
  "MAX_KI",
  "FORTITUDE",
  "REFLEX",
  "WILL",
  "SKILL",
  "FEAT",
  "ALL_SAVES",
  "\n"
};


/* CONT_x */
const char *container_bits[] = {
  "CLOSEABLE",
  "PICKPROOF",
  "CLOSED",
  "LOCKED",
  "\n",
};


/* LIQ_x */
const char *drinks[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "dark ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local speciality",
  "slime mold juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt water",
  "clear water",
  "\n"
};

/* MATERIAL_ */
const char *material_names[] = {
   "bone",
   "ceramic",
   "copper",
   "diamond",
   "gold",
   "iron",
   "leather",
   "mithril",
   "obsidian",
   "steel",
   "stone",
   "silver",
   "wood",
   "glass",
   "organic",
   "currency",
   "paper",
   "cotton",
   "satin",
   "silk",
   "burlap",
   "velvet",
   "platinum",
   "adamantine",
   "wool",
   "onyx",
   "ivory",
   "brass",
   "marble",
   "bronze",
   "pewter",
   "ruby",
   "sapphire",
   "emerald",
   "gemstone",
   "granite",
   "energy",
   "hemp",
   "crystal",
   "earth",
   "\n"
};


/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *domains[] = {
  "Undefined",
  "Air",
  "Animal",
  "Chaos",
  "Death",
  "Destruction",
  "Earth",
  "Evil",
  "Fire",
  "Good",
  "Healing",
  "Knowledge",
  "Law",
  "Luck",
  "Magic",
  "Plant",
  "Protection",
  "Strength",
  "Sun",
  "Travel",
  "Trickery",
  "War",
  "Water",
  "\n",
};

/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *schools[] = {
  "Undefined",
  "Abjuration",
  "Conjuration",
  "Divination",
  "Enchantment",
  "Evocation",
  "Illusion",
  "Necromancy",
  "Transmutation",
  "Universal",
  "\n",
};


/* Constants for Assemblies    *****************************************/
const char *AssemblyTypes[] = {
  "assemble",
  "bake",
  "brew",
  "craft",
  "fletch",
  "knit",
  "make",
  "mix",
  "thatch",
  "weave",
  "\n"
};

/* other constants for liquids ******************************************/


/* one-word alias for each drink */
const char *drinknames[] =
{
  "water",
  "beer",
  "wine",
  "ale",
  "ale",
  "whisky",
  "lemonade",
  "firebreather",
  "local",
  "juice",
  "milk",
  "tea",
  "coffee",
  "blood",
  "salt",
  "water",
  "\n"
};


/* effect of drinks on hunger, thirst, and drunkenness -- see values.doc */
int drink_aff[][3] = {
  {0, 1, 10},
  {3, 2, 5},
  {5, 2, 5},
  {2, 2, 5},
  {1, 2, 5},
  {6, 1, 4},
  {0, 1, 8},
  {10, 0, 0},
  {3, 3, 3},
  {0, 4, -8},
  {0, 3, 6},
  {0, 1, 6},
  {0, 1, 6},
  {0, 2, -1},
  {0, 1, -2},
  {0, 0, 13}
};


/* color of the various drinks */
const char *color_liquid[] =
{
  "clear",
  "brown",
  "clear",
  "brown",
  "dark",
  "golden",
  "red",
  "green",
  "clear",
  "light green",
  "white",
  "brown",
  "black",
  "red",
  "clear",
  "crystal clear",
  "\n"
};


/*
 * level of fullness for drink containers
 * Not used in sprinttype() so no \n.
 */
const char *fullness[] =
{
  "less than half ",
  "about half ",
  "more than half ",
  ""
};


/* mob trigger types */
const char *trig_types[] = {
  "Global", 
  "Random",
  "Command",
  "Speech",
  "Act",
  "Death",
  "Greet",
  "Greet-All",
  "Entry",
  "Receive",
  "Fight",
  "HitPrcnt",
  "Bribe",
  "Load",
  "Memory",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};


/* obj trigger types */
const char *otrig_types[] = {
  "Global",
  "Random",
  "Command",
  "UNUSED",
  "UNUSED",
  "Timer",
  "Get",
  "Drop",
  "Give",
  "Wear",
  "UNUSED",
  "Remove",
  "UNUSED",
  "Load",
  "UNUSED",
  "Cast",
  "Leave",
  "UNUSED",
  "Consume",
  "Time",
  "\n"
};


/* wld trigger types */
const char *wtrig_types[] = {
  "Global",
  "Random",
  "Command",
  "Speech",
  "UNUSED",
  "Zone Reset",
  "Enter",
  "Drop",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "UNUSED",
  "Cast",
  "Leave",
  "Door",
  "UNUSED",
  "Time",
  "\n"
};


/* Taken the SRD under OGL, see ../doc/srd.txt for information */
const char *size_names[] = {
  "fine",
  "diminutive",
  "tiny",
  "small",
  "medium",
  "large",
  "huge",
  "gargantuan",
  "colossal",
  "\n"
};


int rev_dir[] =
{
  /* North */ SOUTH,
  /* East  */ WEST,
  /* South */ NORTH,
  /* West  */ EAST,
  /* Up    */ DOWN,
  /* Down  */ UP,
  /* NW    */ SOUTHEAST,
  /* NE    */ SOUTHWEST,
  /* SE    */ NORTHWEST,
  /* SW    */ NORTHEAST,
  /* In    */ OUTDIR,
  /* Out   */ INDIR
};

int movement_loss[] =
{
  1,	/* Inside     */
  1,	/* City       */
  2,	/* Field      */
  3,	/* Forest     */
  4,	/* Hills      */
  6,	/* Mountains  */
  4,	/* Swimming   */
  1,	/* Unswimable */
  1,	/* Flying     */
  5     /* Underwater */
};

/* Not used in sprinttype(). */
const char *weekdays[] = {
  "the Day of the Moon",
  "the Day of the Deception",
  "the Day of Thunder",
  "the Day of Freedom",
  "the Day of the Great Gods",
  "the Day of the Sun"
};


/* Not used in sprinttype(). */
const char *month_name[] = {
  "Month of Winter",		/* 0 */
  "Month of the Winter Wolf",
  "Month of the Old Forces",
  "Month of the Spring",
  "Month of Nature",
  "Month of the Dragon",
  "Month of the Sun",
  "Month of the Battle",
  "Month of the Dark Shades",
  "Month of the Shadows",
  "Month of the Long Shadows",
  "Month of the Ancient Darkness",
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const char *wield_names[] = {
  "if you were bigger",
  "with ease",
  "one-handed",
  "two-handed",
  "\n"
};


const char *admin_level_names[ADMLVL_IMPL + 2] = {
  "Mortal",
  "Immortal",
  "Builder",
  "Admin",
  "Senior Admin",
  "Implementor",
  "\n",
};


/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
struct aging_data racial_aging_data[NUM_RACES] = {
/*                   adult	start1	start2	start3	middle	old	vener.	maxdice	*/
/* HUMAN        */ { 15,	{{1,4}, {1,6}, {2,6}},	35,	53,	70,	{2,20}	},
/* ELF          */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* GNOME        */ { 40,	{{4,6}, {6,6}, {9,6}},	100,	150,	200,	{3,100}	},
/* DWARF        */ { 40,	{{3,6}, {5,6}, {7,6}},	125,	188,	250,	{2,100}	},
/* HALF_ELF     */ { 20,	{{1,6}, {2,6}, {3,6}},	62,	93,	125,	{3,20}	},
/* HALFLING     */ { 20,	{{2,4}, {3,6}, {4,6}},	50,	75,	100,	{5,20}	},
/* DROW_ELF     */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* HALF_ORC     */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* ANIMAL       */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* CONSTRUCT    */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* DEMON        */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* DRAGON       */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* FISH         */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* GIANT        */ { 15,	{{1,4}, {1,6}, {2,6}},	35,	53,	70,	{2,20}	},
/* GOBLIN       */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* INSECT       */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* ORC          */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* SNAKE        */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* TROLL        */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* MINOTAUR     */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* KOBOLD       */ { 14,	{{1,4}, {1,6}, {2,6}},	30,	45,	60,	{2,10}	},
/* MINDFLAYER   */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* WARHOST      */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
/* FAERIE       */ { 110,	{{4,6}, {6,6}, {10,6}},	175,	263,	350,	{4,100}	},
};


/* Administrative flags */
const char *admin_flag_names[] = {
  "TellAll",
  "SeeInventory",
  "SeeSecret",
  "KnowWeather",
  "FullWhere",
  "Money",
  "EatAnything",
  "NoPoison",
  "WalkAnywhere",
  "NoKeys",
  "InstantKill",
  "NoSteal",
  "TransAll",
  "SwitchMortal",
  "ForceMass",
  "AllHouses",
  "NoDamage",
  "AllShops",
  "CEDIT",
  "\n"
};


const char *spell_schools[] = {
  "Abjuration",
  "Conjuration",
  "Divination",
  "Enchantment",
  "Evocation",
  "Illusion",
  "Necromancy",
  "Transmutation",
  "Universal",
  "\n"
};


const char *cchoice_names[NUM_COLOR + 1] = {
  "normal",
  "room names",
  "room objects",
  "room people",
  "someone hits you",
  "you hit someone",
  "other hit",
  "critical hit",
  "holler",
  "shout",
  "gossip channel",
  "auction channel",
  "congratulations",
  "\n"
};


const char *dr_style_names[NUM_DR_STYLES + 1] = {
  "NONE",
  "admin",
  "weapon material",
  "weapon bonus",
  "spell",
  "magic",
  "\n"
};


/* --- End of constants arrays. --- */

/*
 * Various arrays we count so we can check the world files.  These
 * must be at the bottom of the file so they're pre-declared.
 */
size_t	room_bits_count = sizeof(room_bits) / sizeof(room_bits[0]) - 1,
	action_bits_count = sizeof(action_bits) / sizeof(action_bits[0]) - 1,
	affected_bits_count = sizeof(affected_bits) / sizeof(affected_bits[0]) - 1,
	extra_bits_count = sizeof(extra_bits) / sizeof(extra_bits[0]) - 1,
	wear_bits_count = sizeof(wear_bits) / sizeof(wear_bits[0]) - 1;
