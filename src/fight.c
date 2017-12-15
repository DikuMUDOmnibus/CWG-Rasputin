/* ************************************************************************
*   File: fight.c                                       Part of CircleMUD *
*  Usage: Combat system                                                   *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/fight.c,v 1.20 2005/01/05 16:27:27 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "handler.h"
#include "interpreter.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "feats.h"

/* Structures */
struct char_data *combat_list = NULL;	/* head of l-list of fighting chars */
struct char_data *next_combat_list = NULL;

/* External structures */
extern struct message_list fight_messages[MAX_MESSAGES];

/* External procedures */
char *fread_action(FILE *fl, int nr);
ACMD(do_flee);
ACMD(do_get);
ACMD(do_split);
ACMD(do_sac);
int backstab_dice(struct char_data *ch);
int num_attacks(struct char_data *ch, int offhand);
int ok_damage_shopkeeper(struct char_data *ch, struct char_data *victim);
int obj_savingthrow(int material, int type);
void perform_remove(struct char_data *ch, int pos);
void Crash_rentsave(struct char_data *ch, int cost);

/* local functions */
void perform_group_gain(struct char_data *ch, int base, struct char_data *victim);
void dam_message(int dam, struct char_data *ch, struct char_data *victim, int w_type, int is_crit, int is_reduc);
void appear(struct char_data *ch);
void load_messages(void);
void free_messages(void);
void free_messages_type(struct msg_type *msg);
void check_killer(struct char_data *ch, struct char_data *vict);
void make_corpse(struct char_data *ch);
void change_alignment(struct char_data *ch, struct char_data *victim);
void death_cry(struct char_data *ch);
void raw_kill(struct char_data * ch, struct char_data * killer);
void die(struct char_data * ch, struct char_data * killer);
void group_gain(struct char_data *ch, struct char_data *victim);
void solo_gain(struct char_data *ch, struct char_data *victim);
char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural);
void perform_violence(void);
int compute_armor_class(struct char_data *ch, struct char_data *att);
int compute_base_hit(struct char_data *ch);
struct char_data *find_next_victim(struct char_data *ch);

/* Weapon attack texts */
struct attack_hit_type attack_hit_text[] =
{
  {"hit", "hits"},		/* 0 */
  {"sting", "stings"},
  {"whip", "whips"},
  {"slash", "slashes"},
  {"bite", "bites"},
  {"bludgeon", "bludgeons"},	/* 5 */
  {"crush", "crushes"},
  {"pound", "pounds"},
  {"claw", "claws"},
  {"maul", "mauls"},
  {"thrash", "thrashes"},	/* 10 */
  {"pierce", "pierces"},
  {"blast", "blasts"},
  {"punch", "punches"},
  {"stab", "stabs"}
};

#define IS_WEAPON(type) (((type) >= TYPE_HIT) && ((type) < TYPE_SUFFERING))

/* The Fight related routines */

void appear(struct char_data *ch)
{
  if (affected_by_spell(ch, SPELL_INVISIBLE))
    affect_from_char(ch, SPELL_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_INVISIBLE);

  if (AFF_FLAGGED(ch, AFF_HIDE))
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_HIDE);

  act("$n slowly fades into existence.", FALSE, ch, 0, 0, TO_ROOM);
}


int class_armor_bonus(struct char_data *ch)
{
  int i;

  if ((i = GET_CLASS_RANKS(ch, CLASS_MONK)) && !GET_EQ(ch, WEAR_BODY)) {
    i /= 5;
    i += ability_mod_value(GET_WIS(ch));
    if (i > 0)
      return i * 10;
  }
  return 0;
}

#define BASE_AC 100

int compute_armor_class(struct char_data *ch, struct char_data *att)
{
  int armorclass = GET_ARMOR(ch);

  armorclass += class_armor_bonus(ch);
  armorclass += BASE_AC;

  armorclass += get_size_bonus(get_size(ch)) * 10;

  if (IS_AFFECTED(ch, AFF_STUNNED))
    armorclass -= 20;
  else if (GET_POS(ch) == POS_FIGHTING) {
    armorclass += ability_mod_value(GET_DEX(ch)) * 10;
    if (att == FIGHTING(ch) && HAS_FEAT(ch, FEAT_DODGE))
      armorclass += 10;
  }

  return armorclass;
}


void free_messages_type(struct msg_type *msg)
{
  if (msg->attacker_msg)	free(msg->attacker_msg);
  if (msg->victim_msg)		free(msg->victim_msg);
  if (msg->room_msg)		free(msg->room_msg);
}


void free_messages(void)
{
  int i;

  for (i = 0; i < MAX_MESSAGES; i++)
    while (fight_messages[i].msg) {
      struct message_type *former = fight_messages[i].msg;

      free_messages_type(&former->die_msg);
      free_messages_type(&former->miss_msg);
      free_messages_type(&former->hit_msg);
      free_messages_type(&former->god_msg);

      fight_messages[i].msg = fight_messages[i].msg->next;
      free(former);
    }
}


void load_messages(void)
{
  FILE *fl;
  int i, type;
  struct message_type *messages;
  char chk[128];

  if (!(fl = fopen(MESS_FILE, "r"))) {
    log("SYSERR: Error reading combat message file %s: %s", MESS_FILE, strerror(errno));
    exit(1);
  }

  for (i = 0; i < MAX_MESSAGES; i++) {
    fight_messages[i].a_type = 0;
    fight_messages[i].number_of_attacks = 0;
    fight_messages[i].msg = NULL;
  }

  fgets(chk, 128, fl);
  while (!feof(fl) && (*chk == '\n' || *chk == '*'))
    fgets(chk, 128, fl);

  while (*chk == 'M') {
    fgets(chk, 128, fl);
    sscanf(chk, " %d\n", &type);
    for (i = 0; (i < MAX_MESSAGES) && (fight_messages[i].a_type != type) &&
	 (fight_messages[i].a_type); i++);
    if (i >= MAX_MESSAGES) {
      log("SYSERR: Too many combat messages.  Increase MAX_MESSAGES and recompile.");
      exit(1);
    }
    CREATE(messages, struct message_type, 1);
    fight_messages[i].number_of_attacks++;
    fight_messages[i].a_type = type;
    messages->next = fight_messages[i].msg;
    fight_messages[i].msg = messages;

    messages->die_msg.attacker_msg = fread_action(fl, i);
    messages->die_msg.victim_msg = fread_action(fl, i);
    messages->die_msg.room_msg = fread_action(fl, i);
    messages->miss_msg.attacker_msg = fread_action(fl, i);
    messages->miss_msg.victim_msg = fread_action(fl, i);
    messages->miss_msg.room_msg = fread_action(fl, i);
    messages->hit_msg.attacker_msg = fread_action(fl, i);
    messages->hit_msg.victim_msg = fread_action(fl, i);
    messages->hit_msg.room_msg = fread_action(fl, i);
    messages->god_msg.attacker_msg = fread_action(fl, i);
    messages->god_msg.victim_msg = fread_action(fl, i);
    messages->god_msg.room_msg = fread_action(fl, i);
    fgets(chk, 128, fl);
    while (!feof(fl) && (*chk == '\n' || *chk == '*'))
      fgets(chk, 128, fl);
  }

  fclose(fl);
}


void update_pos(struct char_data *victim)
{
  if ((GET_HIT(victim) > 0) && (GET_POS(victim) > POS_STUNNED))
    return;
  else if (GET_HIT(victim) > 0)
    GET_POS(victim) = POS_STANDING;
  else if (GET_HIT(victim) <= -11)
    GET_POS(victim) = POS_DEAD;
  else if (GET_HIT(victim) <= -6)
    GET_POS(victim) = POS_MORTALLYW;
  else if (GET_HIT(victim) <= -3)
    GET_POS(victim) = POS_INCAP;
  else
    GET_POS(victim) = POS_STUNNED;
}


