/* ************************************************************************
*   File: spec_procs.c                                  Part of CircleMUD *
*  Usage: implementation of special procedures for mobiles/objects/rooms  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/spec_procs.c,v 1.4 2004/12/22 19:39:42 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"

/*   external vars  */
extern struct time_info_data time_info;
extern struct spell_info_type spell_info[];
extern struct guild_info_type guild_info[];
extern const char *class_names[];
extern const char *pc_class_types[];

/* extern functions */
ACMD(do_drop);
ACMD(do_gen_door);
ACMD(do_say);
ACMD(do_action);
void gain_level(struct char_data *ch, int whichclass);

/* local functions */
SPECIAL(guild);
SPECIAL(dump);
SPECIAL(mayor);
void npc_steal(struct char_data *ch, struct char_data *victim);
SPECIAL(snake);
SPECIAL(thief);
SPECIAL(magic_user);
SPECIAL(guild_guard);
SPECIAL(puff);
SPECIAL(fido);
SPECIAL(janitor);
SPECIAL(cityguard);
SPECIAL(pet_shops);
SPECIAL(bank);


/* ********************************************************************
*  Special procedures for mobiles                                     *
******************************************************************** */

SPECIAL(dump)
{
  struct obj_data *k;
  int value = 0;

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    extract_obj(k);
  }

  if (!CMD_IS("drop"))
    return (FALSE);

  do_drop(ch, argument, cmd, SCMD_DROP);

  for (k = world[IN_ROOM(ch)].contents; k; k = world[IN_ROOM(ch)].contents) {
    act("$p vanishes in a puff of smoke!", FALSE, 0, k, 0, TO_ROOM);
    value += MAX(1, MIN(50, GET_OBJ_COST(k) / 10));
    extract_obj(k);
  }

  if (value) {
    send_to_char(ch, "You are awarded for outstanding performance.\r\n");
    act("$n has been awarded for being a good citizen.", TRUE, ch, 0, 0, TO_ROOM);

    if (GET_LEVEL(ch) < 3)
      gain_exp(ch, value);
    else
      GET_GOLD(ch) += value;
  }
  return (TRUE);
}


