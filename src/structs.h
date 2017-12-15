/* ************************************************************************
*   File: structs.h                                     Part of CircleMUD *
*  Usage: header file for central structures and contstants               *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

/*
 * Intended use of this macro is to allow external packages to work with
 * a variety of CircleMUD versions without modifications.  For instance,
 * an IS_CORPSE() macro was introduced in pl13.  Any future code add-ons
 * could take into account the CircleMUD version and supply their own
 * definition for the macro if used on an older version of CircleMUD.
 * You are supposed to compare this with the macro CIRCLEMUD_VERSION()
 * in utils.h.  See there for usage.
 */
#define _CIRCLEMUD	0x030100 /* Major/Minor/Patchlevel - MMmmPP */

/*
 * If you want equipment to be automatically equipped to the same place
 * it was when players rented, set the define below to 1.  Please note
 * that this will require erasing or converting all of your rent files.
 * And of course, you have to recompile everything.  We need this feature
 * for CircleMUD to be complete but we refuse to break binary file
 * compatibility.
 */
#define USE_AUTOEQ	1	/* TRUE/FALSE aren't defined yet. */

/* CWG Version String */
#define CWG_VERSION "CWG Rasputin - 0.6.30b"

/* preamble *************************************************************/

/*
 * As of bpl20, it should be safe to use unsigned data types for the
 * various virtual and real number data types.  There really isn't a
 * reason to use signed anymore so use the unsigned types and get
 * 65,535 objects instead of 32,768.
 *
 * NOTE: This will likely be unconditionally unsigned later.
 */
#define CIRCLE_UNSIGNED_INDEX	1	/* 0 = signed, 1 = unsigned */

#if CIRCLE_UNSIGNED_INDEX
# define IDXTYPE	ush_int
# define NOWHERE	((IDXTYPE)~0)
# define NOTHING	((IDXTYPE)~0)
# define NOBODY		((IDXTYPE)~0)
#else
# define IDXTYPE	sh_int
# define NOWHERE	(-1)	/* nil reference for rooms	*/
# define NOTHING	(-1)	/* nil reference for objects	*/
# define NOBODY		(-1)	/* nil reference for mobiles	*/
#endif

#define SPECIAL(name) \
   int (name)(struct char_data *ch, void *me, int cmd, char *argument)


#define SG_MIN		2 /* Skill gain check must be less than this
			     number in order to be successful. 
			     IE: 1% of a skill gain */

/* room-related defines *************************************************/


/* The cardinal directions: used as index to room_data.dir_option[] */
#define NORTH          0
#define EAST           1
#define SOUTH          2
#define WEST           3
#define UP             4
#define DOWN           5
#define NORTHWEST      6
#define NORTHEAST      7
#define SOUTHEAST      8
#define SOUTHWEST      9
#define INDIR         10
#define OUTDIR        11

/* Room flags: used in room_data.room_flags */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define ROOM_DARK		0   /* Dark			*/
#define ROOM_DEATH		1   /* Death trap		*/
#define ROOM_NOMOB		2   /* MOBs not allowed		*/
#define ROOM_INDOORS	3   /* Indoors			*/
#define ROOM_PEACEFUL	4   /* Violence not allowed	*/
#define ROOM_SOUNDPROOF	5   /* Shouts, gossip blocked	*/
#define ROOM_NOTRACK	6   /* Track won't go through	*/
#define ROOM_NOMAGIC	7   /* Magic not allowed		*/
#define ROOM_TUNNEL		8   /* room for only 1 pers	*/
#define ROOM_PRIVATE	9   /* Can't teleport in		*/
#define ROOM_GODROOM	10  /* LVL_GOD+ only allowed	*/
#define ROOM_HOUSE		11  /* (R) Room is a house	*/
#define ROOM_HOUSE_CRASH 12  /* (R) House needs saving	*/
#define ROOM_ATRIUM		13  /* (R) The door to a house	*/
#define ROOM_OLC		14  /* (R) Modifyable/!compress	*/
#define ROOM_BFS_MARK	15  /* (R) breath-first srch mrk	*/
#define ROOM_VEHICLE    16  /* Requires a vehicle to pass       */
#define ROOM_UNDERGROUND        17  /* Room is below ground      */
#define ROOM_CURRENT     	18  /* Room move with random currents	*/
#define ROOM_TIMED_DT     	19  /* Room has a timed death trap  	*/


/* Exit info: used in room_data.dir_option.exit_info */
#define EX_ISDOOR		(1 << 0)   /* Exit is a door		*/
#define EX_CLOSED		(1 << 1)   /* The door is closed	*/
#define EX_LOCKED		(1 << 2)   /* The door is locked	*/
#define EX_PICKPROOF		(1 << 3)   /* Lock can't be picked	*/
#define EX_SECRET		(1 << 4)   /* The door is hidden        */

#define NUM_EXIT_FLAGS 5

/* Sector types: used in room_data.sector_type */
#define SECT_INSIDE          0		   /* Indoors			*/
#define SECT_CITY            1		   /* In a city			*/
#define SECT_FIELD           2		   /* In a field		*/
#define SECT_FOREST          3		   /* In a forest		*/
#define SECT_HILLS           4		   /* In the hills		*/
#define SECT_MOUNTAIN        5		   /* On a mountain		*/
#define SECT_WATER_SWIM      6		   /* Swimmable water		*/
#define SECT_WATER_NOSWIM    7		   /* Water - need a boat	*/
#define SECT_FLYING	     8		   /* Wheee!			*/
#define SECT_UNDERWATER	     9		   /* Underwater		*/


/* char and mob-related defines *****************************************/

/* PC classes */
/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define CLASS_UNDEFINED	        -1
#define CLASS_WIZARD            0
#define CLASS_CLERIC            1
#define CLASS_ROGUE             2
#define CLASS_FIGHTER           3
#define CLASS_MONK              4
#define CLASS_PALADIN           5
#define CLASS_NPC_EXPERT	6
#define CLASS_NPC_ADEPT		7
#define CLASS_NPC_COMMONER	8
#define CLASS_NPC_ARISTOCRAT	9
#define CLASS_NPC_WARRIOR	10

#define NUM_CLASSES	  11  /* This must be the number of classes!! */

#define RACE_UNDEFINED		-1
#define RACE_HUMAN		0
#define RACE_ELF		1
#define RACE_GNOME		2
#define RACE_DWARF		3
#define RACE_HALF_ELF		4
#define RACE_HALFLING		5
#define RACE_DROW_ELF		6
#define RACE_HALF_ORC		7
#define RACE_ANIMAL		8
#define RACE_CONSTRUCT		9
#define RACE_DEMON		10
#define RACE_DRAGON		11
#define RACE_FISH		12
#define RACE_GIANT		13
#define RACE_GOBLIN		14
#define RACE_INSECT		15
#define RACE_ORC		16
#define RACE_SNAKE		17
#define RACE_TROLL		18
#define RACE_MINOTAUR		19
#define RACE_KOBOLD		20
#define RACE_MINDFLAYER		21
#define RACE_WARHOST		22
#define RACE_FAERIE		23

#define NUM_RACES		24

/* Taken from the SRD under OGL, see ../doc/srd.txt for information */
#define SIZE_UNDEFINED	-1
#define SIZE_FINE	0
#define SIZE_DIMINUTIVE	1
#define SIZE_TINY	2
#define SIZE_SMALL	3
#define SIZE_MEDIUM	4
#define SIZE_LARGE	5
#define SIZE_HUGE	6
#define SIZE_GARGANTUAN	7
#define SIZE_COLOSSAL	8

#define NUM_SIZES         9

#define WIELD_NONE        0
#define WIELD_LIGHT       1
#define WIELD_ONEHAND     2
#define WIELD_TWOHAND     3

/* Number of weapon types */
#define MAX_WEAPON_TYPES            26

/* Critical hit types */
#define CRIT_X2		0
#define CRIT_X3		1
#define CRIT_X4		2

#define MAX_CRIT_TYPE	CRIT_X4

/* Sex */
#define SEX_NEUTRAL   0
#define SEX_MALE      1
#define SEX_FEMALE    2

#define NUM_SEX       3

/* Positions */
#define POS_DEAD       0	/* dead			*/
#define POS_MORTALLYW  1	/* mortally wounded	*/
#define POS_INCAP      2	/* incapacitated	*/
#define POS_STUNNED    3	/* stunned		*/
#define POS_SLEEPING   4	/* sleeping		*/
#define POS_RESTING    5	/* resting		*/
#define POS_SITTING    6	/* sitting		*/
#define POS_FIGHTING   7	/* fighting		*/
#define POS_STANDING   8	/* standing		*/


/* Player flags: used by char_data.act */
#define PLR_KILLER	0   /* Player is a player-killer        */
#define PLR_THIEF	1   /* Player is a player-thief         */
#define PLR_FROZEN	2   /* Player is frozen                 */
#define PLR_DONTSET	3   /* Don't EVER set (ISNPC bit) 	*/
#define PLR_WRITING	4   /* Player writing (board/mail/olc)  */
#define PLR_MAILING	5   /* Player is writing mail           */
#define PLR_CRASH	6   /* Player needs to be crash-saved   */
#define PLR_SITEOK	7   /* Player has been site-cleared     */
#define PLR_NOSHOUT	8   /* Player not allowed to shout/goss */
#define PLR_NOTITLE	9   /* Player not allowed to set title  */
#define PLR_DELETED	10  /* Player deleted - space reusable  */
#define PLR_LOADROOM	11  /* Player uses nonstandard loadroom */
#define PLR_NOWIZLIST	12  /* Player shouldn't be on wizlist  	*/
#define PLR_NODELETE	13  /* Player shouldn't be deleted     	*/
#define PLR_INVSTART	14  /* Player should enter game wizinvis*/
#define PLR_CRYO	15  /* Player is cryo-saved (purge prog)*/
#define PLR_NOTDEADYET	16  /* (R) Player being extracted.     	*/
#define PLR_AGEMID_G	17  /* Player has had pos of middle age	*/
#define PLR_AGEMID_B	18  /* Player has had neg of middle age	*/
#define PLR_AGEOLD_G	19  /* Player has had pos of old age	*/
#define PLR_AGEOLD_B	20  /* Player has had neg of old age	*/
#define PLR_AGEVEN_G	21  /* Player has had pos of venerable age	*/
#define PLR_AGEVEN_B	22  /* Player has had neg of venerable age	*/
#define PLR_OLDAGE	23  /* Player is dead of old age	*/