void check_killer(struct char_data *ch, struct char_data *vict)
{
  if (PLR_FLAGGED(vict, PLR_KILLER) || PLR_FLAGGED(vict, PLR_THIEF))
    return;
  if (PLR_FLAGGED(ch, PLR_KILLER) || IS_NPC(ch) || IS_NPC(vict) || ch == vict)
    return;

  SET_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
  send_to_char(ch, "If you want to be a PLAYER KILLER, so be it...\r\n");
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "PC Killer bit set on %s for initiating attack on %s at %s.",
	    GET_NAME(ch), GET_NAME(vict), world[IN_ROOM(vict)].name);
}


/* start one char fighting another (yes, it is horrible, I know... )  */
void set_fighting(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    return;

  if (FIGHTING(ch)) {
    core_dump();
    return;
  }

  ch->next_fighting = combat_list;
  combat_list = ch;

  if (AFF_FLAGGED(ch, AFF_SLEEP))
    affect_from_char(ch, SPELL_SLEEP);

  FIGHTING(ch) = vict;
  GET_POS(ch) = POS_FIGHTING;

  if (!CONFIG_PK_ALLOWED)
    check_killer(ch, vict);
}



/* remove a char from the list of fighting chars */
void stop_fighting(struct char_data *ch)
{
  struct char_data *temp;

  if (ch == next_combat_list)
    next_combat_list = ch->next_fighting;

  REMOVE_FROM_LIST(ch, combat_list, next_fighting);
  ch->next_fighting = NULL;
  FIGHTING(ch) = NULL;
  GET_POS(ch) = POS_STANDING;
  update_pos(ch);
}



void make_corpse(struct char_data *ch)
{
  char buf2[MAX_NAME_LENGTH + 64];
  struct obj_data *corpse, *o;
  struct obj_data *money;
  int i, x, y;

  corpse = create_obj();

  corpse->item_number = NOTHING;
  IN_ROOM(corpse) = NOWHERE;
  corpse->name = strdup("corpse");

  snprintf(buf2, sizeof(buf2), "The corpse of %s is lying here.", GET_NAME(ch));
  corpse->description = strdup(buf2);

  snprintf(buf2, sizeof(buf2), "the corpse of %s", GET_NAME(ch));
  corpse->short_description = strdup(buf2);

  GET_OBJ_TYPE(corpse) = ITEM_CONTAINER;
  for(x = y = 0; x < EF_ARRAY_MAX || y < TW_ARRAY_MAX; x++, y++) {
    if (x < EF_ARRAY_MAX)
      GET_OBJ_EXTRA_AR(corpse, x) = 0;
    if (y < TW_ARRAY_MAX)
      corpse->wear_flags[y] = 0;
  }
  SET_BIT_AR(GET_OBJ_WEAR(corpse), ITEM_WEAR_TAKE);
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_NODONATE);
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CAPACITY) = 0;	/* You can't store stuff in a corpse */
  GET_OBJ_VAL(corpse, VAL_CONTAINER_CORPSE) = 1;	/* corpse identifier */
  GET_OBJ_VAL(corpse, VAL_CONTAINER_OWNER) = GET_PFILEPOS(ch);	/* corpse identifier */
  GET_OBJ_WEIGHT(corpse) = GET_WEIGHT(ch) + IS_CARRYING_W(ch);
  GET_OBJ_RENT(corpse) = 100000;
  if (IS_NPC(ch))
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_NPC_CORPSE_TIME;
  else
    GET_OBJ_TIMER(corpse) = CONFIG_MAX_PC_CORPSE_TIME;
  SET_BIT_AR(GET_OBJ_EXTRA(corpse), ITEM_UNIQUE_SAVE);

  /* transfer character's inventory to the corpse */
  corpse->contains = ch->carrying;
  for (o = corpse->contains; o != NULL; o = o->next_content)
    o->in_obj = corpse;
  object_list_new_owner(corpse, NULL);

  /* transfer character's equipment to the corpse */
  for (i = 0; i < NUM_WEARS; i++)
    if (GET_EQ(ch, i)) {
      remove_otrigger(GET_EQ(ch, i), ch);
      obj_to_obj(unequip_char(ch, i), corpse);
    }

  /* transfer gold */
  if (GET_GOLD(ch) > 0) {
    /*
     * following 'if' clause added to fix gold duplication loophole
     * The above line apparently refers to the old "partially log in,
     * kill the game character, then finish login sequence" duping
     * bug. The duplication has been fixed (knock on wood) but the
     * test below shall live on, for a while. -gg 3/3/2002
     */
    if (IS_NPC(ch) || ch->desc) {
      money = create_money(GET_GOLD(ch));
      obj_to_obj(money, corpse);
    }
    GET_GOLD(ch) = 0;
  }
  ch->carrying = NULL;
  IS_CARRYING_N(ch) = 0;
  IS_CARRYING_W(ch) = 0;

  obj_to_room(corpse, IN_ROOM(ch));

  if (!IS_NPC(ch))
    Crash_rentsave(ch, 0);
}


/* When ch kills victim */
void change_alignment(struct char_data *ch, struct char_data *victim)
{
  int div = 20;
  /*
   * If you kill a monster with alignment A, you move 1/20th of the way to
   * having alignment -A.
   * Ethical alignments of killer and victim make this faster or slower.
   */
  if (IS_LAWFUL(ch)) {
    if (IS_LAWFUL(victim))
      div /= 2;
    else
      div = div * 2 / 3;
  } else if (IS_CHAOTIC(ch)) {
    if (IS_LAWFUL(victim))
      div *= 2;
    else
      div = div * 3 / 2;
  }
  
  GET_ALIGNMENT(ch) += (-GET_ALIGNMENT(victim) - GET_ALIGNMENT(ch)) / div;
}



void death_cry(struct char_data *ch)
{
  int door;

  act("Your blood freezes as you hear $n's death cry.", FALSE, ch, 0, 0, TO_ROOM);

  for (door = 0; door < NUM_OF_DIRS; door++)
    if (CAN_GO(ch, door))
      send_to_room(world[IN_ROOM(ch)].dir_option[door]->to_room, "Your blood freezes as you hear someone's death cry.\r\n");
}



void raw_kill(struct char_data * ch, struct char_data * killer)
{
  struct affected_type af;
  struct char_data *k, *temp;

  if (FIGHTING(ch))
    stop_fighting(ch);

  while (ch->affected)
    affect_remove(ch, ch->affected);

  while (ch->affectedv)
    affectv_remove(ch, ch->affectedv);

  /* To make ordinary commands work in scripts.  welcor*/  
  GET_POS(ch) = POS_STANDING; 
  
  if (killer) {
    if (death_mtrigger(ch, killer))
      death_cry(ch);
  } else
  death_cry(ch);

  update_pos(ch);

  if (IS_NPC(ch)) {
    make_corpse(ch);
    extract_char(ch);
  } else if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    /* Something killed your spirit. Doh! */
    extract_char(ch);
  } else {
    make_corpse(ch);
    if (FIGHTING(ch))
      stop_fighting(ch);

    for (k = combat_list; k; k = temp) {
      temp = k->next_fighting;
      if (FIGHTING(k) == ch)
        stop_fighting(k);
    }
    /* we can't forget the hunters either... */
    for (temp = character_list; temp; temp = temp->next)
      if (HUNTING(temp) == ch)
        HUNTING(temp) = NULL;

    af.type = -1;
    af.duration = -1;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.specific = 0;
    af.bitvector = AFF_SPIRIT;
    affect_to_char(ch, &af);
    af.bitvector = AFF_ETHEREAL;
    affect_to_char(ch, &af);
    GET_HIT(ch) = 1;
    update_pos(ch);
    save_char(ch);
    Crash_delete_crashfile(ch);
    WAIT_STATE(ch, PULSE_VIOLENCE);
  }
}



