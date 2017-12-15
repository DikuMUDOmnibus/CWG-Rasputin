/* ************************************************************************
*   File: act.offensive.c                               Part of CircleMUD *
*  Usage: player-level commands of an offensive nature                    *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.offensive.c,v 1.6 2005/01/17 03:17:53 zizazat Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"

/* extern functions */
void raw_kill(struct char_data *ch, struct char_data * killer);
void check_killer(struct char_data *ch, struct char_data *vict);
int is_innate_ready(struct char_data *ch, int spellnum);
void add_innate_timer(struct char_data *ch, int spellnum);

/* extern variables */
extern struct spell_info_type spell_info[];

/* local functions */
ACMD(do_assist);
ACMD(do_hit);
ACMD(do_kill);
ACMD(do_order);
ACMD(do_flee);


ACMD(do_assist)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *helpee, *opponent;

  if (FIGHTING(ch)) {
    send_to_char(ch, "You're already fighting!  How can you assist someone else?\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Whom do you wish to assist?\r\n");
  else if (!(helpee = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (helpee == ch)
    send_to_char(ch, "You can't help yourself any more than this!\r\n");
  else {
    /*
     * Hit the same enemy the person you're helping is.
     */
    if (FIGHTING(helpee))
      opponent = FIGHTING(helpee);
    else
      for (opponent = world[IN_ROOM(ch)].people;
	   opponent && (FIGHTING(opponent) != helpee);
	   opponent = opponent->next_in_room)
		;

    if (!opponent)
      act("But nobody is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
    else if (!CAN_SEE(ch, opponent))
      act("You can't see who is fighting $M!", FALSE, ch, 0, helpee, TO_CHAR);
         /* prevent accidental pkill */
    else if (!CONFIG_PK_ALLOWED && !IS_NPC(opponent))	
      act("Use 'murder' if you really want to attack $N.", FALSE,
	  ch, 0, opponent, TO_CHAR);
    else {
      send_to_char(ch, "You join the fight!\r\n");
      act("$N assists you!", 0, helpee, 0, ch, TO_CHAR);
      act("$n assists $N.", FALSE, ch, 0, helpee, TO_NOTVICT);
      hit(ch, opponent, TYPE_UNDEFINED);
    }
  }
}


ACMD(do_hit)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hit who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "They don't seem to be here.\r\n");
  else if (vict == ch) {
    send_to_char(ch, "You hit yourself...OUCH!.\r\n");
    act("$n hits $mself, and says OUCH!", FALSE, ch, 0, vict, TO_ROOM);
  } else if (AFF_FLAGGED(ch, AFF_CHARM) && (ch->master == vict))
    act("$N is just such a good friend, you simply can't hit $M.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!CONFIG_PK_ALLOWED) {
      if (!IS_NPC(vict) && !IS_NPC(ch)) {
	if (subcmd != SCMD_MURDER) {
	  send_to_char(ch, "Use 'murder' to hit another player.\r\n");
	  return;
	} else {
	  check_killer(ch, vict);
	}
      }
      if (AFF_FLAGGED(ch, AFF_CHARM) && !IS_NPC(ch->master) && !IS_NPC(vict))
	return;			/* you can't order a charmed pet to attack a
				 * player */
    }
    if ((GET_POS(ch) == POS_STANDING) && (vict != FIGHTING(ch))) {
      /* normal attack :( */
      hit(ch, vict, TYPE_UNDEFINED);
    } else
      send_to_char(ch, "You do the best you can!\r\n");
  }
}



ACMD(do_kill)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_INSTANTKILL)) {
    do_hit(ch, argument, cmd, subcmd);
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Kill who?\r\n");
  } else {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "They aren't here.\r\n");
    else if (ch == vict)
      send_to_char(ch, "Your mother would be so sad.. :(\r\n");
    else {
      act("You chop $M to pieces!  Ah!  The blood!", FALSE, ch, 0, vict, TO_CHAR);
      act("$N chops you to pieces!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n brutally slays $N!", FALSE, ch, 0, vict, TO_NOTVICT);
      raw_kill(vict, ch);
    }
  }
}