SPECIAL(mayor)
{
  char actbuf[MAX_INPUT_LENGTH];

  const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";
  const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

  static const char *path = NULL;
  static int path_index;
  static bool move = FALSE;

  if (!move) {
    if (time_info.hours == 6) {
      move = TRUE;
      path = open_path;
      path_index = 0;
    } else if (time_info.hours == 20) {
      move = TRUE;
      path = close_path;
      path_index = 0;
    }
  }
  if (cmd || !move || (GET_POS(ch) < POS_SLEEPING) ||
      (GET_POS(ch) == POS_FIGHTING))
    return (FALSE);

  switch (path[path_index]) {
  case '0':
  case '1':
  case '2':
  case '3':
    perform_move(ch, path[path_index] - '0', 1);
    break;

  case 'W':
    GET_POS(ch) = POS_STANDING;
    act("$n awakens and groans loudly.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'S':
    GET_POS(ch) = POS_SLEEPING;
    act("$n lies down and instantly falls asleep.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'a':
    act("$n says 'Hello Honey!'", FALSE, ch, 0, 0, TO_ROOM);
    act("$n smirks.", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'b':
    act("$n says 'What a view!  I must get something done about that dump!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'c':
    act("$n says 'Vandals!  Youngsters nowadays have no respect for anything!'",
	FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'd':
    act("$n says 'Good day, citizens!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'e':
    act("$n says 'I hereby declare the bazaar open!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'E':
    act("$n says 'I hereby declare Midgaard closed!'", FALSE, ch, 0, 0, TO_ROOM);
    break;

  case 'O':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_UNLOCK);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_OPEN);	/* strcpy: OK */
    break;

  case 'C':
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_CLOSE);	/* strcpy: OK */
    do_gen_door(ch, strcpy(actbuf, "gate"), 0, SCMD_LOCK);	/* strcpy: OK */
    break;

  case '.':
    move = FALSE;
    break;

  }

  path_index++;
  return (FALSE);
}


/* ********************************************************************
*  General special procedures for mobiles                             *
******************************************************************** */


void npc_steal(struct char_data *ch, struct char_data *victim)
{
  int gold;

  if (IS_NPC(victim))
    return;
  if (ADM_FLAGGED(victim, ADM_NOSTEAL))
    return;
  if (!CAN_SEE(ch, victim))
    return;

  if (AWAKE(victim) && (rand_number(0, GET_LEVEL(ch)) == 0)) {
    act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, victim, TO_VICT);
    act("$n tries to steal gold from $N.", TRUE, ch, 0, victim, TO_NOTVICT);
  } else {
    /* Steal some gold coins */
    gold = (GET_GOLD(victim) * rand_number(1, 10)) / 100;
    if (gold > 0) {
      GET_GOLD(ch) += gold;
      GET_GOLD(victim) -= gold;
    }
  }
}


/*
 * Quite lethal to low-level characters.
 */
SPECIAL(snake)
{
  if (cmd || GET_POS(ch) != POS_FIGHTING || !FIGHTING(ch))
    return (FALSE);

  if (IN_ROOM(FIGHTING(ch)) != IN_ROOM(ch) || rand_number(0, GET_LEVEL(ch)) != 0)
    return (FALSE);

  act("$n bites $N!", 1, ch, 0, FIGHTING(ch), TO_NOTVICT);
  act("$n bites you!", 1, ch, 0, FIGHTING(ch), TO_VICT);
  call_magic(ch, FIGHTING(ch), 0, SPELL_POISON, GET_LEVEL(ch), CAST_SPELL, NULL);
  return (TRUE);
}


SPECIAL(thief)
{
  struct char_data *cons;

  if (cmd || GET_POS(ch) != POS_STANDING)
    return (FALSE);

  for (cons = world[IN_ROOM(ch)].people; cons; cons = cons->next_in_room)
    if (!IS_NPC(cons) && !ADM_FLAGGED(cons, ADM_NOSTEAL) && !rand_number(0, 4)) {
      npc_steal(ch, cons);
      return (TRUE);
    }

  return (FALSE);
}


SPECIAL(magic_user_orig)
{
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return (FALSE);

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL && IN_ROOM(FIGHTING(ch)) == IN_ROOM(ch))
    vict = FIGHTING(ch);

  /* Hm...didn't pick anyone...I'll wait a round. */
  if (vict == NULL)
    return (TRUE);

  if (GET_LEVEL(ch) > 13 && rand_number(0, 10) == 0)
    cast_spell(ch, vict, NULL, SPELL_POISON, NULL);

  if (GET_LEVEL(ch) > 7 && rand_number(0, 8) == 0)
    cast_spell(ch, vict, NULL, SPELL_BLINDNESS, NULL);

  if (GET_LEVEL(ch) > 12 && rand_number(0, 12) == 0) {
    if (IS_EVIL(ch))
      cast_spell(ch, vict, NULL, SPELL_ENERGY_DRAIN, NULL);
    else if (IS_GOOD(ch))
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
  }

  if (rand_number(0, 4))
    return (TRUE);

  switch (GET_LEVEL(ch)) {
  case 4:
  case 5:
    cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
    break;
  case 6:
  case 7:
    cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
    break;
  case 8:
  case 9:
    cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
    break;
  case 10:
  case 11:
    cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
    break;
  case 12:
  case 13:
    cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
    break;
  case 14:
  case 15:
  case 16:
  case 17:
    cast_spell(ch, vict, NULL, SPELL_COLOR_SPRAY, NULL);
    break;
  default:
    cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
    break;
  }
  return (TRUE);

}


/* ********************************************************************
*  Special procedures for mobiles                                      *
******************************************************************** */

SPECIAL(guild_guard)
{
  int i;
  struct char_data *guard = (struct char_data *)me;
  const char *buf = "The guard humiliates you, and blocks your way.\r\n";
  const char *buf2 = "The guard humiliates $n, and blocks $s way.";

  if (!IS_MOVE(cmd) || AFF_FLAGGED(guard, AFF_BLIND))
    return (FALSE);

  if (ADM_FLAGGED(ch, ADM_WALKANYWHERE))
    return (FALSE);

  for (i = 0; guild_info[i].guild_room != NOWHERE; i++) {
    /* Wrong guild or not trying to enter. */
    if (GET_ROOM_VNUM(IN_ROOM(ch)) != guild_info[i].guild_room || cmd != guild_info[i].direction)
      continue;

    /* Allow the people of the guild through. */
    if (!IS_NPC(ch) && GET_CLASS(ch) == guild_info[i].pc_class)
      continue;

    send_to_char(ch, "%s", buf);
    act(buf2, FALSE, ch, 0, 0, TO_ROOM);
    return (TRUE);
  }

  return (FALSE);
}



SPECIAL(puff)
{
  char actbuf[MAX_INPUT_LENGTH];

  if (cmd)
    return (FALSE);

  switch (rand_number(0, 60)) {
  case 0:
    do_say(ch, strcpy(actbuf, "My god!  It's full of stars!"), 0, 0);	/* strcpy: OK */
    return (TRUE);
  case 1:
    do_say(ch, strcpy(actbuf, "How'd all those fish get up here?"), 0, 0);	/* strcpy: OK */
    return (TRUE);
  case 2:
    do_say(ch, strcpy(actbuf, "I'm a very female dragon."), 0, 0);	/* strcpy: OK */
    return (TRUE);
  case 3:
    do_say(ch, strcpy(actbuf, "I've got a peaceful, easy feeling."), 0, 0);	/* strcpy: OK */
    return (TRUE);
  default:
    return (FALSE);
  }
}



SPECIAL(fido)
{
  struct obj_data *i, *temp, *next_obj;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!IS_CORPSE(i))
      continue;

    act("$n savagely devours a corpse.", FALSE, ch, 0, 0, TO_ROOM);
    for (temp = i->contains; temp; temp = next_obj) {
      next_obj = temp->next_content;
      obj_from_obj(temp);
      obj_to_room(temp, IN_ROOM(ch));
    }
    extract_obj(i);
    return (TRUE);
  }

  return (FALSE);
}