void die(struct char_data * ch, struct char_data * killer)
{
  gain_exp(ch, -(GET_EXP(ch) / 2));
  if (!IS_NPC(ch)) {
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_KILLER);
    REMOVE_BIT_AR(PLR_FLAGS(ch), PLR_THIEF);
  }
  raw_kill(ch, killer);
}



void perform_group_gain(struct char_data *ch, int base,
			     struct char_data *victim)
{
  int share;

  share = MIN(CONFIG_MAX_EXP_GAIN, MAX(1, base * GET_LEVEL(ch)));

  if (share > 1)
    send_to_char(ch, "You receive your share of experience -- %.0f points.\r\n", share*CONFIG_EXP_MULTIPLIER);
  else
    send_to_char(ch, "You receive your share of experience -- one measly little point!\r\n");

  gain_exp(ch, share);
  change_alignment(ch, victim);
}

void group_gain(struct char_data *ch, struct char_data *victim)
{
  int tot_levels, tot_members, base, tot_gain;
  struct char_data *k;
  struct follow_type *f;

  if (!(k = ch->master))
    k = ch;

  if (AFF_FLAGGED(k, AFF_GROUP) && (IN_ROOM(k) == IN_ROOM(ch))) {
    tot_levels = GET_LEVEL(k);
    tot_members = 1;
  } else {
    tot_levels = 0;
    tot_members = 0;
  }

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch)) {
      tot_levels += GET_LEVEL(f->follower);
      tot_members++;
    }

  /* round up to the next highest tot_members */
  tot_gain = (GET_EXP(victim) / 3) + tot_members - 1;

  /* prevent illegal xp creation when killing players */
  if (!IS_NPC(victim))
    tot_gain = MIN(CONFIG_MAX_EXP_LOSS * 2 / 3, tot_gain);

  if (tot_levels >= 1)
    base = MAX(1, tot_gain / tot_levels);
  else
    base = 0;

  if (AFF_FLAGGED(k, AFF_GROUP) && IN_ROOM(k) == IN_ROOM(ch))
    perform_group_gain(k, base, victim);

  for (f = k->followers; f; f = f->next)
    if (AFF_FLAGGED(f->follower, AFF_GROUP) && IN_ROOM(f->follower) == IN_ROOM(ch))
      perform_group_gain(f->follower, base, victim);
}


void solo_gain(struct char_data *ch, struct char_data *victim)
{
  int exp;

  exp = MIN(CONFIG_MAX_EXP_GAIN, GET_EXP(victim) / 3);

  /* Calculate level-difference bonus */
  if (IS_NPC(ch))
    exp += MAX(0, (exp * MIN(4, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);
  else
    exp += MAX(0, (exp * MIN(8, (GET_LEVEL(victim) - GET_LEVEL(ch)))) / 8);

  exp = MAX(exp, 1);

  if (exp > 1)
    send_to_char(ch, "You receive %.0f experience points.\r\n", exp*CONFIG_EXP_MULTIPLIER);
  else
    send_to_char(ch, "You receive one lousy experience point.\r\n");

  gain_exp(ch, exp);
  change_alignment(ch, victim);
}


char *replace_string(const char *str, const char *weapon_singular, const char *weapon_plural)
{
  static char buf[MAX_STRING_LENGTH];
  char *cp = buf;

  for (; *str; str++) {
    if (*str == '#') {
      switch (*(++str)) {
      case 'W':
	for (; *weapon_plural; *(cp++) = *(weapon_plural++));
	break;
      case 'w':
	for (; *weapon_singular; *(cp++) = *(weapon_singular++));
	break;
      default:
	*(cp++) = '#';
	break;
      }
    } else
      *(cp++) = *str;

    *cp = 0;
  }				/* For */

  return (buf);
}


/* message for doing damage with a weapon */
void dam_message(int dam, struct char_data *ch, struct char_data *victim,
		      int w_type, int is_crit, int is_reduc)
{
  char *buf;
  int msgnum;

  static struct dam_weapon_type {
    const char *to_room;
    const char *to_char;
    const char *to_victim;
  } dam_weapons[] = {

    /* use #w for singular (i.e. "slash") and #W for plural (i.e. "slashes") */

    {
      "$n tries to #w $N@n, but misses.@n",	/* 0: 0     */
      "@nYou try to #w $N@n, but miss.@n",
      "$n@n tries to #w you, but misses.@n"
    },

    {
      "@[6]$n tickles $N@[6] as $e #W $M.@n",	/* 1: 1..2  */
      "@[5]You tickle $N@[5] as you #w $M.@n",
      "@[4]$n@[4] tickles you as $e #W you.@n"
    },

    {
      "@[6]$n barely #W $N@[6].@n",		/* 2: 3..4  */
      "@[5]You barely #w $N@[5].@n",
      "@[4]$n@[4] barely #W you.@n"
    },

    {
      "@[6]$n #W $N@[6].@n",			/* 3: 5..6  */
      "@[5]You #w $N@[5].@n",
      "@[4]$n@[4] #W you.@n"
    },

    {
      "@[6]$n #W $N@[6] hard.@n",			/* 4: 7..10  */
      "@[5]You #w $N@[5] hard.@n",
      "@[4]$n@[4] #W you hard.@n"
    },

    {
      "@[6]$n #W $N@[6] very hard.@n",		/* 5: 11..14  */
      "@[5]You #w $N@[5] very hard.@n",
      "@[4]$n@[4] #W you very hard.@n"
    },

    {
      "@[6]$n #W $N@[6] extremely hard.@n",	/* 6: 15..19  */
      "@[5]You #w $N@[5] extremely hard.@n",
      "@[4]$n@[4] #W you extremely hard.@n"
    },

    {
      "@[6]$n massacres $N@[6] to small fragments with $s #w.@n",	/* 7: 19..23 */
      "@[5]You massacre $N@[5] to small fragments with your #w.@n",
      "@[4]$n@[4] massacres you to small fragments with $s #w.@n"
    },

    {
      "@[6]$n OBLITERATES $N@[6] with $s deadly #w!!@n",	/* 8: > 23   */
      "@[5]You OBLITERATE $N@[5] with your deadly #w!!@n",
      "@[4]$n@[4] OBLITERATES you with $s deadly #w!!@n"
    }
  };


  w_type -= TYPE_HIT;		/* Change to base of table with text */

  if (dam == 0)		msgnum = 0;
  else if (dam <= 2)    msgnum = 1;
  else if (dam <= 4)    msgnum = 2;
  else if (dam <= 6)    msgnum = 3;
  else if (dam <= 10)   msgnum = 4;
  else if (dam <= 14)   msgnum = 5;
  else if (dam <= 19)   msgnum = 6;
  else if (dam <= 23)   msgnum = 7;
  else			msgnum = 8;

  /* damage message to onlookers */
  buf = replace_string(dam_weapons[msgnum].to_room,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  if (is_crit)
    strcat(buf, " @[7]*critical hit*@n");
  if (is_reduc)
    strcat(buf, " @[7]*reduced*@n");
  act(buf, FALSE, ch, NULL, victim, TO_NOTVICT);

  /* damage message to damager */
  buf = replace_string(dam_weapons[msgnum].to_char,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  if (is_crit)
    strcat(buf, " @[7]*critical hit*@n");
  if (is_reduc)
    strcat(buf, " @[7]*reduced*@n");
  act(buf, FALSE, ch, NULL, victim, TO_CHAR);

  /* damage message to damagee */
  buf = replace_string(dam_weapons[msgnum].to_victim,
	  attack_hit_text[w_type].singular, attack_hit_text[w_type].plural);
  if (is_crit)
    strcat(buf, " @[7]*critical hit*@n");
  if (is_reduc)
    strcat(buf, " @[7]*reduced*@n");
  act(buf, FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
}


/*
 *  message for doing damage with a spell or skill
 *  C3.0: Also used for weapon damage on miss and death blows
 */
int skill_message(int dam, struct char_data *ch, struct char_data *vict,
		      int attacktype, int is_reduc)
{
  int i, j, nr;
  struct message_type *msg;
  char buf[MAX_STRING_LENGTH];

  struct obj_data *weap = GET_EQ(ch, WEAR_WIELD1);

  for (i = 0; i < MAX_MESSAGES; i++) {
    if (fight_messages[i].a_type == attacktype) {
      nr = dice(1, fight_messages[i].number_of_attacks);
      for (j = 1, msg = fight_messages[i].msg; (j < nr) && msg; j++)
	msg = msg->next;

      if (!IS_NPC(vict) && ADM_FLAGGED(vict, ADM_NODAMAGE)) {
	act(msg->god_msg.attacker_msg, FALSE, ch, weap, vict, TO_CHAR);
	act(msg->god_msg.victim_msg, FALSE, ch, weap, vict, TO_VICT);
	act(msg->god_msg.room_msg, FALSE, ch, weap, vict, TO_NOTVICT);
      } else if (dam != 0) {
        /*
         * Don't send redundant color codes for TYPE_SUFFERING & other types
         * of damage without attacker_msg.
         */
	if (GET_POS(vict) == POS_DEAD) {
          if (msg->die_msg.attacker_msg) {
            snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->die_msg.attacker_msg,
                     is_reduc ? " @[7]*reduced*" : "");
            act(buf, FALSE, ch, weap, vict, TO_CHAR);
          }

          snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->die_msg.victim_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);

          snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->die_msg.room_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, FALSE, ch, weap, vict, TO_NOTVICT);
	} else {
          if (msg->hit_msg.attacker_msg) {
            snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->hit_msg.attacker_msg,
                     is_reduc ? " @[7]*reduced*" : "");
	    act(buf, FALSE, ch, weap, vict, TO_CHAR);
          }

          snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->hit_msg.victim_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);

          snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->hit_msg.room_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, FALSE, ch, weap, vict, TO_NOTVICT);
	}
      } else if (ch != vict) {	/* Dam == 0 */
        if (msg->miss_msg.attacker_msg) {
          snprintf(buf, sizeof(buf), "@[5]%s%s@n", msg->miss_msg.attacker_msg,
                   is_reduc ? " @[7]*reduced*" : "");
	  act(buf, FALSE, ch, weap, vict, TO_CHAR);
        }

        snprintf(buf, sizeof(buf), "@[4]%s%s@n", msg->miss_msg.victim_msg,
                 is_reduc ? " @[7]*reduced*" : "");
	act(buf, FALSE, ch, weap, vict, TO_VICT | TO_SLEEP);

        snprintf(buf, sizeof(buf), "@[6]%s%s@n", msg->miss_msg.room_msg,
                 is_reduc ? " @[7]*reduced*" : "");
	act(buf, FALSE, ch, weap, vict, TO_NOTVICT);
      }
      return (1);
    }
  }
  return (0);
}