/* Mobile flags: used by char_data.act */
#define MOB_SPEC		0  /* Mob has a callable spec-proc   	*/
#define MOB_SENTINEL		1  /* Mob should not move            	*/
#define MOB_SCAVENGER		2  /* Mob picks up stuff on the ground  */
#define MOB_ISNPC		3  /* (R) Automatically set on all Mobs */
#define MOB_AWARE		4  /* Mob can't be backstabbed          */
#define MOB_AGGRESSIVE		5  /* Mob auto-attacks everybody nearby	*/
#define MOB_STAY_ZONE		6  /* Mob shouldn't wander out of zone  */
#define MOB_WIMPY		7  /* Mob flees if severely injured  	*/
#define MOB_AGGR_EVIL		8  /* Auto-attack any evil PC's		*/
#define MOB_AGGR_GOOD		9  /* Auto-attack any good PC's      	*/
#define MOB_AGGR_NEUTRAL	10 /* Auto-attack any neutral PC's   	*/
#define MOB_MEMORY		11 /* remember attackers if attacked    */
#define MOB_HELPER		12 /* attack PCs fighting other NPCs    */
#define MOB_NOCHARM		13 /* Mob can't be charmed         	*/
#define MOB_NOSUMMON		14 /* Mob can't be summoned             */
#define MOB_NOSLEEP		15 /* Mob can't be slept           	*/
#define MOB_NOBASH		16 /* Mob can't be bashed (e.g. trees)  */
#define MOB_NOBLIND		17 /* Mob can't be blinded         	*/
#define MOB_NOKILL		18 /* Mob can't be killed               */
#define MOB_NOTDEADYET		19 /* (R) Mob being extracted.          */

/*  flags: used by char_data.player_specials.pref */
#define PRF_BRIEF	0  /* Room descs won't normally be shown	*/
#define PRF_COMPACT	1  /* No extra CRLF pair before prompts		*/
#define PRF_DEAF	2  /* Can't hear shouts              		*/
#define PRF_NOTELL	3  /* Can't receive tells		    	*/
#define PRF_DISPHP	4  /* Display hit points in prompt  		*/
#define PRF_DISPMANA	5  /* Display mana points in prompt    		*/
#define PRF_DISPMOVE	6  /* Display move points in prompt 		*/
#define PRF_AUTOEXIT	7  /* Display exits in a room          		*/
#define PRF_NOHASSLE	8  /* Aggr mobs won't attack           		*/
#define PRF_QUEST	9  /* On quest					*/
#define PRF_SUMMONABLE	10 /* Can be summoned				*/
#define PRF_NOREPEAT	11 /* No repetition of comm commands		*/
#define PRF_HOLYLIGHT	12 /* Can see in dark				*/
#define PRF_COLOR	13 /* Color					*/
#define PRF_SPARE	14 /* Used to be second color bit		*/
#define PRF_NOWIZ	15 /* Can't hear wizline			*/
#define PRF_LOG1	16 /* On-line System Log (low bit)		*/
#define PRF_LOG2	17 /* On-line System Log (high bit)		*/
#define PRF_NOAUCT	18 /* Can't hear auction channel		*/
#define PRF_NOGOSS	19 /* Can't hear gossip channel			*/
#define PRF_NOGRATZ	20 /* Can't hear grats channel			*/
#define PRF_ROOMFLAGS	21 /* Can see room flags (ROOM_x)		*/
#define PRF_DISPAUTO	22 /* Show prompt HP, MP, MV when < 30%.	*/
#define PRF_CLS         23 /* Clear screen in OasisOLC 			*/
#define PRF_BUILDWALK   24 /* Build new rooms when walking		*/
#define PRF_AFK         25 /* Player is AFK				*/
#define PRF_AUTOLOOT    26 /* Loot everything from a corpse		*/
#define PRF_AUTOGOLD    27 /* Loot gold from a corpse			*/
#define PRF_AUTOSPLIT   28 /* Split gold with group			*/
#define PRF_FULL_EXIT   29 /* Shows full autoexit details		*/
#define PRF_AUTOSAC     30 /* Sacrifice a corpse 			*/
#define PRF_AUTOMEM     31 /* Memorize spells				*/
#define PRF_VIEWORDER   32 /* if you want to see the newest first 	*/
#define PRF_NOCOMPRESS  33 /* If you want to force MCCP2 off          	*/
#define PRF_AUTOASSIST  34 /* Auto-assist toggle                      	*/
#define PRF_DISPKI	35 /* Display ki points in prompt 		*/

/* Player autoexit levels: used as an index to exitlevels           */
#define EXIT_OFF        0       /* Autoexit off                     */
#define EXIT_NORMAL     1       /* Brief display (stock behaviour)  */
#define EXIT_NA         2       /* Not implemented - do not use     */
#define EXIT_COMPLETE   3       /* Full display                     */

#define _exitlevel(ch) (!IS_NPC(ch) ? (PRF_FLAGGED((ch),PRF_AUTOEXIT) ? 1 : 0 ) + (PRF_FLAGGED((ch),PRF_FULL_EXIT) ? 2 : 0 ) : 0 )
#define EXIT_LEV(ch) (_exitlevel(ch))


/* Affect bits: used in char_data.affected_by */
/* WARNING: In the world files, NEVER set the bits marked "R" ("Reserved") */
#define AFF_DONTUSE           0    /* DON'T USE! 		*/
#define AFF_BLIND             1    /* (R) Char is blind         */
#define AFF_INVISIBLE         2    /* Char is invisible         */
#define AFF_DETECT_ALIGN      3    /* Char is sensitive to align*/
#define AFF_DETECT_INVIS      4    /* Char can see invis chars  */
#define AFF_DETECT_MAGIC      5    /* Char is sensitive to magic*/
#define AFF_SENSE_LIFE        6    /* Char can sense hidden life*/
#define AFF_WATERWALK         7    /* Char can walk on water    */
#define AFF_SANCTUARY         8    /* Char protected by sanct.  */
#define AFF_GROUP             9    /* (R) Char is grouped       */
#define AFF_CURSE             10   /* Char is cursed            */
#define AFF_INFRAVISION       11   /* Char can see in dark      */
#define AFF_POISON            12   /* (R) Char is poisoned      */
#define AFF_PROTECT_EVIL      13   /* Char protected from evil  */
#define AFF_PROTECT_GOOD      14   /* Char protected from good  */
#define AFF_SLEEP             15   /* (R) Char magically asleep */
#define AFF_NOTRACK           16   /* Char can't be tracked     */
#define AFF_UNDEAD            17   /* Char is undead 		*/
#define AFF_PARALYZE          18   /* Char is paralized		*/
#define AFF_SNEAK             19   /* Char can move quietly     */
#define AFF_HIDE              20   /* Char is hidden            */
#define AFF_UNUSED20          21   /* Room for future expansion */
#define AFF_CHARM             22   /* Char is charmed         	*/
#define AFF_FLYING            23   /* Char is flying         	*/
#define AFF_WATERBREATH       24   /* Char can breath non O2    */
#define AFF_ANGELIC           25   /* Char is an angelic being  */
#define AFF_ETHEREAL          26   /* Char is ethereal          */
#define AFF_MAGICONLY         27   /* Char only hurt by magic   */
#define AFF_NEXTPARTIAL       28   /* Next action cannot be full*/
#define AFF_NEXTNOACTION      29   /* Next action cannot attack (took full action between rounds) */
#define AFF_STUNNED           30   /* Char is stunned		*/
#define AFF_UNUSED30          31   /* Char is stunned		*/
#define AFF_CDEATH            32   /* Char is undergoing creeping death */
#define AFF_SPIRIT            33   /* Char has no body          */
#define AFF_STONESKIN         34   /* Char has temporary DR     */
#define AFF_SUMMONED          35   /* Char is summoned (i.e. transient */
#define AFF_CELESTIAL         36   /* Char is celestial         */
#define AFF_FIENDISH          37   /* Char is fiendish          */

/* Modes of connectedness: used by descriptor_data.state */
#define CON_PLAYING	 0	/* Playing - Nominal state		*/
#define CON_CLOSE	 1	/* User disconnect, remove character.	*/
#define CON_GET_NAME	 2	/* By what name ..?			*/
#define CON_NAME_CNFRM	 3	/* Did I get that right, x?		*/
#define CON_PASSWORD	 4	/* Password:				*/
#define CON_NEWPASSWD	 5	/* Give me a password for x		*/
#define CON_CNFPASSWD	 6	/* Please retype password:		*/
#define CON_QSEX	 7	/* Sex?					*/
#define CON_QCLASS	 8	/* Class?				*/
#define CON_RMOTD	 9	/* PRESS RETURN after MOTD		*/
#define CON_MENU	 10	/* Your choice: (main menu)		*/
#define CON_EXDESC	 11	/* Enter a new description:		*/
#define CON_CHPWD_GETOLD 12	/* Changing passwd: get old		*/
#define CON_CHPWD_GETNEW 13	/* Changing passwd: get new		*/
#define CON_CHPWD_VRFY   14	/* Verify new password			*/
#define CON_DELCNF1	 15	/* Delete confirmation 1		*/
#define CON_DELCNF2	 16	/* Delete confirmation 2		*/
#define CON_DISCONNECT	 17	/* In-game link loss (leave character)	*/
#define CON_OEDIT	 18	/* OLC mode - object editor		*/
#define CON_REDIT	 19	/* OLC mode - room editor		*/
#define CON_ZEDIT	 20	/* OLC mode - zone info editor		*/
#define CON_MEDIT	 21	/* OLC mode - mobile editor		*/
#define CON_SEDIT	 22	/* OLC mode - shop editor		*/
#define CON_TEDIT	 23	/* OLC mode - text editor		*/
#define CON_CEDIT	 24	/* OLC mode - config editor		*/
#define CON_QRACE        25     /* Race? 				*/
#define CON_ASSEDIT      26     /* OLC mode - Assemblies                */
#define CON_AEDIT        27	/* OLC mode - social (action) edit      */
#define CON_TRIGEDIT     28	/* OLC mode - trigger edit              */
#define CON_RACE_HELP    29	/* Race Help 				*/
#define CON_CLASS_HELP   30	/* Class Help 				*/
#define CON_QANSI	 31	/* Ask for ANSI support     */
#define CON_GEDIT	 32	/* OLC mode - guild editor 		*/
#define CON_QROLLSTATS	 33	/* OLC mode - guild editor 		*/
#define CON_IEDIT        34	/* OLC mode - individual edit		*/
#define CON_LEVELUP	 35	/* Level up menu			*/


/* Colors that the player can define */
#define COLOR_NORMAL			0
#define COLOR_ROOMNAME			1
#define COLOR_ROOMOBJS			2
#define COLOR_ROOMPEOPLE		3
#define COLOR_HITYOU			4
#define COLOR_YOUHIT			5
#define COLOR_OTHERHIT			6
#define COLOR_CRITICAL			7
#define COLOR_HOLLER			8
#define COLOR_SHOUT			9
#define COLOR_GOSSIP			10
#define COLOR_AUCTION			11
#define COLOR_CONGRAT			12

#define NUM_COLOR			13

/* Character equipment positions: used as index for char_data.equipment[] */
/* NOTE: Don't confuse these constants with the ITEM_ bitvectors
   which control the valid places you can wear a piece of equipment */
