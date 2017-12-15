/* ************************************************************************
*   File: act.informative.c                             Part of CircleMUD *
*  Usage: Player-level commands of an informative nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.informative.c,v 1.29 2005/05/29 18:36:37 zizazat Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "screen.h"
#include "constants.h"
#include "dg_scripts.h"
#include "boards.h"
#include "feats.h"

/* extern variables */
extern int top_of_helpt;
extern struct help_index_element *help_table;
extern char *help;
extern struct time_info_data time_info;

extern char *credits;
extern char *news;
extern char *info;
extern char *motd;
extern char *imotd;
extern char *wizlist;
extern char *immlist;
extern char *policies;
extern char *handbook;
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern const char *material_names[];
extern int show_mob_stacking;
extern int show_obj_stacking;

/* extern functions */
ACMD(do_action);
ACMD(do_insult);
int level_exp(int level);
struct time_info_data *real_time_passed(time_t t2, time_t t1);
int compute_armor_class(struct char_data *ch, struct char_data *att);
struct obj_data *find_vehicle_by_vnum(int vnum);
extern struct obj_data *get_obj_in_list_type(int type, struct obj_data *list);
void view_room_by_rnum(struct char_data * ch, int is_in);
int check_disabled(const struct command_info *command);

/* local functions */
int sort_commands_helper(const void *a, const void *b);
void print_object_location(int num, struct obj_data *obj, struct char_data *ch, int recur);
void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode);
void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show);
int show_obj_modifiers(struct obj_data *obj, struct char_data *ch);
ACMD(do_look);
ACMD(do_examine);
ACMD(do_gold);
ACMD(do_score);
ACMD(do_inventory);
ACMD(do_equipment);
ACMD(do_time);
ACMD(do_weather);
ACMD(do_help);
ACMD(do_who);
ACMD(do_users);
ACMD(do_gen_ps);
void perform_mortal_where(struct char_data *ch, char *arg);
void perform_immort_where(struct char_data *ch, char *arg);
ACMD(do_where);
ACMD(do_levels);
ACMD(do_consider);
ACMD(do_diagnose);
ACMD(do_color);
ACMD(do_toggle);
void sort_commands(void);
ACMD(do_commands);
void diag_char_to_char(struct char_data *i, struct char_data *ch);
void diag_obj_to_char(struct obj_data *obj, struct char_data *ch);
void look_at_char(struct char_data *i, struct char_data *ch);
void list_one_char(struct char_data *i, struct char_data *ch);
void list_char_to_char(struct char_data *list, struct char_data *ch);
ACMD(do_exits);
void look_in_direction(struct char_data *ch, int dir);
void look_in_obj(struct char_data *ch, char *arg);
void look_out_window(struct char_data *ch, char *arg);
char *find_exdesc(char *word, struct extra_descr_data *list);
void look_at_target(struct char_data *ch, char *arg, int read);
void search_in_direction(struct char_data * ch, int dir);
ACMD(do_autoexit);
void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode);
void display_spells(struct char_data *ch, struct obj_data *obj);


/* local globals */
int *cmd_sort_info;

/* Portal appearance types */
const char *portal_appearance[] = {
    "All you can see is the glow of the portal.",
    "You see an image of yourself in the room - my, you are looking attractive today.",
    "All you can see is a swirling grey mist.",
    "The scene is of the surrounding countryside, but somehow blurry and lacking focus.",
    "The blackness appears to stretch on forever.",
    "Suddenly, out of the blackness a flaming red eye appears and fixes its gaze upon you.",
    "\n"
};
#define MAX_PORTAL_TYPES        6

/* For show_obj_to_char 'mode'.	/-- arbitrary */
#define SHOW_OBJ_LONG		0
#define SHOW_OBJ_SHORT		1
#define SHOW_OBJ_ACTION		2


void display_spells(struct char_data *ch, struct obj_data *obj)
{
  int i;

  send_to_char(ch, "The spellbook contains the following spells:\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");
  if (!obj->sbinfo)
    return;
  for (i=0; i < SPELLBOOK_SIZE; i++)
    if (obj->sbinfo[i].spellname != 0)
      send_to_char(ch, "%-20s		[%2d]\r\n", spell_info[obj->sbinfo[i].spellname].name ,obj->sbinfo[i].pages);
  return;
}

void display_scroll(struct char_data *ch, struct obj_data *obj)
{
  send_to_char(ch, "The scroll contains the following spell:\r\n");
  send_to_char(ch, "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\r\n");
  send_to_char(ch, "%-20s\r\n", skill_name(GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1)));
  return;
}

void show_obj_to_char(struct obj_data *obj, struct char_data *ch, int mode)
{
  if (!obj || !ch) {
    log("SYSERR: NULL pointer in show_obj_to_char(): obj=%p ch=%p", obj, ch);
    return;
  }

  switch (mode) {
  case SHOW_OBJ_LONG:
    send_to_char(ch, "@[2]");
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    /*
     * hide objects starting with . from non-holylighted people 
     * Idea from Elaseth of TBA 
     */
    if (*obj->description == '.' && (IS_NPC(ch) || !PRF_FLAGGED(ch, PRF_HOLYLIGHT))) 
      return;

    send_to_char(ch, "%s@n", obj->description);
    break;

  case SHOW_OBJ_SHORT:
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) 
      send_to_char(ch, "[%d] %s", GET_OBJ_VNUM(obj), SCRIPT(obj) ? "[TRIG] " : "");
    
    send_to_char(ch, "%s", obj->short_description);
    break;

  case SHOW_OBJ_ACTION:
    switch (GET_OBJ_TYPE(obj)) {
    case ITEM_NOTE:
      if (obj->action_description) {
        char notebuf[MAX_NOTE_LENGTH + 64];

        snprintf(notebuf, sizeof(notebuf), "There is something written on it:\r\n\r\n%s", obj->action_description);
        page_string(ch->desc, notebuf, TRUE);
      } else
	send_to_char(ch, "It's blank.\r\n");
      return;

    case ITEM_BOARD:
      show_board(GET_OBJ_VNUM(obj),ch);
      break;
      
    case ITEM_DRINKCON:
      send_to_char(ch, "It looks like a drink container.\r\n");
      break;

    case ITEM_SPELLBOOK:
      send_to_char(ch, "It looks like an arcane tome.\r\n");
      display_spells(ch, obj);
      break;

    case ITEM_SCROLL:
      send_to_char(ch, "It looks like an arcane scroll.\r\n");
      display_scroll(ch, obj);
      break;

    default:
      send_to_char(ch, "You see nothing special..\r\n");
      break;
    }

    if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
      send_to_char(ch, "The weapon type of %s@n is '%s'.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
      send_to_char(ch, "You could wield it %s.\r\n", wield_names[wield_type(get_size(ch), obj)]);
    }
    diag_obj_to_char(obj, ch);
    send_to_char(ch, "It appears to be made of %s.\r\n",
    material_names[GET_OBJ_MATERIAL(obj)]);
    break;

  default:
    log("SYSERR: Bad display mode (%d) in show_obj_to_char().", mode);
    return;
  }

  if ((show_obj_modifiers(obj, ch) || (mode != SHOW_OBJ_ACTION)))
  send_to_char(ch, "\r\n");
}


int show_obj_modifiers(struct obj_data *obj, struct char_data *ch)
{
  int found = FALSE;

  if (OBJ_FLAGGED(obj, ITEM_INVISIBLE)) {
    send_to_char(ch, " (invisible)");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BLESS) && AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    send_to_char(ch, " ..It glows blue!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_MAGIC) && AFF_FLAGGED(ch, AFF_DETECT_MAGIC)) {
    send_to_char(ch, " ..It glows yellow!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_GLOW)) {
    send_to_char(ch, " ..It has a soft glowing aura!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_HUM)) {
    send_to_char(ch, " ..It emits a faint humming sound!");
    found++;
  }
  if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
    send_to_char(ch, " ..It appears to be broken.");
    found++;
  }
  return(found);
}

void list_obj_to_char(struct obj_data *list, struct char_data *ch, int mode, int show)
{
  struct obj_data *i, *j;
  bool found = FALSE;
  int num;

  for (i = list; i; i = i->next_content) {
    if (i->description == NULL)
      continue;
    if (str_cmp(i->description, "undefined") == 0)
      continue;
    num = 0;
    if (CONFIG_STACK_OBJS) {
      for (j = list; j != i; j = j->next_content)
        if (!str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number))
          break;
      if (j!=i)
        continue;
      for (j = i; j; j = j->next_content)
        if (!str_cmp(j->short_description, i->short_description) &&
            (j->item_number == i->item_number))
          num++;
    }
    if (CAN_SEE_OBJ(ch, i) || (GET_OBJ_TYPE(i) == ITEM_LIGHT)) {
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
      show_obj_to_char(i, ch, mode);
      found = TRUE;
    }
  } /* Loop through all objects in the list */
  if (!found && show)
  send_to_char(ch, " Nothing.\r\n");
}

void diag_obj_to_char(struct obj_data *obj, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."                 },
    {  90, "has a few scuffs."                          },
    {  75, "has some small scuffs and scratches."       },
    {  50, "has quite a few scratches."                 },
    {  30, "has some big nasty scrapes and scratches."  },
    {  15, "looks pretty damaged."                      },
    {   0, "is in awful condition."                     },
    {  -1, "is in need of repair."                      },
  };
  int percent, ar_index;
  const char *objs = OBJS(obj, ch);

  if (GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH) > 0)
    percent = (100 * GET_OBJ_VAL(obj, VAL_ALL_HEALTH)) / GET_OBJ_VAL(obj, VAL_ALL_MAXHEALTH);
  else
    percent = 0;               /* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "\r\n%c%s %s\r\n", UPPER(*objs), objs + 1, diagnosis[ar_index].text);
}

void diag_char_to_char(struct char_data *i, struct char_data *ch)
{
  struct {
    int percent;
    const char *text;
  } diagnosis[] = {
    { 100, "is in excellent condition."			},
    {  90, "has a few scratches."			},
    {  75, "has some small wounds and bruises."		},
    {  50, "has quite a few wounds."			},
    {  30, "has some big nasty wounds and scratches."	},
    {  15, "looks pretty hurt."				},
    {   0, "is in awful condition."			},
    {  -1, "is bleeding awfully from big wounds."	},
  };
  int percent, ar_index;
  const char *pers = PERS(i, ch);

  if (GET_MAX_HIT(i) > 0)
    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
  else
    percent = 0;		/* How could MAX_HIT be < 1?? */

  for (ar_index = 0; diagnosis[ar_index].percent >= 0; ar_index++)
    if (percent >= diagnosis[ar_index].percent)
      break;

  send_to_char(ch, "%c%s %s\r\n", UPPER(*pers), pers + 1, diagnosis[ar_index].text);
}


void look_at_char(struct char_data *i, struct char_data *ch)
{
  int j, found;
  struct obj_data *tmp_obj;

  if (!ch->desc)
    return;

   if (i->description)
    send_to_char(ch, "%s", i->description);
  else
    act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

  if (GET_SEX(i) == SEX_NEUTRAL) 
    send_to_char(ch, "%c%s appears to be %s %s.\r\n", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(RACE(i)), RACE(i));
  else
    send_to_char(ch, "%c%s appears to be %s %s %s.\r\n", UPPER(*GET_NAME(i)), GET_NAME(i) + 1, AN(MAFE(i)), MAFE(i), RACE(i));

  diag_char_to_char(i, ch);

  found = FALSE;
  for (j = 0; !found && j < NUM_WEARS; j++)
    if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j)))
      found = TRUE;

  if (found) {
    send_to_char(ch, "\r\n");	/* act() does capitalization. */
    act("$n is using:", FALSE, i, 0, ch, TO_VICT);
    for (j = 0; j < NUM_WEARS; j++)
      if (GET_EQ(i, j) && CAN_SEE_OBJ(ch, GET_EQ(i, j))) {
	send_to_char(ch, "%s", wear_where[j]);
	show_obj_to_char(GET_EQ(i, j), ch, SHOW_OBJ_SHORT);
      }
  }
  if (ch != i && (IS_ROGUE(ch) || GET_ADMLEVEL(ch))) {
    found = FALSE;
    act("\r\nYou attempt to peek at $s inventory:", FALSE, i, 0, ch, TO_VICT);
    for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
      if (CAN_SEE_OBJ(ch, tmp_obj) &&
          (ADM_FLAGGED(ch, ADM_SEEINV) || (rand_number(0, 20) < GET_LEVEL(ch)))) {
	show_obj_to_char(tmp_obj, ch, SHOW_OBJ_SHORT);
	found = TRUE;
      }
    }

    if (!found)
      send_to_char(ch, "You can't see anything.\r\n");
  }
}