void damage_object(struct char_data *ch, struct char_data *victim)
{
  /* function needs to do two things, attacker's weapon could take damage
     and the attackee could take damage to their armor. */

  struct obj_data *object = NULL;

  int dnum, rnum, snum, wnum;

  object = GET_EQ(ch, WEAR_WIELD1);

  if (object)
    snum = GET_SKILL(ch, GET_OBJ_VAL(object, VAL_WEAPON_SKILL));
  else
    snum = GET_SKILL(ch, SKILL_WP_UNARMED);

  rnum = rand_number(1, 101);

  if (object) {
    if (rnum > snum) {
      if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) &&
          !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
	dnum = dice(1, GET_STR(ch)) - dice(1, GET_DEX(victim));
        if (dnum < 1)
          dnum = 1;
        GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) = GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) - dnum;
        if (GET_OBJ_VAL(object, VAL_WEAPON_HEALTH) < 0) {
          TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
          send_to_char(ch, "Your %s has broken beyond use!\r\n", object->short_description);
          perform_remove(ch, WEAR_WIELD1);
        }
      }
    }
  }

  snum = GET_DEX(victim);

  object = GET_EQ(victim, WEAR_BODY);

  rnum = rand_number(1, 20);

  if (rand_number(1,100) < 70) {
    if (object) {
      if (rnum > snum) {
        if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
            !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
          GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) = GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) - GET_STR(ch);
          if (GET_OBJ_VAL(object, VAL_ARMOR_HEALTH) < 0) {
            TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
            send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
            perform_remove(victim, WEAR_BODY);
          }
        }
      }
    } else {
      wnum = rand_number(0, NUM_WEARS-1);
      object = GET_EQ(victim, wnum);
      if (object) {
        if (rnum > snum) {
          if (!obj_savingthrow(GET_OBJ_MATERIAL(object), SAVING_OBJ_IMPACT) && \
              !OBJ_FLAGGED(object, ITEM_UNBREAKABLE)) {
            GET_OBJ_VAL(object, VAL_ALL_HEALTH) = GET_OBJ_VAL(object, VAL_ALL_HEALTH) - GET_STR(ch);
            if (GET_OBJ_VAL(object, VAL_ALL_HEALTH) < 0) {
              TOGGLE_BIT_AR(GET_OBJ_EXTRA(object), ITEM_BROKEN);
              send_to_char(victim, "Your %s has broken beyond use!\r\n", object->short_description);
              perform_remove(victim, wnum);
            }
          }
        }
      }
    }
  }

  return;
}

/*
 * Alert: As of bpl14, this function returns the following codes:
 *	< 0	Victim died.
 *	= 0	No damage.
 *	> 0	How much damage done.
 */
