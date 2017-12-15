/* ************************************************************************
*   File: act.other.c                                   Part of CircleMUD *
*  Usage: Miscellaneous player-level commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#define __ACT_OTHER_C__

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.other.c,v 1.9 2005/05/29 18:36:37 zizazat Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "house.h"
#include "constants.h"
#include "dg_scripts.h"

/* extern variables */
extern struct spell_info_type spell_info[];
extern const char *class_abbrevs[];
extern const char *race_abbrevs[];

/* extern procedures */
void list_skills(struct char_data *ch);
void appear(struct char_data *ch);
void write_aliases(struct char_data *ch);
void perform_immort_vis(struct char_data *ch);
SPECIAL(shop_keeper);
ACMD(do_gen_comm);
void die(struct char_data *ch, struct char_data * killer);
void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int level);

/* local functions */
ACMD(do_quit);
ACMD(do_save);
ACMD(do_not_here);
ACMD(do_hide);
ACMD(do_steal);
ACMD(do_practice);
ACMD(do_visible);
ACMD(do_title);
int perform_group(struct char_data *ch, struct char_data *vict);
void print_group(struct char_data *ch);
ACMD(do_group);
ACMD(do_ungroup);
ACMD(do_report);
ACMD(do_split);
ACMD(do_use);
ACMD(do_value);
ACMD(do_display);
ACMD(do_gen_write);
ACMD(do_gen_tog);
ACMD(do_file);
int spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
int spell_in_domain(struct char_data *ch, int spellnum);
ACMD(do_scribe);
ACMD(do_pagelength);

ACMD(do_quit)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  if (subcmd != SCMD_QUIT)
    send_to_char(ch, "You have to type quit--no less, to quit!\r\n");
  else if (GET_POS(ch) == POS_FIGHTING)
    send_to_char(ch, "No way!  You're fighting for your life!\r\n");
  else if (GET_POS(ch) < POS_STUNNED) {
    send_to_char(ch, "You die before your time...\r\n");
    die(ch, NULL);
  } else {
    act("$n has left the game.", TRUE, ch, 0, 0, TO_ROOM);
    mudlog(NRM, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s has quit the game.", GET_NAME(ch));
    send_to_char(ch, "Goodbye, friend.. Come back soon!\r\n");

    /*  We used to check here for duping attempts, but we may as well
     *  do it right in extract_char(), since there is no check if a
     *  player rents out and it can leave them in an equally screwy
     *  situation.
     */

    if (CONFIG_FREE_RENT)
      Crash_rentsave(ch, 0);

    /* If someone is quitting in their house, let them load back here. */
    if (!PLR_FLAGGED(ch, PLR_LOADROOM) && ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE))
      GET_LOADROOM(ch) = GET_ROOM_VNUM(IN_ROOM(ch));

    extract_char(ch);		/* Char is saved before extracting. */
  }
}



ACMD(do_save)
{
  if (IS_NPC(ch) || !ch->desc)
    return;

  /* Only tell the char we're saving if they actually typed "save" */
  if (cmd) {
    /*
     * This prevents item duplication by two PC's using coordinated saves
     * (or one PC with a house) and system crashes. Note that houses are
     * still automatically saved without this enabled. This code assumes
     * that guest immortals aren't trustworthy. If you've disabled guest
     * immortal advances from mortality, you may want < instead of <=.
     */
    if (CONFIG_AUTO_SAVE && !GET_ADMLEVEL(ch)) {
      send_to_char(ch, "Saving.\r\n");
      write_aliases(ch);
      return;
    }
    send_to_char(ch, "Saving.\r\n");
  }

  write_aliases(ch);
  save_char(ch);
  Crash_crashsave(ch);
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_HOUSE_CRASH))
    House_crashsave(GET_ROOM_VNUM(IN_ROOM(ch)));
}


/* generic function for commands which are normally overridden by
   special procedures - i.e., shop commands, mail commands, etc. */
ACMD(do_not_here)
{
  send_to_char(ch, "Sorry, but you cannot do that here!\r\n");
}


