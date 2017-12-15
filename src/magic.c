/* ************************************************************************
*   File: magic.c                                       Part of CircleMUD *
*  Usage: low-level functions for magic; spell template code              *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */


#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/magic.c,v 1.16 2005/01/05 16:27:27 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "interpreter.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"

/* external variables */
extern int mini_mud;
extern struct spell_info_type spell_info[];
byte object_saving_throws(int material_type, int type);

/* external functions */
void clearMemory(struct char_data *ch);
void weight_change_object(struct obj_data *obj, int weight);

/* local functions */
int mag_materials(struct char_data *ch, int item0, int item1, int item2, int extract, int verbose);
void perform_mag_groups(int level, struct char_data *ch, struct char_data *tch, int spellnum);
void affect_update(void);
void affect_update_violence(void);
int obj_savingthrow(int material, int type);


int obj_savingthrow(int material, int type)
{
  int save, rnum;

  save = object_saving_throws(material, type);

  rnum = rand_number(1,100);

  if (rnum < save) {
    return (TRUE);
  }

  return (FALSE);
}

/* affect_update: called from comm.c (causes spells to wear off) */
void affect_update(void)
{
  struct affected_type *af, *next;
  struct char_data *i;

  for (i = affect_list; i; i = i->next_affect) {
    for (af = i->affected; af; af = next) {
      next = af->next;
      if (af->duration >= 1)
	af->duration--;
      else if (af->duration == 0) {
	if (af->type > 0)
	  if (!af->next || (af->next->type != af->type) ||
	      (af->next->duration > 0))
	    if (spell_info[af->type].wear_off_msg)
	      send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
	affect_remove(i, af);
      }
    }
  }
}


/*
 *  mag_materials:
 *  Checks for up to 3 vnums (spell reagents) in the player's inventory.
 *
 * No spells implemented in Circle use mag_materials, but you can use
 * it to implement your own spells which require ingredients (i.e., some
 * heal spell which requires a rare herb or some such.)
 */
int mag_materials(struct char_data *ch, int item0, int item1, int item2,
		      int extract, int verbose)
{
  struct obj_data *tobj;
  struct obj_data *obj0 = NULL, *obj1 = NULL, *obj2 = NULL;

  for (tobj = ch->carrying; tobj; tobj = tobj->next_content) {
    if ((item0 > 0) && (GET_OBJ_VNUM(tobj) == item0)) {
      obj0 = tobj;
      item0 = -1;
    } else if ((item1 > 0) && (GET_OBJ_VNUM(tobj) == item1)) {
      obj1 = tobj;
      item1 = -1;
    } else if ((item2 > 0) && (GET_OBJ_VNUM(tobj) == item2)) {
      obj2 = tobj;
      item2 = -1;
    }
  }
  if ((item0 > 0) || (item1 > 0) || (item2 > 0)) {
    if (verbose) {
      switch (rand_number(0, 2)) {
      case 0:
	send_to_char(ch, "A wart sprouts on your nose.\r\n");
	break;
      case 1:
	send_to_char(ch, "Your hair falls out in clumps.\r\n");
	break;
      case 2:
	send_to_char(ch, "A huge corn develops on your big toe.\r\n");
	break;
      }
    }
    return (FALSE);
  }
  if (extract) {
    if (item0 < 0)
      extract_obj(obj0);
    if (item1 < 0)
      extract_obj(obj1);
    if (item2 < 0)
      extract_obj(obj2);
  }
  if (verbose) {
    send_to_char(ch, "A puff of smoke rises from your pack.\r\n");
    act("A puff of smoke rises from $n's pack.", TRUE, ch, NULL, NULL, TO_ROOM);
  }
  return (TRUE);
}



int mag_newsaves(struct char_data *ch, struct char_data *victim, int spellnum, int level, int cast_stat)
{
  int stype;
  int dc;
  int total;
  if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_FORT))
    stype = SAVING_FORTITUDE;
  else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX))
    stype = SAVING_REFLEX;
  else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_WILL))
    stype = SAVING_WILL;
  else
    return FALSE;
  total = GET_SAVE(victim, stype) + rand_number(1, 20);
  dc = spell_info[spellnum].spell_level + level + ability_mod_value(cast_stat);
  if (ch) {
    if (HAS_SCHOOL_FEAT(ch, CFEAT_SPELL_FOCUS, spell_info[spellnum].school))
      dc++;
    if (HAS_SCHOOL_FEAT(ch, CFEAT_GREATER_SPELL_FOCUS, spell_info[spellnum].school))
      dc++;
  }
  if (total >= dc)
    return TRUE;
  return FALSE;
}


/*
 * Every spell that does damage comes through here.  This calculates the
 * amount of damage, adds in any modifiers, determines what the saves are,
 * tests for save and calls damage().
 *
 * -1 = dead, otherwise the amount of damage done.
 */