void list_one_char(struct char_data *i, struct char_data *ch)
{
  const char *positions[] = {
    " is lying here, dead.",
    " is lying here, mortally wounded.",
    " is lying here, incapacitated.",
    " is lying here, stunned.",
    " is sleeping here.",
    " is resting here.",
    " is sitting here.",
    "!FIGHTING!",
    " is standing here."
  };

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS) && IS_NPC(i)) 
     send_to_char(ch, "[%d] %s", GET_MOB_VNUM(i), SCRIPT(i) ? "[TRIG] " : "");
  
  if (IS_NPC(i) && i->long_descr && GET_POS(i) == GET_DEFAULT_POS(i)) {
    if (AFF_FLAGGED(i, AFF_INVISIBLE))
      send_to_char(ch, "*");

    if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
      if (IS_EVIL(i))
	send_to_char(ch, "(@rRed@[3] Aura) ");
      else if (IS_GOOD(i))
	send_to_char(ch, "(@bBlue@[3] Aura) ");
    }
    send_to_char(ch, "%s", i->long_descr);

    if (AFF_FLAGGED(i, AFF_SANCTUARY))
      act("...$e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);
    if (AFF_FLAGGED(i, AFF_BLIND))
      act("...$e is groping around blindly!", FALSE, i, 0, ch, TO_VICT);
    if (affected_by_spell(i, SPELL_FAERIE_FIRE))
      act("@m...$e @mis outlined with purple fire!@m", FALSE, i, 0, ch, TO_VICT);

    return;
  }

  if (IS_NPC(i))
    send_to_char(ch, "%c%s@[3]", UPPER(*i->short_descr), i->short_descr + 1);
  else
    send_to_char(ch, "%s@[3] %s@[3]", i->name, GET_TITLE(i));

  if (AFF_FLAGGED(i, AFF_INVISIBLE))
    send_to_char(ch, " (invisible)");
  if (AFF_FLAGGED(i, AFF_ETHEREAL))
    send_to_char(ch, " (ethereal)");
  if (AFF_FLAGGED(i, AFF_HIDE))
    send_to_char(ch, " (hiding)");
  if (!IS_NPC(i) && !i->desc)
    send_to_char(ch, " (linkless)");
  if (!IS_NPC(i) && PLR_FLAGGED(i, PLR_WRITING))
    send_to_char(ch, " (writing)");
  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (buildwalk)");

  if (!IS_NPC(i) && PRF_FLAGGED(i, PRF_BUILDWALK))
    send_to_char(ch, " (buildwalk)");

  if (GET_POS(i) != POS_FIGHTING)
    send_to_char(ch, "@[3]%s", positions[(int) GET_POS(i)]);
  else {
    if (FIGHTING(i)) {
      send_to_char(ch, " is here, fighting ");
      if (FIGHTING(i) == ch)
	send_to_char(ch, "@rYOU!@[3]");
      else {
	if (IN_ROOM(i) == IN_ROOM(FIGHTING(i)))
	  send_to_char(ch, "%s!", PERS(FIGHTING(i), ch));
	else
	  send_to_char(ch,  "someone who has already left!");
      }
    } else			/* NIL fighting pointer */
      send_to_char(ch, " is here struggling with thin air.");
  }

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN)) {
    if (IS_EVIL(i))
      send_to_char(ch, " (@rRed@[3] Aura)");
    else if (IS_GOOD(i))
      send_to_char(ch, " (@bBlue@[3] Aura)");
  }
  send_to_char(ch, "@n\r\n");

  if (AFF_FLAGGED(i, AFF_SANCTUARY))
    act("@[3]...$e glows with a bright light!@n", FALSE, i, 0, ch, TO_VICT);
  if (affected_by_spell(i, SPELL_FAERIE_FIRE))
    act("@[3]..$e is outlined with purple fire!@n", FALSE, i, 0, ch, TO_VICT);
}

void list_char_to_char(struct char_data *list, struct char_data *ch)
{
  struct char_data *i, *j;
  struct hide_node {
    struct hide_node *next;
    struct char_data *hidden;
  } *hideinfo, *lasthide, *tmphide;
  int num;
  
  hideinfo = lasthide = NULL;
  
  for (i = list; i; i = i->next_in_room) {
    if (AFF_FLAGGED(i, AFF_HIDE) && roll_resisted(i, SKILL_HIDE, ch, SKILL_SPOT)) {
      CREATE(tmphide, struct hide_node, 1);
      tmphide->next = NULL;
      tmphide->hidden = i;
      if (!lasthide) {
        hideinfo = lasthide = tmphide;
      } else {
        lasthide->next = tmphide;
        lasthide = tmphide;
      }
      continue;
    }
  }

  for (i = list; i; i = i->next_in_room) {
    if (ch == i) 
      continue;

    for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
      if (tmphide->hidden == i)
        break;
    if (tmphide)
      continue;

    if (CAN_SEE(ch, i)) {
      num = 0;
      if (CONFIG_STACK_MOBS) {
        /* How many other occurences of this mob are there? */
        for (j = list; j != i; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              break;
          }
     	if (j!=i)
          /* This will be true where we have already found this
	   * mob for an earlier "i".  The continue pops us out of
	   * the main "i" for loop.
	   */
          continue;
 	for (j = i; j; j = j->next_in_room)
          if ( (i->nr           == j->nr            ) &&
               (GET_POS(i)      == GET_POS(j)       ) &&
               (AFF_FLAGS(i)[0]    == AFF_FLAGS(j)[0]     ) &&
               (AFF_FLAGS(i)[1]    == AFF_FLAGS(j)[1]     ) &&
               (AFF_FLAGS(i)[2]    == AFF_FLAGS(j)[2]     ) &&
               (AFF_FLAGS(i)[3]    == AFF_FLAGS(j)[3]     ) &&
               !strcmp(GET_NAME(i), GET_NAME(j))         ) {
            for (tmphide = hideinfo; tmphide; tmphide = tmphide->next)
              if (tmphide->hidden == j)
                break;
            if (!tmphide)
              num++;
        }
      }
      /* Now show this mob's name and other stuff */
      send_to_char(ch, "@[3]");
      if (num > 1)
        send_to_char(ch, "(%2i) ", num);
      list_one_char(i, ch);
      send_to_char(ch, "@n");
    } /* processed a character we can see */
    else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch) &&
           AFF_FLAGGED(i, AFF_INFRAVISION))
      send_to_char(ch, "@[3]You see a pair of glowing red eyes looking your way.@n\r\n");
  } /* loop through all characters in room */
}

void do_auto_exits(room_rnum target_room, struct char_data *ch, int exit_mode)
{
  int door, door_found = 0, has_light = FALSE, i;

  if (exit_mode == EXIT_NORMAL) {
    /* Standard behaviour - just list the available exit directions */
    send_to_char(ch, "@c[ Exits: ");
    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (!W_EXIT(target_room, door) ||
           W_EXIT(target_room, door)->to_room == NOWHERE)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED) &&
          !CONFIG_DISP_CLOSED_DOORS)
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_SECRET) && 
          EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        continue;
      if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
        send_to_char(ch, "@r(%s)@c ", abbr_dirs[door]);
      else
        send_to_char(ch, "%s ", abbr_dirs[door]);
      door_found++;
    }
    send_to_char(ch, "%s]@n\r\n", door_found ? "" : "None!");
  }
  if (exit_mode == EXIT_COMPLETE) {
    send_to_char(ch, "@cObvious Exits:@n\r\n");
    if (IS_AFFECTED(ch, AFF_BLIND)) {
      send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
      return;
    }
    if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
      send_to_char(ch, "It is pitch black...\r\n");
      return;
    }

    /* Is the character using a working light source? */
    for (i = 0; i < NUM_WEARS; i++)
      if (GET_EQ(ch, i))
        if (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_LIGHT)
          if (GET_OBJ_VAL(GET_EQ(ch, i),VAL_LIGHT_HOURS))
            has_light = TRUE;

    for (door = 0; door < NUM_OF_DIRS; door++) {
      if (W_EXIT(target_room, door) &&
          W_EXIT(target_room, door)->to_room != NOWHERE) {
        /* We have a door that leads somewhere */
        if (ADM_FLAGGED(ch, ADM_SEESECRET)) {
          /* Immortals see everything */
          door_found++;
          send_to_char(ch, "%-9s - [%5d] %s.\r\n", dirs[door],
                world[W_EXIT(target_room, door)->to_room].number,
                world[W_EXIT(target_room, door)->to_room].name);
          if (IS_SET(W_EXIT(target_room, door)->exit_info, EX_ISDOOR) ||
              IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)   ) {
            /* This exit has a door - tell all about it */
            send_to_char(ch,"                    The %s%s is %s %s%s.\r\n",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET) ?
                    "secret " : "",
                (W_EXIT(target_room, door)->keyword &&
                 str_cmp(fname(W_EXIT(target_room, door)->keyword), "undefined")) ?
                    fname(W_EXIT(target_room, door)->keyword) : "opening",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED) ?
                    "closed" : "open",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_LOCKED) ?
                    "and locked" : "but unlocked",
                IS_SET(W_EXIT(target_room, door)->exit_info, EX_PICKPROOF) ?
                    " (pickproof)" : "");
          }
        }
        else { /* This is what mortal characters see */
          if (!IS_SET(W_EXIT(target_room, door)->exit_info, EX_CLOSED)) {
            /* And the door is open */
            door_found++;
            send_to_char(ch, "%-9s - %s\r\n", dirs[door],
                IS_DARK(W_EXIT(target_room, door)->to_room) &&
                !CAN_SEE_IN_DARK(ch) && !has_light ?
                "Too dark to tell." :
                world[W_EXIT(target_room, door)->to_room].name);
          } else if (CONFIG_DISP_CLOSED_DOORS &&
              !IS_SET(W_EXIT(target_room, door)->exit_info, EX_SECRET)) {
              /* But we tell them the door is closed */
              door_found++;
              send_to_char(ch, "%-9s - The %s is closed.\r\n", dirs[door],
                  (W_EXIT(target_room, door)->keyword) ?
                  fname(W_EXIT(target_room,door)->keyword) : "opening" );
            }
        }
      }
    }
    if (!door_found)
    send_to_char(ch, " None.\r\n");
  }
}

/*void do_auto_exits(room_rnum target_room, struct char_data *ch)
{
  int door, slen = 0;

  send_to_char(ch, "@c[ Exits: ");

  for (door = 0; door < NUM_OF_DIRS; door++) {
    if (!W_EXIT(target_room, door) || W_EXIT(target_room, door)->to_room == NOWHERE)
      continue;
    if (EXIT_FLAGGED(W_EXIT(target_room, door), EX_CLOSED))
      continue;

    send_to_char(ch, "%s ", abbr_dirs[door]);
    slen++;
  }

  send_to_char(ch, "%s]@n\r\n", slen ? "" : "None!");
}*/


ACMD(do_exits)
{
  /* Why duplicate code? */
  do_auto_exits(IN_ROOM(ch), ch, EXIT_COMPLETE);
}

const char *exitlevels[] = {
  "off", "normal", "n/a", "complete", "\n"};

ACMD(do_autoexit)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  if (IS_NPC(ch))
    return;

  one_argument(argument, arg);


  if (!*arg) {
    send_to_char(ch, "Your current autoexit level is %s.\r\n", exitlevels[EXIT_LEV(ch)]);
    return;
  }
  if (((tp = search_block(arg, exitlevels, FALSE)) == -1)) {
    send_to_char(ch, "Usage: Autoexit { Off | Normal | Complete }\r\n");
    return;
  }
  switch (tp) {
    case EXIT_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_NORMAL:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
    case EXIT_COMPLETE:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_FULL_EXIT);
      break;
  }
  send_to_char(ch, "Your @rautoexit level@n is now %s.\r\n", exitlevels[EXIT_LEV(ch)]);
}