int damage(struct char_data *ch, struct char_data *victim, int dam, int attacktype, int is_crit, int material, int bonus, int spell, int magic)
{
  int reduction = 0, rtest, passed;
  struct damreduct_type *reduct;
  long local_gold = 0;
  char local_buf[256];
  struct char_data *tmp_char;
  struct obj_data *corpse_obj, *coin_obj, *next_obj;

  if (GET_POS(victim) <= POS_DEAD) {
    /* This is "normal"-ish now with delayed extraction. -gg 3/15/2001 */
    if (PLR_FLAGGED(victim, PLR_NOTDEADYET) || MOB_FLAGGED(victim, MOB_NOTDEADYET))
      return (-1);

    log("SYSERR: Attempt to damage corpse '%s' in room #%d by '%s'.",
		GET_NAME(victim), GET_ROOM_VNUM(IN_ROOM(victim)), GET_NAME(ch));
    die(victim, ch);
    return (-1);			/* -je, 7/7/92 */
  }

  /* peaceful rooms */
  if (GET_MOB_VNUM(ch) != DG_CASTER_PROXY && GET_ADMLEVEL(ch) < ADMLVL_IMPL &&
      ch != victim && ROOM_FLAGGED(IN_ROOM(ch), ROOM_PEACEFUL)) {
    send_to_char(ch, "This room just has such a peaceful, easy feeling...\r\n");
    return (0);
  }

  /* shopkeeper protection */
  if (!ok_damage_shopkeeper(ch, victim))
    return (0);

  /* You can't damage an immortal! */
  if (!IS_NPC(victim) && ADM_FLAGGED(victim, ADM_NODAMAGE))
    dam = 0;

  /* Mobs with a NOKILL flag */
  if (MOB_FLAGGED(victim, MOB_NOKILL)) {
    send_to_char(ch, "But they are not to be killed!\r\n");
    return (0);
  }

  if (victim != ch) {
    /* Start the attacker fighting the victim */
    if (GET_POS(ch) > POS_STUNNED && (FIGHTING(ch) == NULL))
      set_fighting(ch, victim);

    /* Start the victim fighting the attacker */
    if (GET_POS(victim) > POS_STUNNED && (FIGHTING(victim) == NULL)) {
      set_fighting(victim, ch);
      if (MOB_FLAGGED(victim, MOB_MEMORY) && !IS_NPC(ch))
	remember(victim, ch);
    }
  }

  /* If you attack a pet, it hates your guts */
  if (victim->master == ch)
    stop_follower(victim);

  /* If the attacker is invisible, he becomes visible */
  if (AFF_FLAGGED(ch, AFF_INVISIBLE) || AFF_FLAGGED(ch, AFF_HIDE))
    appear(ch);

  /* Cut damage in half if victim has sanct, to a minimum 1 */
  if (victim != ch && AFF_FLAGGED(victim, AFF_SANCTUARY) && dam >= 2)
    dam /= 2;

  /* Check for PK if this is not a PK MUD */
  if (!CONFIG_PK_ALLOWED) {
    check_killer(ch, victim);
    if (PLR_FLAGGED(ch, PLR_KILLER) && (ch != victim))
      dam = 0;
  }

  if (dam && GET_CLASS_RANKS(victim, CLASS_MONK) > 19) {
    if (!spell && !magic)
      reduction = 10;
  }

  if (dam && attacktype != TYPE_SUFFERING) {
    for (reduct = victim->damreduct; reduct && reduction > -1; reduct = reduct->next) {
      passed = 0;
      if (reduct->mod > reduction || reduct->mod == -1) {
        for (rtest = 0; !passed && rtest < MAX_DAMREDUCT_MULTI; rtest++)
          if (reduct->damstyle[rtest] != DR_NONE)
            switch (reduct->damstyle[rtest]) {
            case DR_ADMIN:
              if (GET_ADMLEVEL(ch) >= reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_MATERIAL:
              if (material == reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_BONUS:
              if (bonus >= reduct->damstyleval[rtest])
                passed = 1;
              break;
            case DR_SPELL:
              if (!reduct->damstyleval[rtest] && spell > 0)
                passed = 1;
              if (spell == reduct->damstyleval[rtest] && spell)
                passed = 1;
              break;
            case DR_MAGICAL:
              if (magic)
                passed = 1;
              break;
            default:
              log("Unknown DR exception type %d", reduct->damstyle[rtest]);
              continue;
            }
        if (!passed)
          reduction = reduct->mod;
      }
    }
  }

  if (reduction == -1) /* Special - Full reduction */
    reduction = dam;

  if (reduction)
    log("dam total %s -> %s = %d (%d reduction)", GET_NAME(ch), GET_NAME(victim), dam, reduction);

  dam -= reduction;

  /* Set the maximum damage per round and subtract the hit points */
  dam = MAX(MIN(dam, 1000), 0);
  GET_HIT(victim) -= dam;

  update_pos(victim);

  /*
   * skill_message sends a message from the messages file in lib/misc.
   * dam_message just sends a generic "You hit $n extremely hard.".
   * skill_message is preferable to dam_message because it is more
   * descriptive.
   * 
   * If we are _not_ attacking with a weapon (i.e. a spell), always use
   * skill_message. If we are attacking with a weapon: If this is a miss or a
   * death blow, send a skill_message if one exists; if not, default to a
   * dam_message. Otherwise, always send a dam_message.
   */
  if (!IS_WEAPON(attacktype))
    skill_message(dam, ch, victim, attacktype, reduction > 0);
  else {
    if (GET_POS(victim) == POS_DEAD || dam == 0) {
      if (!skill_message(dam, ch, victim, attacktype, reduction > 0))
	dam_message(dam, ch, victim, attacktype, is_crit, reduction > 0);
    } else {
      dam_message(dam, ch, victim, attacktype, is_crit, reduction > 0);
    }
  }

  /* Use send_to_char -- act() doesn't send message if you are DEAD. */
  switch (GET_POS(victim)) {
  case POS_MORTALLYW:
    act("$n is mortally wounded, and will die soon, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are mortally wounded, and will die soon, if not aided.\r\n");
    break;
  case POS_INCAP:
    act("$n is incapacitated and will slowly die, if not aided.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are incapacitated and will slowly die, if not aided.\r\n");
    break;
  case POS_STUNNED:
    act("$n is stunned, but will probably regain consciousness again.", TRUE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You're stunned, but will probably regain consciousness again.\r\n");
    break;
  case POS_DEAD:
    act("$n is dead!  R.I.P.", FALSE, victim, 0, 0, TO_ROOM);
    send_to_char(victim, "You are dead!  Sorry...\r\n\r\nIf you have no other way of resurrecting, you may use the 'resurrect' command,\r\nbut that guarantees an experience penalty.\r\n");
    break;

  default:			/* >= POSITION SLEEPING */
    if (dam > (GET_MAX_HIT(victim) / 4))
      send_to_char(victim, "That really did HURT!\r\n");

    if (GET_HIT(victim) < (GET_MAX_HIT(victim) / 4)) {
      send_to_char(victim, "@[4]You wish that your wounds would stop BLEEDING so much!@n\r\n");
      if (ch != victim && MOB_FLAGGED(victim, MOB_WIMPY))
	do_flee(victim, NULL, 0, 0);
    }
    if (!IS_NPC(victim) && GET_WIMP_LEV(victim) && (victim != ch) &&
	GET_HIT(victim) < GET_WIMP_LEV(victim) && GET_HIT(victim) > 0) {
      send_to_char(victim, "You wimp out, and attempt to flee!\r\n");
      do_flee(victim, NULL, 0, 0);
    }
    break;
  }

  /* Help out poor linkless people who are attacked */
  if (!IS_NPC(victim) && !(victim->desc) && GET_POS(victim) > POS_STUNNED) {
    do_flee(victim, NULL, 0, 0);
    if (!FIGHTING(victim)) {
      act("$n is rescued by divine forces.", FALSE, victim, 0, 0, TO_ROOM);
      GET_WAS_IN(victim) = IN_ROOM(victim);
      char_from_room(victim);
      char_to_room(victim, 0);
    }
  }

  /* stop someone from fighting if they're stunned or worse */
  if (GET_POS(victim) <= POS_STUNNED && FIGHTING(victim) != NULL)
    stop_fighting(victim);

  /* Uh oh.  Victim died. */
  if (GET_POS(victim) == POS_DEAD) {
    if (ch != victim && (IS_NPC(victim) || victim->desc)) {
      if (AFF_FLAGGED(ch, AFF_GROUP))
	group_gain(ch, victim);
      else
        solo_gain(ch, victim);
    }

    if (!IS_NPC(victim)) {
      mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s killed by %s at %s", GET_NAME(victim), GET_NAME(ch), world[IN_ROOM(victim)].name);
      if (MOB_FLAGGED(ch, MOB_MEMORY))
	forget(ch, victim);
    }
    /* Cant determine GET_GOLD on corpse, so do now and store */
    if (IS_NPC(victim)) {
      local_gold = GET_GOLD(victim);
      sprintf(local_buf,"%ld", (long)local_gold);
    }

    die(victim, ch);
    if (IS_AFFECTED(ch, AFF_GROUP) && (local_gold > 0) &&
        PRF_FLAGGED(ch, PRF_AUTOSPLIT) ) {
      generic_find("corpse", FIND_OBJ_ROOM, ch, &tmp_char, &corpse_obj);
      if (corpse_obj) {
        for (coin_obj = corpse_obj->contains; coin_obj; coin_obj = next_obj) {
          next_obj = coin_obj->next_content;
          if (CAN_SEE_OBJ(ch, coin_obj) && isname("coin", coin_obj->name))
            extract_obj(coin_obj);
        }
        do_split(ch,local_buf,0,0);
      }
      /* need to remove the gold from the corpse */
    } else if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOGOLD)) {      do_get(ch, "all.coin corpse", 0, 0);
    }
    if (!IS_NPC(ch) && (ch != victim) && PRF_FLAGGED(ch, PRF_AUTOLOOT)) {
      do_get(ch, "all corpse", 0, 0);
    }
    if (IS_NPC(victim) && !IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOSAC)) { 
       do_sac(ch,"corpse",0,0); 
    } 
    return (-1);
  }
  return (dam);
}