int mag_damage(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum)
{
  int dam = 0;

  if (victim == NULL || ch == NULL)
    return (0);

  switch (spellnum) {
    /* Mostly mages */
  case SPELL_MAGIC_MISSILE:
    dam = dice(MIN(level, 5), 4) + MIN(level, 5);
    break;

  case SPELL_CHILL_TOUCH:	/* chill touch also has an affect */
    dam = dice(1, 6);
    break;

  case SPELL_BURNING_HANDS:
    dam = dice(MIN(level, 5), 4);
    break;

  case SPELL_SHOCKING_GRASP:
    dam = dice(1, 8) + MIN(level, 20);
    break;

  case SPELL_LIGHTNING_BOLT:
  case SPELL_FIREBALL:
    dam = dice(MIN(level, 10), 6);
    break;

  case SPELL_COLOR_SPRAY:
    dam = dice(1, 1) + 1;
    break;

    /* Mostly clerics */
  case SPELL_DISPEL_EVIL:
    dam = dice(6, 8) + 6;
    if (IS_EVIL(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_GOOD(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;

  case SPELL_DISPEL_GOOD:
    dam = dice(6, 8) + 6;
    if (IS_GOOD(ch)) {
      victim = ch;
      dam = GET_HIT(ch) - 1;
    } else if (IS_EVIL(victim)) {
      act("The gods protect $N.", FALSE, ch, 0, victim, TO_CHAR);
      return (0);
    }
    break;

  case SPELL_CALL_LIGHTNING:
    dam = dice(MIN(level, 10), 10);
    break;

  case SPELL_HARM:
    dam = dice(8, 8) + 8;
    break;

  case SPELL_ENERGY_DRAIN:
    if (GET_LEVEL(victim) <= 2)
      dam = 100;
    else
      dam = dice(1, 10);
    break;

    /* Area spells */
  case SPELL_EARTHQUAKE:
    dam = dice(2, 8) + level;
    break;

  case SPELL_INFLICT_LIGHT:
    dam = dice(1, 8) + MIN(level, 5);
    break;
  case SPELL_INFLICT_CRITIC:
    dam = dice(4, 8) + MIN(level, 20);
    break;

  case SPELL_ACID_SPLASH:
  case SPELL_RAY_OF_FROST:
    dam = dice(1, 3);
    break;

  case SPELL_DISRUPT_UNDEAD:
    if (AFF_FLAGGED(victim, AFF_UNDEAD))
      dam = dice(1, 6);
    else {
      send_to_char(ch, "This magic only affects the undead!\r\n");
      dam = 0;
    }
    break;

  case SPELL_ICE_STORM:
  case SPELL_SHOUT:
    dam = dice(5, 6);
    break;

  case SPELL_CONE_OF_COLD:
    dam = dice(MIN(level, 15), 6);
    break;

  } /* switch(spellnum) */

  if (mag_newsaves(ch, victim, spellnum, level, GET_INT(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any injury.@n\r\n");
      dam = 0;
    } else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_HALF)) {
      if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX) &&
          HAS_FEAT(victim, FEAT_EVASION) && 
          (!GET_EQ(victim, WEAR_BODY) ||
          GET_OBJ_TYPE(GET_EQ(victim, WEAR_BODY)) != ITEM_ARMOR ||
          GET_OBJ_VAL(GET_EQ(victim, WEAR_BODY), VAL_ARMOR_SKILL) < ARMOR_TYPE_MEDIUM)) {
        send_to_char(victim, "@g*save*@y Your evasion ability allows you to avoid ANY injury.@n\r\n");
        dam = 0;
      } else {
        send_to_char(victim, "@g*save*@y You take half damage.@n\r\n");
        dam /= 2;
      }
    }
  } else if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_HALF) &&
             IS_SET(spell_info[spellnum].save_flags, MAGSAVE_REFLEX) &&
             HAS_FEAT(victim, FEAT_IMPROVED_EVASION) && 
             (!GET_EQ(victim, WEAR_BODY) ||
              GET_OBJ_TYPE(GET_EQ(victim, WEAR_BODY)) != ITEM_ARMOR ||
              GET_OBJ_VAL(GET_EQ(victim, WEAR_BODY), VAL_ARMOR_SKILL) < ARMOR_TYPE_MEDIUM)) {
    send_to_char(victim, "@r*save*@y Your improved evasion prevents full damage even on failure.@n\r\n");
    dam /= 2;
  }

  /* and finally, inflict the damage */
  return (damage(ch, victim, dam, spellnum, 0, -1, 0, spellnum, 1));
}


/*
 * Every spell that does an affect comes through here.  This determines
 * the effect, whether it is added or replacement, whether it is legal or
 * not, etc.
 *
 * affect_join(vict, aff, add_dur, avg_dur, add_mod, avg_mod)
 */

#define MAX_SPELL_AFFECTS 5	/* change if more needed */