#if 0 /* backstab doesn't exist, sneaky types get automatic sneak attack */
ACMD(do_backstab)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int percent;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_BACKSTAB)) {
    send_to_char(ch, "You have no idea how to do that.\r\n");
    return;
  }

  one_argument(argument, buf);

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Backstab who?\r\n");
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "How can you sneak up on yourself?\r\n");
    return;
  }
  if (!GET_EQ(ch, WEAR_WIELD)) {
    send_to_char(ch, "You need to wield a weapon to make it a success.\r\n");
    return;
  }
  if (GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD), VAL_WEAPON_DAMTYPE) != TYPE_PIERCE - TYPE_HIT) {
    send_to_char(ch, "Only piercing weapons can be used for backstabbing.\r\n");
    return;
  }
  if (FIGHTING(vict)) {
    send_to_char(ch, "You can't backstab a fighting person -- they're too alert!\r\n");
    return;
  }

  if (MOB_FLAGGED(vict, MOB_AWARE) && AWAKE(vict)) {
    act("You notice $N lunging at you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$e notices you lunging at $m!", FALSE, vict, 0, ch, TO_VICT);
    act("$n notices $N lunging at $m!", FALSE, vict, 0, ch, TO_NOTVICT);
    hit(vict, ch, TYPE_UNDEFINED);
    return;
  }

  percent = roll_skill(ch, SKILL_BACKSTAB);

  if (AWAKE(vict) && percent)
    damage(ch, vict, 0, SKILL_BACKSTAB);
  else
    hit(ch, vict, SKILL_BACKSTAB);

  WAIT_STATE(ch, 2 * PULSE_VIOLENCE);
}
#endif


ACMD(do_order)
{
  char name[MAX_INPUT_LENGTH], message[MAX_INPUT_LENGTH];
  bool found = FALSE;
  struct char_data *vict;
  struct follow_type *k;

  half_chop(argument, name, message);

  if (!*name || !*message)
    send_to_char(ch, "Order who to do what?\r\n");
  else if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_ROOM)) && !is_abbrev(name, "followers"))
    send_to_char(ch, "That person isn't here.\r\n");
  else if (ch == vict)
    send_to_char(ch, "You obviously suffer from skitzofrenia.\r\n");
  else {
    if (AFF_FLAGGED(ch, AFF_CHARM)) {
      send_to_char(ch, "Your superior would not aprove of you giving orders.\r\n");
      return;
    }
    if (vict) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$N orders you to '%s'", message);
      act(buf, FALSE, vict, 0, ch, TO_CHAR);
      act("$n gives $N an order.", FALSE, ch, 0, vict, TO_ROOM);

      if ((vict->master != ch) || !AFF_FLAGGED(vict, AFF_CHARM))
	act("$n has an indifferent look.", FALSE, vict, 0, 0, TO_ROOM);
      else {
	send_to_char(ch, "%s", CONFIG_OK);
	command_interpreter(vict, message);
      }
    } else {			/* This is order "followers" */
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n issues the order '%s'.", message);
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      for (k = ch->followers; k; k = k->next) {
	if (IN_ROOM(ch) == IN_ROOM(k->follower))
	  if (AFF_FLAGGED(k->follower, AFF_CHARM)) {
	    found = TRUE;
	    command_interpreter(k->follower, message);
	  }
      }
      if (found)
	send_to_char(ch, "%s", CONFIG_OK);
      else
	send_to_char(ch, "Nobody here is a loyal subject of yours!\r\n");
    }
  }
}



ACMD(do_flee)
{
  int i, attempt;
  struct char_data *was_fighting;

  if (GET_POS(ch) < POS_FIGHTING) {
    send_to_char(ch, "You are in pretty bad shape, unable to flee!\r\n");
    return;
  }

  for (i = 0; i < 6; i++) {
    attempt = rand_number(0, NUM_OF_DIRS - 1);	/* Select a random direction */
    if (CAN_GO(ch, attempt) &&
	!ROOM_FLAGGED(EXIT(ch, attempt)->to_room, ROOM_DEATH)) {
      act("$n panics, and attempts to flee!", TRUE, ch, 0, 0, TO_ROOM);
      was_fighting = FIGHTING(ch);
      if (do_simple_move(ch, attempt, TRUE)) {
	send_to_char(ch, "You flee head over heels.\r\n");
	//if (was_fighting && !IS_NPC(ch)) {
	//  loss = GET_MAX_HIT(was_fighting) - GET_HIT(was_fighting);
	//  loss *= GET_LEVEL(was_fighting);
	//  gain_exp(ch, -loss);
	//}
      } else {
	act("$n tries to flee, but can't!", TRUE, ch, 0, 0, TO_ROOM);
      }
      return;
    }
  }
  send_to_char(ch, "PANIC!  You couldn't escape!\r\n");
}