#define WEAR_UNUSED0    0
#define WEAR_FINGER_R   1
#define WEAR_FINGER_L   2
#define WEAR_NECK_1     3
#define WEAR_NECK_2     4
#define WEAR_BODY       5
#define WEAR_HEAD       6
#define WEAR_LEGS       7
#define WEAR_FEET       8
#define WEAR_HANDS      9
#define WEAR_ARMS      10
#define WEAR_UNUSED1   11
#define WEAR_ABOUT     12
#define WEAR_WAIST     13
#define WEAR_WRIST_R   14
#define WEAR_WRIST_L   15
#define WEAR_WIELD1    16
#define WEAR_WIELD2    17
#define WEAR_BACKPACK  18
#define WEAR_EAR_R     19
#define WEAR_EAR_L     20
#define WEAR_WINGS     21
#define WEAR_MASK      22

#define NUM_WEARS      23	/* This must be the # of eq positions!! */

#define SPELL_LEVEL_0     0 
#define SPELL_LEVEL_1     1
#define SPELL_LEVEL_2     2
#define SPELL_LEVEL_3     3
#define SPELL_LEVEL_4     4
#define SPELL_LEVEL_5     5
#define SPELL_LEVEL_6     6
#define SPELL_LEVEL_7     7
#define SPELL_LEVEL_8     8
#define SPELL_LEVEL_9     9

#define MAX_SPELL_LEVEL   10                    /* how many spell levels */
#define MAX_MEM          (MAX_SPELL_LEVEL * 10) /* how many total spells */

#define DOMAIN_UNDEFINED	-1
#define DOMAIN_AIR		0
#define DOMAIN_ANIMAL		1
#define DOMAIN_CHAOS		2
#define DOMAIN_DEATH		3
#define DOMAIN_DESTRUCTION	4
#define DOMAIN_EARTH		5
#define DOMAIN_EVIL		6
#define DOMAIN_FIRE		7
#define DOMAIN_GOOD		8
#define DOMAIN_HEALING		9
#define DOMAIN_KNOWLEDGE	10
#define DOMAIN_LAW		11
#define DOMAIN_LUCK		12
#define DOMAIN_MAGIC		13
#define DOMAIN_PLANT		14
#define DOMAIN_PROTECTION	15
#define DOMAIN_STRENGTH		16
#define DOMAIN_SUN		17
#define DOMAIN_TRAVEL		18
#define DOMAIN_TRICKERY		19
#define DOMAIN_UNIVERSAL	20
#define DOMAIN_WAR		22
#define DOMAIN_WATER		23
#define DOMAIN_ARTIFACE         24
#define DOMAIN_CHARM            25
#define DOMAIN_COMMUNITY        26
#define DOMAIN_CREATION         27
#define DOMAIN_DARKNESS         28
#define DOMAIN_GLORY            29
#define DOMAIN_LIBERATION       30
#define DOMAIN_MADNESS          31
#define DOMAIN_NOBILITY         32
#define DOMAIN_REPOSE           33
#define DOMAIN_RUNE             34
#define DOMAIN_SCALYKIND        35
#define DOMAIN_WEATHER          36

#define NUM_DOMAINS		37

#define SCHOOL_UNDEFINED	-1
#define SCHOOL_ABJURATION	0
#define SCHOOL_CONJURATION	1
#define SCHOOL_DIVINATION	2
#define SCHOOL_ENCHANTMENT	3
#define SCHOOL_EVOCATION	4
#define SCHOOL_ILLUSION		5
#define SCHOOL_NECROMANCY	6
#define SCHOOL_TRANSMUTATION	7
#define SCHOOL_UNIVERSAL	8

#define NUM_SCHOOLS		9

#define DEITY_UNDEFINED			-1

#define NUM_DEITIES			0

/* Combat feats that apply to a specific weapon type */
#define CFEAT_IMPROVED_CRITICAL			0
#define CFEAT_WEAPON_FINESSE			1
#define CFEAT_WEAPON_FOCUS			2
#define CFEAT_WEAPON_SPECIALIZATION		3
#define CFEAT_GREATER_WEAPON_FOCUS		4
#define CFEAT_GREATER_WEAPON_SPECIALIZATION	5

#define CFEAT_MAX				5

/* Spell feats that apply to a specific school of spells */
#define CFEAT_SPELL_FOCUS			0
#define CFEAT_GREATER_SPELL_FOCUS		1

#define SFEAT_MAX				1

/* object-related defines ********************************************/


/* Item types: used by obj_data.type_flag */
#define ITEM_LIGHT      1		/* Item is a light source	*/
#define ITEM_SCROLL     2		/* Item is a scroll		*/
#define ITEM_WAND       3		/* Item is a wand		*/
#define ITEM_STAFF      4		/* Item is a staff		*/
#define ITEM_WEAPON     5		/* Item is a weapon		*/
#define ITEM_FIREWEAPON 6		/* Unimplemented		*/
#define ITEM_MISSILE    7		/* Unimplemented		*/
#define ITEM_TREASURE   8		/* Item is a treasure, not gold	*/
#define ITEM_ARMOR      9		/* Item is armor		*/
#define ITEM_POTION    10 		/* Item is a potion		*/
#define ITEM_WORN      11		/* Unimplemented		*/
#define ITEM_OTHER     12		/* Misc object			*/
#define ITEM_TRASH     13		/* Trash - shopkeeps won't buy	*/
#define ITEM_TRAP      14		/* Unimplemented		*/
#define ITEM_CONTAINER 15		/* Item is a container		*/
#define ITEM_NOTE      16		/* Item is note 		*/
#define ITEM_DRINKCON  17		/* Item is a drink container	*/
#define ITEM_KEY       18		/* Item is a key		*/
#define ITEM_FOOD      19		/* Item is food			*/
#define ITEM_MONEY     20		/* Item is money (gold)		*/
#define ITEM_PEN       21		/* Item is a pen		*/
#define ITEM_BOAT      22		/* Item is a boat		*/
#define ITEM_FOUNTAIN  23		/* Item is a fountain		*/
#define ITEM_VEHICLE   24               /* Item is a vehicle            */
#define ITEM_HATCH     25               /* Item is a vehicle hatch      */
#define ITEM_WINDOW    26               /* Item is a vehicle window     */
#define ITEM_CONTROL   27               /* Item is a vehicle control    */
#define ITEM_PORTAL    28               /* Item is a portal	        */
#define ITEM_SPELLBOOK 29               /* Item is a spellbook	        */
#define ITEM_BOARD     30               /* Item is a message board 	*/


/* Take/Wear flags: used by obj_data.wear_flags */
#define ITEM_WEAR_TAKE        0  /* Item can be takes         */
#define ITEM_WEAR_FINGER      1  /* Can be worn on finger     */
#define ITEM_WEAR_NECK        2  /* Can be worn around neck   */
#define ITEM_WEAR_BODY        3  /* Can be worn on body       */
#define ITEM_WEAR_HEAD        4  /* Can be worn on head       */
#define ITEM_WEAR_LEGS        5  /* Can be worn on legs       */
#define ITEM_WEAR_FEET        6  /* Can be worn on feet       */
#define ITEM_WEAR_HANDS       7  /* Can be worn on hands      */
#define ITEM_WEAR_ARMS        8  /* Can be worn on arms       */
#define ITEM_WEAR_SHIELD      9  /* Can be used as a shield   */
#define ITEM_WEAR_ABOUT       10 /* Can be worn about body    */
#define ITEM_WEAR_WAIST       11 /* Can be worn around waist  */
#define ITEM_WEAR_WRIST       12 /* Can be worn on wrist      */
#define ITEM_WEAR_WIELD       13 /* Can be wielded            */
#define ITEM_WEAR_HOLD        14 /* Can be held               */
#define ITEM_WEAR_PACK        15 /* Can be worn as a backpack */
#define ITEM_WEAR_EAR         16 /* Can be worn as an earring */
#define ITEM_WEAR_WINGS       17 /* Can be worn as wings      */
#define ITEM_WEAR_MASK        18 /* Can be worn as a mask     */


/* Extra object flags: used by obj_data.extra_flags */
#define ITEM_GLOW            0  /* Item is glowing              */
#define ITEM_HUM             1  /* Item is humming              */
#define ITEM_NORENT          2  /* Item cannot be rented        */
#define ITEM_NODONATE        3  /* Item cannot be donated       */
#define ITEM_NOINVIS         4  /* Item cannot be made invis    */
#define ITEM_INVISIBLE       5  /* Item is invisible            */
#define ITEM_MAGIC           6  /* Item is magical              */
#define ITEM_NODROP          7  /* Item is cursed: can't drop   */
#define ITEM_BLESS           8  /* Item is blessed              */
#define ITEM_ANTI_GOOD       9  /* Not usable by good people    */
#define ITEM_ANTI_EVIL       10 /* Not usable by evil people    */
#define ITEM_ANTI_NEUTRAL    11 /* Not usable by neutral people */
#define ITEM_ANTI_WIZARD     12 /* Not usable by mages          */
#define ITEM_ANTI_CLERIC     13 /* Not usable by clerics        */
#define ITEM_ANTI_ROGUE      14 /* Not usable by thieves        */
#define ITEM_ANTI_FIGHTER    15 /* Not usable by warriors       */
#define ITEM_NOSELL          16 /* Shopkeepers won't touch it   */
#define ITEM_ANTI_DRUID      17 /* Not usable by druids         */
#define ITEM_2H              18 /* Requires two free hands      */
#define ITEM_ANTI_BARD       19 /* Not usable by bards          */
#define ITEM_ANTI_RANGER     20 /* Not usable by rangers        */
#define ITEM_ANTI_PALADIN    21 /* Not usable by paladins       */
#define ITEM_ANTI_HUMAN      22 /* Not usable by humans         */
#define ITEM_ANTI_DWARF      23 /* Not usable by dwarves        */
#define ITEM_ANTI_ELF        24 /* Not usable by elves          */
#define ITEM_ANTI_GNOME      25 /* Not usable by gnomes         */
#define ITEM_UNIQUE_SAVE     26	/* unique object save           */
#define ITEM_BROKEN          27 /* Item is broken hands         */
#define ITEM_UNBREAKABLE     28 /* Item is unbreakable          */
#define ITEM_ANTI_MONK       29 /* Not usable by monks          */
#define ITEM_ANTI_BARBARIAN  30 /* Not usable by barbarians     */
#define ITEM_ANTI_SORCERER   31 /* Not usable by sorcerers      */
#define ITEM_DOUBLE          32 /* Double weapon                */
#define ITEM_ONLY_WIZARD     33 /* Only usable by mages         */
#define ITEM_ONLY_CLERIC     34 /* Only usable by clerics       */
#define ITEM_ONLY_ROGUE      35 /* Only usable by thieves       */
#define ITEM_ONLY_FIGHTER    36 /* Only usable by warriors      */
#define ITEM_ONLY_DRUID      37 /* Only usable by druids        */
#define ITEM_ONLY_BARD       38 /* Only usable by bards         */
#define ITEM_ONLY_RANGER     39 /* Only usable by rangers       */
#define ITEM_ONLY_PALADIN    40 /* Only usable by paladins      */
#define ITEM_ONLY_HUMAN      41 /* Only usable by humans        */
#define ITEM_ONLY_DWARF      42 /* Only usable by dwarves       */
#define ITEM_ONLY_ELF        43 /* Only usable by elves         */
#define ITEM_ONLY_GNOME      44 /* Only usable by gnomes        */
#define ITEM_ONLY_MONK       45 /* Only usable by monks         */
#define ITEM_ONLY_BARBARIAN  46 /* Only usable by barbarians    */
#define ITEM_ONLY_SORCERER   47 /* Only usable by sorcerers     */


