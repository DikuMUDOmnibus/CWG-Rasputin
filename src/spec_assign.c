/* ************************************************************************
*   File: spec_assign.c                                 Part of CircleMUD *
*  Usage: Functions to assign function pointers to objs/mobs/rooms        *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/spec_assign.c,v 1.4 2005/01/06 04:36:15 zizazat Exp $");

#include "structs.h"
#include "db.h"
#include "interpreter.h"
#include "utils.h"


/* external globals */
extern int mini_mud;

/* external functions */
SPECIAL(dump);
SPECIAL(pet_shops);
SPECIAL(postmaster);
SPECIAL(cityguard);
SPECIAL(receptionist);
SPECIAL(cryogenicist);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(mayor);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(bank);
SPECIAL(lyrzaxyn);
SPECIAL(azimer);
SPECIAL(dziak);
SPECIAL(cleric_ao);
SPECIAL(cleric_marduk);

/* local functions */
void assign_mobiles(void);
void assign_objects(void);
void assign_rooms(void);
void ASSIGNROOM(room_vnum room, SPECIAL(fname));
void ASSIGNMOB(mob_vnum mob, SPECIAL(fname));
void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname));

/* functions to perform assignments */

void ASSIGNMOB(mob_vnum mob, SPECIAL(fname))
{
  mob_rnum rnum;

  if ((rnum = real_mobile(mob)) != NOBODY)
    mob_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant mob #%d", mob);
}