ACMD(do_steal)
{
  struct char_data *vict;
  struct obj_data *obj;
  char vict_name[MAX_INPUT_LENGTH], obj_name[MAX_INPUT_LENGTH];
  int roll, detect, diffc = 20, gold, eq_pos, pcsteal = 0, ohoh = 0;

  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return;
  }

  two_arguments(argument, obj_name, vict_name);

  /* STEALING is not an untrained skill */
  if (!GET_SKILL_BASE(ch, SKILL_SLEIGHT_OF_HAND)) {
    send_to_char(ch, "You'd be sure to be caught!\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, vict_name, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Steal what from who?\r\n");
    return;
  } else if (vict == ch) {
    send_to_char(ch, "Come on now, that's rather stupid!\r\n");
    return;
  }
  if (MOB_FLAGGED(vict, MOB_NOKILL)) {
    send_to_char(ch, "That isn't such a good idea...\r\n");
    return;
  }

  roll = roll_skill(ch, SKILL_SLEIGHT_OF_HAND);

  /* Can also add +2 synergy bonus for bluff of 5 or more */
  if (GET_SKILL(ch, SKILL_BLUFF) > 4)
    roll = roll +2;

  if (GET_POS(vict) < POS_SLEEPING)
    detect = 0;
  else
    detect = (roll_skill(vict, SKILL_SPOT) > roll);

  if (!CONFIG_PT_ALLOWED && !IS_NPC(vict))
    pcsteal = 1;

  /* NO NO With Imp's and Shopkeepers, and if player thieving is not allowed */
  if (ADM_FLAGGED(vict, ADM_NOSTEAL) || pcsteal ||
      GET_MOB_SPEC(vict) == shop_keeper)
    roll = -10;		/* Failure */

  if (str_cmp(obj_name, "coins") && str_cmp(obj_name, "gold")) {
    if (!(obj = get_obj_in_list_vis(ch, obj_name, NULL, vict->carrying))) {
      for (eq_pos = 0; eq_pos < NUM_WEARS; eq_pos++)
	if (GET_EQ(vict, eq_pos) &&
	    (isname(obj_name, GET_EQ(vict, eq_pos)->name)) &&
	    CAN_SEE_OBJ(ch, GET_EQ(vict, eq_pos))) {
	  obj = GET_EQ(vict, eq_pos);
	  break;
	}
      if (!obj) {
	act("$E hasn't got that item.", FALSE, ch, 0, vict, TO_CHAR);
	return;
      } else {			/* It is equipment */
	if ((GET_POS(vict) > POS_STUNNED)) {
	  send_to_char(ch, "Steal the equipment now?  Impossible!\r\n");
	  return;
	} else {
          if (!give_otrigger(obj, vict, ch) ||
              !receive_mtrigger(ch, vict, obj) ) {
             send_to_char(ch, "Impossible!\r\n");
             return;
           }
	  act("You unequip $p and steal it.", FALSE, ch, obj, 0, TO_CHAR);
	  act("$n steals $p from $N.", FALSE, ch, obj, vict, TO_NOTVICT);
	  obj_to_char(unequip_char(vict, eq_pos), ch);
	}
      }
    } else {			/* obj found in inventory */
      diffc += GET_OBJ_WEIGHT(obj);
      if (roll >= diffc) {	/* Steal the item */
	if (IS_CARRYING_N(ch) + 1 < CAN_CARRY_N(ch)) {
          if (!give_otrigger(obj, vict, ch) || 
              !receive_mtrigger(ch, vict, obj) ) {
            send_to_char(ch, "Impossible!\r\n");
            return;
          }
	  if (IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj) < CAN_CARRY_W(ch)) {
	    obj_from_char(obj);
	    obj_to_char(obj, ch);
	    send_to_char(ch, "Got it!\r\n");
	  }
	} else
	  send_to_char(ch, "You cannot carry that much.\r\n");
      }
      if (detect > roll) {	/* Are you noticed? */
      ohoh = TRUE;
	send_to_char(ch, "Oops..you were noticed.\r\n");
        if (roll >= diffc) {
	  act("$n has stolen $p from you!", FALSE, ch, obj, vict, TO_VICT);
	  act("$n steals $p from $N.", TRUE, ch, obj, vict, TO_NOTVICT);
    } else {
	  act("$n tried to steal something from you!", FALSE, ch, 0, vict, TO_VICT);
	  act("$n tries to steal something from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
        }
      }
    }
  } else {			/* Steal some coins */
    diffc += 5;	/* People take care of their money */
    if (roll >= diffc) {
      /* Steal some gold coins */
      gold = (GET_GOLD(vict) * rand_number(1, 10)) / 100;
      gold = MIN(1782, gold);
      if (gold > 0) {
	GET_GOLD(ch) += gold;
	GET_GOLD(vict) -= gold;
        if (gold > 1)
	  send_to_char(ch, "Bingo!  You got %d gold coins.\r\n", gold);
	else
	  send_to_char(ch, "You manage to swipe a solitary gold coin.\r\n");
      } else {
	send_to_char(ch, "You couldn't get any gold...\r\n");
      }
    }
    if (detect > roll) {
      ohoh = TRUE;
      send_to_char(ch, "Oops..\r\n");
      if (roll >= diffc) {
        act("$n has stolen your gold!", FALSE, ch, 0, vict, TO_VICT);
        act("$n steals some gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      } else {
        act("You discover that $n has $s hands in your wallet.", FALSE, ch, 0, vict, TO_VICT);
        act("$n tries to steal gold from $N.", TRUE, ch, 0, vict, TO_NOTVICT);
      }
    }
  }

  if (ohoh && IS_NPC(vict) && AWAKE(vict))
    hit(vict, ch, TYPE_UNDEFINED);
}



ACMD(do_practice)
{
  char arg[MAX_INPUT_LENGTH];

  /* if (IS_NPC(ch))
    return; */

  one_argument(argument, arg);

  if (*arg)
    send_to_char(ch, "You can only practice skills in your guild.\r\n");
  else
    list_skills(ch);
}



ACMD(do_visible)
{
  int appeared = 0;

  if (GET_ADMLEVEL(ch)) {
    perform_immort_vis(ch);
  }

  if AFF_FLAGGED(ch, AFF_INVISIBLE) {
    appear(ch);
    appeared = 1;
    send_to_char(ch, "You break the spell of invisibility.\r\n");
  }

  if (AFF_FLAGGED(ch, AFF_ETHEREAL) && affectedv_by_spell(ch, ART_EMPTY_BODY)) {
    affectv_from_char(ch, ART_EMPTY_BODY);
    if (AFF_FLAGGED(ch, AFF_ETHEREAL)) {
      send_to_char(ch, "Returning to the material plane will not be so easy.\r\n");
    } else {
      send_to_char(ch, "You return to the material plane.\r\n");
      if (!appeared)
        act("$n flashes into existence.", FALSE, ch, 0, 0, TO_ROOM);
    }
    appeared = 1;
  }

  if (!appeared)
    send_to_char(ch, "You are already visible.\r\n");
}



ACMD(do_title)
{
  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (IS_NPC(ch))
    send_to_char(ch, "Your title is fine... go away.\r\n");
  else if (PLR_FLAGGED(ch, PLR_NOTITLE))
    send_to_char(ch, "You can't title yourself -- you shouldn't have abused it!\r\n");
  else if (strstr(argument, "(") || strstr(argument, ")"))
    send_to_char(ch, "Titles can't contain the ( or ) characters.\r\n");
  else if (strlen(argument) > MAX_TITLE_LENGTH)
    send_to_char(ch, "Sorry, titles can't be longer than %d characters.\r\n", MAX_TITLE_LENGTH);
  else {
    set_title(ch, argument);
    send_to_char(ch, "Okay, you're now %s %s.\r\n", GET_NAME(ch), GET_TITLE(ch));
  }
}