/*
 * Calculate the hit bonus of the attacker.
 */
int compute_base_hit(struct char_data *ch)
{
  int calc_bonus;

  calc_bonus = GET_ACCURACY_BASE(ch) + get_size_bonus(get_size(ch));
  calc_bonus += GET_ACCURACY_MOD(ch);

  return calc_bonus;
}


/*
 * Find next appropriate target for someone who kills their current target
 */
struct char_data *find_next_victim(struct char_data *ch)
{
  struct char_data *victim;

  if (!ch || IN_ROOM(ch) == NOWHERE)
    return NULL;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
    if (GET_POS(victim) == POS_FIGHTING && FIGHTING(victim) == ch)
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) == AFF_FLAGGED(ch, AFF_ETHEREAL))
        return victim;

  if (!IS_AFFECTED(ch, AFF_GROUP))
    return NULL;

  for (victim = world[IN_ROOM(ch)].people; victim; victim = victim->next_in_room)
    if (IS_NPC(victim) && GET_POS(victim) == POS_FIGHTING &&
        (FIGHTING(victim) == ch->master ||
         FIGHTING(victim)->master == ch ||
         FIGHTING(victim)->master == ch->master))
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) == AFF_FLAGGED(ch, AFF_ETHEREAL))
        return victim;

  return NULL;
}


int dam_dice_scaling_table[][6] = {
/* old  down    up */
{1, 2,	1, 1,	1, 3},
{1, 3,	1, 2,	1, 4},
{1, 4,	1, 3,	1, 6},
{1, 6,	1, 4,	1, 8},
{1, 8,  1, 6,   2, 6},
{1,10,	1, 8,	2, 8},
{2, 6,	1,10,	3, 6},
{2, 8,	2, 6,	3, 8},
{2,10,	2, 8,	4, 8},
{0, 0,	0, 0,	0, 0}
};


void scaleup_dam(int *num, int *size)
{
  int i = 0;
  for (i = 0; dam_dice_scaling_table[i][0]; i++)
    if (dam_dice_scaling_table[i][0] == *num &&
        dam_dice_scaling_table[i][1] == *size) {
      *num = dam_dice_scaling_table[i][4];
      *size = dam_dice_scaling_table[i][5];
      return;
    }
  log("scaleup_dam: No dam_dice_scaling_table entry for %dd%d in fight.c", *num, *size);
}


void scaledown_dam(int *num, int *size)
{
  int i = 0;
  for (i = 0; dam_dice_scaling_table[i][0]; i++)
    if (dam_dice_scaling_table[i][0] == *num &&
        dam_dice_scaling_table[i][1] == *size) {
      *num = dam_dice_scaling_table[i][2];
      *size = dam_dice_scaling_table[i][3];
      return;
    }
  log("scaledown_dam: No dam_dice_scaling_table entry for %dd%d in fight.c", *num, *size);
}


int bare_hand_damage(struct char_data *ch, int code)
{
  int num, size, lvl, sz;
  int scale = 1;

  lvl = GET_CLASS_RANKS(ch, CLASS_MONK);

  if (IS_NPC(ch)) {
    num = ch->mob_specials.damnodice;
    size = ch->mob_specials.damsizedice;
    scale = 0;
  } else if (!lvl) {
    num = 1;
    size = 3;
  } else {
    num = 1;
    switch (lvl) {
    case 0:
    case 1:
    case 2:
    case 3:
      size = 6;
      break;
    case 4:
    case 5:
    case 6:
    case 7:
      size = 8;
      break;
    case 8:
    case 9:
    case 10:
    case 11:
      size = 10;
      break;
    case 12:
    case 13:
    case 14:
    case 15:
      size = 6;
      num = 2;
      break;
    case 16:
    case 17:
    case 18:
    case 19:
      size = 8;
      num = 2;
      break;
    default:
      size = 10;
      num = 2;
      break;
    }
  }

  if (scale) {
    sz = get_size(ch);
    if (sz < SIZE_MEDIUM)
      for (lvl = sz; lvl < SIZE_MEDIUM; lvl++)
        scaledown_dam(&num, &size);
    else if (sz > SIZE_MEDIUM)
      for (lvl = sz; lvl > SIZE_MEDIUM; lvl--)
        scaleup_dam(&num, &size);
  }

  if (code == 0)
    return dice(num, size);
  else if (code == 1)
    return num;
  else
    return size;
}


int crit_range_extension(struct char_data *ch, struct obj_data *weap)
{
  int ext = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_CRITRANGE) + 1 : 1; /* include 20 */
  int tp = weap ? GET_OBJ_VAL(weap, VAL_WEAPON_SKILL) : WEAPON_TYPE_UNARMED;
  int mult = 1;
  if (HAS_COMBAT_FEAT(ch, CFEAT_IMPROVED_CRITICAL, tp))
    mult++;
  return (ext * mult) - 1; /* difference from 20 */
}

int one_hit(struct char_data *ch, struct char_data *victim, struct obj_data *wielded, int w_type, int calc_base_hit, char *damstr, char *critstr, int hitbonus)
{
  int victim_ac, dam = 0, diceroll;
  int is_crit, range, damtimes, strmod, ndice, diesize, sneak;

  if (!victim)
    return -1;

  if (damstr && *damstr)
    damstr = NULL; /* Only build a damstr if we don't have one yet */

  /* Calculate the raw armor including magic armor. */
  victim_ac = compute_armor_class(victim, ch) / 10;

  /* roll the die and take your chances... */
  diceroll = rand_number(1, 20);

  /*
   * Decide whether this is a hit or a miss.
   *
   *  Victim asleep = hit, otherwise:
   *     1   = Automatic miss.
   *   2..19 = Checked vs. AC.
   *    20   = Automatic hit.
   */
  if (diceroll == 20 || !AWAKE(victim))
    dam = TRUE;
  else if (diceroll == 1)
    dam = FALSE;
  else
   dam = (diceroll + calc_base_hit) >= victim_ac;

  if (!dam) {
    /* the attacker missed the victim */
    if (ch->actq) {
      free(ch->actq);
      ch->actq = 0;
    }
    return damage(ch, victim, 0, w_type, 0, -1, 0, 0, 0);
  } else {
    /* okay, we know the guy has been hit.  now calculate damage. */

    damage_object(ch, victim);

    strmod = ability_mod_value(GET_STR(ch));

    range = 0;

    diceroll += (range = crit_range_extension(ch, wielded));
    if (wielded && wielded == GET_EQ(ch, WEAR_WIELD1) &&
        !GET_EQ(ch, WEAR_WIELD2) &&
        wield_type(get_size(ch), wielded) >= WIELD_ONEHAND &&
        !OBJ_FLAGGED(wielded, ITEM_DOUBLE))
      strmod = ability_mod_value(((GET_STR(ch) - 10) * 3 / 2) + 10);

    is_crit = 0;

    damtimes = 1;

    if (diceroll >= 20) {
      diceroll = rand_number(1, 20);
      if ((diceroll + calc_base_hit) >= victim_ac) { /* It's a critical */
      is_crit = 1;
      if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
        diceroll = GET_OBJ_VAL(wielded, VAL_WEAPON_CRITTYPE);
      } else {
        diceroll = 0; /* default to crit type 0 (x2) */
      }
      switch (diceroll) {
      case CRIT_X4:
        damtimes = 4;
        break;
      case CRIT_X3:
        damtimes = 3;
        break;
      case CRIT_X2:
        damtimes = 2;
        break;
      default:
        break;
      }
        if (critstr && !*critstr) {
          if (range)
            sprintf(critstr, "(%d-20x%d)", 20 - range, damtimes);
          else
            sprintf(critstr, "(x%d)", damtimes);
        }
      }
    }

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      ndice = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMDICE);
      diesize = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMSIZE);
    } else {
      ndice = bare_hand_damage(ch, 1);
      diesize = bare_hand_damage(ch, 2);
    }

    if (GET_POS(ch) != POS_FIGHTING && GET_POS(victim) != POS_FIGHTING)
      sneak = backstab_dice(ch);
    else
      sneak = 0;

    /* Start with the damage bonuses: the damroll and strength apply */
    dam = strmod + GET_DAMAGE_MOD(ch);
    if (HAS_FEAT(ch, FEAT_POWER_ATTACK) &&
        GET_POWERATTACK(ch) && GET_STR(ch) > 12)
      dam += GET_POWERATTACK(ch);

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        dam += 2;
    } else {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_SPECIALIZATION,
          WEAPON_TYPE_UNARMED))
        dam += 2;
    }

    if (damstr)
      sprintf(damstr, "%dd%d+%d", ndice, diesize, dam);

    dam *= damtimes;

    if (sneak) {
      dam += dice(sneak, 6);
      if (damstr)
        sprintf(damstr, "%s (+%dd6 sneak)", damstr, sneak);
    }

    while (damtimes--)
      dam += dice(ndice, diesize);

    /*
     * Include a damage multiplier if victim isn't ready to fight:
     *
     * Position sitting  1.33 x normal
     * Position resting  1.66 x normal
     * Position sleeping 2.00 x normal
     * Position stunned  2.33 x normal
     * Position incap    2.66 x normal
     * Position mortally 3.00 x normal
     *
     * Note, this is a hack because it depends on the particular
     * values of the POSITION_XXX constants.
     */
    if (GET_POS(victim) < POS_FIGHTING)
      dam *= 1 + (POS_FIGHTING - GET_POS(victim)) / 3;

    /* at least 1 hp damage min per hit */
    dam = MAX(1, dam);

    dam = damage(ch, victim, dam, w_type, is_crit,
                 wielded ?
                   GET_OBJ_VAL(wielded, VAL_WEAPON_MATERIAL) :
                   (GET_CLASS_RANKS(ch, CLASS_MONK) > 15) ?
                     MATERIAL_ADAMANTINE :
                     MATERIAL_ORGANIC,
                 hitbonus, 0,
                 wielded ?
                   OBJ_FLAGGED(wielded, ITEM_MAGIC) :
                   GET_CLASS_RANKS(ch, CLASS_MONK) > 19);

    if (ch->actq) {
      call_magic(ch, victim, NULL, ch->actq->spellnum,
                 ch->actq->level, CAST_STRIKE, NULL);
      free(ch->actq);
      ch->actq = 0;
    }

    return dam;
  }
}