void ASSIGNOBJ(obj_vnum obj, SPECIAL(fname))
{
  obj_rnum rnum;

  if ((rnum = real_object(obj)) != NOTHING)
    obj_index[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant obj #%d", obj);
}

void ASSIGNROOM(room_vnum room, SPECIAL(fname))
{
  room_rnum rnum;

  if ((rnum = real_room(room)) != NOWHERE)
    world[rnum].func = fname;
  else if (!mini_mud)
    log("SYSERR: Attempt to assign spec to non-existant room #%d", room);
}


/* ********************************************************************
*  Assignments                                                        *
******************************************************************** */

/* assign special procedures to mobiles */
void assign_mobiles(void)
{
  /* Tower of the Ordeal */
  ASSIGNMOB(103, magic_user);

  /* The followers of Dziak */
  ASSIGNMOB(401, magic_user);
  ASSIGNMOB(402, magic_user);
  ASSIGNMOB(403, cleric_marduk);
  ASSIGNMOB(404, dziak);

  /* Buried temple */
  ASSIGNMOB(600, snake);
  ASSIGNMOB(603, magic_user);

  /* Timmoth */
  ASSIGNMOB(1404, cleric_marduk);
  ASSIGNMOB(1405, cleric_marduk);

  /* Elysium */

  /* Aghazstamn's Lair */
  ASSIGNMOB(2104, magic_user);

  /* School of Wizardry */
  ASSIGNMOB(2505, azimer);

  /* First Quest */
  ASSIGNMOB(2902, cleric_marduk);
  ASSIGNMOB(2903, magic_user);

  /* Kortaal */
  ASSIGNMOB(3005, receptionist);
  ASSIGNMOB(3010, postmaster);
  ASSIGNMOB(3012, magic_user);
  ASSIGNMOB(3013, magic_user);
  ASSIGNMOB(3014, magic_user);
  ASSIGNMOB(3015, magic_user);
  ASSIGNMOB(3024, guild_guard);
  ASSIGNMOB(3025, guild_guard);
  ASSIGNMOB(3026, guild_guard);
  ASSIGNMOB(3027, guild_guard);
  ASSIGNMOB(3028, guild_guard);
  ASSIGNMOB(3029, guild_guard);
  ASSIGNMOB(3059, cityguard);
  ASSIGNMOB(3060, cityguard);
  ASSIGNMOB(3061, janitor);
  ASSIGNMOB(3062, fido);
  ASSIGNMOB(3066, fido);

  /* Outside Kortaal */
  ASSIGNMOB(3106, receptionist);
  ASSIGNMOB(3107, receptionist);
  ASSIGNMOB(3115, magic_user);

  /* Kings Roads */
  ASSIGNMOB(3601, cleric_ao);

  /* PC Mobs */
  ASSIGNMOB(4203, magic_user);
  ASSIGNMOB(4204, cleric_ao);
  ASSIGNMOB(4205, magic_user);
  ASSIGNMOB(4206, magic_user);
  ASSIGNMOB(4208, cleric_ao);
  ASSIGNMOB(4213, magic_user);
  ASSIGNMOB(4214, magic_user);

  /* Secret Tunnel */
  ASSIGNMOB(4342, lyrzaxyn);

  /* MORIA */
  ASSIGNMOB(4000, snake);
  ASSIGNMOB(4001, snake);
  ASSIGNMOB(4053, snake);
  ASSIGNMOB(4100, magic_user);
  ASSIGNMOB(4102, snake);
  ASSIGNMOB(4219, cityguard);

  /* Drow City */
  ASSIGNMOB(5103, magic_user);
  ASSIGNMOB(5104, cleric_marduk);
  ASSIGNMOB(5107, magic_user);
  ASSIGNMOB(5108, magic_user);

  /* Castle Kilgrave */
  ASSIGNMOB(5506, magic_user);
  ASSIGNMOB(5507, cleric_marduk);

  /* Kings Forest */
  ASSIGNMOB(6023, receptionist);
  ASSIGNMOB(6033, cleric_ao);
  ASSIGNMOB(6034, cleric_ao);
  ASSIGNMOB(6035, cleric_ao);
  ASSIGNMOB(6036, cleric_ao);
  ASSIGNMOB(6037, cleric_ao);

  /* FOREST */
  ASSIGNMOB(6112, magic_user);
  ASSIGNMOB(6113, snake);
  ASSIGNMOB(6114, magic_user);
  ASSIGNMOB(6115, magic_user);
  ASSIGNMOB(6116, cleric_ao);

  /* SEWERS */
  ASSIGNMOB(7006, snake);
  ASSIGNMOB(7009, magic_user);
  ASSIGNMOB(7015, guild_guard);
  ASSIGNMOB(7016, guild_guard);

  /* Ohari */
  ASSIGNMOB(8002, cityguard);
  ASSIGNMOB(8003, cityguard);
  ASSIGNMOB(8004, cityguard);
  ASSIGNMOB(8005, cityguard);
  ASSIGNMOB(8006, cityguard);
  ASSIGNMOB(8007, cleric_ao);
  ASSIGNMOB(8008, cityguard);
  ASSIGNMOB(8009, magic_user);
  ASSIGNMOB(8013, magic_user);
  ASSIGNMOB(8014, magic_user);
  ASSIGNMOB(8018, cleric_ao);
  ASSIGNMOB(8022, magic_user);

  /* Nangalen */
  ASSIGNMOB(8504, cleric_ao);
  ASSIGNMOB(8517, cleric_ao);
  ASSIGNMOB(8518, cleric_ao);
  ASSIGNMOB(8519, magic_user);
  ASSIGNMOB(8523, magic_user);
  ASSIGNMOB(8524, magic_user);
  ASSIGNMOB(8529, cleric_ao);
  ASSIGNMOB(8534, magic_user);
  /* Church of Ao */
  ASSIGNMOB(9000, cityguard);

  /* Maakan */
  ASSIGNMOB(11001, cleric_ao);
  ASSIGNMOB(11002, cleric_ao);
  ASSIGNMOB(11003, cleric_ao);
  ASSIGNMOB(11004, cleric_ao);
  ASSIGNMOB(11005, cleric_ao);
  ASSIGNMOB(11014, receptionist);
  ASSIGNMOB(11020, guild_guard);

  /* Tekaro */
  ASSIGNMOB(12000, receptionist);
  ASSIGNMOB(12004, receptionist);
  ASSIGNMOB(12006, guild_guard);
  ASSIGNMOB(12010, guild_guard);
  ASSIGNMOB(12008, guild_guard);

  /* Helgor */
  ASSIGNMOB(11803, receptionist);

  /* Woeld */
  ASSIGNMOB(11903, cityguard);
  ASSIGNMOB(11925, receptionist);
  ASSIGNMOB(11927, guild_guard);
}



/* assign special procedures to objects */
void assign_objects(void)
{
  ASSIGNOBJ(3034, bank);	/* atm */
  ASSIGNOBJ(3036, bank);	/* cashcard */
}



/* assign special procedures to rooms */
void assign_rooms(void)
{
  room_rnum i;

  ASSIGNROOM(3030, dump);
  ASSIGNROOM(3035, pet_shops);
  ASSIGNROOM(1491, pet_shops);

  if (CONFIG_DTS_ARE_DUMPS)
    for (i = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH))
	world[i].func = dump;
}
