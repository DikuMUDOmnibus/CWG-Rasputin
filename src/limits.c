/* ************************************************************************
*   File: limits.c                                      Part of CircleMUD *
*  Usage: limits & gain funcs for HMV, exp, hunger/thirst, idle time      *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/limits.c,v 1.12 2005/01/03 06:44:38 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "spells.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "constants.h"

/* local functions */
void run_autowiz(void);

void Crash_rentsave(struct char_data *ch, int cost);
int level_exp(int level);
void update_char_objects(struct char_data *ch);	/* handler.c */
int calc_panalty_exp(struct char_data *ch, int gain);
void update_innate(struct char_data *ch);
void reboot_wizlists(void);
void death_cry(struct char_data *ch);

/* extern vars */
extern const char *class_names[];


/*
 * The hit_limit, mana_limit, and move_limit functions are gone.  They
 * added an unnecessary level of complexity to the internal structure,
 * weren't particularly useful, and led to some annoying bugs.  From the
 * players' point of view, the only difference the removal of these
 * functions will make is that a character's age will now only affect
 * the HMV gain per tick, and _not_ the HMV maximums.
 */

/* manapoint gain pr. game hour */
int mana_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = 12;

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);
      break;
    case POS_SITTING:
      gain += (gain / 4);
      break;
    }

    if (IS_WIZARD(ch) || IS_CLERIC(ch))
      gain *= 2;

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}


/* Hitpoint gain pr. game hour */
int hit_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {

    gain = 12;

    /* Class/Level calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */

    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);
      break;
    case POS_SITTING:
      gain += (gain / 4);
      break;
    }

    if (IS_WIZARD(ch) || IS_CLERIC(ch))
      gain /= 2;	/* Ouch. */

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}



/* move gain pr. game hour */
int move_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = 24;

    /* Class/Level calculations */

    /* Skill/Spell calculations */


    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);
      break;
    case POS_SITTING:
      gain += (gain / 4);
      break;
    }

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}


/* ki gain pr. game hour */
int ki_gain(struct char_data *ch)
{
  int gain;

  if (IS_NPC(ch)) {
    /* Neat and fast */
    gain = GET_LEVEL(ch);
  } else {
    gain = GET_MAX_KI(ch) / 12;

    /* Class calculations */

    /* Skill/Spell calculations */

    /* Position calculations    */
    switch (GET_POS(ch)) {
    case POS_SLEEPING:
      gain *= 2;
      break;
    case POS_RESTING:
      gain += (gain / 2);       /* Divide by 2 */
      break;
    case POS_SITTING:
      gain += (gain / 4);       /* Divide by 4 */
      break;
    }

    if ((GET_COND(ch, FULL) == 0) || (GET_COND(ch, THIRST) == 0))
      gain /= 4;
  }

  if (AFF_FLAGGED(ch, AFF_POISON))
    gain /= 4;

  return (gain);
}


void set_title(struct char_data *ch, char *title)
{
  if (title == NULL) {
    title = class_desc_str(ch, 2, TRUE);
  }

  if (strlen(title) > MAX_TITLE_LENGTH)
    title[MAX_TITLE_LENGTH] = '\0';

  if (GET_TITLE(ch) != NULL)
    free(GET_TITLE(ch));

  GET_TITLE(ch) = strdup(title);
}

void gain_level(struct char_data *ch, int whichclass)
{
  if (whichclass < 0)
    whichclass = GET_CLASS(ch);
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 2 &&
      GET_EXP(ch) >= level_exp(GET_LEVEL(ch) + 1)) {
    if (raise_class_only(ch, whichclass, 1)) {
      GET_CLASS_LEVEL(ch) += 1;
      GET_CLASS(ch) = whichclass; /* Now tracks latest class instead of highest */
      advance_level(ch, whichclass);
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE, "%s advanced level to level %d.",
             GET_NAME(ch), GET_LEVEL(ch));
      send_to_char(ch, "You rise a level!\r\n");
      set_title(ch, NULL);
    } else {
      log("Failure in gain_level: raise_class_only failed for %s", GET_NAME(ch));
      send_to_char(ch, "Cannot raise your level. Please contact an administrator.\r\n");
    }
  }  
}