void look_at_room(room_rnum target_room, struct char_data *ch, int ignore_brief)
{
  struct room_data *rm = &world[IN_ROOM(ch)];
  if (!ch->desc)
    return;

  if (IS_DARK(target_room) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    return;
  } else if (AFF_FLAGGED(ch, AFF_BLIND)) {
    send_to_char(ch, "You see nothing but infinite darkness...\r\n");
    return;
  }
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_ROOMFLAGS)) {
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];

    sprintbitarray(ROOM_FLAGS(target_room), room_bits, RF_ARRAY_MAX, buf);
    sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
    send_to_char(ch, "@[1][%5d] ", GET_ROOM_VNUM(IN_ROOM(ch)));

    send_to_char(ch, "%s%s [ %s] [ %s ]@[0]\r\n",
                     SCRIPT(rm) ? "[TRIG] " : "",
                     world[IN_ROOM(ch)].name, buf, buf2);
  } else
    send_to_char(ch, "@[1]%s@[0]\r\n", world[target_room].name);

  if ((!IS_NPC(ch) && !PRF_FLAGGED(ch, PRF_BRIEF)) || ignore_brief ||
      ROOM_FLAGGED(target_room, ROOM_DEATH))
    send_to_char(ch, "%s", world[target_room].description);

  /* autoexits */
  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_AUTOEXIT))
    do_auto_exits(target_room, ch, EXIT_LEV(ch));

  /* now list characters & objects */
  list_obj_to_char(world[target_room].contents, ch, SHOW_OBJ_LONG, FALSE);
  list_char_to_char(world[target_room].people, ch);
}

void look_in_direction(struct char_data *ch, int dir)
{
  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description)
      send_to_char(ch, "%s", EXIT(ch, dir)->general_description);
    else
      send_to_char(ch, "You see nothing special.\r\n");

    if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) && EXIT(ch, dir)->keyword) {
      if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
           EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED) )
        send_to_char(ch, "The %s is closed.\r\n", fname(EXIT(ch, dir)->keyword));
      else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED))
        send_to_char(ch, "The %s is open.\r\n", fname(EXIT(ch, dir)->keyword));
    }
  } else
    send_to_char(ch, "Nothing special there...\r\n");
}

void look_in_obj(struct char_data *ch, char *arg)
{
  struct obj_data *obj = NULL;
  struct char_data *dummy = NULL;
  int amt, bits;

  if (!*arg)
    send_to_char(ch, "Look in what?\r\n");
  else if (!(bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &dummy, &obj))) {
    send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg), arg);
  } else if (find_exdesc(arg, obj->ex_description) != NULL)
      send_to_char(ch, "There's nothing inside that!\r\n");
    else if ((GET_OBJ_TYPE(obj) == ITEM_PORTAL) && !OBJVAL_FLAGGED(obj, CONT_CLOSEABLE)) { 
    if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < 0) {
      /* You can look through the portal to the destination */
      /* where does this lead to? */
      room_rnum portal_dest = real_room(GET_OBJ_VAL(obj, VAL_PORTAL_DEST)); 
      if (portal_dest == NOWHERE) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else if (IS_DARK(portal_dest) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "You see nothing but infinite darkness...\r\n");
      } else {
       send_to_char(ch, "After seconds of concentration you see the image of %s.\r\n", world[portal_dest].name);
      }
    } else if (GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR) < MAX_PORTAL_TYPES) {
     /* display the appropriate description from the list of descriptions
*/
      send_to_char(ch, "%s\r\n", portal_appearance[GET_OBJ_VAL(obj, VAL_PORTAL_APPEAR)]);
    } else {
      /* We shouldn't really get here, so give a default message */
      send_to_char(ch, "All you can see is the glow of the portal.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_VEHICLE) {
    if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
      send_to_char(ch, "It is closed.\r\n");
    else if (GET_OBJ_VAL(obj, VAL_VEHICLE_APPEAR) < 0) {
      /* You can look inside the vehicle */
      /* where does this lead to? */
      room_rnum vehicle_inside = real_room(GET_OBJ_VAL(obj, VAL_VEHICLE_ROOM)); 
      if (vehicle_inside == NOWHERE) {
        send_to_char(ch, "You cannot see inside that.\r\n");
      } else if (IS_DARK(vehicle_inside) && !CAN_SEE_IN_DARK(ch)) {
        send_to_char(ch, "It is pitch black...\r\n");
      } else {
        send_to_char(ch, "You look inside and see:\r\n");
        look_at_room(vehicle_inside, ch, 0);
      }
    } else {
      send_to_char(ch, "You cannot see inside that.\r\n");
    }
  } else if (GET_OBJ_TYPE(obj) == ITEM_WINDOW) {
    look_out_window(ch, arg);
  } else if ((GET_OBJ_TYPE(obj) != ITEM_DRINKCON) &&
	     (GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN) &&
	     (GET_OBJ_TYPE(obj) != ITEM_CONTAINER)&&
             (GET_OBJ_TYPE(obj) != ITEM_PORTAL))     {
    send_to_char(ch, "There's nothing inside that!\r\n");
  } else if ((GET_OBJ_TYPE(obj) == ITEM_CONTAINER) || 
             (GET_OBJ_TYPE(obj) == ITEM_PORTAL)) {
      if (OBJVAL_FLAGGED(obj, CONT_CLOSED))
	send_to_char(ch, "It is closed.\r\n");
      else {
	send_to_char(ch, "%s", fname(obj->name));
	switch (bits) {
	case FIND_OBJ_INV:
	  send_to_char(ch, " (carried): \r\n");
	  break;
	case FIND_OBJ_ROOM:
	  send_to_char(ch, " (here): \r\n");
	  break;
	case FIND_OBJ_EQUIP:
	  send_to_char(ch, " (used): \r\n");
	  break;
	}

	list_obj_to_char(obj->contains, ch, SHOW_OBJ_SHORT, TRUE);
      }
    } else {		/* item must be a fountain or drink container */
      if (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) <= 0)
	send_to_char(ch, "It is empty.\r\n");
      else {
        if (GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY) < 0)
        {
          char buf2[MAX_STRING_LENGTH];
	  sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's full of a %s liquid.\r\n", buf2);
        }
       else if (GET_OBJ_VAL(obj,VAL_DRINKCON_HOWFULL)>GET_OBJ_VAL(obj,VAL_DRINKCON_CAPACITY)) {
	  send_to_char(ch, "Its contents seem somewhat murky.\r\n"); /* BUG */
	} else {
          char buf2[MAX_STRING_LENGTH];
	  amt = (GET_OBJ_VAL(obj, VAL_DRINKCON_HOWFULL) * 3) / GET_OBJ_VAL(obj, VAL_DRINKCON_CAPACITY);
	  sprinttype(GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID), color_liquid, buf2, sizeof(buf2));
	  send_to_char(ch, "It's %sfull of a %s liquid.\r\n", fullness[amt], buf2);
	}
      }
    }
}



char *find_exdesc(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->description);

  return (NULL);
}


/*
 * Given the argument "look at <target>", figure out what object or char
 * matches the target.  First, see if there is another char in the room
 * with the name.  Then check local objs for exdescs.
 *
 * Thanks to Angus Mezick <angus@EDGIL.CCMAIL.COMPUSERVE.COM> for the
 * suggested fix to this problem.
 */
void look_at_target(struct char_data *ch, char *arg, int read)
{
  int bits, found = FALSE, j, fnum, i = 0, msg = 1, hidelooker;
  struct char_data *found_char = NULL;
  struct obj_data *obj, *found_obj = NULL;
  char *desc;
  char number[MAX_STRING_LENGTH];

  if (!ch->desc)
    return;

  if (!*arg) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  if (read) {
    for (obj = ch->carrying; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	found = TRUE;
	break;
      }
    }
    if(!obj) {
      for (obj = world[ch->in_room].contents; obj;obj=obj->next_content) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  found = TRUE;
	  break;
	}
      }
    }
    if (obj) {
      arg = one_argument(arg, number);
      if (!*number) {
	send_to_char(ch,"Read what?\r\n");
	return;
      }
      
      /* Okay, here i'm faced with the fact that the person could be
	 entering in something like 'read 5' or 'read 4.mail' .. so, whats the
	 difference between the two?  Well, there's a period in the second,
	 so, we'll just stick with that basic difference */
      
      if (isname(number, obj->name)) {
	show_board(GET_OBJ_VNUM(obj), ch);
      } else if ((!isdigit(*number) || (!(msg = atoi(number)))) ||
		 (strchr(number,'.'))) {
	sprintf(arg,"%s %s", number,arg);
	look_at_target(ch, arg, 0);
      } else {
	board_display_msg(GET_OBJ_VNUM(obj), ch, msg);
      }
    }
  } else {
  bits = generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP |
		      FIND_CHAR_ROOM, ch, &found_char, &found_obj);

  /* Is the target a character? */
  if (found_char != NULL) {
    look_at_char(found_char, ch);
    if (ch != found_char) {
      if (AFF_FLAGGED(ch, AFF_HIDE))
        hidelooker = roll_resisted(ch, SKILL_HIDE, found_char, SKILL_SPOT);
      else
        hidelooker = 0;
      if (!hidelooker) {
      if (CAN_SEE(found_char, ch))
	act("$n looks at you.", TRUE, ch, 0, found_char, TO_VICT);
      act("$n looks at $N.", TRUE, ch, 0, found_char, TO_NOTVICT);
    }
    }
    return;
  }

  /* Strip off "number." from 2.foo and friends. */
  if (!(fnum = get_number(&arg))) {
    send_to_char(ch, "Look at what?\r\n");
    return;
  }

  /* Does the argument match an extra desc in the room? */
  if ((desc = find_exdesc(arg, world[IN_ROOM(ch)].ex_description)) != NULL && ++i == fnum) {
    page_string(ch->desc, desc, FALSE);
    return;
  } 

  /* Does the argument match an extra desc in the char's equipment? */
  for (j = 0; j < NUM_WEARS && !found; j++)
    if (GET_EQ(ch, j) && CAN_SEE_OBJ(ch, GET_EQ(ch, j)))
      if ((desc = find_exdesc(arg, GET_EQ(ch, j)->ex_description)) != NULL && ++i == fnum) {
	send_to_char(ch, "%s", desc);
        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_WEAPON) {
          send_to_char(ch, "The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(GET_EQ(ch, j)), weapon_type[(int) GET_OBJ_VAL(GET_EQ(ch, j), VAL_WEAPON_SKILL)]);
        }
        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SPELLBOOK);
          display_spells(ch, GET_EQ(ch, j));
        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SCROLL);
          display_scroll(ch, GET_EQ(ch, j));
        diag_obj_to_char(GET_EQ(ch, j), ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
        material_names[GET_OBJ_MATERIAL(GET_EQ(ch, j))]);
	found = TRUE;
      }

  /* Does the argument match an extra desc in the char's inventory? */
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
          send_to_char(ch, "The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
        }
        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SPELLBOOK);
          display_spells(ch, GET_EQ(ch, j));
        if (GET_OBJ_TYPE(GET_EQ(ch, j)) == ITEM_SCROLL);
          display_scroll(ch, GET_EQ(ch, j));
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
        material_names[GET_OBJ_MATERIAL(obj)]);
	}
	found = TRUE;
      }
  }

  /* Does the argument match an extra desc of an object in the room? */
  for (obj = world[IN_ROOM(ch)].contents; obj && !found; obj = obj->next_content)
    if (CAN_SEE_OBJ(ch, obj))
      if ((desc = find_exdesc(arg, obj->ex_description)) != NULL && ++i == fnum) {
	if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	  show_board(GET_OBJ_VNUM(obj), ch);
	} else {
	send_to_char(ch, "%s", desc);
        if (GET_OBJ_TYPE(obj) == ITEM_WEAPON) {
          send_to_char(ch, "The weapon type of %s is a %s.\r\n", GET_OBJ_SHORT(obj), weapon_type[(int) GET_OBJ_VAL(obj, VAL_WEAPON_SKILL)]);
        }
        diag_obj_to_char(obj, ch);
        send_to_char(ch, "It appears to be made of %s.\r\n",
        material_names[GET_OBJ_MATERIAL(obj)]);
	}
	found = TRUE;
      }

  /* If an object was found back in generic_find */
  if (bits) {
    if (!found)
      show_obj_to_char(found_obj, ch, SHOW_OBJ_ACTION);
    else {
      if (show_obj_modifiers(found_obj, ch))
      send_to_char(ch, "\r\n");
    }
  } else if (!found)
    send_to_char(ch, "You do not see that here.\r\n");
  }
}