SPECIAL(janitor)
{
  struct obj_data *i;

  if (cmd || !AWAKE(ch))
    return (FALSE);

  for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content) {
    if (!CAN_WEAR(i, ITEM_WEAR_TAKE))
      continue;
    if (GET_OBJ_TYPE(i) != ITEM_DRINKCON && GET_OBJ_COST(i) >= 15)
      continue;
    act("$n picks up some trash.", FALSE, ch, 0, 0, TO_ROOM);
    obj_from_room(i);
    obj_to_char(i, ch);
    return (TRUE);
  }

  return (FALSE);
}


SPECIAL(cityguard)
{
  struct char_data *tch, *evil, *spittle;
  int max_evil, min_cha;

  if (cmd || !AWAKE(ch) || FIGHTING(ch))
    return (FALSE);

  max_evil = 1000;
  min_cha = 6;
  spittle = evil = NULL;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (!CAN_SEE(ch, tch))
      continue;

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_KILLER)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER KILLERS!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }

    if (!IS_NPC(tch) && PLR_FLAGGED(tch, PLR_THIEF)) {
      act("$n screams 'HEY!!!  You're one of those PLAYER THIEVES!!!!!!'", FALSE, ch, 0, 0, TO_ROOM);
      hit(ch, tch, TYPE_UNDEFINED);
      return (TRUE);
    }

    if (FIGHTING(tch) && GET_ALIGNMENT(tch) < max_evil && (IS_NPC(tch) || IS_NPC(FIGHTING(tch)))) {
      max_evil = GET_ALIGNMENT(tch);
      evil = tch;
    }

    if (GET_CHA(tch) < min_cha) {
      spittle = tch;
      min_cha = GET_CHA(tch);
    }
  }

  if (evil && GET_ALIGNMENT(FIGHTING(evil)) >= 0) {
    act("$n screams 'PROTECT THE INNOCENT!  BANZAI!  CHARGE!  ARARARAGGGHH!'", FALSE, ch, 0, 0, TO_ROOM);
    hit(ch, evil, TYPE_UNDEFINED);
    return (TRUE);
  }

  /* Reward the socially inept. */
  if (spittle && !rand_number(0, 9)) {
    static int spit_social;

    if (!spit_social)
      spit_social = find_command("spit");

    if (spit_social > 0) {
      char spitbuf[MAX_NAME_LENGTH + 1];

      strncpy(spitbuf, GET_NAME(spittle), sizeof(spitbuf));	/* strncpy: OK */
      spitbuf[sizeof(spitbuf) - 1] = '\0';

      do_action(ch, spitbuf, spit_social, 0);
      return (TRUE);
    }
  }

  return (FALSE);
}