void run_autowiz(void)
{
#if defined(CIRCLE_UNIX) || defined(CIRCLE_WINDOWS)
  if (CONFIG_USE_AUTOWIZ) {
    size_t res;
    char buf[256];

#if defined(CIRCLE_UNIX)
    res = snprintf(buf, sizeof(buf), "nice ../bin/autowiz %d %s %d %s %d &",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE, (int) getpid());
#elif defined(CIRCLE_WINDOWS)
    res = snprintf(buf, sizeof(buf), "autowiz %d %s %d %s",
        CONFIG_MIN_WIZLIST_LEV, WIZLIST_FILE, ADMLVL_IMMORT, IMMLIST_FILE);
#endif /* CIRCLE_WINDOWS */

    /* Abusing signed -> unsigned conversion to avoid '-1' check. */
    if (res < sizeof(buf)) {
      mudlog(CMP, ADMLVL_IMMORT, FALSE, "Initiating autowiz.");
      system(buf);
      reboot_wizlists();
    } else
      log("Cannot run autowiz: command-line doesn't fit in buffer.");
  }
#endif /* CIRCLE_UNIX || CIRCLE_WINDOWS */
}



void gain_exp(struct char_data *ch, int gain)
{
  gain = (gain * CONFIG_EXP_MULTIPLIER);

  if (!IS_NPC(ch) && GET_LEVEL(ch) < 1)
    return;

  if (IS_NPC(ch)) {
    GET_EXP(ch) += gain;
    return;
  }
  if (gain > 0) {
    gain = calc_penalty_exp(ch, gain);
    gain = MIN(CONFIG_MAX_EXP_GAIN, gain);	/* put a cap on the max gain per kill */
    gain = MIN(gain, (level_exp(GET_LEVEL(ch) + 1) -
                       level_exp(GET_LEVEL(ch))));
    GET_EXP(ch) += gain;
    if (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1 &&
        GET_EXP(ch) >= level_exp(GET_LEVEL(ch) + 1))
      send_to_char(ch, "@rYou have earned enough experience to gain a @ylevel@r.@n\r\n");
  } else if (gain < 0) {
    gain = MAX(-CONFIG_MAX_EXP_LOSS, gain);	/* Cap max exp lost per death */
    GET_EXP(ch) += gain;
    if (GET_EXP(ch) < 0)
      GET_EXP(ch) = 0;
  }
}


void gain_exp_regardless(struct char_data *ch, int gain)
{
  int is_altered = FALSE;
  int num_levels = 0;

  gain = (gain * CONFIG_EXP_MULTIPLIER);

  GET_EXP(ch) += gain;
  if (GET_EXP(ch) < 0)
    GET_EXP(ch) = 0;

  if (!IS_NPC(ch)) {
    while (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1 &&
	GET_EXP(ch) >= level_exp(GET_LEVEL(ch) + 1)) {
      if (raise_class_only(ch, GET_CLASS(ch), 1)) {
        GET_CLASS_LEVEL(ch) += 1;
        num_levels++;
        advance_level(ch, GET_CLASS(ch));
        is_altered = TRUE;
      } else {
        log("gain_exp_regardless: raise_class_only() failed for %s", GET_NAME(ch));
      }
    }

    if (is_altered) {
      mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
             "%s advanced %d level%s to level %d.", GET_NAME(ch), num_levels,
             num_levels == 1 ? "" : "s", GET_LEVEL(ch));
      if (num_levels == 1)
        send_to_char(ch, "You rise a level!\r\n");
      else
	send_to_char(ch, "You rise %d levels!\r\n", num_levels);
      set_title(ch, NULL);
    }
  }
}