void mag_affects(int level, struct char_data *ch, struct char_data *victim,
		      int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, GET_INT(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    }
  }

  switch (spellnum) {

  case SPELL_CHILL_TOUCH:
    af[0].location = APPLY_STR;
    af[0].duration = 24;
    af[0].modifier = -1;
    accum_duration = TRUE;
    to_vict = "You feel your strength wither!";
    break;

  case SPELL_MAGE_ARMOR:
    af[0].location = APPLY_AC;
    af[0].modifier = 40;
    af[0].duration = 1 * GET_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "You feel someone protecting you.";
    break;

  case SPELL_BLESS:
    af[0].location = APPLY_ACCURACY;
    af[0].modifier = 2;
    af[0].duration = 6;

    af[1].location = APPLY_SAVING_SPELL;
    af[1].modifier = -1;
    af[1].duration = 6;

    accum_duration = TRUE;
    to_vict = "You feel righteous.";
    break;

  case SPELL_BLINDNESS:
    if (MOB_FLAGGED(victim,MOB_NOBLIND)) {
      send_to_char(ch, "You fail.\r\n");
      return;
    }

    af[0].location = APPLY_ACCURACY;
    af[0].modifier = -4;
    af[0].duration = 2;
    af[0].bitvector = AFF_BLIND;

    af[1].location = APPLY_AC;
    af[1].modifier = -4;
    af[1].duration = 2;
    af[1].bitvector = AFF_BLIND;

    to_room = "$n seems to be blinded!";
    to_vict = "You have been blinded!";
    break;

  case SPELL_BESTOW_CURSE:
    af[0].location = APPLY_ACCURACY;
    af[0].duration = 1 + (level / 2);
    af[0].modifier = -1;
    af[0].bitvector = AFF_CURSE;

    af[1].location = APPLY_DAMAGE;
    af[1].duration = 1 + (level / 2);
    af[1].modifier = -1;
    af[1].bitvector = AFF_CURSE;

    accum_duration = TRUE;
    accum_affect = TRUE;
    to_room = "$n briefly glows red!";
    to_vict = "You feel very uncomfortable.";
    break;

  case SPELL_DETECT_ALIGN:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_ALIGN;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_SEE_INVIS:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_INVIS;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_DETECT_MAGIC:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_DETECT_MAGIC;
    accum_duration = TRUE;
    to_vict = "Your eyes tingle.";
    break;

  case SPELL_FAERIE_FIRE:
    af[0].location = APPLY_AC;
    af[0].modifier = -1; /*should make target easier to hit */
    af[0].duration = 3;
    accum_duration = FALSE;
    to_vict = "Your body flickers with a purplish light.";
    to_room = "$n's body flickers with with a purplish light.";
    break;

  case SPELL_DARKVISION:
    af[0].duration = 12 + level;
    af[0].bitvector = AFF_INFRAVISION;
    accum_duration = TRUE;
    to_vict = "Your eyes glow red.";
    to_room = "$n's eyes glow red.";
    break;

  case SPELL_INVISIBLE:
    if (!victim)
      victim = ch;

    af[0].duration = 12 + (level / 4);
    af[0].modifier = 4;
    af[0].location = APPLY_AC;
    af[0].bitvector = AFF_INVISIBLE;
    accum_duration = TRUE;
    to_vict = "You vanish.";
    to_room = "$n slowly fades out of existence.";
    break;

  case SPELL_POISON:
    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = -2;
    af[0].bitvector = AFF_POISON;
    to_vict = "You feel very sick.";
    to_room = "$n gets violently ill!";
    break;

  case SPELL_PROT_FROM_EVIL:
    af[0].duration = 24;
    af[0].bitvector = AFF_PROTECT_EVIL;
    accum_duration = TRUE;
    to_vict = "You feel invulnerable!";
    break;

  case SPELL_SANCTUARY:
    af[0].duration = 4;
    af[0].bitvector = AFF_SANCTUARY;

    accum_duration = TRUE;
    to_vict = "A white aura momentarily surrounds you.";
    to_room = "$n is surrounded by a white aura.";
    break;

  case SPELL_SLEEP:
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(victim))
      return;
    if (MOB_FLAGGED(victim, MOB_NOSLEEP))
      return;

    af[0].duration = 4 + (level / 4);
    af[0].bitvector = AFF_SLEEP;

    if (GET_POS(victim) > POS_SLEEPING) {
      send_to_char(victim, "You feel very sleepy...  Zzzz......\r\n");
      act("$n goes to sleep.", TRUE, victim, 0, 0, TO_ROOM);
      GET_POS(victim) = POS_SLEEPING;
    }
    break;

  case SPELL_BULL_STRENGTH:
    af[0].location = APPLY_STR;
    af[0].duration = level;
    af[0].modifier = 1 + (rand_number(1, 4));
    accum_duration = FALSE;
    accum_affect = FALSE;
    to_vict = "You feel stronger!";
    break;

  case SPELL_SENSE_LIFE:
    to_vict = "Your feel your awareness improve.";
    af[0].duration = level;
    af[0].bitvector = AFF_SENSE_LIFE;
    accum_duration = TRUE;
    break;

  case SPELL_WATERWALK:
    af[0].duration = 24;
    af[0].bitvector = AFF_WATERWALK;
    accum_duration = TRUE;
    to_vict = "You feel webbing between your toes.";
    break;

  case SPELL_STONESKIN:
    af[0].duration = 1 * GET_LEVEL(ch);
    accum_duration = FALSE;
    to_vict = "Your skin hardens into stone!";
    break;

  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector)) {
	send_to_char(ch, "%s", CONFIG_NOEFFECT);
	return;
      }

  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affect_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}


/*
 * This function is used to provide services to mag_groups.  This function
 * is the one you should change to add new group spells.
 */
void perform_mag_groups(int level, struct char_data *ch,
			struct char_data *tch, int spellnum)
{
  switch (spellnum) {
    case SPELL_MASS_HEAL:
    mag_points(level, ch, tch, SPELL_HEAL);
    break;
  case SPELL_GROUP_ARMOR:
    mag_affects(level, ch, tch, SPELL_MAGE_ARMOR);
    break;
  case SPELL_GROUP_RECALL:
    spell_recall(level, ch, tch, NULL, NULL);
    break;
  }
}


/*
 * Every spell that affects the group should run through here
 * perform_mag_groups contains the switch statement to send us to the right
 * magic.
 *
 * group spells affect everyone grouped with the caster who is in the room,
 * caster last.
 *
 * To add new group spells, you shouldn't have to change anything in
 * mag_groups -- just add a new case to perform_mag_groups.
 */