#define PET_PRICE(pet) (GET_LEVEL(pet) * 300)

SPECIAL(pet_shops)
{
  char buf[MAX_STRING_LENGTH], pet_name[256];
  room_rnum pet_room;
  struct char_data *pet;

  /* Gross. */
  pet_room = IN_ROOM(ch) + 1;

  if (CMD_IS("list")) {
    send_to_char(ch, "Available pets are:\r\n");
    for (pet = world[pet_room].people; pet; pet = pet->next_in_room) {
      /* No, you can't have the Implementor as a pet if he's in there. */
      if (!IS_NPC(pet))
        continue;
      send_to_char(ch, "%8d - %s\r\n", PET_PRICE(pet), GET_NAME(pet));
    }
    return (TRUE);
  } else if (CMD_IS("buy")) {

    two_arguments(argument, buf, pet_name);

    if (!(pet = get_char_room(buf, NULL, pet_room)) || !IS_NPC(pet)) {
      send_to_char(ch, "There is no such pet!\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < PET_PRICE(pet)) {
      send_to_char(ch, "You don't have enough gold!\r\n");
      return (TRUE);
    }
    GET_GOLD(ch) -= PET_PRICE(pet);

    pet = read_mobile(GET_MOB_RNUM(pet), REAL);
    GET_EXP(pet) = 0;
    SET_BIT_AR(AFF_FLAGS(pet), AFF_CHARM);

    if (*pet_name) {
      snprintf(buf, sizeof(buf), "%s %s", pet->name, pet_name);
      /* free(pet->name); don't free the prototype! */
      pet->name = strdup(buf);

      snprintf(buf, sizeof(buf), "%sA small sign on a chain around the neck says 'My name is %s'\r\n",
	      pet->description, pet_name);
      /* free(pet->description); don't free the prototype! */
      pet->description = strdup(buf);
    }
    char_to_room(pet, IN_ROOM(ch));
    add_follower(pet, ch);
    pet->master_id = GET_IDNUM(ch);

    /* Be certain that pets can't get/carry/use/wield/wear items */
    IS_CARRYING_W(pet) = 1000;
    IS_CARRYING_N(pet) = 100;

    send_to_char(ch, "May you enjoy your pet.\r\n");
    act("$n buys $N as a pet.", FALSE, ch, 0, pet, TO_ROOM);

    return (TRUE);
  }

  /* All commands except list and buy */
  return (FALSE);
}



/* ********************************************************************
*  Special procedures for objects                                     *
******************************************************************** */


SPECIAL(bank)
{
  int amount;

  if (CMD_IS("balance")) {
    if (GET_BANK_GOLD(ch) > 0)
      send_to_char(ch, "Your current balance is %d coins.\r\n", GET_BANK_GOLD(ch));
    else
      send_to_char(ch, "You currently have no money deposited.\r\n");
    return (TRUE);
  } else if (CMD_IS("deposit")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to deposit?\r\n");
      return (TRUE);
    }
    if (GET_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins!\r\n");
      return (TRUE);
    }
    GET_GOLD(ch) -= amount;
    GET_BANK_GOLD(ch) += amount;
    send_to_char(ch, "You deposit %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else if (CMD_IS("withdraw")) {
    if ((amount = atoi(argument)) <= 0) {
      send_to_char(ch, "How much do you want to withdraw?\r\n");
      return (TRUE);
    }
    if (GET_BANK_GOLD(ch) < amount) {
      send_to_char(ch, "You don't have that many coins deposited!\r\n");
      return (TRUE);
    }
    GET_GOLD(ch) += amount;
    GET_BANK_GOLD(ch) -= amount;
    send_to_char(ch, "You withdraw %d coins.\r\n", amount);
    act("$n makes a bank transaction.", TRUE, ch, 0, FALSE, TO_ROOM);
    return (TRUE);
  } else
    return (FALSE);
}


SPECIAL(cleric_marduk)
{
  int tmp, num_used = 0;
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 12;

  tmp = rand_number(1, 10);

  if ( (tmp == 7 ) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);
      if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
        cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
        return TRUE;
      }
      if ((tmp == 2) && ( (GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
        cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
        return TRUE;
      }
      if ((tmp == 3) && (GET_LEVEL(ch) > 4 )) {
        cast_spell(ch, vict, NULL, SPELL_BESTOW_CURSE, NULL);
        return TRUE;
      }
      if ((tmp == 4) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
        cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
        return TRUE;
      }
      if ((tmp == 5) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
        cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
        return TRUE;
      }
      if ((tmp == 6) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
        cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
        return TRUE;
      }
      if (tmp == 7) {
        cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
        return TRUE;
      }
      if ((tmp == 8) && (GET_LEVEL(ch) > 6 ) && (!IS_UNDEAD(vict))) {
        cast_spell(ch, vict, NULL, SPELL_POISON, NULL);
        return TRUE;
      }
      if (tmp == 9 && GET_LEVEL(ch) > 8) {
        cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
        return TRUE;
      }
      if ((tmp == 10) && (GET_LEVEL(ch) > 10)) {
        cast_spell(ch, vict, NULL, SPELL_HARM, NULL);
        return TRUE;
      }
      if (tmp == 11) {
        cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
        return TRUE;
      }
      if (tmp == 12 && GET_LEVEL(ch) > 8) {
        cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
        return TRUE;
      }
  }
  return FALSE;
}


SPECIAL(cleric_ao)
{
  int tmp, num_used = 0;
  struct char_data *vict;
  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 8;

  tmp = rand_number(1, 10);

  if ( (tmp == 7 ) || (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);
    if ((tmp == 1) && (GET_LEVEL(ch) > 13)) {
      cast_spell(ch, vict, NULL, SPELL_EARTHQUAKE, NULL);
      return TRUE;
    }
    if ((tmp == 2) && ( (GET_LEVEL(ch) > 8) && (IS_EVIL(vict)))) {
      cast_spell(ch, vict, NULL, SPELL_DISPEL_EVIL, NULL);
      return TRUE;
    }
    if ((tmp == 3) && ((GET_LEVEL(ch) > 8) && (IS_GOOD(vict)))) {
      cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
      return TRUE;
    }
    if ((tmp == 4) && (GET_LEVEL(ch) > 4 && affected_by_spell(ch, SPELL_BESTOW_CURSE))) {
      cast_spell(ch, ch, NULL, SPELL_REMOVE_CURSE, NULL);
      return TRUE;
    }
    if ((tmp == 5) && (GET_LEVEL(ch) > 6 && affected_by_spell(ch, SPELL_POISON))) {
      cast_spell(ch, ch, NULL, SPELL_NEUTRALIZE_POISON, NULL);
      return TRUE;
    }
    if (tmp == 6) {
      cast_spell(ch, ch, NULL, SPELL_CURE_LIGHT, NULL);
      return TRUE;
    }
    if (tmp == 7 && GET_LEVEL(ch) > 8) {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
      return TRUE;
    }
    if (tmp == 8 && GET_LEVEL(ch) > 10) {
      cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
      return TRUE;
    }
    if (tmp == 9) {
      cast_spell(ch, vict, NULL, SPELL_INFLICT_LIGHT, NULL);
      return TRUE;
    }
    if (tmp == 10 && GET_LEVEL(ch) > 8) {
      cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
      return TRUE;
    }
  }
  return FALSE;
}


SPECIAL(dziak)
{
  int tmp, num_used = 0;
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;
  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 9;

  tmp = rand_number(3, 10);

  if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);

    if (tmp == 2 || tmp == 1) {
      cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
      return TRUE;
    }
    if (tmp == 3) {
      cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
      return TRUE;
    }
    if (tmp == 4) {
      cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
      return TRUE;
    }
    if (tmp == 5) {
      cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
      return TRUE;
    }
    if (tmp == 6) {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
      return TRUE;
    }
    if (tmp == 7) {
      cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
      return TRUE;
    }
    if ((tmp == 8) && (IS_GOOD(vict))) {
      cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
      return TRUE;
    }
    if (tmp == 9) {
      cast_spell(ch, ch, NULL, SPELL_HEAL, NULL);
      return TRUE;
    }
  }
  return FALSE;
}