void gain_condition(struct char_data *ch, int condition, int value)
{
  bool intoxicated;

  if (IS_NPC(ch) || GET_COND(ch, condition) == -1)	/* No change */
    return;

  intoxicated = (GET_COND(ch, DRUNK) > 0);

  GET_COND(ch, condition) += value;

  GET_COND(ch, condition) = MAX(0, GET_COND(ch, condition));
  GET_COND(ch, condition) = MIN(24, GET_COND(ch, condition));

  if (GET_COND(ch, condition) || PLR_FLAGGED(ch, PLR_WRITING))
    return;

  switch (condition) {
  case FULL:
    send_to_char(ch, "You are hungry.\r\n");
    break;
  case THIRST:
    send_to_char(ch, "You are thirsty.\r\n");
    break;
  case DRUNK:
    if (intoxicated)
      send_to_char(ch, "You are now sober.\r\n");
    break;
  default:
    break;
  }

}


void check_idling(struct char_data *ch)
{
  if (++(ch->timer) > CONFIG_IDLE_VOID) {
    if (GET_WAS_IN(ch) == NOWHERE && IN_ROOM(ch) != NOWHERE) {
      GET_WAS_IN(ch) = IN_ROOM(ch);
      if (FIGHTING(ch)) {
	stop_fighting(FIGHTING(ch));
	stop_fighting(ch);
      }
      act("$n disappears into the void.", TRUE, ch, 0, 0, TO_ROOM);
      send_to_char(ch, "You have been idle, and are pulled into a void.\r\n");
      save_char(ch);
      Crash_crashsave(ch);
      char_from_room(ch);
      char_to_room(ch, 1);
    } else if (ch->timer > CONFIG_IDLE_RENT_TIME) {
      if (IN_ROOM(ch) != NOWHERE)
	char_from_room(ch);
      char_to_room(ch, 3);
      if (ch->desc) {
	STATE(ch->desc) = CON_DISCONNECT;
	/*
	 * For the 'if (d->character)' test in close_socket().
	 * -gg 3/1/98 (Happy anniversary.)
	 */
	ch->desc->character = NULL;
	ch->desc = NULL;
      }
      if (CONFIG_FREE_RENT)
	Crash_rentsave(ch, 0);
      else
	Crash_idlesave(ch);
      mudlog(CMP, ADMLVL_GOD, TRUE, "%s force-rented and extracted (idle).", GET_NAME(ch));
      extract_char(ch);
    }
  }
}