int perform_group(struct char_data *ch, struct char_data *vict)
{
  if (AFF_FLAGGED(vict, AFF_GROUP) || !CAN_SEE(ch, vict))
    return (0);

  SET_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
  if (ch != vict)
    act("$N is now a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
  act("You are now a member of $n's group.", FALSE, ch, 0, vict, TO_VICT);
  act("$N is now a member of $n's group.", FALSE, ch, 0, vict, TO_NOTVICT);
  return (1);
}


void print_group(struct char_data *ch)
{
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    send_to_char(ch, "But you are not the member of a group!\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    send_to_char(ch, "Your group consists of:\r\n");

    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP)) {
      snprintf(buf, sizeof(buf), "     [%3dH %3dM %3dV %3dK] [%2d %s %s] $N (Head of group)",
	      GET_HIT(k), GET_MANA(k), GET_MOVE(k), GET_KI(k), GET_LEVEL(k),
              CLASS_ABBR(k), RACE_ABBR(k));
      act(buf, FALSE, ch, 0, k, TO_CHAR);
    }

    for (f = k->followers; f; f = f->next) {
      if (!AFF_FLAGGED(f->follower, AFF_GROUP))
	continue;

      snprintf(buf, sizeof(buf), "     [%3dH %3dM %3dV %3dK] [%2d %s %s] $N",
              GET_HIT(f->follower), GET_MANA(f->follower), GET_MOVE(f->follower),
	      GET_KI(f->follower), GET_LEVEL(f->follower), CLASS_ABBR(f->follower),
              RACE_ABBR(f->follower));
      act(buf, FALSE, ch, 0, f->follower, TO_CHAR);
    }
  }
}



ACMD(do_group)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *vict;
  struct follow_type *f;
  int found;

  one_argument(argument, buf);

  if (!*buf) {
    print_group(ch);
    return;
  }

  if (ch->master) {
    act("You can not enroll group members without being head of a group.",
	FALSE, ch, 0, 0, TO_CHAR);
    return;
  }

  if (!str_cmp(buf, "all")) {
    perform_group(ch, ch);
    for (found = 0, f = ch->followers; f; f = f->next)
      found += perform_group(ch, f->follower);
    if (!found)
      send_to_char(ch, "Everyone following you is already in your group.\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if ((vict->master != ch) && (vict != ch))
    act("$N must follow you to enter your group.", FALSE, ch, 0, vict, TO_CHAR);
  else {
    if (!AFF_FLAGGED(vict, AFF_GROUP))
      perform_group(ch, vict);
    else {
      if (ch != vict)
	act("$N is no longer a member of your group.", FALSE, ch, 0, vict, TO_CHAR);
      act("You have been kicked out of $n's group!", FALSE, ch, 0, vict, TO_VICT);
      act("$N has been kicked out of $n's group!", FALSE, ch, 0, vict, TO_NOTVICT);
      REMOVE_BIT_AR(AFF_FLAGS(vict), AFF_GROUP);
    }
  }
}



ACMD(do_ungroup)
{
  char buf[MAX_INPUT_LENGTH];
  struct follow_type *f, *next_fol;
  struct char_data *tch;

  one_argument(argument, buf);

  if (!*buf) {
    if (ch->master || !(AFF_FLAGGED(ch, AFF_GROUP))) {
      send_to_char(ch, "But you lead no group!\r\n");
      return;
    }

    for (f = ch->followers; f; f = next_fol) {
      next_fol = f->next;
      if (AFF_FLAGGED(f->follower, AFF_GROUP)) {
	REMOVE_BIT_AR(AFF_FLAGS(f->follower), AFF_GROUP);
        act("$N has disbanded the group.", TRUE, f->follower, NULL, ch, TO_CHAR);
        if (!AFF_FLAGGED(f->follower, AFF_CHARM))
	  stop_follower(f->follower);
      }
    }

    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_GROUP);
    send_to_char(ch, "You disband the group.\r\n");
    return;
  }
  if (!(tch = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "There is no such person!\r\n");
    return;
  }
  if (tch->master != ch) {
    send_to_char(ch, "That person is not following you!\r\n");
    return;
  }

  if (!AFF_FLAGGED(tch, AFF_GROUP)) {
    send_to_char(ch, "That person isn't in your group.\r\n");
    return;
  }

  REMOVE_BIT_AR(AFF_FLAGS(tch), AFF_GROUP);

  act("$N is no longer a member of your group.", FALSE, ch, 0, tch, TO_CHAR);
  act("You have been kicked out of $n's group!", FALSE, ch, 0, tch, TO_VICT);
  act("$N has been kicked out of $n's group!", FALSE, ch, 0, tch, TO_NOTVICT);
 
  if (!AFF_FLAGGED(tch, AFF_CHARM))
    stop_follower(tch);
}




ACMD(do_report)
{
  char buf[MAX_STRING_LENGTH];
  struct char_data *k;
  struct follow_type *f;

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not a member of any group!\r\n");
    return;
  }

  snprintf(buf, sizeof(buf), "$n reports: %d/%dH, %d/%dM, %d/%dV %d/%dK\r\n",
	  GET_HIT(ch), GET_MAX_HIT(ch),
	  GET_MANA(ch), GET_MAX_MANA(ch),
	  GET_MOVE(ch), GET_MAX_MOVE(ch),
	  GET_KI(ch), GET_MAX_KI(ch));

  k = (ch->master ? ch->master : ch);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && f->follower != ch)
      act(buf, TRUE, ch, NULL, f->follower, TO_VICT);

  if (k != ch)
    act(buf, TRUE, ch, NULL, k, TO_VICT);

  send_to_char(ch, "You report to the group.\r\n");
}