void look_out_window(struct char_data *ch, char *arg)
{
  struct obj_data *i, *viewport = NULL, *vehicle = NULL;
  struct char_data *dummy = NULL;
  room_rnum target_room = NOWHERE;
  int bits, door;

  /* First, lets find something to look out of or through. */
  if (*arg) {
    /* Find this object and see if it is a window */
    if (!(bits = generic_find(arg,
              FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP,
              ch, &dummy, &viewport))) {
      send_to_char(ch, "You don't see that here.\r\n");
      return;
    } else if (GET_OBJ_TYPE(viewport) != ITEM_WINDOW) {
      send_to_char(ch, "You can't look out that!\r\n");
    return;
  }
  } else if (OUTSIDE(ch)) {
      /* yeah, sure stupid */
      send_to_char(ch, "But you are already outside.\r\n");
    return;
  } else {
    /* Look for any old window in the room */
    for (i = world[IN_ROOM(ch)].contents; i; i = i->next_content)
      if ((GET_OBJ_TYPE(i) == ITEM_WINDOW) &&
           isname("window", i->name)) {
        viewport = i;
      continue;
      }
  }
  if (!viewport) {
    /* Nothing suitable to look through */
    send_to_char(ch, "You don't seem to be able to see outside.\r\n");
  } else if (OBJVAL_FLAGGED(viewport, CONT_CLOSEABLE) &&
             OBJVAL_FLAGGED(viewport, CONT_CLOSED)) {
    /* The window is closed */
    send_to_char(ch, "It is closed.\r\n");
  } else {
    if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1) < 0) {
      /* We are looking out of the room */
      if (GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4) < 0) {
        /* Look for the default "outside" room */
        for (door = 0; door < NUM_OF_DIRS; door++)
          if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room != NOWHERE)
              if (!ROOM_FLAGGED(EXIT(ch, door)->to_room, ROOM_INDOORS)) {
                target_room = EXIT(ch, door)->to_room;
                continue;
              }
      } else {
        target_room = real_room(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED4));
      }
    } else {
      /* We are looking out of a vehicle */
      if ( (vehicle = find_vehicle_by_vnum(GET_OBJ_VAL(viewport, VAL_WINDOW_UNUSED1))) );
        target_room = vehicle->in_room;
    }
    if (target_room == NOWHERE) {
      send_to_char(ch, "You don't seem to be able to see outside.\r\n");
    } else {
      if (viewport->action_description)
        act(viewport->action_description, TRUE, ch, viewport, 0, TO_CHAR);
      else
        send_to_char(ch, "You look outside and see:\r\n");
      look_at_room(target_room, ch, 0);
    }
  }
}

ACMD(do_look)
{
  int look_type;

  if (!ch->desc)
    return;

  if (GET_POS(ch) < POS_SLEEPING)
    send_to_char(ch, "You can't see anything but stars!\r\n");
  else if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You can't see a damned thing, you're blind!\r\n");
  else if (IS_DARK(IN_ROOM(ch)) && !CAN_SEE_IN_DARK(ch)) {
    send_to_char(ch, "It is pitch black...\r\n");
    list_char_to_char(world[IN_ROOM(ch)].people, ch);	/* glowing red eyes */
  } else {
    char arg[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];

    if (subcmd == SCMD_READ) {
      one_argument(argument, arg);
      if (!*arg)
	send_to_char(ch, "Read what?\r\n");
      else
	look_at_target(ch, arg, 1);
      return;
    }
    argument = any_one_arg(argument, arg);
    one_argument(argument, arg2);
    if (!*arg) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "You need to search in a particular direction.\r\n");
      else
      look_at_room(IN_ROOM(ch), ch, 1);
    } else if (is_abbrev(arg, "inside")   && EXIT(ch, INDIR) && !*arg2) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, INDIR);
      else
        look_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside") && (subcmd == SCMD_SEARCH) && !*arg2) {
      search_in_direction(ch, INDIR);
    } else if (is_abbrev(arg, "inside")   ||
               is_abbrev(arg, "into")       )  { 
      look_in_obj(ch, arg2);
    } else if ((is_abbrev(arg, "outside") || 
                is_abbrev(arg, "through") ||
	        is_abbrev(arg, "thru")      ) && 
               (subcmd == SCMD_LOOK) && *arg2) {
      look_out_window(ch, arg2);
    } else if (is_abbrev(arg, "outside") && 
               (subcmd == SCMD_LOOK) && !EXIT(ch, OUTDIR)) {
      look_out_window(ch, arg2);
    } else if ((look_type = search_block(arg, dirs, FALSE)) >= 0 ||
               (look_type = search_block(arg, abbr_dirs, FALSE)) >= 0) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
        look_in_direction(ch, look_type);
    } else if ((is_abbrev(arg, "towards")) &&
               ((look_type = search_block(arg2, dirs, FALSE)) >= 0 ||
                (look_type = search_block(arg2, abbr_dirs, FALSE)) >= 0 )) {
      if (subcmd == SCMD_SEARCH)
        search_in_direction(ch, look_type);
      else
      look_in_direction(ch, look_type);
    } else if (is_abbrev(arg, "at")) {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
      else
      look_at_target(ch, arg2, 0);
    } else if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
      look_at_target(ch, arg, 0);
    } else {
      if (subcmd == SCMD_SEARCH)
        send_to_char(ch, "That is not a direction!\r\n");
    else
      look_at_target(ch, arg, 0);
  }
  }
}



ACMD(do_examine)
{
  struct char_data *tmp_char;
  struct obj_data *tmp_object;
  char tempsave[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Examine what?\r\n");
    return;
  }

  /* look_at_target() eats the number. */
  look_at_target(ch, strcpy(tempsave, arg),0);	/* strcpy: OK */

  generic_find(arg, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_CHAR_ROOM |
		      FIND_OBJ_EQUIP, ch, &tmp_char, &tmp_object);

  if (tmp_object) {
    if ((GET_OBJ_TYPE(tmp_object) == ITEM_DRINKCON) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_FOUNTAIN) ||
	(GET_OBJ_TYPE(tmp_object) == ITEM_CONTAINER)) {
      send_to_char(ch, "When you look inside, you see:\r\n");
      look_in_obj(ch, arg);
    }
  }
}



ACMD(do_gold)
{
  if (GET_GOLD(ch) == 0)
    send_to_char(ch, "You're broke!\r\n");
  else if (GET_GOLD(ch) == 1)
    send_to_char(ch, "You have one miserable little gold coin.\r\n");
  else
    send_to_char(ch, "You have %d gold coins.\r\n", GET_GOLD(ch));
}


char *reduct_desc(struct damreduct_type *reduct)
{
  static char buf[MAX_INPUT_LENGTH];
  char buf2[MAX_INPUT_LENGTH];
  int len = 0;
  int slash = 0;
  int i;
  if (reduct->mod == -1)
    len += snprintf(buf + len, sizeof(buf) - len, "FULL");
  else
    len += snprintf(buf + len, sizeof(buf) - len, "%d", reduct->mod);
  for (i = 0; i < MAX_DAMREDUCT_MULTI; i++) {
    switch (reduct->damstyle[i]) {
    case DR_NONE:
      continue;
    case DR_ADMIN:
      snprintf(buf2, sizeof(buf2), "%s", admin_level_names[reduct->damstyleval[i]]);
      break;
    case DR_MATERIAL:
      snprintf(buf2, sizeof(buf2), "%s", material_names[reduct->damstyleval[i]]);
      break;
    case DR_BONUS:
      snprintf(buf2, sizeof(buf2), "%+d", reduct->damstyleval[i]);
      break;
    case DR_SPELL:
      snprintf(buf2, sizeof(buf2), "%s%s", reduct->damstyleval[i] ? "spell " : "", reduct->damstyleval[i] ? spell_info[reduct->damstyleval[i]].name : "spells");
      break;
    default:
      log("reduct_desc: unknown damstyle %d", reduct->damstyle[i]);
    }
    if (slash++)
      len += snprintf(buf + len, sizeof(buf) - len, " or ");
    else
      len += snprintf(buf + len, sizeof(buf) - len, "/");
    len += snprintf(buf + len, sizeof(buf) - len, "%s", buf2);
  }
  if (!slash)
    len += snprintf(buf + len, sizeof(buf) - len, "/--");
  return buf;
}