/* Modifier constants used with obj affects ('A' fields) */
#define APPLY_NONE              0	/* No effect			*/
#define APPLY_STR               1	/* Apply to strength		*/
#define APPLY_DEX               2	/* Apply to dexterity		*/
#define APPLY_INT               3	/* Apply to intelligence	*/
#define APPLY_WIS               4	/* Apply to wisdom		*/
#define APPLY_CON               5	/* Apply to constitution	*/
#define APPLY_CHA		6	/* Apply to charisma		*/
#define APPLY_CLASS             7	/* Reserved			*/
#define APPLY_LEVEL             8	/* Reserved			*/
#define APPLY_AGE               9	/* Apply to age			*/
#define APPLY_CHAR_WEIGHT      10	/* Apply to weight		*/
#define APPLY_CHAR_HEIGHT      11	/* Apply to height		*/
#define APPLY_MANA             12	/* Apply to max mana		*/
#define APPLY_HIT              13	/* Apply to max hit points	*/
#define APPLY_MOVE             14	/* Apply to max move points	*/
#define APPLY_GOLD             15	/* Reserved			*/
#define APPLY_EXP              16	/* Reserved			*/
#define APPLY_AC               17	/* Apply to Armor Class		*/
#define APPLY_ACCURACY         18	/* Apply to accuracy		*/
#define APPLY_DAMAGE           19	/* Apply to damage 		*/
#define APPLY_SAVING_PARA      20	/* Apply to save throw: paralz	*/
#define APPLY_SAVING_ROD       21	/* Apply to save throw: rods	*/
#define APPLY_SAVING_PETRI     22	/* Apply to save throw: petrif	*/
#define APPLY_SAVING_BREATH    23	/* Apply to save throw: breath	*/
#define APPLY_SAVING_SPELL     24	/* Apply to save throw: spells	*/
#define APPLY_RACE             25       /* Apply to race                */
#define APPLY_TURN_LEVEL       26       /* Apply to turn undead         */
#define APPLY_SPELL_LVL_0      27       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_1      28       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_2      29       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_3      30       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_4      31       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_5      32       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_6      33       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_7      34       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_8      35       /* Apply to spell cast per day  */
#define APPLY_SPELL_LVL_9      36       /* Apply to spell cast per day  */
#define APPLY_KI               37	/* Apply to max ki		*/
#define APPLY_FORTITUDE        38	/* Apply to fortitue save	*/
#define APPLY_REFLEX           39	/* Apply to reflex save		*/
#define APPLY_WILL             40	/* Apply to will save		*/
#define APPLY_SKILL            41       /* Apply to a specific skill    */
#define APPLY_FEAT             42       /* Apply to a specific feat     */
#define APPLY_ALLSAVES         43       /* Apply to all 3 save types 	*/
#define APPLY_RESISTANCE       44       /* Apply to all 3 save types 	*/


/* Container flags - value[1] */
#define CONT_CLOSEABLE      (1 << 0)	/* Container can be closed	*/
#define CONT_PICKPROOF      (1 << 1)	/* Container is pickproof	*/
#define CONT_CLOSED         (1 << 2)	/* Container is closed		*/
#define CONT_LOCKED         (1 << 3)	/* Container is locked		*/


/* Some different kind of liquids for use in values of drink containers */
#define LIQ_WATER      0
#define LIQ_BEER       1
#define LIQ_WINE       2
#define LIQ_ALE        3
#define LIQ_DARKALE    4
#define LIQ_WHISKY     5
#define LIQ_LEMONADE   6
#define LIQ_FIREBRT    7
#define LIQ_LOCALSPC   8
#define LIQ_SLIME      9
#define LIQ_MILK       10
#define LIQ_TEA        11
#define LIQ_COFFE      12
#define LIQ_BLOOD      13
#define LIQ_SALTWATER  14
#define LIQ_CLEARWATER 15

#define MATERIAL_BONE           0
#define MATERIAL_CERAMIC        1
#define MATERIAL_COPPER         2
#define MATERIAL_DIAMOND        3
#define MATERIAL_GOLD           4
#define MATERIAL_IRON           5
#define MATERIAL_LEATHER        6
#define MATERIAL_MITHRIL        7
#define MATERIAL_OBSIDIAN       8
#define MATERIAL_STEEL          9
#define MATERIAL_STONE          10
#define MATERIAL_SILVER         11
#define MATERIAL_WOOD           12
#define MATERIAL_GLASS          13
#define MATERIAL_ORGANIC        14
#define MATERIAL_CURRENCY       15
#define MATERIAL_PAPER          16
#define MATERIAL_COTTON         17
#define MATERIAL_SATIN          18
#define MATERIAL_SILK           19
#define MATERIAL_BURLAP         20
#define MATERIAL_VELVET         21
#define MATERIAL_PLATINUM       22
#define MATERIAL_ADAMANTINE     23
#define MATERIAL_WOOL           24
#define MATERIAL_ONYX           25
#define MATERIAL_IVORY          26
#define MATERIAL_BRASS          27
#define MATERIAL_MARBLE         28
#define MATERIAL_BRONZE         29
#define MATERIAL_PEWTER         30
#define MATERIAL_RUBY           31
#define MATERIAL_SAPPHIRE       32
#define MATERIAL_EMERALD        33
#define MATERIAL_GEMSTONE       34
#define MATERIAL_GRANITE        35
#define MATERIAL_ENERGY         36
#define MATERIAL_HEMP           37
#define MATERIAL_CRYSTAL        38
#define MATERIAL_EARTH  	39

#define NUM_MATERIALS           40

/* other miscellaneous defines *******************************************/


/* Player conditions */
#define DRUNK        0
#define FULL         1
#define THIRST       2


/* Sun state for weather_data */
#define SUN_DARK	0
#define SUN_RISE	1
#define SUN_LIGHT	2
#define SUN_SET		3


/* Sky conditions for weather_data */
#define SKY_CLOUDLESS	0
#define SKY_CLOUDY	1
#define SKY_RAINING	2
#define SKY_LIGHTNING	3


/* Rent codes */
#define RENT_UNDEF      0
#define RENT_CRASH      1
#define RENT_RENTED     2
#define RENT_CRYO       3
#define RENT_FORCED     4
#define RENT_TIMEDOUT   5


/* for the 128bits */
#define RF_ARRAY_MAX    4
#define PM_ARRAY_MAX    4
#define PR_ARRAY_MAX    4
#define AF_ARRAY_MAX    4
#define TW_ARRAY_MAX    4
#define EF_ARRAY_MAX    4
#define AD_ARRAY_MAX	4
#define FT_ARRAY_MAX	4


/* other #defined constants **********************************************/

/*
 * ADMLVL_IMPL should always be the HIGHEST possible admin level, and
 * ADMLVL_IMMORT should always be the LOWEST immortal level.
 */
#define ADMLVL_NONE		0
#define ADMLVL_IMMORT		1
#define ADMLVL_BUILDER          2
#define ADMLVL_GOD		3
#define ADMLVL_GRGOD		4
#define ADMLVL_IMPL		5

/* First character level that forces epic levels */
#define LVL_EPICSTART		21

/*
 * ADM flags - define admin privs for chars
 */
#define ADM_TELLALL		0	/* Can use 'tell all' to broadcast GOD */
#define ADM_SEEINV		1	/* Sees other chars inventory IMM */
#define ADM_SEESECRET		2	/* Sees secret doors IMM */
#define ADM_KNOWWEATHER		3	/* Knows details of weather GOD */
#define ADM_FULLWHERE		4	/* Full output of 'where' command IMM */
#define ADM_MONEY 		5	/* Char has a bottomless wallet GOD */
#define ADM_EATANYTHING 	6	/* Char can eat anything GOD */
#define ADM_NOPOISON	 	7	/* Char can't be poisoned IMM */
#define ADM_WALKANYWHERE	8	/* Char has unrestricted walking IMM */
#define ADM_NOKEYS		9	/* Char needs no keys for locks GOD */
#define ADM_INSTANTKILL		10	/* "kill" command is instant IMPL */
#define ADM_NOSTEAL		11	/* Char cannot be stolen from IMM */
#define ADM_TRANSALL		12	/* Can use 'trans all' GRGOD */
#define ADM_SWITCHMORTAL	13	/* Can 'switch' to a mortal PC body IMPL */
#define ADM_FORCEMASS		14	/* Can force rooms or all GRGOD */
#define ADM_ALLHOUSES		15	/* Can enter any house GRGOD */
#define ADM_NODAMAGE		16	/* Cannot be damaged IMM */
#define ADM_ALLSHOPS		17	/* Can use all shops GOD */
#define ADM_CEDIT		18	/* Can use cedit IMPL */

/* Level of the 'freeze' command */
#define ADMLVL_FREEZE	ADMLVL_GRGOD

#define NUM_OF_DIRS	12	/* number of directions in a room (nsewud) */

/*
 * OPT_USEC determines how many commands will be processed by the MUD per
 * second and how frequently it does socket I/O.  A low setting will cause
 * actions to be executed more frequently but will increase overhead due to
 * more cycling to check.  A high setting (e.g. 1 Hz) may upset your players
 * as actions (such as large speedwalking chains) take longer to be executed.
 * You shouldn't need to adjust this.
 */
#define OPT_USEC	100000		/* 10 passes per second */
#define PASSES_PER_SEC	(1000000 / OPT_USEC)
#define RL_SEC		* PASSES_PER_SEC

#define PULSE_ZONE	(CONFIG_PULSE_ZONE RL_SEC)
#define PULSE_MOBILE    (CONFIG_PULSE_MOBILE RL_SEC)
#define PULSE_VIOLENCE  (CONFIG_PULSE_VIOLENCE RL_SEC)
#define PULSE_AUTOSAVE	(CONFIG_PULSE_AUTOSAVE RL_SEC)
#define PULSE_IDLEPWD	(CONFIG_PULSE_IDLEPWD RL_SEC)
#define PULSE_SANITY	(CONFIG_PULSE_SANITY RL_SEC)
#define PULSE_USAGE	(CONFIG_PULSE_SANITY * 60 RL_SEC)   /* 5 mins */
#define PULSE_TIMESAVE	(CONFIG_PULSE_TIMESAVE * 60 RL_SEC) /* should be >= SECS_PER_MUD_HOUR */
#define PULSE_CURRENT	(CONFIG_PULSE_CURRENT RL_SEC)