ACMD(do_split)
{
  char buf[MAX_INPUT_LENGTH];
  int amount, num, share, rest;
  size_t len;
  struct char_data *k;
  struct follow_type *f;

  if (IS_NPC(ch))
    return;

  one_argument(argument, buf);

  if (is_number(buf)) {
    amount = atoi(buf);
    if (amount <= 0) {
      send_to_char(ch, "Sorry, you can't do that.\r\n");
      return;
    }
    if (amount > GET_GOLD(ch)) {
      send_to_char(ch, "You don't seem to have that much gold to split.\r\n");
      return;
    }
    k = (ch->master ? ch->master : ch);

    if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch)))
      num = 1;
    else
      num = 0;

    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)))
	num++;

    if (num && AFF_FLAGGED(ch, AFF_GROUP)) {
      share = amount / num;
      rest = amount % num;
    } else {
      send_to_char(ch, "With whom do you wish to share your gold?\r\n");
      return;
    }

    GET_GOLD(ch) -= share * (num - 1);

    /* Abusing signed/unsigned to make sizeof work. */
    len = snprintf(buf, sizeof(buf), "%s splits %d coins; you receive %d.\r\n",
		GET_NAME(ch), amount, share);
    if (rest && len < sizeof(buf)) {
      snprintf(buf + len, sizeof(buf) - len,
		"%d coin%s %s not splitable, so %s keeps the money.\r\n", rest,
		(rest == 1) ? "" : "s", (rest == 1) ? "was" : "were", GET_NAME(ch));
    }
    if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch) &&
		!IS_NPC(k) && k != ch) {
      GET_GOLD(k) += share;
      send_to_char(k, "%s", buf);
    }

    for (f = k->followers; f; f = f->next) {
      if (AFF_FLAGGED(f->follower, AFF_GROUP) &&
	  (!IS_NPC(f->follower)) &&
	  (IN_ROOM(f->follower) == IN_ROOM(ch)) &&
	  f->follower != ch) {

	GET_GOLD(f->follower) += share;
	send_to_char(f->follower, "%s", buf);
      }
    }
    send_to_char(ch, "You split %d coins among %d members -- %d coins each.\r\n",
	    amount, num, share);

    if (rest) {
      send_to_char(ch, "%d coin%s %s not splitable, so you keep the money.\r\n",
		rest, (rest == 1) ? "" : "s", (rest == 1) ? "was" : "were");
      GET_GOLD(ch) += rest;
    }
  } else {
    send_to_char(ch, "How many coins do you wish to split with your group?\r\n");
    return;
  }
}



ACMD(do_use)
{
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];
  struct obj_data *mag_item;

  half_chop(argument, arg, buf);
  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", CMD_NAME);
    return;
  }

  mag_item = GET_EQ(ch, WEAR_WIELD2);

  if (!mag_item || !isname(arg, mag_item->name)) {
    switch (subcmd) {
    case SCMD_RECITE:
    case SCMD_QUAFF:
      if (!(mag_item = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
	return;
      }
      break;
    case SCMD_USE:
      send_to_char(ch, "You don't seem to be holding %s %s.\r\n", AN(arg), arg);
      return;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_use.", subcmd);
      return;
    }
  }
  switch (subcmd) {
  case SCMD_QUAFF:
    if (GET_OBJ_TYPE(mag_item) != ITEM_POTION) {
      send_to_char(ch, "You can only quaff potions.\r\n");
      return;
    }
    break;
  case SCMD_RECITE:
    if (GET_OBJ_TYPE(mag_item) != ITEM_SCROLL) {
      send_to_char(ch, "You can only recite scrolls.\r\n");
      return;
    }
    break;
  case SCMD_USE:
    if ((GET_OBJ_TYPE(mag_item) != ITEM_WAND) &&
	(GET_OBJ_TYPE(mag_item) != ITEM_STAFF)) {
      send_to_char(ch, "You can't seem to figure out how to use it.\r\n");
      return;
    }
    break;
  }

  mag_objectmagic(ch, mag_item, buf);
}



ACMD(do_value)
{
  char arg[MAX_INPUT_LENGTH];
  int value_lev;

  one_argument(argument, arg);

  if (!*arg) {
    switch (subcmd) {
    case SCMD_WIMPY:
        if (GET_WIMP_LEV(ch)) {
          send_to_char(ch, "Your current wimp level is %d hit points.\r\n", GET_WIMP_LEV(ch));
          return;
        } else {
          send_to_char(ch, "At the moment, you're not a wimp.  (sure, sure...)\r\n");
          return;
        }
      break;
    case SCMD_POWERATT:
        if (GET_POWERATTACK(ch)) {
          send_to_char(ch, "Your current power attack level -%d accuracy +%ddamage.\r\n",
                       GET_POWERATTACK(ch), GET_POWERATTACK(ch));
          return;
        } else {
          send_to_char(ch, "You are not currently using power attack.\r\n");
          return;
        }
      break;
    }
  }

  if (isdigit(*arg)) {
    switch (subcmd) {
    case SCMD_WIMPY:
      /* 'wimp_level' is a player_special. -gg 2/25/98 */
      if (IS_NPC(ch))
        return;
      if ((value_lev = atoi(arg)) != 0) {
        if (value_lev < 0)
	  send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (value_lev > GET_MAX_HIT(ch))
	  send_to_char(ch, "That doesn't make much sense, now does it?\r\n");
        else if (value_lev > (GET_MAX_HIT(ch) / 2))
	  send_to_char(ch, "You can't set your wimp level above half your hit points.\r\n");
        else {
	  send_to_char(ch, "Okay, you'll wimp out if you drop below %d hit points.\r\n", value_lev);
	  GET_WIMP_LEV(ch) = value_lev;
        }
      } else {
        send_to_char(ch, "Okay, you'll now tough out fights to the bitter end.\r\n");
        GET_WIMP_LEV(ch) = 0;
      }
      break;
    case SCMD_POWERATT:
      if ((value_lev = atoi(arg)) != 0) {
        if (value_lev < 0)
	  send_to_char(ch, "Heh, heh, heh.. we are jolly funny today, eh?\r\n");
        else if (value_lev > GET_ACCURACY_BASE(ch))
	  send_to_char(ch, "You may only specify a value up to %d.\r\n", GET_ACCURACY_BASE(ch));
        else {
	  send_to_char(ch, "Okay, you'll sacrifice %d points of accuracy for damage.\r\n", value_lev);
	  GET_POWERATTACK(ch) = value_lev;
        }
      } else {
        send_to_char(ch, "Okay, you will no longer use power attack.\r\n");
        GET_POWERATTACK(ch) = 0;
      }
      break;
    default:
      log("Unknown subcmd to do_value %d called by %s", subcmd, GET_NAME(ch));
      break;
    }
  } else
    send_to_char(ch, "Specify a value.  (0 to disable)\r\n");
}