ACMD(do_score)
{
  struct damreduct_type *reduct;
  int penalty;
  char penstr[80];

  if (IS_NPC(ch))
    return;

  send_to_char(ch, "@r=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=@n\r\n");
  send_to_char(ch, "Name : %s %s@n\r\n", GET_NAME(ch), GET_TITLE(ch));
  send_to_char(ch, "@r=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=@n\r\n");
  send_to_char(ch, "Class: %s Race: %s Level: %d\r\n", pc_class_types[(int)GET_CLASS(ch)], pc_race_types[(int)GET_RACE(ch)], GET_LEVEL(ch));
  if (CONFIG_ALLOW_MULTICLASS && GET_LEVEL(ch) > GET_CLASS_RANKS(ch, GET_CLASS(ch))) {
    send_to_char(ch, "Ranks: %s\r\n", class_desc_str(ch, 2, 0));
  }
  if (GET_ADMLEVEL(ch))
    send_to_char(ch, "@rAdmin Level@n: @y%d - %s@n\r\n", GET_ADMLEVEL(ch), admin_level_names[GET_ADMLEVEL(ch)]);
  send_to_char(ch, "@rAlignment@n: %s%s@n (@rE@n-@gG@n: %s%d@n, @yC-L@n: @y%d@n) Diety: (None)\r\n", IS_EVIL(ch) ? "@r" : IS_GOOD(ch) ? "@g" : "@y", alignments[ALIGN_TYPE(ch)], IS_EVIL(ch) ? "@r" : IS_GOOD(ch) ? "@g" : "@y", GET_ALIGNMENT(ch), GET_ETHIC_ALIGNMENT(ch));
  send_to_char(ch, "@rSize@n: @y%s@n @rAge@n: @y%d@n @rGender@n: @y%s@n @rHeight@n: @y%dcm@n @rWeight@n: @y%dkg@n\r\n", size_names[get_size(ch)], GET_AGE(ch), genders[(int)GET_SEX(ch)], GET_HEIGHT(ch), GET_WEIGHT(ch));
  send_to_char(ch, "@rStr@n: [@m%2d@n(@y%+d@n)] @rDex@n: [@m%2d@n(@y%+d@n)] @rHit Points@n : @m%d@n(@y%d@n)\r\n", GET_STR(ch), ability_mod_value(GET_STR(ch)), GET_DEX(ch), ability_mod_value(GET_DEX(ch)), GET_HIT(ch), GET_MAX_HIT(ch));
  send_to_char(ch, "@rCon@n: [@m%2d@n(@y%+d@n)] @rInt@n: [@m%2d@n(@y%+d@n)] @rArmor Class@n: @B%.1f@n\r\n", GET_CON(ch), ability_mod_value(GET_CON(ch)), GET_INT(ch), ability_mod_value(GET_INT(ch)), ((float)compute_armor_class(ch, NULL))/10);
  send_to_char(ch, "@rWis@n: [@m%2d@n(@y%+d@n)] @rCha@n: [@m%2d@n(@y%+d@n)] @rBase Attack Bonus@n: @m%d@n(@y%+d@n)\r\n", GET_WIS(ch), ability_mod_value(GET_WIS(ch)), GET_CHA(ch), ability_mod_value(GET_CHA(ch)), GET_ACCURACY_BASE(ch), ability_mod_value(GET_STR(ch)));
  send_to_char(ch, "@rFortitude@n: [@m%d(@y%+d@n)]  @rReflex@n: [@m%d(@y%+d@n)]  @rWill@n: [@m%d(@y%+d@n)]  @rKi@n: @m%d@n(@y%d@n)\r\n", GET_SAVE(ch, SAVING_FORTITUDE), GET_SAVE_MOD(ch, SAVING_FORTITUDE), GET_SAVE(ch, SAVING_REFLEX), GET_SAVE_MOD(ch, SAVING_REFLEX), GET_SAVE(ch, SAVING_WILL), GET_SAVE_MOD(ch, SAVING_WILL), GET_KI(ch), GET_MAX_KI(ch));
  penalty = 100 - calc_penalty_exp(ch, 100);
  if (penalty)
    snprintf(penstr, sizeof(penstr), " (%d%% penalty)", penalty);
  else
    penstr[0] = 0;

  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAP - 1)
    send_to_char(ch, "@rExperience Points@n : @m%d@n @R%s@n (@y%d @Mtill next level@n)\r\n", GET_EXP(ch), penstr, level_exp(GET_LEVEL(ch) + 1) - GET_EXP(ch));

  send_to_char(ch, "@rGold@n: @Y%d @rBank@n: @Y%d@n\r\n", GET_GOLD(ch), GET_BANK_GOLD(ch));
  if (ch->hit_breakdown[0] || ch->hit_breakdown[1]) {
    send_to_char(ch, "@rBreakdown of your last attack@n:\r\n");
    if (ch->hit_breakdown[0] && ch->hit_breakdown[0][0] && ch->dam_breakdown[0])
      send_to_char(ch, "@rPrimary attack@n: @y%s.@n", ch->hit_breakdown[0]);
      if (ch->dam_breakdown[0])
        send_to_char(ch, "@y dam %s %s@n\r\n", ch->dam_breakdown[0],
                     ch->crit_breakdown[0] ? ch->crit_breakdown[0] : "");
      else
        send_to_char(ch, "\r\n");
      send_to_char(ch, "@rOffhand attack@n: @y%s.@n", ch->hit_breakdown[1]);
      if (ch->dam_breakdown[1])
        send_to_char(ch, "@y dam %s %s@n\r\n", ch->dam_breakdown[1],
                     ch->crit_breakdown[1] ? ch->crit_breakdown[1] : "");
      else
        send_to_char(ch, "\r\n");
  }
  if (IS_ARCANE(ch) && GET_SPELLFAIL(ch))
    send_to_char(ch, "Your armor causes %d%% failure in arcane spells with somatic components.\r\n", GET_SPELLFAIL(ch));

  if (ch->damreduct)
    for (reduct = ch->damreduct; reduct; reduct = reduct->next)
      send_to_char(ch, "@rDamage reduction@n: @g%s@n\r\n", reduct_desc(reduct));

  switch (GET_POS(ch)) {
  case POS_DEAD:
    send_to_char(ch, "You are DEAD!\r\n");
  break;
  case POS_MORTALLYW:
    send_to_char(ch, "You are mortally wounded! You should seek help!\r\n");
  break;
  case POS_INCAP:
    send_to_char(ch, "You are incapacitated, slowly fading away...\r\n");
  break;
  case POS_STUNNED:
    send_to_char(ch, "You are stunned! You can't move!\r\n");
  break;
  case POS_SLEEPING:
    send_to_char(ch, "You are sleeping.\r\n");
  break;
  case POS_RESTING:
    send_to_char(ch, "You are resting.\r\n");
  break;
  case POS_SITTING:
    send_to_char(ch, "You are sitting.\r\n");
  break;
  case POS_FIGHTING:
    send_to_char(ch, "You are fighting %s.\r\n", FIGHTING(ch) ? PERS(FIGHTING(ch), ch) : "thin air");
  break;
  case POS_STANDING:
    send_to_char(ch, "You are standing.\r\n");
  break;
  default:
    send_to_char(ch, "You are floating.\r\n");
  break;
  }

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You are intoxicated.\r\n");

  if (GET_COND(ch, FULL) == 0)
    send_to_char(ch, "You are hungry.\r\n");

  if (GET_COND(ch, THIRST) == 0)
    send_to_char(ch, "You are thirsty.\r\n");

  if (AFF_FLAGGED(ch, AFF_BLIND))
    send_to_char(ch, "You have been blinded!\r\n");

  if (AFF_FLAGGED(ch, AFF_INVISIBLE))
    send_to_char(ch, "You are invisible.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_INVIS))
    send_to_char(ch, "You are sensitive to the presence of invisible things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SANCTUARY))
    send_to_char(ch, "You are protected by Sanctuary.\r\n");

  if (AFF_FLAGGED(ch, AFF_POISON))
    send_to_char(ch, "You are poisoned!\r\n");

  if (AFF_FLAGGED(ch, AFF_CHARM))
    send_to_char(ch, "You have been charmed!\r\n");

  if (affected_by_spell(ch, SPELL_MAGE_ARMOR))
    send_to_char(ch, "You feel protected.\r\n");

  if (AFF_FLAGGED(ch, AFF_INFRAVISION))
    send_to_char(ch, "You can see in darkness with infravision.\r\n");

  if (PRF_FLAGGED(ch, PRF_SUMMONABLE))
    send_to_char(ch, "You are summonable by other players.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_ALIGN))
    send_to_char(ch, "You see into the hearts of others.\r\n");

  if (AFF_FLAGGED(ch, AFF_DETECT_MAGIC))
    send_to_char(ch, "You are sensitive to the magical nature of things.\r\n");

  if (AFF_FLAGGED(ch, AFF_SPIRIT))
    send_to_char(ch, "You have died are are part of the SPIRIT world!\r\n");

  if (AFF_FLAGGED(ch, AFF_ETHEREAL))
    send_to_char(ch, "You are ethereal and cannot interact with normal space!\r\n");

}


ACMD(do_inventory)
{
  send_to_char(ch, "You are carrying:\r\n");
  list_obj_to_char(ch->carrying, ch, SHOW_OBJ_SHORT, TRUE);
}


ACMD(do_equipment)
{
  int i, found = 0;

  send_to_char(ch, "You are using:\r\n");
  for (i = 0; i < NUM_WEARS; i++) {
    if (GET_EQ(ch, i)) {
      if (CAN_SEE_OBJ(ch, GET_EQ(ch, i))) {
	send_to_char(ch, "%s", wear_where[i]);
	if (((i == WEAR_WIELD1 || i == WEAR_WIELD2) &&
             (GET_OBJ_TYPE(GET_EQ(ch, i)) == ITEM_WEAPON) &&
             !is_proficient_with_weapon(ch, GET_OBJ_VAL(GET_EQ(ch, i), VAL_WEAPON_SKILL))) ||
	    (i == WEAR_BODY && !is_proficient_with_armor(ch, GET_OBJ_VAL(GET_EQ(ch, i), VAL_ARMOR_SKILL))))
          send_to_char(ch, "(unskilled) ");
	show_obj_to_char(GET_EQ(ch, i), ch, SHOW_OBJ_SHORT);
	found = TRUE;
      } else {
	send_to_char(ch, "%s", wear_where[i]);
	send_to_char(ch, "Something.\r\n");
	found = TRUE;
      }
    }
  }
  if (!found)
    send_to_char(ch, " Nothing.\r\n");
}


ACMD(do_time)
{
  const char *suf;
  int weekday, day;

  /* day in [1..30] */
  day = time_info.day + 1;

  /* 30 days in a month, 6 days a week */
  weekday = day % 6;

  send_to_char(ch, "It is %d o'clock %s, on %s.\r\n",
	  (time_info.hours % 12 == 0) ? 12 : (time_info.hours % 12),
	  time_info.hours >= 12 ? "pm" : "am", weekdays[weekday]);

  /*
   * Peter Ajamian <peter@PAJAMIAN.DHS.ORG> supplied the following as a fix
   * for a bug introduced in the ordinal display that caused 11, 12, and 13
   * to be incorrectly displayed as 11st, 12nd, and 13rd.  Nate Winters
   * <wintersn@HOTMAIL.COM> had already submitted a fix, but it hard-coded a
   * limit on ordinal display which I want to avoid.	-dak
   */

  suf = "th";

  if (((day % 100) / 10) != 1) {
    switch (day % 10) {
    case 1:
      suf = "st";
      break;
    case 2:
      suf = "nd";
      break;
    case 3:
      suf = "rd";
      break;
    }
  }

  send_to_char(ch, "The %d%s Day of the %s, Year %d.\r\n",
	  day, suf, month_name[time_info.month], time_info.year);
}


ACMD(do_weather)
{
  const char *sky_look[] = {
    "cloudless",
    "cloudy",
    "rainy",
    "lit by flashes of lightning"
  };

  if (OUTSIDE(ch))
    {
    send_to_char(ch, "The sky is %s and %s.\r\n", sky_look[weather_info.sky],
	    weather_info.change >= 0 ? "you feel a warm wind from south" :
	     "your foot tells you bad weather is due");
    if (ADM_FLAGGED(ch, ADM_KNOWWEATHER))
      send_to_char(ch, "Pressure: %d (change: %d), Sky: %d (%s)\r\n",
                 weather_info.pressure,
                 weather_info.change,
                 weather_info.sky,
                 sky_look[weather_info.sky]);
    }
  else
    send_to_char(ch, "You have no feeling about the weather at all.\r\n");
}


ACMD(do_help)
{
  int chk, bot, top, mid, minlen;

  if (!ch->desc)
    return;

  skip_spaces(&argument);

  if (!*argument) {
    page_string(ch->desc, help, 0);
    return;
  }
  if (!help_table) {
    send_to_char(ch, "No help available.\r\n");
    return;
  }

  bot = 0;
  top = top_of_helpt;
  minlen = strlen(argument);

  for (;;) {
    mid = (bot + top) / 2;

    if (bot > top) {
      send_to_char(ch, "There is no help on that word.\r\n");
      mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s tried to get help on %s", GET_NAME(ch), argument);
      return;
    } else if (!(chk = strn_cmp(argument, help_table[mid].keyword, minlen))) {
      /* trace backwards to find first matching entry. Thanks Jeff Fink! */
      while ((mid > 0) &&
	 (!(chk = strn_cmp(argument, help_table[mid - 1].keyword, minlen))))
	mid--;
      page_string(ch->desc, help_table[mid].entry, 0);
      return;
    } else {
      if (chk > 0)
        bot = mid + 1;
      else
        top = mid - 1;
    }
  }
}



#define WHO_FORMAT \
"format: who [minlev[-maxlev]] [-n name] [-s] [-o] [-q] [-r] [-z]\r\n"

/* FIXME: This whole thing just needs rewritten. */
ACMD(do_who)
{
  struct descriptor_data *d;
  struct char_data *tch;
  char name_search[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  char mode;
  int low = 0, high = CONFIG_LEVEL_CAP, localwho = 0, questwho = 0;
  int showclass = 0, short_list = 0, outlaws = 0, num_can_see = 0;
  int who_room = 0, showrace = 0;
  char *line_color = "@n";


  skip_spaces(&argument);
  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  name_search[0] = '\0';

  while (*buf) {
    char arg[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (isdigit(*arg)) {
      sscanf(arg, "%d-%d", &low, &high);
      strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
    } else if (*arg == '-') {
      mode = *(arg + 1);       /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'z':
	localwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 's':
	short_list = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'q':
	questwho = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	half_chop(buf1, name_search, buf);
	break;
      case 'r':
	who_room = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      default:
	send_to_char(ch, "%s", WHO_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", WHO_FORMAT);
      return;
    }
  }				/* end while (parser) */

  send_to_char(ch, "Players\r\n-------\r\n");

  for (d = descriptor_list; d; d = d->next) {
    if (!IS_PLAYING(d))
      continue;

    if (d->original)
      tch = d->original;
    else if (!(tch = d->character))
      continue;

    if (GET_ADMLEVEL(tch) >= ADMLVL_IMMORT)
      line_color = "@y";
    else
      line_color = "@n";
    if (*name_search && str_cmp(GET_NAME(tch), name_search) &&
	!strstr(GET_TITLE(tch), name_search))
      continue;
    if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
      continue;
    if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	!PLR_FLAGGED(tch, PLR_THIEF))
      continue;
    if (questwho && !PRF_FLAGGED(tch, PRF_QUEST))
      continue;
    if (localwho && world[IN_ROOM(ch)].zone != world[IN_ROOM(tch)].zone)
      continue;
    if (who_room && (IN_ROOM(tch) != IN_ROOM(ch)))
      continue;
    if (showclass && !(showclass & (1 << GET_CLASS(tch))))
      continue;
     if (showrace && !(showrace & (1 << GET_RACE(tch))))
       continue;
    if (short_list) {
      send_to_char(ch, "%s[%2d %s %s] %-12.12s@n%s", line_color,
	      GET_LEVEL(tch), CLASS_ABBR(tch), RACE_ABBR(tch), GET_NAME(tch),
	      ((!(++num_can_see % 3)) ? "\r\n" : ""));
    } else {
      num_can_see++;
      if (CONFIG_ALLOW_MULTICLASS)
        send_to_char(ch, "%s[%2d %s] %s %s%s (%s)",
              line_color, GET_LEVEL(tch), RACE_ABBR(tch), GET_NAME(tch),
              GET_TITLE(tch), line_color, class_desc_str(tch, TRUE, 0));
      else
        send_to_char(ch, "%s[%2d %s %s] %s%s%s%s",
	      line_color, GET_LEVEL(tch), CLASS_ABBR(tch), RACE_ABBR(tch),
              GET_NAME(tch), *GET_TITLE(tch) ? " " : "", GET_TITLE(tch),
              line_color);

      if (GET_ADMLEVEL(tch))
        send_to_char(ch, " (%s)", admin_level_names[GET_ADMLEVEL(tch)]);

      if (GET_INVIS_LEV(tch))
	send_to_char(ch, " (i%d)", GET_INVIS_LEV(tch));
      else if (AFF_FLAGGED(tch, AFF_INVISIBLE))
	send_to_char(ch, " (invis)");

      if (PLR_FLAGGED(tch, PLR_MAILING))
	send_to_char(ch, " (mailing)");
      else if (d->olc)
        send_to_char(ch, " (OLC)");
      else if (PLR_FLAGGED(tch, PLR_WRITING))
	send_to_char(ch, " (writing)");

      if (d->original)
        send_to_char(ch, " (out of body)");

      if (d->connected == CON_OEDIT)
        send_to_char(ch, " (Object Edit)");
      if (d->connected == CON_MEDIT)
        send_to_char(ch, " (Mobile Edit)");
      if (d->connected == CON_ZEDIT)
        send_to_char(ch, " (Zone Edit)");
      if (d->connected == CON_SEDIT)
        send_to_char(ch, " (Shop Edit)");
      if (d->connected == CON_REDIT)
        send_to_char(ch, " (Room Edit)");
      if (d->connected == CON_TEDIT)
        send_to_char(ch, " (Text Edit)");
      if (d->connected == CON_TRIGEDIT)
        send_to_char(ch, " (Trigger Edit)");
      if (d->connected == CON_AEDIT)
        send_to_char(ch, " (Social Edit)");
      if (d->connected == CON_CEDIT)
        send_to_char(ch, " (Configuration Edit)");

      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
      send_to_char(ch, " (Buildwalking)");

      if (PRF_FLAGGED(tch, PRF_DEAF))
	send_to_char(ch, " (deaf)");
      if (PRF_FLAGGED(tch, PRF_NOTELL))
	send_to_char(ch, " (notell)");
      if (PRF_FLAGGED(tch, PRF_NOGOSS))
        send_to_char(ch, " (nogos)");
      if (PRF_FLAGGED(tch, PRF_QUEST))
	send_to_char(ch, " (quest)");
      if (PLR_FLAGGED(tch, PLR_THIEF))
	send_to_char(ch, " (THIEF)");
      if (PLR_FLAGGED(tch, PLR_KILLER))
	send_to_char(ch, " (KILLER)");
      if (PRF_FLAGGED(tch, PRF_BUILDWALK))
	send_to_char(ch, " (Buildwalking)");
      if (PRF_FLAGGED(tch, PRF_AFK))
        send_to_char(ch, " (AFK)");
      if (PRF_FLAGGED(tch, PRF_NOWIZ))
        send_to_char(ch, " (nowiz)");
      send_to_char(ch, "@n\r\n");
    }				/* endif shortlist */
  }				/* end of for */
  if (short_list && (num_can_see % 4))
    send_to_char(ch, "\r\n");
  if (num_can_see == 0)
    send_to_char(ch, "\r\nNobody at all!\r\n");
  else if (num_can_see == 1)
    send_to_char(ch, "\r\nOne lonely character displayed.\r\n");
  else
    send_to_char(ch, "\r\n%d characters displayed.\r\n", num_can_see);
}


#define USERS_FORMAT \
"format: users [-l minlevel[-maxlevel]] [-n name] [-h host] [-o] [-p]\r\n"

/* BIG OL' FIXME: Rewrite it all. Similar to do_who(). */
ACMD(do_users)
{
  char line[200], line2[220], idletime[10];
  char state[30], *timeptr, mode;
  char name_search[MAX_INPUT_LENGTH], host_search[MAX_INPUT_LENGTH];
  struct char_data *tch;
  struct descriptor_data *d;
  int low = 0, high = CONFIG_LEVEL_CAP, num_can_see = 0;
  int showclass = 0, outlaws = 0, playing = 0, deadweight = 0, showrace = 0;
  char buf[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  host_search[0] = name_search[0] = '\0';

  strcpy(buf, argument);	/* strcpy: OK (sizeof: argument == buf) */
  while (*buf) {
    char buf1[MAX_INPUT_LENGTH];

    half_chop(buf, arg, buf1);
    if (*arg == '-') {
      mode = *(arg + 1);  /* just in case; we destroy arg in the switch */
      switch (mode) {
      case 'o':
      case 'k':
	outlaws = 1;
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'p':
	playing = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'd':
	deadweight = 1;
	strcpy(buf, buf1);	/* strcpy: OK (sizeof: buf1 == buf) */
	break;
      case 'l':
	playing = 1;
	half_chop(buf1, arg, buf);
	sscanf(arg, "%d-%d", &low, &high);
	break;
      case 'n':
	playing = 1;
	half_chop(buf1, name_search, buf);
	break;
      case 'h':
	playing = 1;
	half_chop(buf1, host_search, buf);
	break;
      default:
	send_to_char(ch, "%s", USERS_FORMAT);
	return;
      }				/* end of switch */

    } else {			/* endif */
      send_to_char(ch, "%s", USERS_FORMAT);
      return;
    }
  }				/* end while (parser) */
  send_to_char(ch,
         "Num Name         State          Idl Login    C Site\r\n"
         "--- ------------ -------------- --- -------- - -----------------------\r\n");

  one_argument(argument, arg);

  for (d = descriptor_list; d; d = d->next) {
    if (STATE(d) != CON_PLAYING && playing)
      continue;
    if (STATE(d) == CON_PLAYING && deadweight)
      continue;
    if (IS_PLAYING(d)) {
      if (d->original)
	tch = d->original;
      else if (!(tch = d->character))
	continue;

      if (*host_search && !strstr(d->host, host_search))
	continue;
      if (*name_search && str_cmp(GET_NAME(tch), name_search))
	continue;
      if (!CAN_SEE(ch, tch) || GET_LEVEL(tch) < low || GET_LEVEL(tch) > high)
	continue;
      if (outlaws && !PLR_FLAGGED(tch, PLR_KILLER) &&
	  !PLR_FLAGGED(tch, PLR_THIEF))
	continue;
      if (showclass && !(showclass & (1 << GET_CLASS(tch))))
	continue;
      if (showrace && !(showrace & (1 << GET_RACE(tch))))
        continue;
      if (GET_INVIS_LEV(ch) > GET_LEVEL(ch))
	continue;
    }

    timeptr = asctime(localtime(&d->login_time));
    timeptr += 11;
    *(timeptr + 8) = '\0';

    if (STATE(d) == CON_PLAYING && d->original)
      strcpy(state, "Switched");
    else
      strcpy(state, connected_types[STATE(d)]);

    if (d->character && STATE(d) == CON_PLAYING && GET_ADMLEVEL(d->character) <= GET_ADMLEVEL(ch))
      sprintf(idletime, "%3d", d->character->timer *
	      SECS_PER_MUD_HOUR / SECS_PER_REAL_MIN);
    else
      strcpy(idletime, "");

    sprintf(line, "%3d %-12s %-14s %-3s %-8s %1s ", d->desc_num,
	d->original && d->original->name ? d->original->name :
	d->character && d->character->name ? d->character->name :
	"UNDEFINED", state, idletime, timeptr,
        d->comp->state ? d->comp->state == 1 ? "?" : "Y" : "N");

    if (d->host && *d->host)
      sprintf(line + strlen(line), "[%s]\r\n", d->host);
    else
      strcat(line, "[Hostname unknown]\r\n");

    if (STATE(d) != CON_PLAYING) {
      sprintf(line2, "@g%s@n", line);
      strcpy(line, line2);
    }
    if (STATE(d) != CON_PLAYING ||
		(STATE(d) == CON_PLAYING && CAN_SEE(ch, d->character))) {
      send_to_char(ch, "%s", line);
      num_can_see++;
    }
  }

  send_to_char(ch, "\r\n%d visible sockets connected.\r\n", num_can_see);
}


/* Generic page_string function for displaying text */
ACMD(do_gen_ps)
{
  int patch_num, show_patches;
  char arg[MAX_INPUT_LENGTH];
  const char *version_args[] = {"full", "complete", "\n"}; 
  one_argument(argument, arg);
  
  switch (subcmd) {
  case SCMD_CREDITS:
    page_string(ch->desc, credits, 0);
    break;
  case SCMD_NEWS:
    page_string(ch->desc, news, 0);
    break;
  case SCMD_INFO:
    page_string(ch->desc, info, 0);
    break;
  case SCMD_WIZLIST:
    page_string(ch->desc, wizlist, 0);
    break;
  case SCMD_IMMLIST:
    page_string(ch->desc, immlist, 0);
    break;
  case SCMD_HANDBOOK:
    page_string(ch->desc, handbook, 0);
    break;
  case SCMD_POLICIES:
    page_string(ch->desc, policies, 0);
    break;
  case SCMD_MOTD:
    page_string(ch->desc, motd, 0);
    break;
  case SCMD_IMOTD:
    page_string(ch->desc, imotd, 0);
    break;
  case SCMD_CLEAR:
    send_to_char(ch, "\033[H\033[J");
    break;
  case SCMD_VERSION:
    if (!*arg)
      show_patches = FALSE;
    else
      if (search_block(arg, version_args, FALSE ) != -1)
        show_patches = TRUE;
      else
      show_patches = FALSE;
    send_to_char(ch, "%s\r\n", circlemud_version);
    send_to_char(ch, "%s\r\n", oasisolc_version);
    send_to_char(ch, "%s\r\n", DG_SCRIPT_VERSION);
    send_to_char(ch, "%s\r\n", CWG_VERSION);
    if (show_patches == TRUE) {
      send_to_char(ch, "The following patches have been installed:\r\n");
      for (patch_num = 0; **(patch_list + patch_num) != '\n'; patch_num++)
        send_to_char(ch, "%2d: %s\r\n", patch_num, patch_list[patch_num]);
    }
    break;
  case SCMD_WHOAMI:
    send_to_char(ch, "%s\r\n", GET_NAME(ch));
    break;
  default:
    log("SYSERR: Unhandled case in do_gen_ps. (%d)", subcmd);
    return;
  }
}


void perform_mortal_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct descriptor_data *d;

  if (!*arg) {
    send_to_char(ch, "Players in your Zone\r\n--------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || d->character == ch)
	continue;
      if ((i = (d->original ? d->original : d->character)) == NULL)
	continue;
      if (IN_ROOM(i) == NOWHERE || !CAN_SEE(ch, i))
	continue;
      if (world[IN_ROOM(ch)].zone != world[IN_ROOM(i)].zone)
	continue;
      send_to_char(ch, "%-20s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
    }
  } else {			/* print only FIRST char, not all. */
    for (i = character_list; i; i = i->next) {
      if (IN_ROOM(i) == NOWHERE || i == ch)
	continue;
      if (!CAN_SEE(ch, i) || world[IN_ROOM(i)].zone != world[IN_ROOM(ch)].zone)
	continue;
      if (!isname(arg, i->name))
	continue;
      send_to_char(ch, "%-25s - %s\r\n", GET_NAME(i), world[IN_ROOM(i)].name);
      return;
    }
    send_to_char(ch, "Nobody around by that name.\r\n");
  }
}


void print_object_location(int num, struct obj_data *obj, struct char_data *ch,
			        int recur)
{
  if (num > 0)
    send_to_char(ch, "O%3d. %-25s - ", num, obj->short_description);
  else
    send_to_char(ch, "%33s", " - ");

  if (obj->proto_script)
    send_to_char(ch, "[TRIG]");

  if (IN_ROOM(obj) != NOWHERE)
    send_to_char(ch, "[%5d] %s\r\n", GET_ROOM_VNUM(IN_ROOM(obj)), world[IN_ROOM(obj)].name);
  else if (obj->carried_by)
    send_to_char(ch, "carried by %s\r\n", PERS(obj->carried_by, ch));
  else if (obj->worn_by)
    send_to_char(ch, "worn by %s\r\n", PERS(obj->worn_by, ch));
  else if (obj->in_obj) {
    send_to_char(ch, "inside %s%s\r\n", obj->in_obj->short_description, (recur ? ", which is" : " "));
    if (recur)
      print_object_location(0, obj->in_obj, ch, recur);
  } else
    send_to_char(ch, "in an unknown location\r\n");
}



void perform_immort_where(struct char_data *ch, char *arg)
{
  struct char_data *i;
  struct obj_data *k;
  struct descriptor_data *d;
  int num = 0, found = 0;

  if (!*arg) {
    send_to_char(ch, "Players\r\n-------\r\n");
    for (d = descriptor_list; d; d = d->next)
      if (IS_PLAYING(d)) {
	i = (d->original ? d->original : d->character);
	if (i && CAN_SEE(ch, i) && (IN_ROOM(i) != NOWHERE)) {
	  if (d->original)
	    send_to_char(ch, "%-20s - [%5d] %s (in %s)\r\n",
		GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(d->character)),
		world[IN_ROOM(d->character)].name, GET_NAME(d->character));
	  else
	    send_to_char(ch, "%-20s - [%5d] %s\r\n", GET_NAME(i), GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name);
	}
      }
  } else {
    for (i = character_list; i; i = i->next)
      if (CAN_SEE(ch, i) && IN_ROOM(i) != NOWHERE && isname(arg, i->name)) {
	found = 1;
	send_to_char(ch, "M%3d. %-25s - [%5d] %-25s %s\r\n", ++num, GET_NAME(i),
		GET_ROOM_VNUM(IN_ROOM(i)), world[IN_ROOM(i)].name,
		(IS_NPC(i) && i->proto_script) ? "[TRIG]" : "");
      }
    for (num = 0, k = object_list; k; k = k->next)
      if (CAN_SEE_OBJ(ch, k) && isname(arg, k->name)) {
	found = 1;
	print_object_location(++num, k, ch, TRUE);
      }
    if (!found)
      send_to_char(ch, "Couldn't find any such thing.\r\n");
  }
}



ACMD(do_where)
{
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (ADM_FLAGGED(ch, ADM_FULLWHERE))
    perform_immort_where(ch, arg);
  else
    perform_mortal_where(ch, arg);
}



ACMD(do_levels)
{
  char buf[MAX_STRING_LENGTH];
  size_t i, len = 0, nlen;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You ain't nothin' but a hound-dog.\r\n");
    return;
  }

  for (i = 1; i < CONFIG_LEVEL_CAP; i++) {
    if (i == CONFIG_LEVEL_CAP - 1)
      nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d          : \r\n",
           CONFIG_LEVEL_CAP - 1, level_exp(CONFIG_LEVEL_CAP - 1));
    else
    nlen = snprintf(buf + len, sizeof(buf) - len, "[%2d] %8d-%-8d : \r\n", i,
		level_exp(i), level_exp(i + 1) - 1);
    if (len + nlen >= sizeof(buf) || nlen < 0)
      break;
    len += nlen;
  }

  page_string(ch->desc, buf, TRUE);
}



ACMD(do_consider)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *victim;
  int diff;

  one_argument(argument, buf);

  if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM))) {
    send_to_char(ch, "Consider killing who?\r\n");
    return;
  }
  if (victim == ch) {
    send_to_char(ch, "Easy!  Very easy indeed!\r\n");
    return;
  }
  if (!IS_NPC(victim)) {
    send_to_char(ch, "Would you like to borrow a cross and a shovel?\r\n");
    return;
  }
  diff = (GET_LEVEL(victim) - GET_LEVEL(ch));

  if (diff <= -10)
    send_to_char(ch, "Now where did that chicken go?\r\n");
  else if (diff <= -5)
    send_to_char(ch, "You could do it with a needle!\r\n");
  else if (diff <= -2)
    send_to_char(ch, "Easy.\r\n");
  else if (diff <= -1)
    send_to_char(ch, "Fairly easy.\r\n");
  else if (diff == 0)
    send_to_char(ch, "The perfect match!\r\n");
  else if (diff <= 1)
    send_to_char(ch, "You would need some luck!\r\n");
  else if (diff <= 2)
    send_to_char(ch, "You would need a lot of luck!\r\n");
  else if (diff <= 3)
    send_to_char(ch, "You would need a lot of luck and great equipment!\r\n");
  else if (diff <= 5)
    send_to_char(ch, "Do you feel lucky, punk?\r\n");
  else if (diff <= 10)
    send_to_char(ch, "Are you mad!?\r\n");
  else if (diff <= 100)
    send_to_char(ch, "You ARE mad!\r\n");
}