/* Variables for the output buffering system */
#define MAX_SOCK_BUF            (12 * 1024) /* Size of kernel's sock buf   */
#define MAX_PROMPT_LENGTH       96          /* Max length of prompt        */
#define GARBAGE_SPACE		32          /* Space for **OVERFLOW** etc  */
#define SMALL_BUFSIZE		1024        /* Static output buffer size   */
/* Max amount of output that can be buffered */
#define LARGE_BUFSIZE	   (MAX_SOCK_BUF - GARBAGE_SPACE - MAX_PROMPT_LENGTH)

#define HISTORY_SIZE		5	/* Keep last 5 commands. */
#define MAX_STRING_LENGTH	16384   
#define MAX_INPUT_LENGTH	256	/* Max length per *line* of input */
#define MAX_RAW_INPUT_LENGTH	512	/* Max size of *raw* input */
#define MAX_MESSAGES		60
#define MAX_NAME_LENGTH		20
#define MAX_PWD_LENGTH		30
#define MAX_TITLE_LENGTH	120
#define HOST_LENGTH		30
#define EXDSCR_LENGTH		240
#define MAX_TONGUE		3
#define MAX_SKILLS		200
#define MAX_AFFECT		32
#define MAX_OBJ_AFFECT		6
#define MAX_NOTE_LENGTH		1000	/* arbitrary */
#define SKILL_TABLE_SIZE	1000
#define SPELLBOOK_SIZE		50
#define MAX_FEATS	        750

/* define the largest set of commands for a trigger */
#define MAX_CMD_LENGTH          16384 /* 16k should be plenty and then some */


/*
 * A MAX_PWD_LENGTH of 10 will cause BSD-derived systems with MD5 passwords
 * and GNU libc 2 passwords to be truncated.  On BSD this will enable anyone
 * with a name longer than 5 character to log in with any password.  If you
 * have such a system, it is suggested you change the limit to 20.
 *
 * Please note that this will erase your player files.  If you are not
 * prepared to do so, simply erase these lines but heed the above warning.
 */
#if defined(HAVE_UNSAFE_CRYPT) && MAX_PWD_LENGTH == 10
#error You need to increase MAX_PWD_LENGTH to at least 20.
#error See the comment near these errors for more explanation.
#endif

/**********************************************************************
* Structures                                                          *
**********************************************************************/


typedef signed char		sbyte;
typedef unsigned char		ubyte;
typedef signed short int	sh_int;
typedef unsigned short int	ush_int;
#if !defined(__cplusplus)	/* Anyone know a portable method? */
typedef char			bool;
#endif

#if !defined(CIRCLE_WINDOWS) || defined(LCC_WIN32)	/* Hm, sysdep.h? */
typedef signed char			byte;
#endif

/* Various virtual (human-reference) number types. */
typedef IDXTYPE room_vnum;
typedef IDXTYPE obj_vnum;
typedef IDXTYPE mob_vnum;
typedef IDXTYPE zone_vnum;
typedef IDXTYPE shop_vnum;
typedef IDXTYPE trig_vnum;
typedef IDXTYPE guild_vnum;

/* Various real (array-reference) number types. */
typedef IDXTYPE room_rnum;
typedef IDXTYPE obj_rnum;
typedef IDXTYPE mob_rnum;
typedef IDXTYPE zone_rnum;
typedef IDXTYPE shop_rnum;
typedef IDXTYPE trig_rnum;
typedef IDXTYPE guild_rnum;


/*
 * Bitvector type for 32 bit unsigned long bitvectors.
 * 'unsigned long long' will give you at least 64 bits if you have GCC.
 *
 * Since we don't want to break the pfiles, you'll have to search throughout
 * the code for "bitvector_t" and change them yourself if you'd like this
 * extra flexibility.
 */
typedef unsigned long int	bitvector_t;

/* Extra description: used in objects, mobiles, and rooms */
struct extra_descr_data {
   char	*keyword;                 /* Keyword in look/examine          */
   char	*description;             /* What to see                      */
   struct extra_descr_data *next; /* Next in list                     */
};


/* object-related structures ******************************************/
#define NUM_OBJ_VAL_POSITIONS 16

#define VAL_ALL_HEALTH                4
#define VAL_ALL_MAXHEALTH             5
#define VAL_ALL_MATERIAL              7
/*
 * Uses for generic object values on specific object types
 * Please use these instead of numbers to prevent overlaps.
 */
#define VAL_LIGHT_UNUSED1             0
#define VAL_LIGHT_UNUSED2             1
#define VAL_LIGHT_HOURS               2
#define VAL_LIGHT_UNUSED4             3
#define VAL_LIGHT_HEALTH              4
#define VAL_LIGHT_MAXHEALTH           5
#define VAL_LIGHT_UNUSED7             6
#define VAL_LIGHT_MATERIAL            7
#define VAL_SCROLL_LEVEL              0
#define VAL_SCROLL_SPELL1             1
#define VAL_SCROLL_SPELL2             2
#define VAL_SCROLL_SPELL3             3
#define VAL_SCROLL_HEALTH             4
#define VAL_SCROLL_MAXHEALTH          5
#define VAL_SCROLL_UNUSED7            6
#define VAL_SCROLL_MATERIAL           7
#define VAL_WAND_LEVEL                0
#define VAL_WAND_MAXCHARGES           1
#define VAL_WAND_CHARGES              2
#define VAL_WAND_SPELL                3
#define VAL_WAND_HEALTH               4
#define VAL_WAND_MAXHEALTH            5
#define VAL_WAND_UNUSED7              6
#define VAL_WAND_MATERIAL             7
#define VAL_STAFF_LEVEL               0
#define VAL_STAFF_MAXCHARGES          1
#define VAL_STAFF_CHARGES             2
#define VAL_STAFF_SPELL               3
#define VAL_STAFF_HEALTH              4
#define VAL_STAFF_MAXHEALTH           5
#define VAL_STAFF_UNUSED7             6
#define VAL_STAFF_MATERIAL            7
#define VAL_WEAPON_SKILL              0
#define VAL_WEAPON_DAMDICE            1
#define VAL_WEAPON_DAMSIZE            2
#define VAL_WEAPON_DAMTYPE            3
#define VAL_WEAPON_HEALTH             4
#define VAL_WEAPON_MAXHEALTH          5
#define VAL_WEAPON_CRITTYPE           6
#define VAL_WEAPON_MATERIAL           7
#define VAL_WEAPON_CRITRANGE          8
#define VAL_FIREWEAPON_UNUSED1        0
#define VAL_FIREWEAPON_UNUSED2        1
#define VAL_FIREWEAPON_UNUSED3        2
#define VAL_FIREWEAPON_UNUSED4        3
#define VAL_FIREWEAPON_HEALTH         4
#define VAL_FIREWEAPON_MAXHEALTH      5
#define VAL_FIREWEAPON_UNUSED7        6
#define VAL_FIREWEAPON_MATERIAL       7
#define VAL_MISSILE_UNUSED1           0
#define VAL_MISSILE_UNUSED2           1
#define VAL_MISSILE_UNUSED3           2
#define VAL_MISSILE_UNUSED4           3
#define VAL_MISSILE_HEALTH            4
#define VAL_MISSILE_MAXHEALTH         5
#define VAL_MISSILE_UNUSED7           6
#define VAL_MISSILE_MATERIAL          7
#define VAL_TREASURE_UNUSED1          0
#define VAL_TREASURE_UNUSED2          1
#define VAL_TREASURE_UNUSED3          2
#define VAL_TREASURE_UNUSED4          3
#define VAL_TREASURE_HEALTH           4
#define VAL_TREASURE_MAXHEALTH        5
#define VAL_TREASURE_UNUSED7          6
#define VAL_TREASURE_MATERIAL         7
#define VAL_ARMOR_APPLYAC             0
#define VAL_ARMOR_SKILL               1
#define VAL_ARMOR_MAXDEXMOD           2
#define VAL_ARMOR_CHECK               3
#define VAL_ARMOR_HEALTH              4
#define VAL_ARMOR_MAXHEALTH           5
#define VAL_ARMOR_SPELLFAIL           6
#define VAL_ARMOR_MATERIAL            7
#define VAL_POTION_LEVEL              0
#define VAL_POTION_SPELL1             1
#define VAL_POTION_SPELL2             2
#define VAL_POTION_SPELL3             3
#define VAL_POTION_HEALTH             4
#define VAL_POTION_MAXHEALTH          5
#define VAL_POTION_UNUSED7            6
#define VAL_POTION_MATERIAL           7
#define VAL_WORN_UNUSED1              0
#define VAL_WORN_UNUSED2              1
#define VAL_WORN_UNUSED3              2
#define VAL_WORN_UNUSED4              3
#define VAL_WORN_HEALTH               4
#define VAL_WORN_MAXHEALTH            5
#define VAL_WORN_UNUSED7              6
#define VAL_WORN_MATERIAL             7
#define VAL_OTHER_UNUSED1             0
#define VAL_OTHER_UNUSED2             1
#define VAL_OTHER_UNUSED3             2
#define VAL_OTHER_UNUSED4             3
#define VAL_OTHER_HEALTH              4
#define VAL_OTHER_MAXHEALTH           5
#define VAL_OTHER_UNUSED7             6
#define VAL_OTHER_MATERIAL            7
#define VAL_TRASH_UNUSED1             0
#define VAL_TRASH_UNUSED2             1
#define VAL_TRASH_UNUSED3             2
#define VAL_TRASH_UNUSED4             3
#define VAL_TRASH_HEALTH              4
#define VAL_TRASH_MAXHEALTH           5
#define VAL_TRASH_UNUSED7             6
#define VAL_TRASH_MATERIAL            7
#define VAL_TRAP_SPELL                0
#define VAL_TRAP_HITPOINTS            1
#define VAL_TRAP_UNUSED3              2
#define VAL_TRAP_UNUSED4              3
#define VAL_TRAP_HEALTH               4
#define VAL_TRAP_MAXHEALTH            5
#define VAL_TRAP_UNUSED7              6
#define VAL_TRAP_MATERIAL             7
#define VAL_CONTAINER_CAPACITY        0
#define VAL_CONTAINER_FLAGS           1
#define VAL_CONTAINER_KEY             2
#define VAL_CONTAINER_CORPSE          3
#define VAL_CONTAINER_HEALTH          4
#define VAL_CONTAINER_MAXHEALTH       5
#define VAL_CONTAINER_UNUSED7         6
#define VAL_CONTAINER_MATERIAL        7
#define VAL_CONTAINER_OWNER           8
#define VAL_NOTE_LANGUAGE             0
#define VAL_NOTE_UNUSED2              1
#define VAL_NOTE_UNUSED3              2
#define VAL_NOTE_UNUSED4              3
#define VAL_NOTE_HEALTH               4
#define VAL_NOTE_MAXHEALTH            5
#define VAL_NOTE_UNUSED7              6
#define VAL_NOTE_MATERIAL             7
#define VAL_DRINKCON_CAPACITY         0
#define VAL_DRINKCON_HOWFULL          1
#define VAL_DRINKCON_LIQUID           2
#define VAL_DRINKCON_POISON           3
#define VAL_DRINKCON_HEALTH           4
#define VAL_DRINKCON_MAXHEALTH        5
#define VAL_DRINKCON_UNUSED7          6
#define VAL_DRINKCON_MATERIAL         7
#define VAL_KEY_UNUSED1               0
#define VAL_KEY_UNUSED2               1
#define VAL_KEY_KEYCODE               2
#define VAL_KEY_UNUSED4               3
#define VAL_KEY_HEALTH                4
#define VAL_KEY_MAXHEALTH             5
#define VAL_KEY_UNUSED7               6
#define VAL_KEY_MATERIAL              7
#define VAL_FOOD_FOODVAL              0
#define VAL_FOOD_UNUSED2              1
#define VAL_FOOD_UNUSED3              2
#define VAL_FOOD_POISON               3
#define VAL_FOOD_HEALTH               4
#define VAL_FOOD_MAXHEALTH            5
#define VAL_FOOD_UNUSED7              6
#define VAL_FOOD_MATERIAL             7
#define VAL_MONEY_SIZE                0
#define VAL_MONEY_UNUSED2             1
#define VAL_MONEY_UNUSED3             2
#define VAL_MONEY_UNUSED4             3
#define VAL_MONEY_HEALTH              4
#define VAL_MONEY_MAXHEALTH           5
#define VAL_MONEY_UNUSED7             6
#define VAL_MONEY_MATERIAL            7
#define VAL_PEN_UNUSED1               0
#define VAL_PEN_UNUSED2               1
#define VAL_PEN_UNUSED3               2
#define VAL_PEN_UNUSED4               3
#define VAL_PEN_HEALTH                4
#define VAL_PEN_MAXHEALTH             5
#define VAL_PEN_UNUSED7               6
#define VAL_PEN_MATERIAL              7
#define VAL_BOAT_UNUSED1              0
#define VAL_BOAT_UNUSED2              1
#define VAL_BOAT_UNUSED3              2
#define VAL_BOAT_UNUSED4              3
#define VAL_BOAT_HEALTH               4
#define VAL_BOAT_MAXHEALTH            5
#define VAL_BOAT_UNUSED7              6
#define VAL_BOAT_MATERIAL             7
#define VAL_FOUNTAIN_CAPACITY         0
#define VAL_FOUNTAIN_HOWFULL          1
#define VAL_FOUNTAIN_LIQUID           2
#define VAL_FOUNTAIN_POISON           3
#define VAL_FOUNTAIN_HEALTH           4
#define VAL_FOUNTAIN_MAXHEALTH        5
#define VAL_FOUNTAIN_UNUSED7          6
#define VAL_FOUNTAIN_MATERIAL         7
#define VAL_VEHICLE_ROOM              0
#define VAL_VEHICLE_UNUSED2           1
#define VAL_VEHICLE_UNUSED3           2
#define VAL_VEHICLE_APPEAR            3
#define VAL_VEHICLE_HEALTH            4
#define VAL_VEHICLE_MAXHEALTH         5
#define VAL_VEHICLE_UNUSED7           6
#define VAL_VEHICLE_MATERIAL          7
#define VAL_HATCH_DEST                0
#define VAL_HATCH_FLAGS               1
#define VAL_HATCH_DCSKILL             2
#define VAL_HATCH_DCMOVE              3
#define VAL_HATCH_HEALTH              4
#define VAL_HATCH_MAXHEALTH           5
#define VAL_HATCH_UNUSED7             6
#define VAL_HATCH_MATERIAL            7
#define VAL_HATCH_DCLOCK              8
#define VAL_HATCH_DCHIDE              9
#define VAL_WINDOW_UNUSED1            0
#define VAL_WINDOW_UNUSED2            1
#define VAL_WINDOW_UNUSED3            2
#define VAL_WINDOW_UNUSED4            3
#define VAL_WINDOW_HEALTH             4
#define VAL_WINDOW_MAXHEALTH          5
#define VAL_WINDOW_UNUSED7            6
#define VAL_WINDOW_MATERIAL           7
#define VAL_CONTROL_UNUSED1           0
#define VAL_CONTROL_UNUSED2           1
#define VAL_CONTROL_UNUSED3           2
#define VAL_CONTROL_UNUSED4           3
#define VAL_CONTROL_HEALTH            4
#define VAL_CONTROL_MAXHEALTH         5
#define VAL_CONTROL_UNUSED7           6
#define VAL_CONTROL_MATERIAL          7
#define VAL_PORTAL_DEST               0
#define VAL_PORTAL_DCSKILL            1
#define VAL_PORTAL_DCMOVE             2
#define VAL_PORTAL_APPEAR             3
#define VAL_PORTAL_HEALTH             4
#define VAL_PORTAL_MAXHEALTH          5
#define VAL_PORTAL_UNUSED7            6
#define VAL_PORTAL_MATERIAL           7
#define VAL_PORTAL_DCLOCK             8
#define VAL_PORTAL_DCHIDE             9
#define VAL_BOARD_READ                0
#define VAL_BOARD_WRITE               1
#define VAL_BOARD_ERASE               2
#define VAL_BOARD_UNUSED4             3
#define VAL_BOARD_HEALTH              4
#define VAL_BOARD_MAXHEALTH           5
#define VAL_BOARD_UNUSED7             6
#define VAL_BOARD_MATERIAL            7
#define VAL_DOOR_DCLOCK               8
#define VAL_DOOR_DCHIDE               9
 