void mag_groups(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *k;
  struct follow_type *f, *f_next;

  if (ch == NULL)
    return;

  if (!AFF_FLAGGED(ch, AFF_GROUP))
    return;
  if (ch->master != NULL)
    k = ch->master;
  else
    k = ch;
  for (f = k->followers; f; f = f_next) {
    f_next = f->next;
    tch = f->follower;
    if (IN_ROOM(tch) != IN_ROOM(ch))
      continue;
    if (!AFF_FLAGGED(tch, AFF_GROUP))
      continue;
    if (ch == tch)
      continue;
    perform_mag_groups(level, ch, tch, spellnum);
  }

  if ((k != ch) && AFF_FLAGGED(k, AFF_GROUP))
    perform_mag_groups(level, ch, k, spellnum);
  perform_mag_groups(level, ch, ch, spellnum);
}


/*
 * mass spells affect every creature in the room except the caster.
 *
 * No spells of this class currently implemented.
 */
void mag_masses(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *tch_next;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch_next) {
    tch_next = tch->next_in_room;
    if (tch == ch)
      continue;

    switch (spellnum) {
    }
  }
}


/*
 * Every spell that affects an area (room) runs through here.  These are
 * generally offensive spells.  This calls mag_damage to do the actual
 * damage -- all spells listed here must also have a case in mag_damage()
 * in order for them to work.
 *
 *  area spells have limited targets within the room.
 */
void mag_areas(int level, struct char_data *ch, int spellnum)
{
  struct char_data *tch, *next_tch;
  const char *to_char = NULL, *to_room = NULL;

  if (ch == NULL)
    return;

  /*
   * to add spells to this fn, just add the message here plus an entry
   * in mag_damage for the damaging part of the spell.
   */
  switch (spellnum) {
  case SPELL_EARTHQUAKE:
    to_char = "You gesture and the earth begins to shake all around you!";
    to_room ="$n gracefully gestures and the earth begins to shake violently!";
    break;
  }

  if (to_char != NULL)
    act(to_char, FALSE, ch, 0, 0, TO_CHAR);
  if (to_room != NULL)
    act(to_room, FALSE, ch, 0, 0, TO_ROOM);
  

  for (tch = world[IN_ROOM(ch)].people; tch; tch = next_tch) {
    next_tch = tch->next_in_room;

    /*
     * The skips: 1: the caster
     *            2: immortals
     *            3: if no pk on this mud, skips over all players
     *            4: pets (charmed NPCs)
     */

    if (tch == ch)
      continue;
    if (ADM_FLAGGED(tch, ADM_NODAMAGE))
      continue;
    if (!CONFIG_PK_ALLOWED && !IS_NPC(ch) && !IS_NPC(tch))
      continue;
    if (!IS_NPC(ch) && IS_NPC(tch) && AFF_FLAGGED(tch, AFF_CHARM))
      continue;

    /* Doesn't matter if they die here so we don't check. -gg 6/24/98 */
    mag_damage(level, ch, tch, spellnum);
  }
}


/*
 *  Every spell which summons/gates/conjours a mob comes through here.
 */

mob_vnum monsum_list_lg_1[] = { 300, 301, 302, NOBODY };
mob_vnum monsum_list_ng_1[] = { 300, 301, 302, 303, 304, NOBODY };
mob_vnum monsum_list_cg_1[] = { 302, 303, 304, NOBODY };
mob_vnum monsum_list_ln_1[] = { 300, 301, 305, 306, NOBODY };
mob_vnum monsum_list_nn_1[] = { 302, 307, 308, NOBODY };
mob_vnum monsum_list_cn_1[] = { 303, 304, 309, 310, 311, 312, NOBODY };
mob_vnum monsum_list_le_1[] = { 305, 306, 307, 308, NOBODY };
mob_vnum monsum_list_ne_1[] = { 305, 306, 307, 308, 309, 310, 311, 312, NOBODY };
mob_vnum monsum_list_ce_1[] = { 307, 308, 309, 310, 311, 312, NOBODY };

mob_vnum monsum_list_lg_2[] = { NOBODY };
mob_vnum monsum_list_ng_2[] = { NOBODY };
mob_vnum monsum_list_cg_2[] = { NOBODY };
mob_vnum monsum_list_ln_2[] = { NOBODY };
mob_vnum monsum_list_nn_2[] = { NOBODY };
mob_vnum monsum_list_cn_2[] = { NOBODY };
mob_vnum monsum_list_le_2[] = { NOBODY };
mob_vnum monsum_list_ne_2[] = { NOBODY };
mob_vnum monsum_list_ce_2[] = { NOBODY };

mob_vnum monsum_list_lg_3[] = { NOBODY };
mob_vnum monsum_list_ng_3[] = { NOBODY };
mob_vnum monsum_list_cg_3[] = { NOBODY };
mob_vnum monsum_list_ln_3[] = { NOBODY };
mob_vnum monsum_list_nn_3[] = { NOBODY };
mob_vnum monsum_list_cn_3[] = { NOBODY };
mob_vnum monsum_list_le_3[] = { NOBODY };
mob_vnum monsum_list_ne_3[] = { NOBODY };
mob_vnum monsum_list_ce_3[] = { NOBODY };

mob_vnum monsum_list_lg_4[] = { NOBODY };
mob_vnum monsum_list_ng_4[] = { NOBODY };
mob_vnum monsum_list_cg_4[] = { NOBODY };
mob_vnum monsum_list_ln_4[] = { NOBODY };
mob_vnum monsum_list_nn_4[] = { NOBODY };
mob_vnum monsum_list_cn_4[] = { NOBODY };
mob_vnum monsum_list_le_4[] = { NOBODY };
mob_vnum monsum_list_ne_4[] = { NOBODY };
mob_vnum monsum_list_ce_4[] = { NOBODY };