#if 0 /* Not implemented yet */
ACMD(do_disarm)
{
  int roll, goal;
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *weap;
  struct char_data *vict;
  int success = FALSE;

  if (IS_NPC(ch) || !GET_SKILL(ch, SKILL_DISARM)) {
    send_to_char(ch, "You have no idea how.\r\n");
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  one_argument(argument, arg);

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    if (FIGHTING(ch) && IN_ROOM(ch) == IN_ROOM(FIGHTING(ch)))
      vict = FIGHTING(ch);
    else {
      send_to_char(ch, "Disarm who?\r\n");
      return;
    }
  }
if (vict == ch) {
    send_to_char(ch, "Try REMOVE and DROP instead...\r\n");
    return;
  }

  weap = GET_EQ(vict, WEAR_WIELD1);
  if (!weap) {
    send_to_char(ch, "But your opponent is not wielding a weapon!\r\n");
    return;
  }

  goal = GET_SKILL(ch, SKILL_DISARM) / 3;

  roll = rand_number(0, 101);
  roll -= GET_DEX(ch);   /* Improve odds */
  roll += GET_DEX(vict); /* Degrade odds */

  if (GET_LEVEL(vict) >= LVL_IMMORT) /* No disarming an immort. */
    roll = 1000;
  if (GET_LEVEL(ch) >= LVL_IMMORT)   /* But immorts never fail! */
    roll = -1000;

  if (roll <= goal) {
    success = TRUE;
    if ((weap = GET_EQ(vict, WEAR_WIELD1))) {
      act("You disarm $p from $N's hand!", FALSE, ch, weap, vict, TO_CHAR);
      act("$n disarms $p from your hand!", FALSE, ch, weap, vict, TO_VICT);
      act("$n disarms $p from $N's hand!", FALSE, ch, weap, vict, TO_NOTVICT);
      obj_to_room(unequip_char(vict, WEAR_WIELD1), IN_ROOM(vict));
    } else {
      log("SYSERR: do_disarm(), should have a weapon to be disarmed, but lost it!");
    }
  } else {
      act("You fail to disarm $N.", FALSE, ch, weap, vict, TO_CHAR);
      act("$n fails to disarm you.", FALSE, ch, weap, vict, TO_VICT);
      act("$n fails to disarm $N.", FALSE, ch, weap, vict, TO_NOTVICT);
  }

  if (!GET_LEVEL(ch) >= LVL_IMMORT)
    WAIT_STATE(ch, PULSE_VIOLENCE);

  /* Set them fighting?  Comment out if you don't like. */
  if (success && IS_NPC(vict))
    set_fighting(ch, vict);
}
#endif