void hit(struct char_data *ch, struct char_data *victim, int type)
{
  struct obj_data *wielded;
  int w_type, calc_base_hit, weap, lastweap, abil, notfirst, cleave, hitbonus, i;
  int status, attacks, fullextra = 0;
  room_rnum loc;
  char *hitstr, *damstr, *critstr;
  char buf[MAX_INPUT_LENGTH];

  if (!ch || !victim)
    return;

  if (IS_AFFECTED(ch, AFF_STUNNED))
    return;

  /* check if the character has a fight trigger */
  fight_mtrigger(ch);

  loc = IN_ROOM(victim);

  /* Do some sanity checking, in case someone flees, etc. */
  if (IN_ROOM(ch) != IN_ROOM(victim)) {
    if (FIGHTING(ch) && FIGHTING(ch) == victim)
      stop_fighting(ch);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_NEXTNOACTION)) {
    REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTNOACTION);
    return;
  }

  if (AFF_FLAGGED(ch, AFF_NEXTPARTIAL) || GET_POS(ch) != POS_FIGHTING)
    lastweap = WEAR_WIELD1;
  else
    lastweap = WEAR_WIELD2;

  cleave = 0;
  for (weap = WEAR_WIELD1; weap <= lastweap; weap++) {
    wielded = GET_EQ(ch, weap);

    if (!ch->hit_breakdown[weap - WEAR_WIELD1])
      CREATE(ch->hit_breakdown[weap - WEAR_WIELD1], char, MAX_INPUT_LENGTH);
    if (!ch->dam_breakdown[weap - WEAR_WIELD1])
      CREATE(ch->dam_breakdown[weap - WEAR_WIELD1], char, MAX_INPUT_LENGTH);
    if (!ch->crit_breakdown[weap - WEAR_WIELD1]) {
      CREATE(ch->crit_breakdown[weap - WEAR_WIELD1], char, MAX_INPUT_LENGTH);
      ch->crit_breakdown[weap - WEAR_WIELD1][0] = 0;
    }
    hitstr = ch->hit_breakdown[weap - WEAR_WIELD1];
    damstr = ch->dam_breakdown[weap - WEAR_WIELD1];
    critstr = ch->crit_breakdown[weap - WEAR_WIELD1];
    hitstr[0] = damstr[0] = 0;

    /* Find the weapon type (for display purposes only) */
    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON) {
      w_type = GET_OBJ_VAL(wielded, VAL_WEAPON_DAMTYPE) + TYPE_HIT;
      strncpy(hitstr, weapon_type[GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)], MAX_INPUT_LENGTH);
    } else {
      if (IS_NPC(ch) && ch->mob_specials.attack_type != 0)
      w_type = ch->mob_specials.attack_type + TYPE_HIT;
      else
      w_type = TYPE_HIT;
      strncpy(hitstr, weapon_type[WEAPON_TYPE_UNARMED], MAX_INPUT_LENGTH);
    }

    /* Calculate chance of hit. Lower THAC0 is better for attacker. */
    calc_base_hit = compute_base_hit(ch);

    abil = 0;
    if (weap != WEAR_WIELD1) {
      if (!wielded || GET_OBJ_TYPE(wielded) != ITEM_WEAPON) {
        hitstr[0] = 0;
        break;
      }
      if (!HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING))
        calc_base_hit -= 4; /* Offhand hits at a penalty */
      abil = ability_mod_value(GET_STR(ch));
    } else {
      if (GET_STR(ch) > GET_DEX(ch))
        abil = ability_mod_value(GET_STR(ch));
      else if ((!wielded && HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FINESSE, WEAPON_TYPE_UNARMED)) ||
               (wielded && wield_type(get_size(ch), wielded) == WIELD_LIGHT &&
                HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FINESSE, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))) {
        abil = ability_mod_value(GET_DEX(ch));
        if (GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_ARMOR)
          abil -= GET_OBJ_VAL(GET_EQ(ch, WEAR_WIELD2), VAL_ARMOR_CHECK);
        if (abil < ability_mod_value(GET_STR(ch)))
          abil = ability_mod_value(GET_STR(ch));
      } else
        abil = ability_mod_value(GET_STR(ch));
    }

    if (GET_EQ(ch, WEAR_WIELD1) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD1)) == ITEM_WEAPON &&
        GET_EQ(ch, WEAR_WIELD2) && GET_OBJ_TYPE(GET_EQ(ch, WEAR_WIELD2)) == ITEM_WEAPON) {
      calc_base_hit -= 2;
      if (!HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING))
        calc_base_hit -= 2;
      if (wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD2)) != WIELD_LIGHT)
        calc_base_hit -= 2;
    }

    calc_base_hit += abil;

    if (HAS_FEAT(ch, FEAT_POWER_ATTACK) && GET_POWERATTACK(ch) && GET_STR(ch) > 12)
      calc_base_hit -= GET_POWERATTACK(ch);

    /*
     * If something (casting a spell, doing some other partial action) has
     * marked the attacker NEXTPARTIAL, they cannot do a full attack for
     * one combat round.
     */
    if (AFF_FLAGGED(ch, AFF_NEXTPARTIAL)) {
      attacks = 1;
      REMOVE_BIT_AR(AFF_FLAGS(ch), AFF_NEXTPARTIAL);
    } else if (GET_POS(ch) != POS_FIGHTING) { /* initial attack */
      attacks = 1;
    } else {
      attacks = num_attacks(ch, weap != WEAR_WIELD1);
    }

    if (attacks > 1 && !(weap - WEAR_WIELD1) && GET_CLASS_RANKS(ch, CLASS_MONK) && !GET_EQ(ch, WEAR_WIELD1) && !GET_EQ(ch, WEAR_WIELD2)) {
      if (GET_CLASS_RANKS(ch, CLASS_MONK) > 10)
        fullextra = 2;
      else
        fullextra = 1;
      if (GET_CLASS_RANKS(ch, CLASS_MONK) < 5)
        calc_base_hit -= 2;
      else if (GET_CLASS_RANKS(ch, CLASS_MONK) < 9)
        calc_base_hit -= 1;
    }

    if (wielded && GET_OBJ_TYPE(wielded) == ITEM_WEAPON &&
      !is_proficient_with_weapon(ch, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
      calc_base_hit -= 4; /* Lack of proficiency yields less accuracy */

    calc_base_hit -= GET_ARMORCHECK(ch);

    if (wielded) {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        calc_base_hit++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, GET_OBJ_VAL(wielded, VAL_WEAPON_SKILL)))
        calc_base_hit++;
    } else {
      if (HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, WEAPON_TYPE_UNARMED))
        calc_base_hit++;
      if (HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, WEAPON_TYPE_UNARMED))
        calc_base_hit++;
    }

    if (wielded) {
      hitbonus = 0;
      for (i = 0; i < MAX_OBJ_AFFECT; i++)
        if (wielded->affected[i].location == APPLY_ACCURACY)
          hitbonus += wielded->affected[i].modifier;
    } else {
      hitbonus = HAS_FEAT(ch, FEAT_KI_STRIKE);
    }

    sprintf(buf, ", %d attack%s, ", attacks + fullextra, (attacks + fullextra) == 1 ? "" : "s");
    strncat(hitstr, buf, MAX_INPUT_LENGTH);

    notfirst = 0;
    do {
      if (AFF_FLAGGED(victim, AFF_ETHEREAL) != AFF_FLAGGED(ch, AFF_ETHEREAL))
        victim = find_next_victim(ch);
      if (!victim)
        return;
      status = one_hit(ch, victim, wielded, w_type, calc_base_hit, damstr, critstr, hitbonus);
      sprintf(buf, "%s%+d", notfirst++ ? "/" : "", calc_base_hit);
      strncat(hitstr, buf, MAX_INPUT_LENGTH);
      /* check if the victim has a hitprcnt trigger */
      if (status > -1)
        hitprcnt_mtrigger(victim);
      else {
        victim = find_next_victim(ch);
        if (victim) {
          status = 0;
          if (HAS_FEAT(ch, FEAT_GREAT_CLEAVE) ||
              (!cleave++ && HAS_FEAT(ch, FEAT_CLEAVE))) {
            act("$n follows through and attacks $N!", FALSE, ch, NULL, victim, TO_NOTVICT);
            act("You follow through and attack $N!", FALSE, ch, NULL, victim, TO_CHAR);
            act("$n follows through and attacks YOU!", FALSE, ch, NULL, victim, TO_VICT | TO_SLEEP);
            status = one_hit(ch, victim, wielded, w_type, calc_base_hit, damstr, critstr, hitbonus);
  
            /* check if the victim has a hitprcnt trigger */
            if (status > -1)
              hitprcnt_mtrigger(victim);
            else {
              victim = find_next_victim(ch);
              if (victim)
                status = 0;
            }
          }
        } else
          return;
      }

      if (!fullextra) {
        calc_base_hit -= 5;
        attacks--;
      } else {
        fullextra--;
      }
    } while (status > -1 && loc == IN_ROOM(victim) && attacks);
  }
}