struct obj_affected_type {
   int location;       /* Which ability to change (APPLY_XXX) */
   int specific;       /* Some locations have parameters      */
   int modifier;       /* How much it changes by              */
};

struct obj_spellbook_spell {
   int spellname;	/* Which spell is written */
   int pages;		/* How many pages does it take up */
};

/* ================== Memory Structure for Objects ================== */
struct obj_data {
   obj_vnum item_number;	/* Where in data-base			*/
   room_rnum in_room;		/* In what room -1 when conta/carr	*/

   int  value[NUM_OBJ_VAL_POSITIONS];   /* Values of the item (see list)    */
   byte type_flag;      /* Type of item                        */
   int  level;           /* Minimum level of object.            */
   int  wear_flags[TW_ARRAY_MAX]; /* Where you can wear it     */
   int  extra_flags[EF_ARRAY_MAX]; /* If it hums, glows, etc.  */
   int  weight;         /* Weigt what else                     */
   int  cost;           /* Value when sold (gp.)               */
   int  cost_per_day;   /* Cost to keep pr. real day           */
   int  timer;          /* Timer for object                    */
   int  bitvector[AF_ARRAY_MAX]; /* To set chars bits          */
   int  size;           /* Size class of object                */

   struct obj_affected_type affected[MAX_OBJ_AFFECT];  /* affects */

   char	*name;                    /* Title of object :get etc.        */
   char	*description;		  /* When in room                     */
   char	*short_description;       /* when worn/carry/in cont.         */
   char	*action_description;      /* What to write when used          */
   struct extra_descr_data *ex_description; /* extra descriptions     */
   struct char_data *carried_by;  /* Carried by :NULL in room/conta   */
   struct char_data *worn_by;	  /* Worn by?			      */
   sh_int worn_on;		  /* Worn where?		      */

   struct obj_data *in_obj;       /* In what object NULL when none    */
   struct obj_data *contains;     /* Contains objects                 */

   long id;                       /* used by DG triggers              */
   time_t generation;             /* creation time for dupe check     */
   unsigned long long unique_id;  /* random bits for dupe check       */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;    /* script info for the object       */

   struct obj_data *next_content; /* For 'contains' lists             */
   struct obj_data *next;         /* For the object list              */

   struct obj_spellbook_spell *sbinfo;  /* For spellbook info */
};
/* ======================================================================= */


/* room-related structures ************************************************/


struct room_direction_data {
   char	*general_description;       /* When look DIR.			*/

   char	*keyword;		/* for open/close			*/

   sh_int /*bitvector_t*/ exit_info;	/* Exit info			*/
   obj_vnum key;		/* Key's number (-1 for no key)		*/
   room_rnum to_room;		/* Where direction leads (NOWHERE)	*/
   int dclock;			/* DC to pick the lock			*/
   int dchide;			/* DC to find hidden			*/
   int dcskill;			/* Skill req. to move through exit	*/
   int dcmove;			/* DC for skill to move through exit	*/
   int failsavetype;		/* Saving Throw type on skill fail	*/
   int dcfailsave;		/* DC to save against on fail		*/
   int failroom;		/* Room # to put char in when fail > 5  */
   int totalfailroom;		/* Room # if char fails save < 5	*/
};


/* ================== Memory Structure for room ======================= */
struct room_data {
   room_vnum number;		/* Rooms number	(vnum)		      */
   zone_rnum zone;              /* Room zone (for resetting)          */
   int	sector_type;            /* sector type (move/hide)            */
   char	*name;                  /* Rooms name 'You are ...'           */
   char	*description;           /* Shown when entered                 */
   struct extra_descr_data *ex_description; /* for examine/look       */
   struct room_direction_data *dir_option[NUM_OF_DIRS]; /* Directions */
   int room_flags[RF_ARRAY_MAX];   /* DEATH,DARK ... etc */

   struct trig_proto_list *proto_script; /* list of default triggers  */
   struct script_data *script;  /* script info for the object         */

   byte light;                  /* Number of lightsources in room     */
   SPECIAL(*func);

   struct obj_data *contents;   /* List of items in room              */
   struct char_data *people;    /* List of NPC / PC in room           */

   int timed;                   /* For timed Dt's                     */
};
/* ====================================================================== */


/* char-related structures ************************************************/


/* memory structure for characters */
struct memory_rec_struct {
   long	id;
   struct memory_rec_struct *next;
};

typedef struct memory_rec_struct memory_rec;


/* This structure is purely intended to be an easy way to transfer */
/* and return information about time (real or mudwise).            */
struct time_info_data {
   int hours, day, month;
   sh_int year;
};


/* These data contain information about a players time data */
struct time_data {
   time_t birth;	/* This represents the characters current age        */
   time_t created;	/* This does not change                              */
   time_t maxage;	/* This represents death by natural causes           */
   time_t logon;	/* Time of the last logon (used to calculate played) */
   time_t played;	/* This is the total accumulated time played in secs */
};


/* The pclean_criteria_data is set up in config.c and used in db.c to
   determine the conditions which will cause a player character to be
   deleted from disk if the automagic pwipe system is enabled (see config.c).
*/
struct pclean_criteria_data {
  int level;		/* max level for this time limit	*/
  int days;		/* time limit in days			*/
}; 


/* Char's abilities. */
struct abil_data {
   sbyte str;            /* New stats can go over 18 freely, no more /xx */
   sbyte intel;
   sbyte wis;
   sbyte dex;
   sbyte con;
   sbyte cha;
};


/*
 * Specials needed only by PCs, not NPCs.  Space for this structure is
 * not allocated in memory for NPCs, but it is for PCs. This structure
 * can be changed freely.
 */