mob_vnum monsum_list_lg_5[] = { NOBODY };
mob_vnum monsum_list_ng_5[] = { NOBODY };
mob_vnum monsum_list_cg_5[] = { NOBODY };
mob_vnum monsum_list_ln_5[] = { NOBODY };
mob_vnum monsum_list_nn_5[] = { NOBODY };
mob_vnum monsum_list_cn_5[] = { NOBODY };
mob_vnum monsum_list_le_5[] = { NOBODY };
mob_vnum monsum_list_ne_5[] = { NOBODY };
mob_vnum monsum_list_ce_5[] = { NOBODY };

mob_vnum monsum_list_lg_6[] = { NOBODY };
mob_vnum monsum_list_ng_6[] = { NOBODY };
mob_vnum monsum_list_cg_6[] = { NOBODY };
mob_vnum monsum_list_ln_6[] = { NOBODY };
mob_vnum monsum_list_nn_6[] = { NOBODY };
mob_vnum monsum_list_cn_6[] = { NOBODY };
mob_vnum monsum_list_le_6[] = { NOBODY };
mob_vnum monsum_list_ne_6[] = { NOBODY };
mob_vnum monsum_list_ce_6[] = { NOBODY };

mob_vnum monsum_list_lg_7[] = { NOBODY };
mob_vnum monsum_list_ng_7[] = { NOBODY };
mob_vnum monsum_list_cg_7[] = { NOBODY };
mob_vnum monsum_list_ln_7[] = { NOBODY };
mob_vnum monsum_list_nn_7[] = { NOBODY };
mob_vnum monsum_list_cn_7[] = { NOBODY };
mob_vnum monsum_list_le_7[] = { NOBODY };
mob_vnum monsum_list_ne_7[] = { NOBODY };
mob_vnum monsum_list_ce_7[] = { NOBODY };

mob_vnum monsum_list_lg_8[] = { NOBODY };
mob_vnum monsum_list_ng_8[] = { NOBODY };
mob_vnum monsum_list_cg_8[] = { NOBODY };
mob_vnum monsum_list_ln_8[] = { NOBODY };
mob_vnum monsum_list_nn_8[] = { NOBODY };
mob_vnum monsum_list_cn_8[] = { NOBODY };
mob_vnum monsum_list_le_8[] = { NOBODY };
mob_vnum monsum_list_ne_8[] = { NOBODY };
mob_vnum monsum_list_ce_8[] = { NOBODY };

mob_vnum monsum_list_lg_9[] = { NOBODY };
mob_vnum monsum_list_ng_9[] = { NOBODY };
mob_vnum monsum_list_cg_9[] = { NOBODY };
mob_vnum monsum_list_ln_9[] = { NOBODY };
mob_vnum monsum_list_nn_9[] = { NOBODY };
mob_vnum monsum_list_cn_9[] = { NOBODY };
mob_vnum monsum_list_le_9[] = { NOBODY };
mob_vnum monsum_list_ne_9[] = { NOBODY };
mob_vnum monsum_list_ce_9[] = { NOBODY };

mob_vnum *monsum_list[9][9] = {
  { monsum_list_lg_1, monsum_list_ng_1, monsum_list_cg_1,
    monsum_list_ln_1, monsum_list_nn_1, monsum_list_cn_1,
    monsum_list_le_1, monsum_list_ne_1, monsum_list_ce_1 },
  { monsum_list_lg_2, monsum_list_ng_2, monsum_list_cg_2,
    monsum_list_ln_2, monsum_list_nn_2, monsum_list_cn_2,
    monsum_list_le_2, monsum_list_ne_2, monsum_list_ce_2 },
  { monsum_list_lg_3, monsum_list_ng_3, monsum_list_cg_3,
    monsum_list_ln_3, monsum_list_nn_3, monsum_list_cn_3,
    monsum_list_le_3, monsum_list_ne_3, monsum_list_ce_3 },
  { monsum_list_lg_4, monsum_list_ng_4, monsum_list_cg_4,
    monsum_list_ln_4, monsum_list_nn_4, monsum_list_cn_4,
    monsum_list_le_4, monsum_list_ne_4, monsum_list_ce_4 },
  { monsum_list_lg_5, monsum_list_ng_5, monsum_list_cg_5,
    monsum_list_ln_5, monsum_list_nn_5, monsum_list_cn_5,
    monsum_list_le_5, monsum_list_ne_5, monsum_list_ce_5 },
  { monsum_list_lg_6, monsum_list_ng_6, monsum_list_cg_6,
    monsum_list_ln_6, monsum_list_nn_6, monsum_list_cn_6,
    monsum_list_le_6, monsum_list_ne_6, monsum_list_ce_6 },
  { monsum_list_lg_7, monsum_list_ng_7, monsum_list_cg_7,
    monsum_list_ln_7, monsum_list_nn_7, monsum_list_cn_7,
    monsum_list_le_7, monsum_list_ne_7, monsum_list_ce_7 },
  { monsum_list_lg_8, monsum_list_ng_8, monsum_list_cg_8,
    monsum_list_ln_8, monsum_list_nn_8, monsum_list_cn_8,
    monsum_list_le_8, monsum_list_ne_8, monsum_list_ce_8 },
  { monsum_list_lg_9, monsum_list_ng_9, monsum_list_cg_9,
    monsum_list_ln_9, monsum_list_nn_9, monsum_list_cn_9,
    monsum_list_le_9, monsum_list_ne_9, monsum_list_ce_9 }
};

/*
 * These use act(), don't put the \r\n.
 */