ACMD(do_display)
{
  size_t i;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Mosters don't need displays.  Go away.\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Usage: prompt { { H | M | V | K } | all | auto | none }\r\n");
    return;
  }

  if (!str_cmp(argument, "auto")) {
    TOGGLE_BIT_AR(PRF_FLAGS(ch), PRF_DISPAUTO);
    send_to_char(ch, "Auto prompt %sabled.\r\n", PRF_FLAGGED(ch, PRF_DISPAUTO) ? "en" : "dis");
    return;
  }

  if (!str_cmp(argument, "on") || !str_cmp(argument, "all")) {
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
  } else if (!str_cmp(argument, "off") || !str_cmp(argument, "none")) {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
  } else {
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
    REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);

    for (i = 0; i < strlen(argument); i++) {
      switch (LOWER(argument[i])) {
      case 'h':
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPHP);
	break;
      case 'm':
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMANA);
	break;
      case 'v':
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPMOVE);
	break;
      case 'k':
	SET_BIT_AR(PRF_FLAGS(ch), PRF_DISPKI);
	break;
      default:
	send_to_char(ch, "Usage: prompt { { H | M | V | K } | all | auto | none }\r\n");
	return;
      }
    }
  }

  send_to_char(ch, "%s", CONFIG_OK);
}



ACMD(do_gen_write)
{
  FILE *fl;
  char *tmp;
  const char *filename;
  struct stat fbuf;
  time_t ct;

  switch (subcmd) {
  case SCMD_BUG:
    filename = BUG_FILE;
    break;
  case SCMD_TYPO:
    filename = TYPO_FILE;
    break;
  case SCMD_IDEA:
    filename = IDEA_FILE;
    break;
  default:
    return;
  }

  ct = time(0);
  tmp = asctime(localtime(&ct));

  if (IS_NPC(ch)) {
    send_to_char(ch, "Monsters can't have ideas - Go away.\r\n");
    return;
  }

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "That must be a mistake...\r\n");
    return;
  }
  mudlog(CMP, ADMLVL_IMMORT, FALSE, "%s %s: %s", GET_NAME(ch), CMD_NAME, argument);

  if (stat(filename, &fbuf) < 0) {
    perror("SYSERR: Can't stat() file");
    return;
  }
  if (fbuf.st_size >= CONFIG_MAX_FILESIZE) {
    send_to_char(ch, "Sorry, the file is full right now.. try again later.\r\n");
    return;
  }
  if (!(fl = fopen(filename, "a"))) {
    perror("SYSERR: do_gen_write");
    send_to_char(ch, "Could not open the file.  Sorry.\r\n");
    return;
  }
  fprintf(fl, "%-8s (%6.6s) [%5d] %s\n", GET_NAME(ch), (tmp + 4),
	  GET_ROOM_VNUM(IN_ROOM(ch)), argument);
  fclose(fl);
  send_to_char(ch, "Okay.  Thanks!\r\n");
}



#define TOG_OFF 0
#define TOG_ON  1