struct player_special_data {
  char *poofin;			/* Description on arrival of a god.     */
  char *poofout;		/* Description upon a god's exit.       */
  struct alias_data *aliases;	/* Character's aliases                  */
  long last_tell;		/* idnum of last tell from              */
  void *last_olc_targ;		/* olc control                          */
  int last_olc_mode;		/* olc control                          */
  char *host;			/* host of last logon                   */
  int spell_level[MAX_SPELL_LEVEL];
				/* bonus to number of spells memorized */
  int memcursor;		/* points to the next free slot in spellmem */
  int wimp_level;		/* Below this # of hit points, flee!	*/
  byte freeze_level;		/* Level of god who froze char, if any	*/
  sh_int invis_level;		/* level of invisibility		*/
  room_vnum load_room;		/* Which room to place char in		*/
  int pref[PR_ARRAY_MAX];	/* preference flags for PC's.		*/
  ubyte bad_pws;		/* number of bad password attemps	*/
  sbyte conditions[3];		/* Drunk, full, thirsty			*/
  int skill_points;		/* Skill points earned from race HD	*/
  int class_skill_points[NUM_CLASSES];
				/* Skill points earned from a class	*/
  int olc_zone;			/* Zone where OLC is permitted		*/
  int speaking;			/* Language currently speaking		*/
  int tlevel;			/* Turning level			*/
  int ability_trains;		/* How many stat points can you train?	*/
  int spellmem[MAX_MEM];	/* Spell slots				*/
  int feat_points;		/* How many general feats you can take	*/
  int epic_feat_points;		/* How many epic feats you can take	*/
  int class_feat_points[NUM_CLASSES];
				/* How many class feats you can take	*/
  int epic_class_feat_points[NUM_CLASSES];
				/* How many epic class feats 		*/
  int domain[NUM_DOMAINS];
  int school[NUM_SCHOOLS];
  int diety;
  int spell_mastery_points;
  char *color_choices[NUM_COLOR];
  ubyte page_length;
				/* Choices for custom colors		*/
};


/* this can be used for skills that can be used per-day */
struct memorize_node {
   int		timer;			/* how many ticks till memorized */
   int		spell; 			/* the spell number */
   struct 	memorize_node *next; 	/* link to the next node */
};

struct innate_node {
   int timer;
   int spellnum;
   struct innate_node *next;
};

/* Specials used by NPCs, not PCs */
struct mob_special_data {
   memory_rec *memory;	    /* List of attackers to remember	       */
   byte	attack_type;        /* The Attack Type Bitvector for NPC's     */
   byte default_pos;        /* Default position for NPC                */
   byte damnodice;          /* The number of damage dice's	       */
   byte damsizedice;        /* The size of the damage dice's           */
   int newitem;             /* Check if mob has new inv item       */
};


/* An affect structure. */
struct affected_type {
   sh_int type;          /* The type of spell that caused this      */
   sh_int duration;      /* For how long its effects will last      */
   int modifier;         /* This is added to apropriate ability     */
   int location;         /* Tells which ability to change(APPLY_XXX)*/
   int specific;         /* Some locations have parameters          */
   long /*bitvector_t*/	bitvector; /* Tells which bits to set (AFF_XXX) */

   struct affected_type *next;
};


#define MAX_DAMREDUCT_MULTI	5

#define DR_NONE			0
#define DR_ADMIN		1
#define DR_MATERIAL		2
#define DR_BONUS		3
#define DR_SPELL		4
#define DR_MAGICAL		5

#define NUM_DR_STYLES		6

struct damreduct_type {
  sh_int spell;
  sh_int feat;
  struct obj_data *eq;
  int mod;
  int damstyle[MAX_DAMREDUCT_MULTI];
  int damstyleval[MAX_DAMREDUCT_MULTI];

  struct damreduct_type *next;
};


/* Queued spell entry */
struct queued_act {
   int level;
   int spellnum;
};

/* Structure used for chars following other chars */
struct follow_type {
   struct char_data *follower;
   struct follow_type *next;
};


#define LEVELTYPE_CLASS	1
#define LEVELTYPE_RACE	2

struct level_learn_entry {
  struct level_learn_entry *next;
  int location;
  int specific;
  byte value;
};

struct levelup_data {
  struct levelup_data *next;	/* Form a linked list			*/
  struct levelup_data *prev;	/* Form a linked list			*/
  byte type;			/* LEVELTYPE_ value			*/
  byte spec;			/* Specific class or race		*/
  byte level;			/* Level ir HD # for that class or race	*/

  byte hp_roll;			/* Straight die-roll value with no mods	*/
  byte mana_roll;		/* Straight die-roll value with no mods	*/
  byte ki_roll;			/* Straight die-roll value with no mods	*/
  byte move_roll;		/* Straight die-roll value with no mods	*/

  byte accuracy;		/* Hit accuracy change			*/
  byte fort;			/* Fortitude change			*/
  byte reflex;			/* Reflex change			*/
  byte will;			/* Will change				*/

  byte add_skill;		/* Total added skill points		*/
  byte add_gen_feats;		/* General feat points			*/
  byte add_epic_feats;		/* General epic feat points		*/
  byte add_class_feats;		/* Class feat points			*/
  byte add_class_epic_feats;	/* Epic class feat points		*/

  struct level_learn_entry *skills;	/* Head of linked list		*/
  struct level_learn_entry *feats;	/* Head of linked list		*/
};


/* ================== Structure for player/non-player ===================== */
struct char_data {
  int pfilepos;			/* playerfile pos			*/
  mob_rnum nr;			/* Mob's rnum				*/
  room_rnum in_room;		/* Location (real room number)		*/
  room_rnum was_in_room;	/* location for linkdead people		*/
  int wait;			/* wait for how many loops		*/

  char passwd[MAX_PWD_LENGTH+1];
				/* character's password			*/
  char *name;			/* PC / NPC s name (kill ...  )		*/
  char *short_descr;		/* for NPC 'actions'			*/
  char *long_descr;		/* for 'look'				*/
  char *description;		/* Extra descriptions                   */
  char *title;			/* PC / NPC's title                     */
  int size;			/* Size class of char                   */
  byte sex;			/* PC / NPC's sex                       */
  byte race;			/* PC / NPC's race                      */
  int race_level;		/* PC / NPC's racial level / hit dice   */
  int level_adj;		/* PC level adjustment                  */
  byte chclass;			/* Last class taken                     */
  int chclasses[NUM_CLASSES];	/* Ranks in all classes        */
  int epicclasses[NUM_CLASSES];	/* Ranks in all epic classes */
  struct levelup_data *level_info;
				/* Info on gained levels */
  int level;			/* PC / NPC's level                     */
  int admlevel;			/* PC / NPC's admin level               */
  int admflags[AD_ARRAY_MAX];	/* Bitvector for admin privs		*/
  sh_int hometown;		/* PC s Hometown (zone)                 */
  struct time_data time;	/* PC's AGE in days			*/
  ubyte weight;			/* PC / NPC's weight                    */
  ubyte height;			/* PC / NPC's height                    */

  struct abil_data real_abils;	/* Abilities without modifiers   */
  struct abil_data aff_abils;	/* Abils with spells/stones/etc  */
  struct player_special_data *player_specials;
				/* PC specials				*/
  struct mob_special_data mob_specials;
				/* NPC specials				*/

  struct affected_type *affected;
				/* affected by what spells		*/
  struct affected_type *affectedv;
				/* affected by what combat spells	*/
  struct damreduct_type *damreduct;
				/* damage resistances			*/
  struct queued_act *actq;	/* queued spells / other actions	*/

  struct obj_data *equipment[NUM_WEARS];
				/* Equipment array			*/
  struct obj_data *carrying;	/* Head of list				*/

  char *hit_breakdown[2];	/* description of last attack roll breakdowns */
  char *dam_breakdown[2];	/* description of last damage roll breakdowns */
  char *crit_breakdown[2];	/* description of last damage roll breakdowns */

  struct descriptor_data *desc;	/* NULL for mobiles			*/
  long id;			/* used by DG triggers			*/

  struct trig_proto_list *proto_script;
				/* list of default triggers		*/
  struct script_data *script;	/* script info for the object		*/
  struct script_memory *memory;	/* for mob memory triggers		*/

  struct char_data *next_in_room;
				/* For room->people - list		*/
  struct char_data *next;	/* For either monster or ppl-list	*/
  struct char_data *next_fighting;
				/* For fighting list			*/
  struct char_data *next_affect;/* For affect wearoff			*/
  struct char_data *next_affectv;
				/* For round based affect wearoff	*/

  struct follow_type *followers;/* List of chars followers		*/
  struct char_data *master;	/* Who is char following?		*/
  long master_id;

  struct memorize_node *memorized;
  struct innate_node *innate;

  struct char_data *fighting;	/* Opponent				*/
  struct char_data *hunting;	/* Char hunted by this char		*/

  byte position;		/* Standing, fighting, sleeping, etc.	*/

  int carry_weight;		/* Carried weight			*/
  byte carry_items;		/* Number of items carried		*/
  int timer;			/* Timer for update			*/

  byte feats[MAX_FEATS + 1];	/* Feats (booleans and counters)	*/
  int combat_feats[CFEAT_MAX+1][FT_ARRAY_MAX];
				/* One bitvector array per CFEAT_ type	*/
  int school_feats[SFEAT_MAX+1];/* One bitvector array per CFEAT_ type	*/

  byte skills[SKILL_TABLE_SIZE + 1];
				/* array of skills/spells/arts/etc	*/
  byte skillmods[SKILL_TABLE_SIZE + 1];
				/* array of skill mods			*/

  int alignment;		/* +-1000 for alignment good vs. evil	*/
  int alignment_ethic;		/* +-1000 for alignment law vs. chaos	*/
  long idnum;			/* player's idnum; -1 for mobiles	*/
  int act[PM_ARRAY_MAX];	/* act flag for NPC's; player flag for PC's */

  int affected_by[AF_ARRAY_MAX];/* Bitvector for current affects	*/
  sh_int saving_throw[3];	/* Saving throw				*/
  sh_int apply_saving_throw[3];	/* Saving throw bonuses			*/

  int powerattack;		/* Setting for power attack level	*/

  sh_int mana;
  sh_int max_mana;		/* Max mana for PC/NPC			*/
  sh_int hit;
  sh_int max_hit;		/* Max hit for PC/NPC			*/
  sh_int move;
  sh_int max_move;		/* Max move for PC/NPC			*/
  sh_int ki;
  sh_int max_ki;		/* Max ki for PC/NPC			*/

  sh_int armor;			/* Internally stored *10		*/
  int gold;			/* Money carried			*/
  int bank_gold;		/* Gold the char has in a bank account	*/
  int exp;			/* The experience of the player		*/

  int accuracy;			/* Base hit accuracy			*/
  int accuracy_mod;		/* Any bonus or penalty to the accuracy	*/
  int damage_mod;		/* Any bonus or penalty to the damage	*/

  sh_int spellfail;		/* Total spell failure %                 */
  sh_int armorcheck;		/* Total armorcheck penalty with proficiency forgiveness */
  sh_int armorcheckall;		/* Total armorcheck penalty regardless of proficiency */
};