const char *mag_summon_msgs[] = {
  "\r\n",
  "$n animates a corpse!",
  "$n summons extraplanar assistance!"
};

/* Defined mobiles. */
#define MOB_ELEMENTAL_BASE	20	/* Only one for now. */
#define MOB_ZOMBIE		11
#define MOB_AERIALSERVANT	19

void mag_summons(int level, struct char_data *ch, struct obj_data *obj,
		      int spellnum, const char *arg)
{
  struct char_data *mob = NULL;
  struct obj_data *tobj, *next_obj;
  int msg = 0, num = 1, handle_corpse = FALSE, affs = 0, affvs = 0, assist = 0, i, j, count;
  char *buf = NULL, buf2[MAX_INPUT_LENGTH];
  int lev;
  mob_vnum mob_num;

  if (ch == NULL)
    return;

  lev = spell_info[spellnum].spell_level;

  switch (spellnum) {
  case SPELL_ANIMATE_DEAD:
    if (obj == NULL) {
      send_to_char(ch, "With what corpse?\r\n");
      return;
    }
    if (!IS_CORPSE(obj)) {
      send_to_char(ch, "That's not a corpse!\r\n");
      return;
    }
    handle_corpse = TRUE;
    msg = 11;
    mob_num = MOB_ZOMBIE;
    break;

  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    mob_num = NOBODY;
    affvs = 1;
    assist = 1;
    if (arg) {
      buf = (char *)arg;
      skip_spaces(&buf);
      if (!*buf)
        buf = NULL;
    }
    j = ALIGN_TYPE(ch);
    if (buf) {
      buf = any_one_arg(buf, buf2);
      for (i = lev - 1; i >= 0; i--) {
        for (count = 0; monsum_list[i][j][count] != NOBODY; count++) {
          mob_num = monsum_list[i][j][count];
          if (real_mobile(mob_num) == NOBODY)
            mob_num = NOBODY;
          else if (!is_name(buf2, mob_proto[real_mobile(mob_num)].name))
            mob_num = NOBODY;
          else
            break;
        }
        if (mob_num != NOBODY)
          break;
      }
      if (mob_num == NOBODY) {
        send_to_char(ch, "That's not a name for a monster you can summon. Summoning something else.\r\n");
      } else {
        log("lev=%d, i=%d, ngen=%d", lev, i, lev - i);
        switch (lev - i) {
        case 1:
          num = 1;
          break;
        case 2:
          num = rand_number(1, 3);
          break;
        default:
          num = rand_number(1, 4) + 1;
          break;
        }
      }
    }
    if (mob_num == NOBODY) {
      num = 1;
      for (count = 0; monsum_list[lev - 1][j][count] != NOBODY; count++);
      if (!count) {
        log("No monsums for spell level %d align %s", lev, alignments[j]);
        return;
      }
      count--;
      mob_num = monsum_list[lev - 1][j][rand_number(0, count)];
    }
    break;

  default:
    return;
  }

  if (AFF_FLAGGED(ch, AFF_CHARM)) {
    send_to_char(ch, "You are too giddy to have any followers!\r\n");
    return;
  }
  for (i = 0; i < num; i++) {
    if (!(mob = read_mobile(mob_num, VIRTUAL))) {
      send_to_char(ch, "You don't quite remember how to summon that creature.\r\n");
      return;
    }
    char_to_room(mob, IN_ROOM(ch));
    if (affs)
      mag_affects(level, ch, mob, spellnum);
    if (affvs)
      mag_affectsv(level, ch, mob, spellnum);
    IS_CARRYING_W(mob) = 0;
    IS_CARRYING_N(mob) = 0;
    SET_BIT_AR(AFF_FLAGS(mob), AFF_CHARM);
    act(mag_summon_msgs[msg], FALSE, ch, 0, mob, TO_ROOM);
    load_mtrigger(mob);
    add_follower(mob, ch);
    if (assist && FIGHTING(ch)) {
      set_fighting(mob, FIGHTING(ch));
    }
    mob->master_id = GET_IDNUM(ch);
  }
  if (handle_corpse) {
    for (tobj = obj->contains; tobj; tobj = next_obj) {
      next_obj = tobj->next_content;
      obj_from_obj(tobj);
      obj_to_char(tobj, mob);
    }
    extract_obj(obj);
  }
}


void mag_points(int level, struct char_data *ch, struct char_data *victim,
		     int spellnum)
{
  int healing = 0, move = 0;
  int tmp;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_CURE_LIGHT:
    healing = dice(1, 8) + MIN(level, 5);
    send_to_char(victim, "You feel better.\r\n");
    break;
  case SPELL_CURE_CRITIC:
    healing = dice(4, 8) + MIN(level, 20);
    send_to_char(victim, "You feel a lot better!\r\n");
    break;
  case SPELL_HEAL:
    healing = 100 + dice(3, 8);
    send_to_char(victim, "A warm feeling floods your body.\r\n");
    if (AFF_FLAGGED(ch, AFF_CDEATH)) {
      affectv_from_char(ch, ART_QUIVERING_PALM);
      send_to_char(ch, "Your nerves settle slightly\r\n");
    }
    break;
  case ART_WHOLENESS_OF_BODY:
    healing = GET_MAX_HIT(victim) - GET_HIT(victim);
    healing = MAX(0, healing);
    tmp = GET_KI(ch) / 2;
    if (tmp > healing)
      tmp = healing;
    else {
      healing = tmp;
    }
    GET_KI(ch) -= tmp * 2;
    break;
  }
  GET_HIT(victim) = MIN(GET_MAX_HIT(victim), GET_HIT(victim) + healing);
  GET_MOVE(victim) = MIN(GET_MAX_MOVE(victim), GET_MOVE(victim) + move);
  update_pos(victim);
}