struct fightsort_elem {
  struct char_data *ch;
  int init;
  int dex;
};

struct fightsort_elem *fightsort_table = NULL;
int fightsort_table_size = 0;

void free_fightsort()
{
  if (fightsort_table)
    free(fightsort_table);
  fightsort_table_size = 0;
}

static int fightsort_compare(const void *a1, const void *b1)
{
  struct fightsort_elem *a = (struct fightsort_elem *) a1;
  struct fightsort_elem *b = (struct fightsort_elem *) b1;
  if (a->init < b->init)
    return -1;
  if (a->init > b->init)
    return 1;
  if (a->dex < b->dex)
    return -1;
  if (a->dex > b->dex)
    return 1;
  return 0;
}

/* control the fights going on.  Called every round from comm.c. */
void perform_violence(void)
{
  struct char_data *ch;
  struct follow_type *k;
  int i, j;
  ACMD(do_assist);

  i = 0;
  for (ch = combat_list; ch; ch = ch->next_fighting) {
    if (i >= fightsort_table_size) {
      if (fightsort_table) {
        RECREATE(fightsort_table, struct fightsort_elem, fightsort_table_size + 128);
        fightsort_table_size += 128;
      } else {
        CREATE(fightsort_table, struct fightsort_elem, 128);
        fightsort_table_size = 128;
      }
    }
    fightsort_table[i].ch = ch;
    fightsort_table[i].init = rand_number(1, 20) + dex_mod_capped(ch) + 4 * HAS_FEAT(ch, FEAT_IMPROVED_INITIATIVE);
    fightsort_table[i].dex = GET_DEX(ch);
    i++;
  }

  /* sort into initiative order */
  qsort(fightsort_table, i, sizeof(struct fightsort_elem), fightsort_compare);

  /* lowest initiative is at the bottom, and we're constructing combat_list
   * backwards */
  combat_list = NULL;
  for (j = 0; j < i; j++) {
    fightsort_table[j].ch->next_fighting = combat_list;
    combat_list = fightsort_table[j].ch;
  }

  for (ch = combat_list; ch; ch = next_combat_list) {
    next_combat_list = ch->next_fighting;

    if (FIGHTING(ch) == NULL || IN_ROOM(ch) != IN_ROOM(FIGHTING(ch))) {
      stop_fighting(ch);
      continue;
    }

    if (IS_AFFECTED(ch, AFF_PARALYZE)) {
      send_to_char(ch, "Your muscles won't respond!\r\n");
      continue;
    }

    if (IS_NPC(ch)) {
      if (GET_MOB_WAIT(ch) > 0) {
	GET_MOB_WAIT(ch) -= PULSE_VIOLENCE;
	continue;
      }
      GET_MOB_WAIT(ch) = 0;
      if (GET_POS(ch) < POS_FIGHTING) {
	GET_POS(ch) = POS_FIGHTING;
	act("$n scrambles to $s feet!", TRUE, ch, 0, 0, TO_ROOM);
      }
    }

    if (GET_POS(ch) < POS_FIGHTING) {
      send_to_char(ch, "You can't fight while sitting!!\r\n");
      continue;
    }

    for (k = ch->followers; k; k=k->next) {
      /* should followers auto-assist master? */
      if (!IS_NPC(k->follower) && !FIGHTING(k->follower) && PRF_FLAGGED(k->follower, PRF_AUTOASSIST) &&
        (k->follower->in_room == ch->in_room))
        do_assist(k->follower, GET_NAME(ch), 0, 0);
    }

    /* should master auto-assist followers?  */
    if (ch->master && PRF_FLAGGED(ch->master, PRF_AUTOASSIST) && 
      FIGHTING(ch) && !FIGHTING(ch->master) && 
      (ch->master->in_room == ch->in_room)) 
      do_assist(ch->master, GET_NAME(ch), 0, 0); 

    hit(ch, FIGHTING(ch), TYPE_UNDEFINED);
    if (MOB_FLAGGED(ch, MOB_SPEC) && GET_MOB_SPEC(ch) && !MOB_FLAGGED(ch, MOB_NOTDEADYET)) {
      char actbuf[MAX_INPUT_LENGTH] = "";
      (GET_MOB_SPEC(ch)) (ch, ch, 0, actbuf);
    }
  }
}