/* ====================================================================== */


/* descriptor-related structures ******************************************/


struct txt_block {
   char	*text;
   int aliased;
   struct txt_block *next;
};


struct txt_q {
   struct txt_block *head;
   struct txt_block *tail;
};

struct compr {
    int state; /* 0 - off. 1 - waiting for response. 2 - compress2 on */

#ifdef HAVE_ZLIB_H
    Bytef *buff_out;
    int total_out; /* size of input buffer */
    int size_out; /* size of data in output buffer */

    Bytef *buff_in;
    int total_in; /* size of input buffer */
    int size_in; /* size of data in input buffer */

    z_streamp stream;
#endif /* HAVE_ZLIB_H */
};

struct descriptor_data {
   socket_t	descriptor;	/* file descriptor for socket		*/
   char	host[HOST_LENGTH+1];	/* hostname				*/
   byte	bad_pws;		/* number of bad pw attemps this login	*/
   byte idle_tics;		/* tics idle at password prompt		*/
   int	connected;		/* mode of 'connectedness'		*/
   int	desc_num;		/* unique num assigned to desc		*/
   time_t login_time;		/* when the person connected		*/
   char *showstr_head;		/* for keeping track of an internal str	*/
   char **showstr_vector;	/* for paging through texts		*/
   int  showstr_count;		/* number of pages to page through	*/
   int  showstr_page;		/* which page are we currently showing?	*/
   char	**str;			/* for the modify-str system		*/
   char *backstr;		/* backup string for modify-str system	*/
   size_t max_str;	        /* maximum size of string in modify-str	*/
   long	mail_to;		/* name for mail system			*/
   int	has_prompt;		/* is the user at a prompt?             */
   char	inbuf[MAX_RAW_INPUT_LENGTH];  /* buffer for raw input		*/
   char	last_input[MAX_INPUT_LENGTH]; /* the last input			*/
   char small_outbuf[SMALL_BUFSIZE];  /* standard output buffer		*/
   char *output;		/* ptr to the current output buffer	*/
   char **history;		/* History of commands, for ! mostly.	*/
   int	history_pos;		/* Circular array position.		*/
   int  bufptr;			/* ptr to end of current output		*/
   int	bufspace;		/* space left in the output buffer	*/
   struct txt_block *large_outbuf; /* ptr to large buffer, if we need it */
   struct txt_q input;		/* q of unprocessed input		*/
   struct char_data *character;	/* linked to char			*/
   struct char_data *original;	/* original char if switched		*/
   struct descriptor_data *snooping; /* Who is this char snooping	*/
   struct descriptor_data *snoop_by; /* And who is snooping this char	*/
   struct descriptor_data *next; /* link to next descriptor		*/
   struct oasis_olc_data *olc;   /* OLC info                            */
   struct compr *comp;                /* compression info */
};


/* other miscellaneous structures ***************************************/


struct msg_type {
   char	*attacker_msg;  /* message to attacker */
   char	*victim_msg;    /* message to victim   */
   char	*room_msg;      /* message to room     */
};


struct message_type {
   struct msg_type die_msg;	/* messages when death			*/
   struct msg_type miss_msg;	/* messages when miss			*/
   struct msg_type hit_msg;	/* messages when hit			*/
   struct msg_type god_msg;	/* messages when hit on god		*/
   struct message_type *next;	/* to next messages of this kind.	*/
};


struct message_list {
   int	a_type;			/* Attack type				*/
   int	number_of_attacks;	/* How many attack messages to chose from. */
   struct message_type *msg;	/* List of messages.			*/
};

/* used in the socials */
struct social_messg {
  int act_nr;
  char *command;               /* holds copy of activating command */
  char *sort_as;              /* holds a copy of a similar command or
                               * abbreviation to sort by for the parser */
  int hide;                   /* ? */
  int min_victim_position;    /* Position of victim */
  int min_char_position;      /* Position of char */
  int min_level_char;          /* Minimum level of socialing char */

  /* No argument was supplied */
  char *char_no_arg;
  char *others_no_arg;

  /* An argument was there, and a victim was found */
  char *char_found;
  char *others_found;
  char *vict_found;

  /* An argument was there, as well as a body part, and a victim was found */
  char *char_body_found;
  char *others_body_found;
  char *vict_body_found;

  /* An argument was there, but no victim was found */
  char *not_found;

  /* The victim turned out to be the character */
  char *char_auto;
  char *others_auto;

  /* If the char cant be found search the char's inven and do these: */
  char *char_obj_found;
  char *others_obj_found;
};


struct weather_data {
   int	pressure;	/* How is the pressure ( Mb ) */
   int	change;	/* How fast and what way does it change. */
   int	sky;	/* How is the sky. */
   int	sunlight;	/* And how much sun. */
};


/*
 * Element in monster and object index-tables.
 *
 * NOTE: Assumes sizeof(mob_vnum) >= sizeof(obj_vnum)
 */
struct index_data {
   mob_vnum	vnum;	/* virtual number of this mob/obj		*/
   int		number;	/* number of existing units of this mob/obj	*/
   SPECIAL(*func);

   char *farg;         /* string argument for special function     */
   struct trig_data *proto;     /* for triggers... the trigger     */
};

/* linked list for mob/object prototype trigger lists */
struct trig_proto_list {
  int vnum;                             /* vnum of the trigger   */
  struct trig_proto_list *next;         /* next trigger          */
};

struct guild_info_type {
  int pc_class;
  room_vnum guild_room;
  int direction;
};

/*
 * Config structs
 * 
 */
 
 /*
 * The game configuration structure used for configurating the game play 
 * variables.
 */
struct game_data {
  int pk_allowed;         /* Is player killing allowed? 	  */
  int pt_allowed;         /* Is player thieving allowed?	  */
  int level_can_shout;	  /* Level player must be to shout.	  */
  int holler_move_cost;	  /* Cost to holler in move points.	  */
  int tunnel_size;        /* Number of people allowed in a tunnel.*/
  int max_exp_gain;       /* Maximum experience gainable per kill.*/
  int max_exp_loss;       /* Maximum experience losable per death.*/
  int max_npc_corpse_time;/* Num tics before NPC corpses decompose*/
  int max_pc_corpse_time; /* Num tics before PC corpse decomposes.*/
  int idle_void;          /* Num tics before PC sent to void(idle)*/
  int idle_rent_time;     /* Num tics before PC is autorented.	  */
  int idle_max_level;     /* Level of players immune to idle.     */
  int dts_are_dumps;      /* Should items in dt's be junked?	  */
  int load_into_inventory;/* Objects load in immortals inventory. */
  int track_through_doors;/* Track through doors while closed?    */
  int level_cap;          /* You cannot level to this level       */
  int stack_mobs;	  /* Turn mob stacking on                 */
  int stack_objs;	  /* Turn obj stacking on                 */
  int mob_fighting;       /* Allow mobs to attack other mobs.     */	 
  char *OK;               /* When player receives 'Okay.' text.	  */
  char *NOPERSON;         /* 'No-one by that name here.'	  */
  char *NOEFFECT;         /* 'Nothing seems to happen.'	          */
  int disp_closed_doors;  /* Display closed doors in autoexit?	  */
  int reroll_player;      /* Players can reroll stats on creation */
  int enable_compression; /* Enable MCCP2 stream compression      */
  int enable_languages;   /* Enable spoken languages              */
  int all_items_unique;   /* Treat all items as unique 		  */
  float exp_multiplier;     /* Experience gain  multiplier	  */
};



/*
 * The rent and crashsave options.
 */
struct crash_save_data {
  int free_rent;          /* Should the MUD allow rent for free?  */
  int max_obj_save;       /* Max items players can rent.          */
  int min_rent_cost;      /* surcharge on top of item costs.	  */
  int auto_save;          /* Does the game automatically save ppl?*/
  int autosave_time;      /* if auto_save=TRUE, how often?        */
  int crash_file_timeout; /* Life of crashfiles and idlesaves.    */
  int rent_file_timeout;  /* Lifetime of normal rent files in days*/
};


/*
 * The room numbers. 
 */
struct room_numbers {
  room_vnum mortal_start_room;	/* vnum of room that mortals enter at.  */
  room_vnum immort_start_room;  /* vnum of room that immorts enter at.  */
  room_vnum frozen_start_room;  /* vnum of room that frozen ppl enter.  */
  room_vnum donation_room_1;    /* vnum of donation room #1.            */
  room_vnum donation_room_2;    /* vnum of donation room #2.            */
  room_vnum donation_room_3;    /* vnum of donation room #3.	        */
};


/*
 * The game operational constants.
 */
struct game_operation {
  ush_int DFLT_PORT;        /* The default port to run the game.  */
  char *DFLT_IP;            /* Bind to all interfaces.		  */
  char *DFLT_DIR;           /* The default directory (lib).	  */
  char *LOGNAME;            /* The file to log messages to.	  */
  int max_playing;          /* Maximum number of players allowed. */
  int max_filesize;         /* Maximum size of misc files.	  */
  int max_bad_pws;          /* Maximum number of pword attempts.  */
  int siteok_everyone;	    /* Everyone from all sites are SITEOK.*/
  int nameserver_is_slow;   /* Is the nameserver slow or fast?	  */
  int use_new_socials;      /* Use new or old socials file ?      */
  int auto_save_olc;        /* Does OLC save to disk right away ? */
  char *MENU;               /* The MAIN MENU.			  */
  char *WELC_MESSG;	    /* The welcome message.		  */
  char *START_MESSG;        /* The start msg for new characters.  */
};

/*
 * The Autowizard options.
 */
struct autowiz_data {
  int use_autowiz;        /* Use the autowiz feature?		*/
  int min_wizlist_lev;    /* Minimun level to show on wizlist.	*/
};

/* This is for the tick system.
 *
 */
 
struct tick_data {
  int pulse_violence;
  int pulse_mobile;
  int pulse_zone;
  int pulse_autosave;
  int pulse_idlepwd;
  int pulse_sanity;
  int pulse_usage;
  int pulse_timesave;
  int pulse_current;
};

/*
 * The character advancement (leveling) options.
 */
struct advance_data {
  int allow_multiclass; /* Allow advancement in multiple classes     */
};

/*
 * The main configuration structure;
 */
struct config_data {
  char                   *CONFFILE;	/* config file path	 */
  struct game_data       play;		/* play related config   */
  struct crash_save_data csd;		/* rent and save related */
  struct room_numbers    room_nums;	/* room numbers          */
  struct game_operation  operation;	/* basic operation       */
  struct autowiz_data    autowiz;	/* autowiz related stuff */
  struct advance_data    advance;   /* char advancement stuff */
  struct tick_data       ticks;
};

/*
 * Data about character aging
 */
struct aging_data {
  int adult;		/* Adulthood */
  int classdice[3][2];	/* Dice info for starting age based on class age type */
  int middle;		/* Middle age */
  int old;		/* Old age */
  int venerable;	/* Venerable age */
  int maxdice[2];	/* For roll to determine natural death beyond venerable */
};