ACMD(do_turn)
{
  struct char_data *vict;
  int turn_level, percent;
  int turn_difference=0, turn_result=0, turn_roll=0;
  int i;
  char buf[MAX_STRING_LENGTH];

  one_argument(argument,buf);

  if (!IS_CLERIC(ch))   {
    send_to_char(ch, "You do not possess the favor of your God.\r\n");
    return;
  }
  if (!(vict=get_char_room_vis(ch, buf, NULL)))   {
    send_to_char(ch, "Turn who?\r\n");
    return;
  }
  if (vict==ch)   {
    send_to_char(ch, "How do you plan to turn yourself?\r\n");
    return;
  }
  if (!IS_UNDEAD(vict)) {
    send_to_char(ch, "You can only attempt to turn undead!\r\n");
    return;
  }

  if (CONFIG_ALLOW_MULTICLASS) {
    turn_level = GET_TLEVEL(ch);
    for (i = 0; i < NUM_CLASSES; i++) {
      if (spell_info[ABIL_TURNING].min_level[i] <= GET_CLASS_RANKS(ch, i)) {
        if (i == CLASS_CLERIC)
          turn_level += GET_CLASS_RANKS(ch, i);
        else
          turn_level += MAX(0, GET_CLASS_RANKS(ch, i) - 2);
      }
    }
  } else {
  if (GET_CLASS(ch) != CLASS_CLERIC)
    turn_level = ((GET_LEVEL(ch) - 2) + GET_TLEVEL(ch));
  else
    turn_level = (GET_LEVEL(ch) + GET_TLEVEL(ch));
  }

  percent=roll_skill(ch, ABIL_TURNING);

  if (!percent)   {
    send_to_char(ch, "You lost your concentration!\r\n");
    return;
  }

  turn_difference = (turn_level - GET_LEVEL(vict));
  turn_roll = rand_number(1, 20);

  switch (turn_difference)   {
    case -5:
    case -4:
      if (turn_roll >= 20)
        turn_result=1;
      break;
    case -3:
      if (turn_roll >= 17)
        turn_result=1;
      break;
    case -2:
      if (turn_roll >= 15)
        turn_result=1;
      break;
    case -1:
      if (turn_roll >= 13)
        turn_result=1;
      break;
    case 0:
      if (turn_roll >= 11)
        turn_result=1;
      break;
    case 1:
      if (turn_roll >= 9)
        turn_result=1;
      break;
    case 2:
      if (turn_roll >= 6)
        turn_result=1;
      break;
    case 3:
      if (turn_roll >= 3)
        turn_result=1;
      break;
    case 4:
    case 5:
      if (turn_roll >= 2)
        turn_result=1;
      break;
    default:
      turn_result=0;
      break;
  }

  if (turn_difference <= -6)   
    turn_result=0; 
  else if (turn_difference >= 6)
    turn_result=2;

  switch (turn_result)   {
    case 0:  /* Undead resists turning */
      act("$N blasphemously mocks your faith!",FALSE,ch,0,vict,TO_CHAR);
      act("You blasphemously mock $N and $S faith!", FALSE, vict, 0, ch, TO_CHAR);
      act("$n blasphemously mocks $N and $S faith!", FALSE, vict, 0, ch, TO_NOTVICT);
      hit(vict, ch, TYPE_UNDEFINED);
      break;
    case 1:  /* Undead is turned */
      act("The power of your faith overwhelms $N, who flees!", FALSE, ch, 0, vict, TO_CHAR);
      act("The power of $N's faith overwhelms you! You flee in terror!!!", FALSE, vict, 0, ch, TO_CHAR);
      act("The power of $N's faith overwhelms $n, who flees!", FALSE, vict, 0, ch, TO_NOTVICT);
      do_flee(vict,0,0,0);
      break;
    case 2:  /* Undead is automatically destroyed */
      act("The mighty force of your faith blasts $N out of existence!", FALSE,ch,0,vict, TO_CHAR);
      act("The mighty force of $N's faith blasts you out of existence!",FALSE,vict,0,ch, TO_CHAR);
      act("The mighty force of $N's faith blasts $n out of existence!",FALSE,vict,0,ch, TO_NOTVICT);
      raw_kill(vict, ch);
      break;
  }
}

ACMD(do_lay_hands)
{
  struct char_data *vict;
  int percent, prob, healing, sacrifice;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Whom do you wish to lay hands on?\r\n");
    return;
  }

  if (!is_innate_ready(ch, ABIL_LAY_HANDS)) {
    send_to_char(ch, "You do not have the energy to lay hands on someone.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  if (vict == ch) {
    send_to_char(ch, "You can't lay hands on yourself!\r\n");
    return;
  }

  if ((!IS_NPC(ch)) && (!GET_SKILL(ch, ABIL_LAY_HANDS))) {
    send_to_char(ch, "You have no idea how to lay hands to heal someone.\r\n");
    return;
  }

  if (GET_HIT(vict) >= GET_MAX_HIT(vict)) {
    send_to_char(ch, "They are already at maximum health!\r\n");
    return;
  }

  healing = 4 * GET_LEVEL(ch);
  sacrifice = 2 * GET_LEVEL(ch);

  if (sacrifice >= GET_HIT(ch)) {
    send_to_char(ch, "The sacrifice would be too much for you to handle!\r\n");
    return;
  }

  percent = rand_number(1, 101);
  prob = GET_SKILL(ch, ABIL_LAY_HANDS);

  if (prob > percent) {
    act("You lay hands on $n.", FALSE, vict, 0, ch, TO_VICT);
    act("$N lays hands on you!", FALSE, vict, 0, ch, TO_CHAR);
    act("$N lays hands on $n.", FALSE, ch, 0, vict, TO_NOTVICT);
    GET_HIT(vict) += healing;
    GET_HIT(ch) -= sacrifice;
    WAIT_STATE(ch, PULSE_VIOLENCE);
  } else {
    send_to_char(ch, "Your attempt to lay hands fails.\r\n");
        WAIT_STATE(ch, PULSE_VIOLENCE);
  }
  /* set the timer until the skill can be used again -Cyric*/
  add_innate_timer(ch, ABIL_LAY_HANDS);
}