/* Update PCs, NPCs, and objects */
void point_update(void)
{
  struct char_data *i, *next_char;
  struct obj_data *j, *next_thing, *jj, *next_thing2;

  /* characters */
  for (i = character_list; i; i = next_char) {
    next_char = i->next;
	
    gain_condition(i, FULL, -1);
    gain_condition(i, DRUNK, -1);
    gain_condition(i, THIRST, -1);
	
    if (GET_POS(i) >= POS_STUNNED) {
      GET_HIT(i) = MIN(GET_HIT(i) + hit_gain(i), GET_MAX_HIT(i));
      GET_MANA(i) = MIN(GET_MANA(i) + mana_gain(i), GET_MAX_MANA(i));
      GET_MOVE(i) = MIN(GET_MOVE(i) + move_gain(i), GET_MAX_MOVE(i));
      GET_KI(i) = MIN(GET_KI(i) + ki_gain(i), GET_MAX_KI(i));
      if (AFF_FLAGGED(i, AFF_POISON))
	if (damage(i, i, 2, SPELL_POISON, 0, -1, 0, 0, 0) == -1)
	  continue;	/* Oops, they died. -gg 6/24/98 */
      if (GET_POS(i) <= POS_STUNNED)
	update_pos(i);
    } else if (GET_POS(i) == POS_INCAP) {
      if (damage(i, i, 1, TYPE_SUFFERING, 0, -1, 0, 0, 0) == -1)
	continue;
    } else if (GET_POS(i) == POS_MORTALLYW) {
      if (damage(i, i, 2, TYPE_SUFFERING, 0, -1, 0, 0, 0) == -1)
	continue;
    }
    if (!IS_NPC(i)) {
      update_char_objects(i);
      update_innate(i);
      if (GET_ADMLEVEL(i) < CONFIG_IDLE_MAX_LEVEL)
	check_idling(i);
    }
  }

  /* objects */
  for (j = object_list; j; j = next_thing) {
    next_thing = j->next;	/* Next in object list */

    /* If this is a corpse */
    if (IS_CORPSE(j)) {
      /* timer count down */
      if (GET_OBJ_TIMER(j) > 0)
	GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {

	if (j->carried_by)
	  act("$p decays in your hands.", FALSE, j->carried_by, j, 0, TO_CHAR);
	else if ((IN_ROOM(j) != NOWHERE) && (world[IN_ROOM(j)].people)) {
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
	  act("A quivering horde of maggots consumes $p.",
	      TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
	}
	for (jj = j->contains; jj; jj = next_thing2) {
	  next_thing2 = jj->next_content;	/* Next in inventory */
	  obj_from_obj(jj);

	  if (j->in_obj)
	    obj_to_obj(jj, j->in_obj);
	  else if (j->carried_by)
	    obj_to_room(jj, IN_ROOM(j->carried_by));
	  else if (IN_ROOM(j) != NOWHERE)
	    obj_to_room(jj, IN_ROOM(j));
	  else
	    core_dump();
	}
	extract_obj(j);
      }
    }

    if (GET_OBJ_TYPE(j) == ITEM_PORTAL) {
      if (GET_OBJ_TIMER(j) > 0)
          GET_OBJ_TIMER(j)--;

      if (!GET_OBJ_TIMER(j)) {
        act("A glowing portal fades from existence.",
  	   TRUE, world[IN_ROOM(j)].people, j, 0, TO_ROOM);
        act("A glowing portal fades from existence.",
  	   TRUE, world[IN_ROOM(j)].people, j, 0, TO_CHAR);
        extract_obj(j);
      }
    }

    /* If the timer is set, count it down and at 0, try the trigger */
    /* note to .rej hand-patchers: make this last in your point-update() */
    else if (GET_OBJ_TIMER(j)>0) {
      GET_OBJ_TIMER(j)--; 
      if (!GET_OBJ_TIMER(j))
        timer_otrigger(j);
    }
  }
}

void timed_dt(struct char_data *ch)
{ 
  struct char_data *vict;
  room_rnum rrnum;
  
  if (ch == NULL) {
  /* BY -WELCOR
    first make sure all rooms in the world have thier 'timed'
    value decreased if its not -1.
    */
  
  for (rrnum = 0; rrnum < top_of_world; rrnum++)
    world[rrnum].timed -= (world[rrnum].timed != -1);
  
  for (vict = character_list; vict ;vict = vict->next){
    if (IS_NPC(vict))
      continue;   

    if (IN_ROOM(vict) == NOWHERE)
      continue;
    
    if(!ROOM_FLAGGED(IN_ROOM(vict), ROOM_TIMED_DT))
      continue;
    
    timed_dt(vict);
   }
  return;
  }
  
  /*Called with a non-null ch. let's check the room. */
  
  /*if the room wasn't triggered (i.e timed wasn't set), just set it
    and return again.
  */
  
  if (world[IN_ROOM(ch)].timed < 0) {
    world[IN_ROOM(ch)].timed = rand_number(2, 5);
    return;
  }
  
  /* We know ch is in a dt room with timed >= 0 - see if its the end.
  *
  */
  if (world[IN_ROOM(ch)].timed == 0) {
    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
      if (IS_NPC(vict))
        continue;
      if (GET_ADMLEVEL(vict) >= ADMLVL_IMMORT)
        continue;

      /* Skip those alread dead people */
      /* extract char() jest sets the bit*/
      if (PLR_FLAGGED(vict, PLR_NOTDEADYET))
        continue;
             
      log_death_trap(vict);
      death_cry(vict);
      extract_char(vict);
    }
  }
}