ACMD(do_gen_tog)
{
  long result;

  const char *tog_messages[][2] = {
    {"You are now safe from summoning by other players.\r\n",
    "You may now be summoned by other players.\r\n"},
    {"Nohassle disabled.\r\n",
    "Nohassle enabled.\r\n"},
    {"Brief mode off.\r\n",
    "Brief mode on.\r\n"},
    {"Compact mode off.\r\n",
    "Compact mode on.\r\n"},
    {"You can now hear tells.\r\n",
    "You are now deaf to tells.\r\n"},
    {"You can now hear auctions.\r\n",
    "You are now deaf to auctions.\r\n"},
    {"You can now hear shouts.\r\n",
    "You are now deaf to shouts.\r\n"},
    {"You can now hear gossip.\r\n",
    "You are now deaf to gossip.\r\n"},
    {"You can now hear the congratulation messages.\r\n",
    "You are now deaf to the congratulation messages.\r\n"},
    {"You can now hear the Wiz-channel.\r\n",
    "You are now deaf to the Wiz-channel.\r\n"},
    {"You are no longer part of the Quest.\r\n",
    "Okay, you are part of the Quest!\r\n"},
    {"You will no longer see the room flags.\r\n",
    "You will now see the room flags.\r\n"},
    {"You will now have your communication repeated.\r\n",
    "You will no longer have your communication repeated.\r\n"},
    {"HolyLight mode off.\r\n",
    "HolyLight mode on.\r\n"},
    {"Nameserver_is_slow changed to NO; IP addresses will now be resolved.\r\n",
    "Nameserver_is_slow changed to YES; sitenames will no longer be resolved.\r\n"},
    {"Autoexits disabled.\r\n",
    "Autoexits enabled.\r\n"},
    {"Will no longer track through doors.\r\n",
    "Will now track through doors.\r\n"},
    {"Buildwalk Off.\r\n",
    "Buildwalk On.\r\n"},
    {"AFK flag is now off.\r\n",
    "AFK flag is now on.\r\n"},
    {"You will no longer Auto-Assist.\r\n",
     "You will now Auto-Assist.\r\n"},
    {"Autoloot disabled.\r\n",
    "Autoloot enabled.\r\n"},
    {"Autogold disabled.\r\n",
    "Autogold enabled.\r\n"},
    {"Will no longer clear screen in OLC.\r\n",
    "Will now clear screen in OLC.\r\n"},
    {"Autosplit disabled.\r\n",
    "Autosplit enabled.\r\n"},
    {"Autosac disabled.\r\n",
    "Autosac enabled.\r\n"},
    {"You will no longer attempt to be sneaky.\r\n",
    "You will try to move as silently as you can.\r\n"},
    {"You will no longer attempt to stay hidden.\r\n",
    "You will try to stay hidden.\r\n"},
    {"You will no longer automatically memorize spells in your list.\r\n",
    "You will automatically memorize spells in your list.\r\n"},
    {"Viewing newest board messages first.\r\n",
      "Viewing eldest board messages first.  Wierdo.\r\n"},
    {"Compression will be used if your client supports it.\r\n",
     "Compression will not be used even if your client supports it.\r\n"}
  };


  if (IS_NPC(ch))
    return;

  switch (subcmd) {
  case SCMD_NOSUMMON:
    result = PRF_TOG_CHK(ch, PRF_SUMMONABLE);
    break;
  case SCMD_NOHASSLE:
    result = PRF_TOG_CHK(ch, PRF_NOHASSLE);
    break;
  case SCMD_BRIEF:
    result = PRF_TOG_CHK(ch, PRF_BRIEF);
    break;
  case SCMD_COMPACT:
    result = PRF_TOG_CHK(ch, PRF_COMPACT);
    break;
  case SCMD_NOTELL:
    result = PRF_TOG_CHK(ch, PRF_NOTELL);
    break;
  case SCMD_NOAUCTION:
    result = PRF_TOG_CHK(ch, PRF_NOAUCT);
    break;
  case SCMD_DEAF:
    result = PRF_TOG_CHK(ch, PRF_DEAF);
    break;
  case SCMD_NOGOSSIP:
    result = PRF_TOG_CHK(ch, PRF_NOGOSS);
    break;
  case SCMD_NOGRATZ:
    result = PRF_TOG_CHK(ch, PRF_NOGRATZ);
    break;
  case SCMD_NOWIZ:
    result = PRF_TOG_CHK(ch, PRF_NOWIZ);
    break;
  case SCMD_QUEST:
    result = PRF_TOG_CHK(ch, PRF_QUEST);
    break;
  case SCMD_ROOMFLAGS:
    result = PRF_TOG_CHK(ch, PRF_ROOMFLAGS);
    break;
  case SCMD_NOREPEAT:
    result = PRF_TOG_CHK(ch, PRF_NOREPEAT);
    break;
  case SCMD_HOLYLIGHT:
    result = PRF_TOG_CHK(ch, PRF_HOLYLIGHT);
    break;
  case SCMD_SLOWNS:
    result = (CONFIG_NS_IS_SLOW = !CONFIG_NS_IS_SLOW);
    break;
  case SCMD_AUTOEXIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOEXIT);
    break;
  case SCMD_TRACK:
    result = (CONFIG_TRACK_T_DOORS = !CONFIG_TRACK_T_DOORS);
    break;
  case SCMD_AFK:
    result = PRF_TOG_CHK(ch, PRF_AFK);
    if (PRF_FLAGGED(ch, PRF_AFK))
      act("$n has gone AFK.", TRUE, ch, 0, 0, TO_ROOM);
    else
      act("$n has come back from AFK.", TRUE, ch, 0, 0, TO_ROOM);
    break;
  case SCMD_AUTOLOOT:
    result = PRF_TOG_CHK(ch, PRF_AUTOLOOT);
    break;
  case SCMD_AUTOGOLD:
    result = PRF_TOG_CHK(ch, PRF_AUTOGOLD);
    break;
  case SCMD_CLS:
    result = PRF_TOG_CHK(ch, PRF_CLS);
    break;
  case SCMD_BUILDWALK:
    if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER) {
      send_to_char(ch, "Builders only, sorry.\r\n");  	
      return;
    }
    result = PRF_TOG_CHK(ch, PRF_BUILDWALK);
    if (PRF_FLAGGED(ch, PRF_BUILDWALK))
      mudlog(CMP, GET_LEVEL(ch), TRUE, 
             "OLC: %s turned buildwalk on. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    else
      mudlog(CMP, GET_LEVEL(ch), TRUE,
             "OLC: %s turned buildwalk off. Allowed zone %d", GET_NAME(ch), GET_OLC_ZONE(ch));
    break;
  case SCMD_AUTOSPLIT:
    result = PRF_TOG_CHK(ch, PRF_AUTOSPLIT);
    break;
  case SCMD_AUTOSAC: 
    result = PRF_TOG_CHK(ch, PRF_AUTOSAC); 
    break; 
  case SCMD_SNEAK:
    result = AFF_TOG_CHK(ch, AFF_SNEAK);
    break;
  case SCMD_HIDE:
    result = AFF_TOG_CHK(ch, AFF_HIDE);
    break;
  case SCMD_AUTOMEM: 
    result = PRF_TOG_CHK(ch, PRF_AUTOMEM); 
    break; 
  case SCMD_VIEWORDER:
    result = PRF_TOG_CHK(ch, PRF_VIEWORDER);
    break;
  case SCMD_NOCOMPRESS:
    if (CONFIG_ENABLE_COMPRESSION) {
      result = PRF_TOG_CHK(ch, PRF_NOCOMPRESS);
      break;
    } else {
      send_to_char(ch, "Sorry, compression is globally disabled.\r\n");
    }
  case SCMD_AUTOASSIST:
    result = PRF_TOG_CHK(ch, PRF_AUTOASSIST);
    break;
  default:
    log("SYSERR: Unknown subcmd %d in do_gen_toggle.", subcmd);
    return;
  }

  if (result)
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_ON]);
  else
    send_to_char(ch, "%s", tog_messages[subcmd][TOG_OFF]);

  return;
}