void mag_unaffects(int level, struct char_data *ch, struct char_data *victim,
		        int spellnum)
{
  int spell = 0, msg_not_affected = TRUE;
  const char *to_vict = NULL, *to_room = NULL;

  if (victim == NULL)
    return;

  switch (spellnum) {
  case SPELL_HEAL:
    /*
     * Heal also restores health, so don't give the "no effect" message
     * if the target isn't afflicted by the 'blindness' spell.
     */
    msg_not_affected = FALSE;
    /* fall-through */
  case SPELL_REMOVE_BLINDNESS:
    spell = SPELL_BLINDNESS;
    to_vict = "Your vision returns!";
    to_room = "There's a momentary gleam in $n's eyes.";
    break;
  case SPELL_NEUTRALIZE_POISON:
    spell = SPELL_POISON;
    to_vict = "A warm feeling runs through your body!";
    to_room = "$n looks better.";
    break;
  case SPELL_REMOVE_CURSE:
    spell = SPELL_BESTOW_CURSE;
    to_vict = "You don't feel so unlucky.";
    break;
  default:
    log("SYSERR: unknown spellnum %d passed to mag_unaffects.", spellnum);
    return;
  }

  if (!affected_by_spell(victim, spell)) {
    if (msg_not_affected)
      send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  affect_from_char(victim, spell);
  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);

}


void mag_alter_objs(int level, struct char_data *ch, struct obj_data *obj,
		         int spellnum)
{
  const char *to_char = NULL, *to_room = NULL;

  if (obj == NULL)
    return;

  switch (spellnum) {
    case SPELL_BLESS:
      if (!OBJ_FLAGGED(obj, ITEM_BLESS) &&
	  (GET_OBJ_WEIGHT(obj) <= 5 * level)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_BLESS);
	to_char = "$p glows briefly.";
      }
      break;
    case SPELL_BESTOW_CURSE:
      if (!OBJ_FLAGGED(obj, ITEM_NODROP)) {
	SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
	if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
	  GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE)--;
	to_char = "$p briefly glows red.";
      }
      break;
    case SPELL_INVISIBLE:
      if (!OBJ_FLAGGED(obj, ITEM_NOINVIS | ITEM_INVISIBLE)) {
        SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_INVISIBLE);
        to_char = "$p vanishes.";
      }
      break;
    case SPELL_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && !GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
      GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 1;
      to_char = "$p steams briefly.";
      }
      break;
    case SPELL_REMOVE_CURSE:
      if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
        REMOVE_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_NODROP);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON)
          GET_OBJ_VAL(obj, VAL_WEAPON_DAMSIZE)++;
        to_char = "$p briefly glows blue.";
      }
      break;
    case SPELL_NEUTRALIZE_POISON:
      if (((GET_OBJ_TYPE(obj) == ITEM_DRINKCON) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOUNTAIN) ||
         (GET_OBJ_TYPE(obj) == ITEM_FOOD)) && GET_OBJ_VAL(obj, VAL_FOOD_POISON)) {
        GET_OBJ_VAL(obj, VAL_FOOD_POISON) = 0;
        to_char = "$p steams briefly.";
      }
      break;
  }

  if (to_char == NULL)
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
  else
    act(to_char, TRUE, ch, obj, 0, TO_CHAR);

  if (to_room != NULL)
    act(to_room, TRUE, ch, obj, 0, TO_ROOM);
  else if (to_char != NULL)
    act(to_char, TRUE, ch, obj, 0, TO_ROOM);

}



void mag_creations(int level, struct char_data *ch, int spellnum)
{
  struct obj_data *tobj;
  obj_vnum z;

  if (ch == NULL)
    return;
  /* level = MAX(MIN(level, LVL_IMPL), 1); - Hm, not used. */

  switch (spellnum) {
  case SPELL_CREATE_FOOD:
    z = 10;
    break;
  default:
    send_to_char(ch, "Spell unimplemented, it would seem.\r\n");
    return;
  }

  if (!(tobj = read_object(z, VIRTUAL))) {
    send_to_char(ch, "I seem to have goofed.\r\n");
    log("SYSERR: spell_creations, spell %d, obj %d: obj not found",
	    spellnum, z);
    return;
  }
  add_unique_id(tobj);
  obj_to_char(tobj, ch);
  act("$n creates $p.", FALSE, ch, tobj, 0, TO_ROOM);
  act("You create $p.", FALSE, ch, tobj, 0, TO_CHAR);
  load_otrigger(tobj);
}

/* affect_update_violence: called from fight.c (causes spells to wear off) */
void affect_update_violence(void)
{
  struct affected_type *af, *next;
  struct char_data *i;
  int dam;
  int maxdam;

  for (i = affectv_list; i; i = i->next_affectv) {
    for (af = i->affectedv; af; af = next) {
      next = af->next;
      if (af->duration >= 1) {
        af->duration--;
        switch (af->type) {
        case ART_EMPTY_BODY:
          if (GET_KI(i) >= 10) {
            GET_KI(i) -= 10;
          } else {
            af->duration = 0; /* Wear it off immediately! No more ki */
          }
        }
      } else if (af->duration == -1) {     /* No action */
        continue;
      }
      if (!af->duration) {
        if ((af->type > 0) && (af->type < SKILL_TABLE_SIZE))
          if (!af->next || (af->next->type != af->type) ||
              (af->next->duration > 0))
            if (spell_info[af->type].wear_off_msg)
              send_to_char(i, "%s\r\n", spell_info[af->type].wear_off_msg);
          if (af->bitvector == AFF_SUMMONED) {
            stop_follower(i);
            if (!DEAD(i))
              extract_char(i);
          }
          if (af->type == ART_QUIVERING_PALM) {
            maxdam = GET_HIT(i) + 8;
            dam = GET_MAX_HIT(i) * 3 / 4;
            dam = MIN(dam, maxdam);
            dam = MAX(0, dam);
            log("Creeping death strike doing %d dam", dam);
            damage(i, i, dam, af->type, 0, -1, 0, 0, 1);
          }
        affectv_remove(i, af);
      }
    }
  }
}