ACMD(do_diagnose)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  one_argument(argument, buf);

  if (*buf) {
    if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else
      diag_char_to_char(vict, ch);
  } else {
    if (FIGHTING(ch))
      diag_char_to_char(FIGHTING(ch), ch);
    else
      send_to_char(ch, "Diagnose who?\r\n");
  }
}


const char *ctypes[] = {
  "off", "on", "\n"
};

char *cchoice_to_str(char *col)
{
  static char buf[READ_SIZE];
  char *s = NULL;
  int i = 0;
  int fg = 0;
  int needfg = 0;
  int bold = 0;

  if (!col) {
    buf[0] = 0;
    return buf;
  }
  while (*col) {
    if (strchr(ANSISTART, *col)) {
      col++;
    } else {
      switch (*col) {
      case ANSISEP:
      case ANSIEND:
        s = NULL;
        break;
      case '0':
        s = NULL;
        break;
      case '1':
        bold = 1;
        s = NULL;
        break;
      case '5':
        s = "blinking";
        break;
      case '7':
        s = "reverse";
        break;
      case '8':
        s = "invisible";
        break;
      case '3':
        col++;
        fg = 1;
        switch (*col) {
        case '0':
          s = bold ? "grey" : "black";
          bold = 0;
          fg = 1;
          break;
        case '1':
          s = "red";
          fg = 1;
          break;
        case '2':
          s = "green";
          fg = 1;
          break;
        case '3':
          s = "yellow";
          fg = 1;
          break;
        case '4':
          s = "blue";
          fg = 1;
          break;
        case '5':
          s = "magenta";
          fg = 1;
          break;
        case '6':
          s = "cyan";
          fg = 1;
          break;
        case '7':
          s = "white";
          fg = 1;
          break;
        case 0:
          s = NULL;
          break;
        }
        break;
      case '4':
        col++;
        switch (*col) {
        case '0':
          s = "on black";
          needfg = 1;
          bold = 0;
        case '1':
          s = "on red";
          needfg = 1;
          bold = 0;
        case '2':
          s = "on green";
          needfg = 1;
          bold = 0;
        case '3':
          s = "on yellow";
          needfg = 1;
          bold = 0;
        case '4':
          s = "on blue";
          needfg = 1;
          bold = 0;
        case '5':
          s = "on magenta";
          needfg = 1;
          bold = 0;
        case '6':
          s = "on cyan";
          needfg = 1;
          bold = 0;
        case '7':
          s = "on white";
          needfg = 1;
          bold = 0;
        default:
          s = "underlined";
          break;
        }
        break;
      default:
        s = NULL;
        break;
      }
      if (s) {
        if (needfg && !fg) {
          i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
          fg = 1;
        }
        if (i)
          i += snprintf(buf + i, sizeof(buf) - i, " ");
        if (bold) {
          i += snprintf(buf + i, sizeof(buf) - i, "bright ");
          bold = 0;
        }
        i += snprintf(buf + i, sizeof(buf) - i, "%s", s ? s : "null 1");
        s = NULL;
      }
      col++;
    }
  }
  if (!fg)
    i += snprintf(buf + i, sizeof(buf) - i, "%snormal", i ? " " : "");
  return buf;
}