ACMD(do_file)
{
  FILE *req_file;
  int cur_line = 0,
  num_lines = 0,
  req_lines = 0,
  i,
  j;
  int l;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], line[READ_SIZE];
  char buf[MAX_STRING_LENGTH];

  struct file_struct {
    char *cmd;
    char level;
    char *file;
 } fields[] = {
     { "none",           ADMLVL_IMPL,    "Does Nothing" },
     { "bug",            ADMLVL_IMPL,    "../lib/misc/bugs"},
     { "typo",           ADMLVL_IMPL,   "../lib/misc/typos"},
     { "ideas",          ADMLVL_IMPL,    "../lib/misc/ideas"},
     { "xnames",         ADMLVL_IMPL,     "../lib/misc/xnames"},
     { "levels",         ADMLVL_IMPL,    "../log/levels" },
     { "rip",            ADMLVL_IMPL,    "../log/rip" },
     { "players",        ADMLVL_IMPL,    "../log/newplayers" },
     { "rentgone",       ADMLVL_IMPL,    "../log/rentgone" },
     { "errors",         ADMLVL_IMPL,    "../log/errors" },
     { "godcmds",        ADMLVL_IMPL,    "../log/godcmds" },
     { "syslog",         ADMLVL_IMPL,    "../syslog" },
     { "crash",          ADMLVL_IMPL,    "../syslog.CRASH" },
     { "\n", 0, "\n" }
};

   skip_spaces(&argument);

   if (!*argument) {
     strcpy(buf, "USAGE: file <option> <num lines>\r\n\r\nFile options:\r\n");
     for (j = 0, i = 1; fields[i].level; i++)
       if (fields[i].level <= GET_LEVEL(ch))
         sprintf(buf+strlen(buf), "%-15s%s\r\n", fields[i].cmd, fields[i].file);
     send_to_char(ch, buf);
     return;
   }

   two_arguments(argument, field, value);

   for (l = 0; *(fields[l].cmd) != '\n'; l++)
     if (!strncmp(field, fields[l].cmd, strlen(field)))
     break;

   if(*(fields[l].cmd) == '\n') {
     send_to_char(ch, "That is not a valid option!\r\n");
     return;
   }

   if (GET_ADMLEVEL(ch) < fields[l].level) {
     send_to_char(ch, "You are not godly enough to view that file!\r\n");
     return;
   }

   if(!*value)
     req_lines = 15; /* default is the last 15 lines */
   else
     req_lines = atoi(value);
   
   if (!(req_file=fopen(fields[l].file,"r"))) {
     mudlog(BRF, ADMLVL_IMPL, TRUE,
            "SYSERR: Error opening file %s using 'file' command.",
            fields[l].file);
     return;
   }

   get_line(req_file,line);
   while (!feof(req_file)) {
     num_lines++;
     get_line(req_file,line);
   }
   rewind(req_file);

   req_lines = MIN(MIN(req_lines, num_lines),150);
   
   buf[0] = '\0';

   get_line(req_file,line);
   while (!feof(req_file)) {
     cur_line++;
     if(cur_line > (num_lines - req_lines)) 
       sprintf(buf+strlen(buf),"%s\r\n", line);
	   
     get_line(req_file,line);
   }
   fclose(req_file);

   page_string(ch->desc, buf, 1);

}

ACMD(do_compare)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj1, *obj2;
  struct char_data *tchar;
  int value1 = 0, value2 = 0, o1, o2;
  char *msg = NULL;

  two_arguments(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Compare what to what?\n\r");
    return;
  }

  o1 = generic_find(arg1, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj1);
  o2 = generic_find(arg2, FIND_OBJ_INV| FIND_OBJ_EQUIP, ch, &tchar, &obj2);

  if (!o1 || !o2) {
    send_to_char(ch, "You do not have that item.\r\n");
    return;
  }
  if ( obj1 == obj2 ) {
    msg = "You compare $p to itself.  It looks about the same.";
  } else if (GET_OBJ_TYPE(obj1) != GET_OBJ_TYPE(obj2)) {
    msg = "You can't compare $p and $P.";
  } else {
    switch ( GET_OBJ_TYPE(obj1) ) {
      default:
      msg = "You can't compare $p and $P.";
      break;
      case ITEM_ARMOR:
      value1 = GET_OBJ_VAL(obj1, VAL_ARMOR_APPLYAC);
      value2 = GET_OBJ_VAL(obj2, VAL_ARMOR_APPLYAC);
      break;
      case ITEM_WEAPON:
      value1 = (1 + GET_OBJ_VAL(obj1, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj1, VAL_WEAPON_DAMDICE);
      value2 = (1 + GET_OBJ_VAL(obj2, VAL_WEAPON_DAMSIZE)) * GET_OBJ_VAL(obj2, VAL_WEAPON_DAMDICE);
      break;
    }
  }

  if ( msg == NULL ) {
    if ( value1 == value2 )
      msg = "$p and $P look about the same.";
    else if ( value1  > value2 )
      msg = "$p looks better than $P.";
    else
      msg = "$p looks worse than $P.";
  }

  act( msg, FALSE, ch, obj1, obj2, TO_CHAR );
  return;
}

ACMD(do_break)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int brk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you break SOMETHING.\r\n");
    return;
  }

  if (!(brk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to break!\r\n");
    return;
  }


  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, "Seems like it's already broken!\r\n");
    return;
  }

  /* Ok, break it! */
  send_to_char(ch, "You ruin %s.\r\n", obj->short_description);
  act("$n ruins $p.", FALSE, ch, obj, 0, TO_ROOM);
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = -1;
  TOGGLE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);

  return;
}

ACMD(do_fix)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;
  struct char_data *dummy = NULL;
  int brk;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Usually you fix SOMETHING.\r\n");
    return;
  }

  if (!(brk = generic_find(arg, FIND_OBJ_INV|FIND_OBJ_EQUIP, ch, &dummy, &obj))) { 
    send_to_char(ch, "Can't seem to find what you want to fix!\r\n");
    return;
  }

  if ((brk) && !OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, "But it isn't even broken!\r\n");
    return;
  }

  send_to_char(ch, "You repair %s.\r\n", obj->short_description);
  act("$n repair $p.", FALSE, ch, obj, 0, TO_ROOM);
  GET_OBJ_VAL(obj, VAL_ALL_HEALTH) = GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
  TOGGLE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BROKEN);

  return;
}