SPECIAL(azimer)
{
  int tmp, num_used = 0;
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 8;

  tmp = rand_number(3, 10);

  if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);

    if (tmp == 2 || tmp == 1) {
      cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
      return TRUE;
    }
    if (tmp == 3) {
      cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
      return TRUE;
    }
    if (tmp == 4) {
      cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
      return TRUE;
    }
    if (tmp == 5) {
      cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
      return TRUE;
    }
    if (tmp == 6) {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
      return TRUE;
    }
    if (tmp == 7) {
      cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
      return TRUE;
    }
    if ((tmp == 8) && (IS_GOOD(vict))) {
      cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
      return TRUE;
    }
  }
  return FALSE;
}


SPECIAL(lyrzaxyn)
{
  int tmp, num_used = 0;
  struct char_data *vict;

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 8;

  tmp = rand_number(3, 10);

  if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);

    if (tmp == 2 || tmp == 1) {
      cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
      return TRUE;
    }
    if (tmp == 3) {
      cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
      return TRUE;
    }
    if (tmp == 4) {
      cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
      return TRUE;
    }
    if (tmp == 5) {
      cast_spell(ch, vict, NULL, SPELL_FIREBALL, NULL);
      return TRUE;
    }
    if (tmp == 6) {
      cast_spell(ch, ch, NULL, SPELL_CURE_CRITIC, NULL);
      return TRUE;
    }
    if (tmp == 7) {
      cast_spell(ch, vict, NULL, SPELL_INFLICT_CRITIC, NULL);
      return TRUE;
    }
    if ((tmp == 8) && (IS_GOOD(vict))) {
      cast_spell(ch, vict, NULL, SPELL_DISPEL_GOOD, NULL);
      return TRUE;
    }
  }
  return FALSE;
}