int str_to_cchoice(char *str, char *choice)
{
  char buf[MAX_STRING_LENGTH];
  int bold = 0, blink = 0, uline = 0, rev = 0, invis = 0, fg = 0, bg = 0, error = 0;
  int i, len = MAX_INPUT_LENGTH;
  struct {
    char *name; 
    int *ptr;
  } attribs[] = {
    { "bright", &bold },
    { "bold", &bold },
    { "underlined", &uline },
    { "reverse", &rev },
    { "blinking", &blink },
    { "invisible", &invis },
    { NULL, NULL }
  };
  struct {
    char *name;
    int val;
    int bold;
  } colors[] = {
    { "default", -1, 0 },
    { "normal", -1, 0 },
    { "black", 0, 0 },
    { "red", 1, 0 },
    { "green", 2, 0 },
    { "yellow", 3, 0 },
    { "blue", 4, 0 },
    { "magenta", 5, 0 },
    { "cyan", 6, 0 },
    { "white", 7, 0 },
    { "grey", 0, 1 },
    { "gray", 0, 1 },
    { NULL, 0, 0 }
  };
  skip_spaces(&str);
  if (isdigit(*str)) { /* Accept a raw code */
    strcpy(choice, str);
    for (i = 0; choice[i] && (isdigit(choice[i]) || choice[i] == ';'); i++);
    error = choice[i] != 0;
    choice[i] = 0;
    return error;
  }
  while (*str) {
    str = any_one_arg(str, buf);
    if (!strcmp(buf, "on")) {
      bg = 1;
      continue;
    }
    if (!fg) {
      for (i = 0; attribs[i].name; i++)
        if (!strncmp(attribs[i].name, buf, strlen(buf)))
          break;
      if (attribs[i].name) {
        *(attribs[i].ptr) = 1;
        continue;
      }
    }
    for (i = 0; colors[i].name; i++)
      if (!strncmp(colors[i].name, buf, strlen(buf)))
        break;
    if (!colors[i].name) {
      error = 1;
      continue;
    }
    if (colors[i].val != -1) {
      if (bg == 1) {
        bg = 40 + colors[i].val;
      } else {
        fg = 30 + colors[i].val;
        if (colors[i].bold)
          bold = 1;
      }
    }
  }
  choice[0] = i = 0;
  if (bold)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BOLD);
  if (uline)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_UNDERLINE);
  if (blink)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_BLINK);
  if (rev)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_REVERSE);
  if (invis)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_INVIS);
  if (!i)
    i += snprintf(choice + i, len - i, "%s%s", i ? ANSISEPSTR : "" , AA_NORMAL);
  if (fg && fg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , fg);
  if (bg && bg != -1)
    i += snprintf(choice + i, len - i, "%s%d", i ? ANSISEPSTR : "" , bg);

  return error;
}

char *default_color_choices[NUM_COLOR + 1] = {
/* COLOR_NORMAL */	AA_NORMAL,
/* COLOR_ROOMNAME */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_ROOMOBJS */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_ROOMPEOPLE */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_HITYOU */	AA_NORMAL ANSISEPSTR AF_RED,
/* COLOR_YOUHIT */	AA_NORMAL ANSISEPSTR AF_GREEN,
/* COLOR_OTHERHIT */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_CRITICAL */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_HOLLER */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_SHOUT */	AA_BOLD ANSISEPSTR AF_YELLOW,
/* COLOR_GOSSIP */	AA_NORMAL ANSISEPSTR AF_YELLOW,
/* COLOR_AUCTION */	AA_NORMAL ANSISEPSTR AF_CYAN,
/* COLOR_CONGRAT */	AA_NORMAL ANSISEPSTR AF_GREEN,
NULL
};