/* new spell memorization code */
/* remove a spell from a character's innate linked list */
void innate_remove(struct char_data * ch, struct innate_node * inn)
{
  struct innate_node *temp;

  if (ch->innate == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(inn, ch->innate, next);
  free(inn);
}

void innate_add(struct char_data * ch, int innate, int timer)
{
  struct innate_node * inn;

  CREATE(inn, struct innate_node, 1);
  inn->timer = timer;
  inn->spellnum = innate;
  inn->next = ch->innate;
  ch->innate = inn;
}

/* Returns true if the spell/skillnum is innate to the character
   (as opposed to just practiced).
 */
int is_innate(struct char_data *ch, int spellnum) 
{
  switch(spellnum) {
  case SPELL_FAERIE_FIRE:
    if(GET_RACE(ch) == RACE_DROW_ELF)
      return TRUE;
    break;
  }
  return FALSE;
}

/* returns FALSE if the spell is found in the innate linked list. 
  This just means the ability is ready IF they can even get the
  ability. (see is_innate). 
*/
int is_innate_ready(struct char_data *ch, int spellnum)
{
  struct innate_node *inn, *next_inn;

  for(inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if(inn->spellnum == spellnum)
      return FALSE;
  }
  return TRUE;
}

/* Adds a node to the linked list with a timer which indicates
   that the innate skill/spell is NOT ready to be used.

   is_innate_ready(...) should be called first to determine if
   there is already a timer set for the spellnum.
*/
void add_innate_timer(struct char_data *ch, int spellnum) 
{
  int timer = 6; /* number of ticks */

  switch(spellnum) {
    case SPELL_FAERIE_FIRE:
      timer = 6;   
      break;
    case ABIL_LAY_HANDS:
      timer = 12;
      break;
  }
  if(is_innate_ready(ch, spellnum)) {
    innate_add(ch, spellnum, timer);
  } else {
    send_to_char(ch, "BUG!\r\n");
  }
}


/* called when a player enters the game to set permanent affects.
   Usually these will stay set, but certain circumstances will
   cause them to wear off (ie. removing eq with a perm affect).
*/
void add_innate_affects(struct char_data *ch) 
{
  switch(GET_RACE(ch)) {
  case RACE_ELF:
  case RACE_DROW_ELF:
  case RACE_DWARF:
  case RACE_HALF_ELF:
  case RACE_MINDFLAYER:
  case RACE_HALFLING:
    affect_modify(ch, APPLY_NONE, 0, 0, AFF_INFRAVISION, TRUE);
    break;
  }
  affect_total(ch);
}

/* Called to update the innate timers */
void update_innate(struct char_data *ch)
{
  struct innate_node *inn, *next_inn;

  for (inn = ch->innate; inn; inn = next_inn) {
    next_inn = inn->next;
    if (inn->timer > 0) { 
      inn->timer--;
    } else {
      switch(inn->spellnum) {
      case ABIL_LAY_HANDS:
        send_to_char(ch, "Your special healing abilities have returned.\r\n");
        break;
      default:
        send_to_char(ch, "You are now able to use your innate %s again.\r\n", spell_info[inn->spellnum].name);
        break;
      }  
      innate_remove(ch, inn);
    }
  } 
}

int spell_in_book(struct obj_data *obj, int spellnum)
{
  int i;
  bool found = FALSE;

  if (!obj->sbinfo)
    return FALSE;

  for (i=0; i < SPELLBOOK_SIZE; i++)
    if (obj->sbinfo[i].spellname == spellnum) {
      found = TRUE;
      break;
    }

  if (found)
    return 1;

  return 0;
}

int spell_in_scroll(struct obj_data *obj, int spellnum)
{
  if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) == spellnum)
    return TRUE;

  return FALSE;
}

int spell_in_domain(struct char_data *ch, int spellnum)
{
  if (spell_info[spellnum].domain == DOMAIN_UNDEFINED) {
    return FALSE;
  }

  return TRUE;
}


room_vnum freeres[] = {
/* LAWFUL_GOOD */	1000,
/* NEUTRAL_GOOD */	1000,
/* CHAOTIC_GOOD */	1000,
/* LAWFUL_NEUTRAL */	1000,
/* NEUTRAL_NEUTRAL */	1000,
/* CHAOTIC_NEUTRAL */	1000,
/* LAWFUL_EVIL */	1000,
/* NEUTRAL_EVIL */	1000,
/* CHAOTIC_EVIL */	1000
};


ACMD(do_resurrect)
{
  room_rnum rm;
  struct affected_type *af, *next_af;

  if (IS_NPC(ch)) {
    send_to_char(ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!AFF_FLAGGED(ch, AFF_SPIRIT)) {
    send_to_char(ch, "But you're not even dead!\r\n");
    return;
  }

  send_to_char(ch, "You take an experience penalty and pray for charity resurrection.\r\n");
  gain_exp(ch, -(level_exp(GET_LEVEL(ch)) - level_exp(GET_LEVEL(ch) - 1)));

  for (af = ch->affected; af; af = next_af) {
    next_af = af->next;
    if (af->location == APPLY_NONE && af->type == -1 &&
        (af->bitvector == AFF_SPIRIT || af->bitvector == AFF_ETHEREAL))
      affect_remove(ch, af);
  }

  if (GET_HIT(ch) < 1)
    GET_HIT(ch) = 1;

  if ((rm = real_room(freeres[ALIGN_TYPE(ch)])) == NOWHERE)
    rm = real_room(CONFIG_MORTAL_START);

  if (rm != NOWHERE) {
    char_from_room(ch);
    char_to_room(ch, rm);
    look_at_room(IN_ROOM(ch), ch, 0);
  }

  act("$n's body forms in a pool of @Bblue light@n.", TRUE, ch, 0, 0, TO_ROOM);
}

ACMD(do_pagelength)
{
  char arg[MAX_INPUT_LENGTH];

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "You current page length is set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else if (is_number(arg)) {
    GET_PAGE_LENGTH(ch) = MIN(MAX(atoi(arg), 5), 255);
    send_to_char(ch, "Okay, your page length is now set to %d lines.\r\n",
                 GET_PAGE_LENGTH(ch));
  } else {
    send_to_char(ch, "Please specify a number of lines (5 - 255).\r\n");
  }
}