void mag_affectsv(int level, struct char_data *ch, struct char_data *victim,
                      int spellnum)
{
  struct affected_type af[MAX_SPELL_AFFECTS];
  bool accum_affect = FALSE, accum_duration = FALSE;
  const char *to_vict = NULL, *to_room = NULL;
  int i;


  if (victim == NULL || ch == NULL)
    return;

  for (i = 0; i < MAX_SPELL_AFFECTS; i++) {
    af[i].type = spellnum;
    af[i].bitvector = 0;
    af[i].modifier = 0;
    af[i].location = APPLY_NONE;
  }

  if (mag_newsaves(ch, victim, spellnum, level, GET_INT(ch))) {
    if (IS_SET(spell_info[spellnum].save_flags, MAGSAVE_PARTIAL | MAGSAVE_NONE)) {
      send_to_char(victim, "@g*save*@y You avoid any lasting affects.@n\r\n");
      return;
    } 
  }

  switch (spellnum) {
  case SPELL_PARALIZE:
    af[0].duration = level/2;
    af[0].bitvector = AFF_PARALYZE;
    accum_duration = FALSE;
    to_vict = "You feel your limbs freeze!";
    to_room = "$n suddenly freezes in place!";
    break;
  case ART_STUNNING_FIST:
    af[0].duration = 1;
    af[0].bitvector = AFF_STUNNED;
    accum_duration = FALSE;
    to_vict = "You are in a stunned daze!";
    to_room = "$n is stunned.";
    break;
  case ART_EMPTY_BODY:
    af[0].duration = GET_KI(ch) / 10;
    af[0].bitvector = AFF_ETHEREAL;
    accum_duration = FALSE;
    to_vict = "You switch to the ethereal plane.";
    to_room = "$n disappears.";
    break;
  case ART_QUIVERING_PALM:
    if (GET_LEVEL(ch) <= GET_LEVEL(victim)) {
      send_to_char(ch, "They are too high level for that.\r\n");
      return;
    }
    af[0].duration = MAX(6, 20 - level);
    af[0].bitvector = AFF_CDEATH;
    accum_duration = FALSE;
    to_vict = "You feel death closing in.";
    break;
  case SPELL_RESISTANCE:
    af[0].duration = 12;
    af[0].location = APPLY_ALLSAVES;
    af[0].modifier = 1;
    accum_duration = FALSE;
    to_vict = "You glow briefly with a silvery light.";
    to_room = "$n glows briefly with a silvery light.";
    break;
  case SPELL_DAZE:
    if (GET_HITDICE(victim) < 5)
      af[0].bitvector = AFF_NEXTNOACTION;
      accum_duration = FALSE;
      to_vict = "You are struck dumb by a flash of bright light!";
      to_room = "$n is struck dumb by a flash of bright light!";
    break;
  case SPELL_SUMMON_MONSTER_I:
  case SPELL_SUMMON_MONSTER_II:
  case SPELL_SUMMON_MONSTER_III:
  case SPELL_SUMMON_MONSTER_IV:
  case SPELL_SUMMON_MONSTER_V:
  case SPELL_SUMMON_MONSTER_VI:
  case SPELL_SUMMON_MONSTER_VII:
  case SPELL_SUMMON_MONSTER_VIII:
  case SPELL_SUMMON_MONSTER_IX:
    af[0].duration = level;
    af[0].bitvector = AFF_SUMMONED;
    accum_duration = FALSE;
    to_vict = "You are summoned to assist $N!";
    to_room = "$n appears, ready for action.";
    break;
  }

  /*
   * If this is a mob that has this affect set in its mob file, do not
   * perform the affect.  This prevents people from un-sancting mobs
   * by sancting them and waiting for it to fade, for example.
   */
  if (IS_NPC(victim) && !affected_by_spell(victim, spellnum))
    for (i = 0; i < MAX_SPELL_AFFECTS; i++)
      if (AFF_FLAGGED(victim, af[i].bitvector)) {
        send_to_char(ch, "%s", CONFIG_NOEFFECT);
        return;
      }
  /*
   * If the victim is already affected by this spell, and the spell does
   * not have an accumulative effect, then fail the spell.
   */
  if (affected_by_spell(victim,spellnum) && !(accum_duration||accum_affect)) {
    send_to_char(ch, "%s", CONFIG_NOEFFECT);
    return;
  }

  for (i = 0; i < MAX_SPELL_AFFECTS; i++)
    if (af[i].bitvector || (af[i].location != APPLY_NONE))
      affectv_join(victim, af+i, accum_duration, FALSE, accum_affect, FALSE);

  if (to_vict != NULL)
    act(to_vict, FALSE, victim, 0, ch, TO_CHAR);
  if (to_room != NULL)
    act(to_room, TRUE, victim, 0, ch, TO_ROOM);
}