ACMD(do_color)
{
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  char *p;
  char *col;
  int tp, len;

  if (IS_NPC(ch))
    return;

  p = any_one_arg(argument, arg);

  if (!*arg) {
    len = snprintf(buf, sizeof(buf), "Currently, color is %s.\r\n", ctypes[COLOR_LEV(ch)]);
    if (COLOR_LEV(ch)) {
      len += snprintf(buf + len, sizeof(buf) - len, "\r\nYour color choices:\r\n");
      for (tp = 0; tp < NUM_COLOR; tp++) {
        len += snprintf(buf + len, sizeof(buf) - len, " %2d: %-20s - ", tp,
                        cchoice_names[tp]);
        col = ch->player_specials->color_choices[tp];
        if (!col) {
          len += snprintf(buf + len, sizeof(buf) - len, "(default) ");
          col = default_color_choices[tp];
        }
        len += snprintf(buf + len, sizeof(buf) - len, "%s (%s)\r\n", cchoice_to_str(col), col);
      }
    }
    page_string(ch->desc, buf, TRUE);
    return;
  }
  if (isdigit(*arg)) {
    tp = atoi(arg);
    if (tp < 0 || tp >= NUM_COLOR) {
      send_to_char(ch, "Custom color selection out of range.\r\n");
      return;
    }
    skip_spaces(&p);
    if (!strcmp(p, "default")) {
      if (ch->player_specials->color_choices[tp])
        free(ch->player_specials->color_choices[tp]);
      ch->player_specials->color_choices[tp] = NULL;
      send_to_char(ch, "Using default color for %s\r\n", cchoice_names[tp]);
    } else if (str_to_cchoice(p, buf)) {
      send_to_char(ch, "Invalid color choice.\r\n\r\n"
"Format: code | ( [ attributes ] [ foreground | \"default\" ] [ \"on\" background ] )\r\n"
"Attributes: underlined blink reverse invisible\r\n"
"Foreground colors: black, grey, red, bright red, green, bright green, yellow,\r\n"
"                   bright yellow, blue, bright blue, magenta,\r\n"
"                   bright magenta, cyan, bright cyan, white, bright white\r\n"
"Background colors: black, red, green, yellow, blue, magenta, cyan, white\r\n"
"Examples: red, bright blue on yellow, 1;34;45\r\n");
    } else {
      if (ch->player_specials->color_choices[tp])
        free(ch->player_specials->color_choices[tp]);
      ch->player_specials->color_choices[tp] = strdup(buf);
      send_to_char(ch, "Setting color %d to %s: @[%d]sample@n\r\n", tp, cchoice_to_str(buf), tp);
    }
    return;
  } else if (((tp = search_block(arg, ctypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: color [ off | on | number [ color choice ] ]\r\n");
    return;
  }
  switch (tp) {
    case C_OFF:
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
      break;
    case C_ON:
      SET_BIT_AR(PRF_FLAGS(ch), PRF_COLOR);
      break;
  }
  send_to_char(ch, "Your color is now @o%s@n.\r\n", ctypes[tp]);
}


ACMD(do_toggle)
{
  char buf2[4];

  if (IS_NPC(ch))
    return;

  if (GET_WIMP_LEV(ch) == 0)
    strcpy(buf2, "OFF");	/* strcpy: OK */
  else
    sprintf(buf2, "%-3.3d", GET_WIMP_LEV(ch));	/* sprintf: OK */

  if (GET_ADMLEVEL(ch)) {
    send_to_char(ch,
          "      Buildwalk: %-3s    "
          "Clear Screen in OLC: %-3s\r\n",
        ONOFF(PRF_FLAGGED(ch, PRF_BUILDWALK)),
        ONOFF(PRF_FLAGGED(ch, PRF_CLS))
    );

    send_to_char(ch,
	  "      No Hassle: %-3s    "
	  "      Holylight: %-3s    "
	  "     Room Flags: %-3s\r\n",
	ONOFF(PRF_FLAGGED(ch, PRF_NOHASSLE)),
	ONOFF(PRF_FLAGGED(ch, PRF_HOLYLIGHT)),
	ONOFF(PRF_FLAGGED(ch, PRF_ROOMFLAGS))
    );
  }

  send_to_char(ch,
	  "Hit Pnt Display: %-3s    "
	  "     Brief Mode: %-3s    "
	  " Summon Protect: %-3s\r\n"

	  "   Move Display: %-3s    "
	  "   Compact Mode: %-3s    "
	  "       On Quest: %-3s\r\n"

	  "   Mana Display: %-3s    "
	  "         NoTell: %-3s    "
	  "   Repeat Comm.: %-3s\r\n"

	  "     Ki Display: %-3s    "
	  "           Deaf: %-3s    "
	  "     Wimp Level: %-3s\r\n"

	  " Gossip Channel: %-3s    "
	  "Auction Channel: %-3s    "
	  "  Grats Channel: %-3s\r\n"

          "      Auto Loot: %-3s    "
          "      Auto Gold: %-3s    "
	  "    Color Level: %s\r\n"

          "     Auto Split: %-3s    "
          "       Auto Sac: %-3s    "
          "       Auto Mem: %-3s\r\n"

          "     View Order: %-3s    "
          "    Auto Assist: %-3s    "
	  " Auto Show Exit: %-3s",

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPHP)),
	  ONOFF(PRF_FLAGGED(ch, PRF_BRIEF)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_SUMMONABLE)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMOVE)),
	  ONOFF(PRF_FLAGGED(ch, PRF_COMPACT)),
	  YESNO(PRF_FLAGGED(ch, PRF_QUEST)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPMANA)),
	  ONOFF(PRF_FLAGGED(ch, PRF_NOTELL)),
	  YESNO(!PRF_FLAGGED(ch, PRF_NOREPEAT)),

	  ONOFF(PRF_FLAGGED(ch, PRF_DISPKI)),
	  YESNO(PRF_FLAGGED(ch, PRF_DEAF)),
	  buf2,

	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGOSS)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOAUCT)),
	  ONOFF(!PRF_FLAGGED(ch, PRF_NOGRATZ)),

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOLOOT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOGOLD)),
	  ctypes[COLOR_LEV(ch)],

          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSPLIT)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOSAC)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOMEM)),

          ONOFF(PRF_FLAGGED(ch, PRF_VIEWORDER)),
          ONOFF(PRF_FLAGGED(ch, PRF_AUTOASSIST)),
	  ONOFF(PRF_FLAGGED(ch, PRF_AUTOEXIT)));

          if (CONFIG_ENABLE_COMPRESSION) {
            send_to_char(ch, "    Compression: %-3s\r\n", ONOFF(!PRF_FLAGGED(ch, PRF_NOCOMPRESS)));
          }
}


int sort_commands_helper(const void *a, const void *b)
{
  return strcmp(complete_cmd_info[*(const int *)a].sort_as,
                complete_cmd_info[*(const int *)b].sort_as);
}


void sort_commands(void)
{
  int a, num_of_cmds = 0;

  while (complete_cmd_info[num_of_cmds].command[0] != '\n')
    num_of_cmds++;
  num_of_cmds++;	/* \n */

  CREATE(cmd_sort_info, int, num_of_cmds);

  for (a = 0; a < num_of_cmds; a++)
    cmd_sort_info[a] = a;

  /* Don't sort the RESERVED or \n entries. */
  qsort(cmd_sort_info + 1, num_of_cmds - 2, sizeof(int), sort_commands_helper);
}


ACMD(do_commands)
{
  int no, i, cmd_num;
  int wizhelp = 0, socials = 0;
  struct char_data *vict;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument, arg);

  if (*arg) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) || IS_NPC(vict)) {
      send_to_char(ch, "Who is that?\r\n");
      return;
    }
    if (GET_LEVEL(ch) < GET_LEVEL(vict)) {
      send_to_char(ch, "You can't see the commands of people above your level.\r\n");
      return;
    }
  } else
    vict = ch;

  if (subcmd == SCMD_SOCIALS)
    socials = 1;
  else if (subcmd == SCMD_WIZHELP)
    wizhelp = 1;

  send_to_char(ch, "The following %s%s are available to %s:\r\n",
	  wizhelp ? "privileged " : "",
	  socials ? "socials" : "commands",
	  vict == ch ? "you" : GET_NAME(vict));

  /* cmd_num starts at 1, not 0, to remove 'RESERVED' */
  for (no = 1, cmd_num = 1; complete_cmd_info[cmd_sort_info[cmd_num]].command[0] != '\n'; cmd_num++) {
    i = cmd_sort_info[cmd_num];

    if (complete_cmd_info[i].minimum_level < 0 ||
        GET_LEVEL(vict) < complete_cmd_info[i].minimum_level)
      continue;

    if (complete_cmd_info[i].minimum_admlevel < 0 ||
        GET_ADMLEVEL(vict) < complete_cmd_info[i].minimum_admlevel)
      continue;

    if ((complete_cmd_info[i].minimum_admlevel >= ADMLVL_IMMORT) != wizhelp)
      continue;

    if (!wizhelp && socials != (complete_cmd_info[i].command_pointer == do_action ||
                                complete_cmd_info[i].command_pointer == do_insult))
      continue;

     if (check_disabled(&complete_cmd_info[i]))
      sprintf(arg, "(%s)", complete_cmd_info[i].command);
    else  
      sprintf(arg, "%s", complete_cmd_info[i].command);

    send_to_char(ch, "%-11s%s", arg, no++ % 7 == 0 ? "\r\n" : "");

  }

  if (no % 7 != 1)
    send_to_char(ch, "\r\n");
}

ACMD(do_whois)
{
  const char *immlevels[ADMLVL_IMPL + 1] = {
  "[Mortal]",          /* lowest admin level */
  "[Immortal]",        /* lowest admin level +1 */
  "[Creator]",         /* lowest admin level +2 */
  "[God]",             /* lowest admin level +3 */
  "[Greater God]",     /* lowest admin level +4 */
  "[Implementor]",     /* lowest admin level +5 */
  };

  struct char_data *victim = 0;
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Who?\r\n");
  } else {
  CREATE(victim, struct char_data, 1);
  clear_char(victim);
  CREATE(victim->player_specials, struct player_special_data, 1);
  if (load_char(argument, victim) >= 0) {
    if (GET_ADMLEVEL(victim) >= ADMLVL_IMMORT)
      send_to_char(ch, "%s Level %d %s %s\r\n", immlevels[GET_ADMLEVEL(victim)],
                   GET_LEVEL(victim), GET_NAME(victim), GET_TITLE(victim));
    else 
      send_to_char(ch, "Level %d %s - %s %s\r\n", GET_LEVEL(victim),
                   CLASS_ABBR(victim), GET_NAME(victim), GET_TITLE(victim));
    } else {
    send_to_char(ch, "There is no such player.\r\n"); 
    }
    free(victim); 
  }
}

#define DOOR_DCHIDE(ch, door)           (EXIT(ch, door)->dchide)

void search_in_direction(struct char_data * ch, int dir)
{
  int check=FALSE, skill_lvl, dchide=20;

  send_to_char(ch, "You search for secret doors.\r\n");
  act("$n searches the area intently.", TRUE, ch, 0, 0, TO_ROOM);

  /* SEARCHING is allowed untrained */
  skill_lvl = roll_skill(ch, SKILL_SEARCH);
  if (IS_ELF(ch) || IS_DROW_ELF(ch)) 
    skill_lvl = skill_lvl + 2;
  if (IS_HALF_ELF(ch))
    skill_lvl = skill_lvl + 1;

  if (EXIT(ch, dir))
    dchide = DOOR_DCHIDE(ch, dir);

  if (skill_lvl > dchide)
    check = TRUE;

  if (EXIT(ch, dir)) {
    if (EXIT(ch, dir)->general_description &&
        !EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, EXIT(ch, dir)->general_description);
    else if (!EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET))
      send_to_char(ch, "There is a normal exit there.\r\n");
    else if (EXIT_FLAGGED(EXIT(ch, dir), EX_ISDOOR) &&
             EXIT_FLAGGED(EXIT(ch, dir), EX_SECRET) &&
             EXIT(ch, dir)->keyword && (check == TRUE)  )
      send_to_char(ch, "There is a hidden door keyword: '%s' %sthere.\r\n",
                   fname (EXIT(ch, dir)->keyword),
                   (EXIT_FLAGGED(EXIT(ch, dir), EX_CLOSED)) ? "" : "open ");
    else
      send_to_char(ch, "There is no exit there.\r\n");
  } else
    send_to_char(ch, "There is no exit there.\r\n");
}