SPECIAL(magic_user)
{
  int tmp, num_used = 0;
  struct char_data *vict;

  if (IS_NPC(ch) && GET_POS(ch) > POS_SITTING && GET_CLASS(ch) == CLASS_WIZARD) {
    if (!affected_by_spell(ch, SPELL_MAGE_ARMOR)) {
      cast_spell(ch, ch, NULL, SPELL_MAGE_ARMOR, NULL);
      return TRUE;
    }
  }

  if (cmd || GET_POS(ch) != POS_FIGHTING)
    return FALSE;

  /* pseudo-randomly choose someone in the room who is fighting me */
  for (vict = world[ch->in_room].people; vict; vict = vict->next_in_room)
    if (FIGHTING(vict) == ch && !rand_number(0, 4))
      break;

  /* if I didn't pick any of those, then just slam the guy I'm fighting */
  if (vict == NULL)
    vict = FIGHTING(ch);

  num_used = 6;

  tmp = rand_number(2, 10);

  if ( (tmp == 8) || (tmp == 9) || (tmp == 10)) {
    tmp = rand_number(1, num_used);

    if ((tmp == 1) && GET_LEVEL(ch) > 1) {
      cast_spell(ch, vict, NULL, SPELL_CHILL_TOUCH, NULL);
      return TRUE;
    }
    if ((tmp == 2) && !affected_by_spell(ch, SPELL_MAGE_ARMOR)) {
      cast_spell(ch, ch, NULL, SPELL_MAGE_ARMOR, NULL);
      return TRUE;
    }
    if ((tmp == 3) && GET_LEVEL(ch) > 1) {
      cast_spell(ch, vict, NULL, SPELL_BURNING_HANDS, NULL);
      return TRUE;
    }
    if ((tmp == 4) && GET_LEVEL(ch) > 1) {
      cast_spell(ch, vict, NULL, SPELL_MAGIC_MISSILE, NULL);
      return TRUE;
    }
    if ((tmp == 5) && GET_LEVEL(ch) > 5) {
      cast_spell(ch, vict, NULL, SPELL_SHOCKING_GRASP, NULL);
      return TRUE;
    }
    if ((tmp == 6) && GET_LEVEL(ch) > 9) {
      cast_spell(ch, vict, NULL, SPELL_LIGHTNING_BOLT, NULL);
      return TRUE;
    }
  }
  return FALSE;
}

