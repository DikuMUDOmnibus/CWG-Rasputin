/* ************************************************************************
*   File: act.item.c                                    Part of CircleMUD *
*  Usage: object handling routines -- get/drop and container handling     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.item.c,v 1.6 2005/01/05 16:41:27 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "constants.h"
#include "dg_scripts.h"
#include "oasis.h"
#include "assemblies.h"
#include "boards.h"

/* extern functions */
char *find_exdesc(char *word, struct extra_descr_data *list);
void item_check(struct obj_data *object, struct char_data *ch);
int is_proficient_with_armor(struct char_data *ch, int armor_type);
int is_proficient_with_weapon(struct char_data *ch, int weapon_type);

/* local functions */
int can_take_obj(struct char_data *ch, struct obj_data *obj);
void get_check_money(struct char_data *ch, struct obj_data *obj);
int perform_get_from_room(struct char_data *ch, struct obj_data *obj);
void get_from_room(struct char_data *ch, char *arg, int amount);
void perform_give_gold(struct char_data *ch, struct char_data *vict, int amount);
void perform_give(struct char_data *ch, struct char_data *vict, struct obj_data *obj);
int perform_drop(struct char_data *ch, struct obj_data *obj, byte mode, const char *sname, room_rnum RDR);
void perform_drop_gold(struct char_data *ch, int amount, byte mode, room_rnum RDR);
struct char_data *give_find_vict(struct char_data *ch, char *arg);
void weight_change_object(struct obj_data *obj, int weight);
void perform_put(struct char_data *ch, struct obj_data *obj, struct obj_data *cont);
void name_from_drinkcon(struct obj_data *obj);
void get_from_container(struct char_data *ch, struct obj_data *cont, char *arg, int mode, int amount);
void name_to_drinkcon(struct obj_data *obj, int type);
void wear_message(struct char_data *ch, struct obj_data *obj, int where);
void perform_wear(struct char_data *ch, struct obj_data *obj, int where);
int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg);
void perform_get_from_container(struct char_data *ch, struct obj_data *obj, struct obj_data *cont, int mode);
void perform_remove(struct char_data *ch, int pos);
int hands(struct char_data * ch);
int max_carry_weight(struct char_data *ch);
ACMD(do_assemble);
ACMD(do_remove);
ACMD(do_put);
ACMD(do_get);
ACMD(do_drop);
ACMD(do_give);
ACMD(do_drink);
ACMD(do_eat);
ACMD(do_pour);
ACMD(do_wear);
ACMD(do_wield);
ACMD(do_grab);

ACMD(do_assemble)
{
  long         lVnum = NOTHING;
  struct obj_data *pObject = NULL;
  char buf[MAX_STRING_LENGTH];

  skip_spaces(&argument);

  if (*argument == '\0') {
    send_to_char(ch, "What would you like to %s?\r\n", CMD_NAME);
    return;
  } else if ((lVnum = assemblyFindAssembly(argument)) < 0) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  } else if (assemblyGetType(lVnum) != subcmd) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  } else if (!assemblyCheckComponents(lVnum, ch)) {
    send_to_char(ch, "You haven't got all the things you need.\r\n");
    return;
  }

  /* Create the assembled object. */
  if ((pObject = read_object(lVnum, VIRTUAL)) == NULL ) {
    send_to_char(ch, "You can't %s %s %s.\r\n", CMD_NAME, AN(argument), argument);
    return;
  }
  add_unique_id(pObject);

  /* Now give the object to the character. */
  obj_to_char(pObject, ch);

  /* Tell the character they made something. */
  sprintf(buf, "You %s $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_CHAR);

  /* Tell the room the character made something. */
  sprintf(buf, "$n %ss $p.", CMD_NAME);
  act(buf, FALSE, ch, pObject, NULL, TO_ROOM);
}

void perform_put(struct char_data *ch, struct obj_data *obj,
		      struct obj_data *cont)
{

  if (!drop_otrigger(obj, ch))
    return;

  if (!obj) /* object might be extracted by drop_otrigger */
    return;

  if ((GET_OBJ_TYPE(cont) == ITEM_CONTAINER) &&
      (GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY) > 0) &&
      (GET_OBJ_WEIGHT(cont) + GET_OBJ_WEIGHT(obj) > GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)))
    act("$p won't fit in $P.", FALSE, ch, obj, cont, TO_CHAR);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP) && IN_ROOM(cont) != NOWHERE)
    act("You can't get $p out of your hand.", FALSE, ch, obj, NULL, TO_CHAR);
  else {
    obj_from_char(obj);
    obj_to_obj(obj, cont);

    act("$n puts $p in $P.", TRUE, ch, obj, cont, TO_ROOM);

    /* Yes, I realize this is strange until we have auto-equip on rent. -gg */
    if (OBJ_FLAGGED(obj, ITEM_NODROP) && !OBJ_FLAGGED(cont, ITEM_NODROP)) {
      SET_BIT_AR(GET_OBJ_EXTRA(cont), ITEM_NODROP);
      act("You get a strange feeling as you put $p in $P.", FALSE,
                ch, obj, cont, TO_CHAR);
    } else
      act("You put $p in $P.", FALSE, ch, obj, cont, TO_CHAR);
    /* If object placed in portal or vehicle, move it to the portal destination */
    if ((GET_OBJ_TYPE(cont) == ITEM_PORTAL) ||
        (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)) {
      obj_from_obj(obj);
      obj_to_room(obj, real_room(GET_OBJ_VAL(cont, VAL_CONTAINER_CAPACITY)));
      if (GET_OBJ_TYPE(cont) == ITEM_PORTAL) {
        act("What? $U$p disappears from $P in a puff of smoke!", 
	     TRUE, ch, obj, cont, TO_ROOM);
        act("What? $U$p disappears from $P in a puff of smoke!", 
	     FALSE, ch, obj, cont, TO_CHAR);
      }
    }
  }
}


/* The following put modes are supported by the code below:

	1) put <object> <container>
	2) put all.<object> <container>
	3) put all <container>

	<container> must be in inventory or on ground.
	all objects to be put into container must be in inventory.
*/

ACMD(do_put)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj, *cont;
  struct char_data *tmp_char;
  int obj_dotmode, cont_dotmode, found = 0, howmany = 1;
  char *theobj, *thecont;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (*arg3 && is_number(arg1)) {
    howmany = atoi(arg1);
    theobj = arg2;
    thecont = arg3;
  } else {
    theobj = arg1;
    thecont = arg2;
  }
  obj_dotmode = find_all_dots(theobj);
  cont_dotmode = find_all_dots(thecont);

  if (!*theobj)
    send_to_char(ch, "Put what in what?\r\n");
  else if (cont_dotmode != FIND_INDIV)
    send_to_char(ch, "You can only put things into one container at a time.\r\n");
  else if (!*thecont) {
    send_to_char(ch, "What do you want to put %s in?\r\n", obj_dotmode == FIND_INDIV ? "it" : "them");
  } else {
    generic_find(thecont, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
    if (!cont)
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(thecont), thecont);
    else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && 
             (GET_OBJ_TYPE(cont) != ITEM_PORTAL) &&
           (GET_OBJ_TYPE(cont) != ITEM_VEHICLE) )
      act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
    else if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
      send_to_char(ch, "You'd better open it first!\r\n");
    else {
      if (obj_dotmode == FIND_INDIV) {	/* put <obj> <container> */
	if (!(obj = get_obj_in_list_vis(ch, theobj, NULL, ch->carrying)))
	  send_to_char(ch, "You aren't carrying %s %s.\r\n", AN(theobj), theobj);
	else if (obj == cont && howmany == 1)
	  send_to_char(ch, "You attempt to fold it into itself, but fail.\r\n");
	else {
	  while (obj && howmany) {
	    next_obj = obj->next_content;
            if (obj != cont) {
              howmany--;
	      perform_put(ch, obj, cont);
            }
	    obj = get_obj_in_list_vis(ch, theobj, NULL, next_obj);
	  }
	}
      } else {
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (obj != cont && CAN_SEE_OBJ(ch, obj) &&
	      (obj_dotmode == FIND_ALL || isname(theobj, obj->name))) {
	    found = 1;
	    perform_put(ch, obj, cont);
	  }
	}
	if (!found) {
	  if (obj_dotmode == FIND_ALL)
	    send_to_char(ch, "You don't seem to have anything to put in it.\r\n");
	  else
	    send_to_char(ch, "You don't seem to have any %ss.\r\n", theobj);
	}
      }
    }
  }
}



int can_take_obj(struct char_data *ch, struct obj_data *obj)
{
  if (AFF_FLAGGED(ch, AFF_SPIRIT)) {
    act("You can't take anything, you're a ghost!", FALSE, ch, obj, 0, TO_CHAR);
    return(0);
  } else if (!(CAN_WEAR(obj, ITEM_WEAR_TAKE))) {
    act("$p: you can't take that!", FALSE, ch, obj, 0, TO_CHAR);
    return(0);
  } else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch)) {
    act("$p: you can't carry that many items.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  } else if ((IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(obj)) > CAN_CARRY_W(ch)) {
    act("$p: you can't carry that much weight.", FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }
  return (1);
}


void get_check_money(struct char_data *ch, struct obj_data *obj)
{
  int value = GET_OBJ_VAL(obj, VAL_MONEY_SIZE);

  if (GET_OBJ_TYPE(obj) != ITEM_MONEY || value <= 0)
    return;

  extract_obj(obj);

  GET_GOLD(ch) += value;

  if (value == 1)
    send_to_char(ch, "There was 1 coin.\r\n");
  else
    send_to_char(ch, "There were %d coins.\r\n", value);
}


void perform_get_from_container(struct char_data *ch, struct obj_data *obj,
				     struct obj_data *cont, int mode)
{
  if (mode == FIND_OBJ_INV || can_take_obj(ch, obj)) {
    if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
      act("$p: you can't hold any more items.", FALSE, ch, obj, 0, TO_CHAR);
    else if (get_otrigger(obj, ch)) { 
      obj_from_obj(obj);
      obj_to_char(obj, ch);
      act("You get $p from $P.", FALSE, ch, obj, cont, TO_CHAR);
      act("$n gets $p from $P.", TRUE, ch, obj, cont, TO_ROOM);
      if (IS_NPC(ch)) {
        item_check(obj, ch);
      }
      get_check_money(ch, obj);
    }
  }
}


void get_from_container(struct char_data *ch, struct obj_data *cont,
			     char *arg, int mode, int howmany)
{
  struct obj_data *obj, *next_obj;
  int obj_dotmode, found = 0;

  obj_dotmode = find_all_dots(arg);

  if (OBJVAL_FLAGGED(cont, CONT_CLOSED))
    act("$p is closed.", FALSE, ch, cont, 0, TO_CHAR);
  else if (obj_dotmode == FIND_INDIV) {
    if (!(obj = get_obj_in_list_vis(ch, arg, NULL, cont->contains))) {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "There doesn't seem to be %s %s in $p.", AN(arg), arg);
      act(buf, FALSE, ch, cont, 0, TO_CHAR);
    } else {
      struct obj_data *obj_next;
      while (obj && howmany--) {
        obj_next = obj->next_content;
        perform_get_from_container(ch, obj, cont, mode);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (obj_dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = cont->contains; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (obj_dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_container(ch, obj, cont, mode);
      }
    }
    if (!found) {
      if (obj_dotmode == FIND_ALL)
	act("$p seems to be empty.", FALSE, ch, cont, 0, TO_CHAR);
      else {
        char buf[MAX_STRING_LENGTH];

	snprintf(buf, sizeof(buf), "You can't seem to find any %ss in $p.", arg);
	act(buf, FALSE, ch, cont, 0, TO_CHAR);
      }
    }
  }
}


int perform_get_from_room(struct char_data *ch, struct obj_data *obj)
{
  if (can_take_obj(ch, obj) && get_otrigger(obj, ch)) {
    obj_from_room(obj);
    obj_to_char(obj, ch);
    act("You get $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n gets $p.", TRUE, ch, obj, 0, TO_ROOM);
    if (IS_NPC(ch))
      item_check(obj, ch);
    get_check_money(ch, obj);
  }
  return (0);
}

char *find_exdesc_keywords(char *word, struct extra_descr_data *list)
{
  struct extra_descr_data *i;

  for (i = list; i; i = i->next)
    if (isname(word, i->keyword))
      return (i->keyword);

  return (NULL);
}

void get_from_room(struct char_data *ch, char *arg, int howmany)
{
  struct obj_data *obj, *next_obj;
  int dotmode, found = 0;
  char *descword;

  /* Are they trying to take something in a room extra description? */
  if (find_exdesc(arg, world[IN_ROOM(ch)].ex_description) != NULL) {
    send_to_char(ch, "You can't take %s %s.\r\n", AN(arg), arg);
    return;
  }
  
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_INDIV) {
   if ((descword = find_exdesc_keywords(arg, world[IN_ROOM(ch)].ex_description)) != NULL)
     send_to_char(ch, "%s: you can't take that!\r\n", fname(descword));
   else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)))
      send_to_char(ch, "You don't see %s %s here.\r\n", AN(arg), arg);
    else {
      struct obj_data *obj_next;
      while(obj && howmany--) {
	obj_next = obj->next_content;
        perform_get_from_room(ch, obj);
        obj = get_obj_in_list_vis(ch, arg, NULL, obj_next);
      }
    }
  } else {
    if (dotmode == FIND_ALLDOT && !*arg) {
      send_to_char(ch, "Get all of what?\r\n");
      return;
    }
    for (obj = world[IN_ROOM(ch)].contents; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) &&
	  (dotmode == FIND_ALL || isname(arg, obj->name))) {
	found = 1;
	perform_get_from_room(ch, obj);
      }
    }
    if (!found) {
      if (dotmode == FIND_ALL)
	send_to_char(ch, "There doesn't seem to be anything here.\r\n");
      else
	send_to_char(ch, "You don't see any %ss here.\r\n", arg);
    }
  }
}



ACMD(do_get)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char arg3[MAX_INPUT_LENGTH];

  int cont_dotmode, found = 0, mode;
  struct obj_data *cont;
  struct char_data *tmp_char;

  one_argument(two_arguments(argument, arg1, arg2), arg3);	/* three_arguments */

  if (!*arg1)
    send_to_char(ch, "Get what?\r\n");
  else if (!*arg2)
    get_from_room(ch, arg1, 1);
  else if (is_number(arg1) && !*arg3)
    get_from_room(ch, arg2, atoi(arg1));
  else {
    int amount = 1;
    if (is_number(arg1)) {
      amount = atoi(arg1);
      strcpy(arg1, arg2);	/* strcpy: OK (sizeof: arg1 == arg2) */
      strcpy(arg2, arg3);	/* strcpy: OK (sizeof: arg2 == arg3) */
    }
    cont_dotmode = find_all_dots(arg2);
    if (cont_dotmode == FIND_INDIV) {
      mode = generic_find(arg2, FIND_OBJ_INV | FIND_OBJ_ROOM, ch, &tmp_char, &cont);
      if (!cont)
	send_to_char(ch, "You don't have %s %s.\r\n", AN(arg2), arg2);
      else if (GET_OBJ_TYPE(cont) == ITEM_VEHICLE)
        send_to_char(ch, "You will need to enter it first.\r\n");
      else if ((GET_OBJ_TYPE(cont) != ITEM_CONTAINER) && 
           !((GET_OBJ_TYPE(cont) == ITEM_PORTAL) && (OBJVAL_FLAGGED(cont, CONT_CLOSEABLE))))
	act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
      else
	get_from_container(ch, cont, arg1, mode, amount);
    } else {
      if (cont_dotmode == FIND_ALLDOT && !*arg2) {
	send_to_char(ch, "Get from all of what?\r\n");
	return;
      }
      for (cont = ch->carrying; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    found = 1;
	    get_from_container(ch, cont, arg1, FIND_OBJ_INV, amount);
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    found = 1;
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	  }
	}
      for (cont = world[IN_ROOM(ch)].contents; cont; cont = cont->next_content)
	if (CAN_SEE_OBJ(ch, cont) &&
	    (cont_dotmode == FIND_ALL || isname(arg2, cont->name))) {
	  if (GET_OBJ_TYPE(cont) == ITEM_CONTAINER) {
	    get_from_container(ch, cont, arg1, FIND_OBJ_ROOM, amount);
	    found = 1;
	  } else if (cont_dotmode == FIND_ALLDOT) {
	    act("$p is not a container.", FALSE, ch, cont, 0, TO_CHAR);
	    found = 1;
	  }
	}
      if (!found) {
	if (cont_dotmode == FIND_ALL)
	  send_to_char(ch, "You can't seem to find any containers.\r\n");
	else
	  send_to_char(ch, "You can't seem to find any %ss here.\r\n", arg2);
      }
    }
  }
}


void perform_drop_gold(struct char_data *ch, int amount,
		            byte mode, room_rnum RDR)
{
  struct obj_data *obj;

  if (amount <= 0)
    send_to_char(ch, "Heh heh heh.. we are jolly funny today, eh?\r\n");
  else if (GET_GOLD(ch) < amount)
    send_to_char(ch, "You don't have that many coins!\r\n");
  else {
    if (mode != SCMD_JUNK) {
      WAIT_STATE(ch, PULSE_VIOLENCE);	/* to prevent coin-bombing */
      obj = create_money(amount);
      if (mode == SCMD_DONATE) {
	send_to_char(ch, "You throw some gold into the air where it disappears in a puff of smoke!\r\n");
	act("$n throws some gold into the air where it disappears in a puff of smoke!",
	    FALSE, ch, 0, 0, TO_ROOM);
	obj_to_room(obj, RDR);
	act("$p suddenly appears in a puff of orange smoke!", 0, 0, obj, 0, TO_ROOM);
      } else {
        char buf[MAX_STRING_LENGTH];

        if (!drop_wtrigger(obj, ch)) {
          extract_obj(obj);
          return;
        }

        if (!drop_wtrigger(obj, ch) && (obj)) { /* obj may be purged */
          extract_obj(obj);
          return;
        }

	snprintf(buf, sizeof(buf), "$n drops %s.", money_desc(amount));
	act(buf, TRUE, ch, 0, 0, TO_ROOM);

	send_to_char(ch, "You drop some gold.\r\n");
	obj_to_room(obj, IN_ROOM(ch));
      }
    } else {
      char buf[MAX_STRING_LENGTH];

      snprintf(buf, sizeof(buf), "$n drops %s which disappears in a puff of smoke!", money_desc(amount));
      act(buf, FALSE, ch, 0, 0, TO_ROOM);

      send_to_char(ch, "You drop some gold which disappears in a puff of smoke!\r\n");
    }
    GET_GOLD(ch) -= amount;
  }
}


#define VANISH(mode) ((mode == SCMD_DONATE || mode == SCMD_JUNK) ? \
		      "  It vanishes in a puff of smoke!" : "")

int perform_drop(struct char_data *ch, struct obj_data *obj,
		     byte mode, const char *sname, room_rnum RDR)
{
  char buf[MAX_STRING_LENGTH];
  int value;

  if (!drop_otrigger(obj, ch))
    return 0;
  
  if ((mode == SCMD_DROP) && !drop_wtrigger(obj, ch))
    return 0;

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    snprintf(buf, sizeof(buf), "You can't %s $p, it must be CURSED!", sname);
    act(buf, FALSE, ch, obj, 0, TO_CHAR);
    return (0);
  }

  snprintf(buf, sizeof(buf), "You %s $p.%s", sname, VANISH(mode));
  act(buf, FALSE, ch, obj, 0, TO_CHAR);

  snprintf(buf, sizeof(buf), "$n %ss $p.%s", sname, VANISH(mode));
  act(buf, TRUE, ch, obj, 0, TO_ROOM);

  obj_from_char(obj);

  if ((mode == SCMD_DONATE) && OBJ_FLAGGED(obj, ITEM_NODONATE))
    mode = SCMD_JUNK;

  switch (mode) {
  case SCMD_DROP:
    obj_to_room(obj, IN_ROOM(ch));
    return (0);
  case SCMD_DONATE:
    obj_to_room(obj, RDR);
    act("$p suddenly appears in a puff a smoke!", FALSE, 0, obj, 0, TO_ROOM);
    return (0);
  case SCMD_JUNK:
    value = MAX(1, MIN(200, GET_OBJ_COST(obj) / 16));
    extract_obj(obj);
    return (value);
  default:
    log("SYSERR: Incorrect argument %d passed to perform_drop.", mode);
    break;
  }

  return (0);
}



ACMD(do_drop)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  room_rnum RDR = 0;
  byte mode = SCMD_DROP;
  int dotmode, amount = 0, multi, num_don_rooms;
  const char *sname;

  switch (subcmd) {
  case SCMD_JUNK:
    sname = "junk";
    mode = SCMD_JUNK;
    break;
  case SCMD_DONATE:
    sname = "donate";
    mode = SCMD_DONATE;
    /* fail + double chance for room 1   */
    num_don_rooms = (CONFIG_DON_ROOM_1 != NOWHERE) * 2 +       
                    (CONFIG_DON_ROOM_2 != NOWHERE)     +
                    (CONFIG_DON_ROOM_3 != NOWHERE)     + 1 ; 
    switch (rand_number(0, num_don_rooms)) {
    case 0:
      mode = SCMD_JUNK;
      break;
    case 1:
    case 2:
      RDR = real_room(CONFIG_DON_ROOM_1);
      break;
    case 3: RDR = real_room(CONFIG_DON_ROOM_2); break;
    case 4: RDR = real_room(CONFIG_DON_ROOM_3); break;

    }
    if (RDR == NOWHERE) {
      send_to_char(ch, "Sorry, you can't donate anything right now.\r\n");
      return;
    }
    break;
  default:
    sname = "drop";
    break;
  }

  argument = one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "What do you want to %s?\r\n", sname);
    return;
  } else if (is_number(arg)) {
    multi = atoi(arg);
    one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg))
      perform_drop_gold(ch, multi, mode, RDR);
    else if (multi <= 0)
      send_to_char(ch, "Yeah, that makes sense.\r\n");
    else if (!*arg)
      send_to_char(ch, "What do you want to %s %d of?\r\n", sname, multi);
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      do {
        next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
        amount += perform_drop(ch, obj, mode, sname, RDR);
        obj = next_obj;
      } while (obj && --multi);
    }
  } else {
    dotmode = find_all_dots(arg);

    /* Can't junk or donate all */
    if ((dotmode == FIND_ALL) && (subcmd == SCMD_JUNK || subcmd == SCMD_DONATE)) {
      if (subcmd == SCMD_JUNK)
	send_to_char(ch, "Go to the dump if you want to junk EVERYTHING!\r\n");
      else
	send_to_char(ch, "Go do the donation room if you want to donate EVERYTHING!\r\n");
      return;
    }
    if (dotmode == FIND_ALL) {
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be carrying anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  amount += perform_drop(ch, obj, mode, sname, RDR);
	}
    } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
	send_to_char(ch, "What do you want to %s all of?\r\n", sname);
	return;
      }
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);

      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	amount += perform_drop(ch, obj, mode, sname, RDR);
	obj = next_obj;
      }
    } else {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	amount += perform_drop(ch, obj, mode, sname, RDR);
    }
  }

  if (amount && (subcmd == SCMD_JUNK)) {
    send_to_char(ch, "You have been rewarded by the gods!\r\n");
    act("$n has been rewarded by the gods!", TRUE, ch, 0, 0, TO_ROOM);
    GET_GOLD(ch) += amount;
  }
}


void perform_give(struct char_data *ch, struct char_data *vict,
		       struct obj_data *obj)
{
  if (!give_otrigger(obj, ch, vict)) 
    return;
  if (!receive_mtrigger(vict, ch, obj))
    return;

  if (OBJ_FLAGGED(obj, ITEM_NODROP)) {
    act("You can't let go of $p!!  Yeech!", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  if (IS_CARRYING_N(vict) >= CAN_CARRY_N(vict)) {
    act("$N seems to have $S hands full.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  if (GET_OBJ_WEIGHT(obj) + IS_CARRYING_W(vict) > CAN_CARRY_W(vict)) {
    act("$E can't carry that much weight.", FALSE, ch, 0, vict, TO_CHAR);
    return;
  }
  obj_from_char(obj);
  obj_to_char(obj, vict);
  act("You give $p to $N.", FALSE, ch, obj, vict, TO_CHAR);
  act("$n gives you $p.", FALSE, ch, obj, vict, TO_VICT);
  act("$n gives $p to $N.", TRUE, ch, obj, vict, TO_NOTVICT);
}

/* utility function for give */
struct char_data *give_find_vict(struct char_data *ch, char *arg)
{
  struct char_data *vict;

  skip_spaces(&arg);
  if (!*arg)
    send_to_char(ch, "To who?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "What's the point of that?\r\n");
  else
    return (vict);

  return (NULL);
}


void perform_give_gold(struct char_data *ch, struct char_data *vict,
		            int amount)
{
  char buf[MAX_STRING_LENGTH];

  if (amount <= 0) {
    send_to_char(ch, "Heh heh heh ... we are jolly funny today, eh?\r\n");
    return;
  }
  if ((GET_GOLD(ch) < amount) && (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))) {
    send_to_char(ch, "You don't have that many coins!\r\n");
    return;
  }
  send_to_char(ch, "%s", CONFIG_OK);

  snprintf(buf, sizeof(buf), "$n gives you %d gold coin%s.", amount, amount == 1 ? "" : "s");
  act(buf, FALSE, ch, 0, vict, TO_VICT);

  snprintf(buf, sizeof(buf), "$n gives %s to $N.", money_desc(amount));
  act(buf, TRUE, ch, 0, vict, TO_NOTVICT);

  if (IS_NPC(ch) || !ADM_FLAGGED(ch, ADM_MONEY))
    GET_GOLD(ch) -= amount;
  GET_GOLD(vict) += amount;

  bribe_mtrigger(vict, ch, amount);
}


ACMD(do_give)
{
  char arg[MAX_STRING_LENGTH];
  int amount, dotmode;
  struct char_data *vict;
  struct obj_data *obj, *next_obj;

  argument = one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Give what to who?\r\n");
  else if (is_number(arg)) {
    amount = atoi(arg);
    argument = one_argument(argument, arg);
    if (!str_cmp("coins", arg) || !str_cmp("coin", arg)) {
      one_argument(argument, arg);
      if ((vict = give_find_vict(ch, arg)) != NULL)
	perform_give_gold(ch, vict, amount);
      return;
    } else if (!*arg)	/* Give multiple code. */
      send_to_char(ch, "What do you want to give %d of?\r\n", amount);
    else if (!(vict = give_find_vict(ch, argument)))
      return;
    else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) 
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg);
    else {
      while (obj && amount--) {
	next_obj = get_obj_in_list_vis(ch, arg, NULL, obj->next_content);
	perform_give(ch, vict, obj);
	obj = next_obj;
      }
    }
  } else {
    char buf1[MAX_INPUT_LENGTH];

    one_argument(argument, buf1);
    if (!(vict = give_find_vict(ch, buf1)))
      return;
    dotmode = find_all_dots(arg);
    if (dotmode == FIND_INDIV) {
      if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
	send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
      else
	perform_give(ch, vict, obj);
    } else {
      if (dotmode == FIND_ALLDOT && !*arg) {
	send_to_char(ch, "All of what?\r\n");
	return;
      }
      if (!ch->carrying)
	send_to_char(ch, "You don't seem to be holding anything.\r\n");
      else
	for (obj = ch->carrying; obj; obj = next_obj) {
	  next_obj = obj->next_content;
	  if (CAN_SEE_OBJ(ch, obj) &&
	      ((dotmode == FIND_ALL || isname(arg, obj->name))))
	    perform_give(ch, vict, obj);
	}
    }
  }
}



void weight_change_object(struct obj_data *obj, int weight)
{
  struct obj_data *tmp_obj;
  struct char_data *tmp_ch;

  if (IN_ROOM(obj) != NOWHERE) {
    GET_OBJ_WEIGHT(obj) += weight;
  } else if ((tmp_ch = obj->carried_by)) {
    obj_from_char(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_char(obj, tmp_ch);
  } else if ((tmp_obj = obj->in_obj)) {
    obj_from_obj(obj);
    GET_OBJ_WEIGHT(obj) += weight;
    obj_to_obj(obj, tmp_obj);
  } else {
    log("SYSERR: Unknown attempt to subtract weight from an object.");
  }
}



void name_from_drinkcon(struct obj_data *obj)
{
  char *new_name, *cur_name, *next;
  const char *liqname;
  int liqlen, cpylen;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  liqname = drinknames[GET_OBJ_VAL(obj, VAL_DRINKCON_LIQUID)];
  if (!isname(liqname, obj->name)) {
    log("SYSERR: Can't remove liquid '%s' from '%s' (%d) item.", liqname, obj->name, obj->item_number);
    return;
  }

  liqlen = strlen(liqname);
  CREATE(new_name, char, strlen(obj->name) - strlen(liqname)); /* +1 for NUL, -1 for space */

  for (cur_name = obj->name; cur_name; cur_name = next) {
    if (*cur_name == ' ')
      cur_name++;

    if ((next = strchr(cur_name, ' ')))
      cpylen = next - cur_name;
    else
      cpylen = strlen(cur_name);

    if (!strn_cmp(cur_name, liqname, liqlen))
      continue;

    if (*new_name)
      strcat(new_name, " ");	/* strcat: OK (size precalculated) */
    strncat(new_name, cur_name, cpylen);	/* strncat: OK (size precalculated) */
  }

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);
  obj->name = new_name;
}



void name_to_drinkcon(struct obj_data *obj, int type)
{
  char *new_name;

  if (!obj || (GET_OBJ_TYPE(obj) != ITEM_DRINKCON && GET_OBJ_TYPE(obj) != ITEM_FOUNTAIN))
    return;

  CREATE(new_name, char, strlen(obj->name) + strlen(drinknames[type]) + 2);
  sprintf(new_name, "%s %s", obj->name, drinknames[type]);	/* sprintf: OK */

  if (GET_OBJ_RNUM(obj) == NOTHING || obj->name != obj_proto[GET_OBJ_RNUM(obj)].name)
    free(obj->name);

  obj->name = new_name;
}



ACMD(do_drink)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *temp;
  struct affected_type af;
  int amount, weight;
  int on_ground = 0;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    send_to_char(ch, "Drink from what?\r\n");
    return;
  }
  if (!(temp = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    if (!(temp = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    } else
      on_ground = 1;
  }
  if ((GET_OBJ_TYPE(temp) != ITEM_DRINKCON) &&
      (GET_OBJ_TYPE(temp) != ITEM_FOUNTAIN)) {
    send_to_char(ch, "You can't drink from that!\r\n");
    return;
  }
  if (on_ground && (GET_OBJ_TYPE(temp) == ITEM_DRINKCON)) {
    send_to_char(ch, "You have to be holding that to drink from it.\r\n");
    return;
  }
  if ((GET_COND(ch, DRUNK) > 10) && (GET_COND(ch, THIRST) > 0)) {
    /* The pig is drunk */
    send_to_char(ch, "You can't seem to get close enough to your mouth.\r\n");
    act("$n tries to drink but misses $s mouth!", TRUE, ch, 0, 0, TO_ROOM);
    return;
  }
  if ((GET_COND(ch, FULL) > 20) && (GET_COND(ch, THIRST) > 0)) {
    send_to_char(ch, "Your stomach can't contain anymore!\r\n");
    return;
  }
  if (!GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL)) {
    send_to_char(ch, "It's empty.\r\n");
    return;
  }
  
  if (!consume_otrigger(temp, ch, OCMD_DRINK))  /* check trigger */
    return;
    
  if (subcmd == SCMD_DRINK) {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "$n drinks %s from $p.", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    act(buf, TRUE, ch, temp, 0, TO_ROOM);

    send_to_char(ch, "You drink the %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    if (temp->action_description)
      act(temp->action_description, TRUE, ch, temp, 0, TO_CHAR);

    if (drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK] > 0)
      amount = (25 - GET_COND(ch, THIRST)) / drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK];
    else
      amount = rand_number(3, 10);

  } else {
    act("$n sips from $p.", TRUE, ch, temp, 0, TO_ROOM);
    send_to_char(ch, "It tastes like %s.\r\n", drinks[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)]);
    amount = 1;
  }

  amount = MIN(amount, GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL));

  /* You can't subtract more than the object weighs
     Objects that are eternal (max capacity -1) don't get a
     weight subtracted */
  if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
  {
  weight = MIN(amount, GET_OBJ_WEIGHT(temp));
  weight_change_object(temp, -weight);	/* Subtract amount */
  }

  gain_condition(ch, DRUNK,  drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][DRUNK]  * amount / 4);
  gain_condition(ch, FULL,   drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][FULL]   * amount / 4);
  gain_condition(ch, THIRST, drink_aff[GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID)][THIRST] * amount / 4);

  if (GET_COND(ch, DRUNK) > 10)
    send_to_char(ch, "You feel drunk.\r\n");

  if (GET_COND(ch, THIRST) > 20)
    send_to_char(ch, "You don't feel thirsty any more.\r\n");

  if (GET_COND(ch, FULL) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(temp, VAL_DRINKCON_POISON)) {	/* The crap was poisoned ! */
    send_to_char(ch, "Oops, it tasted rather strange!\r\n");
    act("$n chokes and utters some strange sounds.", TRUE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 3;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  /* empty the container, and no longer poison.
     Only remove if it's max capacity > 0, not eternal */
  if (GET_OBJ_VAL(temp, VAL_DRINKCON_CAPACITY) > 0)
  {
  GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL) -= amount;
  if (!GET_OBJ_VAL(temp, VAL_DRINKCON_HOWFULL)) {	/* The last bit */
    name_from_drinkcon(temp);
    GET_OBJ_VAL(temp, VAL_DRINKCON_LIQUID) = 0;
    GET_OBJ_VAL(temp, VAL_DRINKCON_POISON) = 0;
  }
  }
  return;
}



ACMD(do_eat)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *food;
  struct affected_type af;
  int amount;

  one_argument(argument, arg);

  if (IS_NPC(ch))	/* Cannot use GET_COND() on mobs. */
    return;

  if (!*arg) {
    send_to_char(ch, "Eat what?\r\n");
    return;
  }
  if (!(food = get_obj_in_list_vis(ch, arg, NULL, ch->carrying))) {
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
    return;
  }
  if (subcmd == SCMD_TASTE && ((GET_OBJ_TYPE(food) == ITEM_DRINKCON) ||
			       (GET_OBJ_TYPE(food) == ITEM_FOUNTAIN))) {
    do_drink(ch, argument, 0, SCMD_SIP);
    return;
  }
  if ((GET_OBJ_TYPE(food) != ITEM_FOOD) && !ADM_FLAGGED(ch, ADM_EATANYTHING)) {
    send_to_char(ch, "You can't eat THAT!\r\n");
    return;
  }
  if (GET_COND(ch, FULL) > 20) {/* Stomach full */
    send_to_char(ch, "You are too full to eat more!\r\n");
    return;
  }

  if (!consume_otrigger(food, ch, OCMD_EAT))  /* check trigger */
    return;

  if (subcmd == SCMD_EAT) {
    act("You eat $p.", FALSE, ch, food, 0, TO_CHAR);
    if (food->action_description)
      act(food->action_description, FALSE, ch, food, 0, TO_CHAR);
    act("$n eats $p.", TRUE, ch, food, 0, TO_ROOM);
  } else {
    act("You nibble a little bit of $p.", FALSE, ch, food, 0, TO_CHAR);
    act("$n tastes a little bit of $p.", TRUE, ch, food, 0, TO_ROOM);
  }

  amount = (subcmd == SCMD_EAT ? GET_OBJ_VAL(food, VAL_FOOD_FOODVAL) : 1);

  gain_condition(ch, FULL, amount);

  if (GET_COND(ch, FULL) > 20)
    send_to_char(ch, "You are full.\r\n");

  if (GET_OBJ_VAL(food, VAL_FOOD_POISON) && !ADM_FLAGGED(ch, ADM_NOPOISON)) {
    /* The crap was poisoned ! */
    send_to_char(ch, "Oops, that tasted rather strange!\r\n");
    act("$n coughs and utters some strange sounds.", FALSE, ch, 0, 0, TO_ROOM);

    af.type = SPELL_POISON;
    af.duration = amount * 2;
    af.modifier = 0;
    af.location = APPLY_NONE;
    af.bitvector = AFF_POISON;
    affect_join(ch, &af, FALSE, FALSE, FALSE, FALSE);
  }
  if (subcmd == SCMD_EAT)
    extract_obj(food);
  else {
    if (!(--GET_OBJ_VAL(food, VAL_FOOD_FOODVAL))) {
      send_to_char(ch, "There's nothing left now.\r\n");
      extract_obj(food);
    }
  }
}


ACMD(do_pour)
{
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  struct obj_data *from_obj = NULL, *to_obj = NULL;
  int amount = 0;

  two_arguments(argument, arg1, arg2);

  if (subcmd == SCMD_POUR) {
    if (!*arg1) {		/* No arguments */
      send_to_char(ch, "From what do you want to pour?\r\n");
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_DRINKCON) {
      send_to_char(ch, "You can't pour from that!\r\n");
      return;
    }
  }
  if (subcmd == SCMD_FILL) {
    if (!*arg1) {		/* no arguments */
      send_to_char(ch, "What do you want to fill?  And what are you filling it from?\r\n");
      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if (GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) {
      act("You can't fill $p!", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!*arg2) {		/* no 2nd argument */
      act("What do you want to fill $p from?", FALSE, ch, to_obj, 0, TO_CHAR);
      return;
    }
    if (!(from_obj = get_obj_in_list_vis(ch, arg2, NULL, world[IN_ROOM(ch)].contents))) {
      send_to_char(ch, "There doesn't seem to be %s %s here.\r\n", AN(arg2), arg2);
      return;
    }
    if (GET_OBJ_TYPE(from_obj) != ITEM_FOUNTAIN) {
      act("You can't fill something from $p.", FALSE, ch, from_obj, 0, TO_CHAR);
      return;
    }
  }
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) == 0) {
    act("The $p is empty.", FALSE, ch, from_obj, 0, TO_CHAR);
    return;
  }
  if (subcmd == SCMD_POUR) {	/* pour */
    if (!*arg2) {
      send_to_char(ch, "Where do you want it?  Out or in what?\r\n");
      return;
    }
    if (!str_cmp(arg2, "out")) {
      if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
      {
      act("$n empties $p.", TRUE, ch, from_obj, 0, TO_ROOM);
      act("You empty $p.", FALSE, ch, from_obj, 0, TO_CHAR);

      weight_change_object(from_obj, -GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL)); /* Empty */

      name_from_drinkcon(from_obj);
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
      GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
      }
      else
      {
        send_to_char(ch, "You can't possibly pour that container out!\r\n");
      }

      return;
    }
    if (!(to_obj = get_obj_in_list_vis(ch, arg2, NULL, ch->carrying))) {
      send_to_char(ch, "You can't find it!\r\n");
      return;
    }
    if ((GET_OBJ_TYPE(to_obj) != ITEM_DRINKCON) &&
	(GET_OBJ_TYPE(to_obj) != ITEM_FOUNTAIN)) {
      send_to_char(ch, "You can't pour anything into that.\r\n");
      return;
    }
  }
  if (to_obj == from_obj) {
    send_to_char(ch, "A most unproductive effort.\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) != 0) &&
      (GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) != GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID))) {
    send_to_char(ch, "There is already another liquid in it!\r\n");
    return;
  }
  if ((GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) < 0) ||
      (!(GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) < GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY)))) {
    send_to_char(ch, "There is no room for more.\r\n");
    return;
  }
  if (subcmd == SCMD_POUR)
    send_to_char(ch, "You pour the %s into the %s.", drinks[GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID)], arg2);

  if (subcmd == SCMD_FILL) {
    act("You gently fill $p from $P.", FALSE, ch, to_obj, from_obj, TO_CHAR);
    act("$n gently fills $p from $P.", TRUE, ch, to_obj, from_obj, TO_ROOM);
  }
  /* New alias */
  if (GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) == 0)
    name_to_drinkcon(to_obj, GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID));

  /* First same type liq. */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_LIQUID) = GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID);

  /* Then how much to pour */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
  {
  GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) -= (amount =
			 (GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY) - GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL)));

  GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) = GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);

  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) < 0) {	/* There was too little */
    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
    amount += GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL);
    name_from_drinkcon(from_obj);
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_HOWFULL) = 0;
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_LIQUID) = 0;
    GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON) = 0;
  }
  }
  else
  {
    GET_OBJ_VAL(to_obj, VAL_DRINKCON_HOWFULL) = GET_OBJ_VAL(to_obj, VAL_DRINKCON_CAPACITY);
  }

  /* Then the poison boogie */
  GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) =
    (GET_OBJ_VAL(to_obj, VAL_DRINKCON_POISON) || GET_OBJ_VAL(from_obj, VAL_DRINKCON_POISON));

  /* And the weight boogie for non-eternal from_objects */
  if (GET_OBJ_VAL(from_obj, VAL_DRINKCON_CAPACITY) > 0)
  { 
  weight_change_object(from_obj, -amount);
  }
  weight_change_object(to_obj, amount);	/* Add weight */
}



void wear_message(struct char_data *ch, struct obj_data *obj, int where)
{
  const char *wear_messages[][2] = {
    {"$n lights $p and holds it.",
    "You light $p and hold it."},

    {"$n slides $p on to $s right ring finger.",
    "You slide $p on to your right ring finger."},

    {"$n slides $p on to $s left ring finger.",
    "You slide $p on to your left ring finger."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p around $s neck.",
    "You wear $p around your neck."},

    {"$n wears $p on $s body.",
    "You wear $p on your body."},

    {"$n wears $p on $s head.",
    "You wear $p on your head."},

    {"$n puts $p on $s legs.",
    "You put $p on your legs."},

    {"$n wears $p on $s feet.",
    "You wear $p on your feet."},

    {"$n puts $p on $s hands.",
    "You put $p on your hands."},

    {"$n wears $p on $s arms.",
    "You wear $p on your arms."},

    {"$n straps $p around $s arm as a shield.",
    "You start to use $p as a shield."},

    {"$n wears $p about $s body.",
    "You wear $p around your body."},

    {"$n wears $p around $s waist.",
    "You wear $p around your waist."},

    {"$n puts $p on around $s right wrist.",
    "You put $p on around your right wrist."},

    {"$n puts $p on around $s left wrist.",
    "You put $p on around your left wrist."},

    {"$n wields $p.",
    "You wield $p."},

    {"$n grabs $p.",
    "You grab $p."},

    {"$n shoulders $p.",
    "You shoulder $p."},

    {"$n puts $p in $s right ear.",
    "You put $p in your right ear."},

    {"$n puts $p in $s left ear.",
    "You put $p in your left ear."},

    {"$n wears $p as wings.",
    "You wear $p as wings."},

    {"$n covers $s face with $p.",
    "You wear $p over your face."}

  };

  act(wear_messages[where][0], TRUE, ch, obj, 0, TO_ROOM);
  act(wear_messages[where][1], FALSE, ch, obj, 0, TO_CHAR);
}

int hands(struct char_data * ch)
{
  int x;

  if (GET_EQ(ch, WEAR_WIELD1)) {
    if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD1), ITEM_2H) || wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD1)) == WIELD_TWOHAND) {
      x = 2;
    } else
    x = 1;
  } else x = 0;

  if (GET_EQ(ch, WEAR_WIELD2)) {
    if (OBJ_FLAGGED(GET_EQ(ch, WEAR_WIELD2), ITEM_2H) || wield_type(get_size(ch), GET_EQ(ch, WEAR_WIELD2)) == WIELD_TWOHAND) {
      x += 2;
    } else
    x += 1;
  }

  return x;
}

void perform_wear(struct char_data *ch, struct obj_data *obj, int where)
{
  /*
   * ITEM_WEAR_TAKE is used for objects that do not require special bits
   * to be put into that position (e.g. you can hold any object, not just
   * an object with a HOLD bit.)
   */

  int wear_bitvectors[] = {
    ITEM_WEAR_TAKE, ITEM_WEAR_FINGER, ITEM_WEAR_FINGER, ITEM_WEAR_NECK,
    ITEM_WEAR_NECK, ITEM_WEAR_BODY, ITEM_WEAR_HEAD, ITEM_WEAR_LEGS,
    ITEM_WEAR_FEET, ITEM_WEAR_HANDS, ITEM_WEAR_ARMS, ITEM_WEAR_SHIELD,
    ITEM_WEAR_ABOUT, ITEM_WEAR_WAIST, ITEM_WEAR_WRIST, ITEM_WEAR_WRIST,
    ITEM_WEAR_TAKE, ITEM_WEAR_TAKE, ITEM_WEAR_PACK, ITEM_WEAR_EAR,
    ITEM_WEAR_EAR, ITEM_WEAR_WINGS, ITEM_WEAR_MASK
  };

  const char *already_wearing[] = {
    "You're already using a light.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something on both of your ring fingers.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You can't wear anything else around your neck.\r\n",
    "You're already wearing something on your body.\r\n",
    "You're already wearing something on your head.\r\n",
    "You're already wearing something on your legs.\r\n",
    "You're already wearing something on your feet.\r\n",
    "You're already wearing something on your hands.\r\n",
    "You're already wearing something on your arms.\r\n",
    "You're already using a shield.\r\n",
    "You're already wearing something about your body.\r\n",
    "You already have something around your waist.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something around both of your wrists.\r\n",
    "You're already wielding a weapon.\r\n",
    "You're already holding something.\r\n",
    "You're already wearing something on your back.\r\n",
    "YOU SHOULD NEVER SEE THIS MESSAGE.  PLEASE REPORT.\r\n",
    "You're already wearing something in both ears.\r\n",
    "You're already wearing something as wings.\r\n",
    "You're already wearing something as a mask.\r\n"
  };

  /* first, make sure that the wear position is valid. */
  if (!CAN_WEAR(obj, wear_bitvectors[where])) {
    act("You can't wear $p there.", FALSE, ch, obj, 0, TO_CHAR);
    return;
  }
  /* for neck, finger, and wrist, try pos 2 if pos 1 is already full */
  if ((where == WEAR_FINGER_R) || (where == WEAR_NECK_1) || (where == WEAR_WRIST_R) || (where == WEAR_EAR_R) || (where == WEAR_WIELD1))
    if (GET_EQ(ch, where))
      where++;

  /* checks for 2H sanity */
  if ((OBJ_FLAGGED(obj, ITEM_2H)||(wield_type(get_size(ch), obj) == WIELD_TWOHAND)) && hands(ch) > 0) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (((where == WEAR_WIELD1)  ||
       (where == WEAR_WIELD2)) && (hands(ch) > 1)) {
    send_to_char(ch, "Seems like you might not have enough free hands.\r\n");
    return;
  }

  if (GET_EQ(ch, where)) {
    send_to_char(ch, "%s", already_wearing[where]);
    return;
  }

  /* See if a trigger disallows it */
  if (!wear_otrigger(obj, ch, where) || (obj->carried_by != ch))
    return;

  wear_message(ch, obj, where);
  obj_from_char(obj);
  equip_char(ch, obj, where);
}



int find_eq_pos(struct char_data *ch, struct obj_data *obj, char *arg)
{
  int where = -1;

  const char *keywords[] = {
    "!RESERVED!",
    "finger",
    "!RESERVED!",
    "neck",
    "!RESERVED!",
    "body",
    "head",
    "legs",
    "feet",
    "hands",
    "arms",
    "shield",
    "about",
    "waist",
    "wrist",
    "!RESERVED!",
    "!RESERVED!",
    "!RESERVED!",
    "back",
    "ear",
    "\r!RESERVED!",
    "wing",
    "mask",
    "\n"
  };

  if (!arg || !*arg) {
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER))      where = WEAR_FINGER_R;
    if (CAN_WEAR(obj, ITEM_WEAR_NECK))        where = WEAR_NECK_1;
    if (CAN_WEAR(obj, ITEM_WEAR_BODY))        where = WEAR_BODY;
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD))        where = WEAR_HEAD;
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS))        where = WEAR_LEGS;
    if (CAN_WEAR(obj, ITEM_WEAR_FEET))        where = WEAR_FEET;
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS))       where = WEAR_HANDS;
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS))        where = WEAR_ARMS;
    if (CAN_WEAR(obj, ITEM_WEAR_SHIELD))      where = WEAR_WIELD2;
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT))       where = WEAR_ABOUT;
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST))       where = WEAR_WAIST;
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST))       where = WEAR_WRIST_R;
    if (CAN_WEAR(obj, ITEM_WEAR_HOLD))        where = WEAR_WIELD2;
    if (CAN_WEAR(obj, ITEM_WEAR_PACK))        where = WEAR_BACKPACK;
    if (CAN_WEAR(obj, ITEM_WEAR_EAR))         where = WEAR_EAR_R;
    if (CAN_WEAR(obj, ITEM_WEAR_WINGS))       where = WEAR_WINGS;
    if (CAN_WEAR(obj, ITEM_WEAR_MASK))        where = WEAR_MASK;
  } else if (((where = search_block(arg, keywords, FALSE)) < 0) || (*arg=='!')) {
    send_to_char(ch, "'%s'?  What part of your body is THAT?\r\n", arg);
    return -1;
  }
  return (where);
}



ACMD(do_wear)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  struct obj_data *obj, *next_obj;
  int where, dotmode, items_worn = 0;

  two_arguments(argument, arg1, arg2);

  if (!*arg1) {
    send_to_char(ch, "Wear what?\r\n");
    return;
  }
  dotmode = find_all_dots(arg1);

  if (*arg2 && (dotmode != FIND_INDIV)) {
    send_to_char(ch, "You can't specify the same body location for more than one item!\r\n");
    return;
  }
  if (dotmode == FIND_ALL) {
    for (obj = ch->carrying; obj; obj = next_obj) {
      next_obj = obj->next_content;
      if (CAN_SEE_OBJ(ch, obj) && (where = find_eq_pos(ch, obj, 0)) >= 0) {
        if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj)) {
          act("$p: you are not experienced enough to use that.",
              FALSE, ch, obj, 0, TO_CHAR);
        } else if (OBJ_FLAGGED(obj, ITEM_BROKEN)) {
          act("$p: it seems to be broken.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
          items_worn++;
          if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) 
            && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
            send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
          perform_wear(ch, obj, where);
        }
      }
    }
    if (!items_worn)
      send_to_char(ch, "You don't seem to have anything wearable.\r\n");
  } else if (dotmode == FIND_ALLDOT) {
    if (!*arg1) {
      send_to_char(ch, "Wear all of what?\r\n");
      return;
    }
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have any %ss.\r\n", arg1);
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else
      while (obj) {
	next_obj = get_obj_in_list_vis(ch, arg1, NULL, obj->next_content);
	if ((where = find_eq_pos(ch, obj, 0)) >= 0) {
          if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL))
            && GET_OBJ_TYPE(obj) == ITEM_ARMOR) 
            send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
	  perform_wear(ch, obj, where);
	} else
	  act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
	obj = next_obj;
      }
  } else {
    if (!(obj = get_obj_in_list_vis(ch, arg1, NULL, ch->carrying)))
      send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg1), arg1);
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else if (GET_LEVEL(ch) < GET_OBJ_LEVEL(obj))
      send_to_char(ch, "You are not experienced enough to use that.\r\n");
    else {
      if ((where = find_eq_pos(ch, obj, arg2)) >= 0) {
        if (!is_proficient_with_armor(ch, GET_OBJ_VAL(obj, VAL_ARMOR_SKILL)) 
          && GET_OBJ_TYPE(obj) == ITEM_ARMOR) 
          send_to_char(ch, "You have no proficiency with this type of armor.\r\nYour fighting and physical skills will be greatly impeded.\r\n");
	perform_wear(ch, obj, where);
      } else if (!*arg2)
	act("You can't wear $p.", FALSE, ch, obj, 0, TO_CHAR);
    }
  }
}



ACMD(do_wield)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Wield what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (!CAN_WEAR(obj, ITEM_WEAR_WIELD))
      send_to_char(ch, "You can't wield that.\r\n");
    else if (GET_OBJ_WEIGHT(obj) > (max_carry_weight(ch) / 10))
      send_to_char(ch, "It's too heavy for you to use.\r\n");
    else if (OBJ_FLAGGED(obj, ITEM_BROKEN))
      send_to_char(ch, "But it seems to be broken!\r\n");
    else {
      if (!IS_NPC(ch) && !is_proficient_with_weapon(ch, GET_OBJ_VAL(obj, VAL_WEAPON_SKILL))
        && GET_OBJ_TYPE(obj) == ITEM_ARMOR)
        send_to_char(ch, "You have no proficiency with this type of weapon.\r\nYour attack accuracy will be greatly reduced.\r\n");
      perform_wear(ch, obj, WEAR_WIELD1);
    }
  }
}



ACMD(do_grab)
{
  char arg[MAX_INPUT_LENGTH];
  struct obj_data *obj;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Hold what?\r\n");
  else if (!(obj = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))
    send_to_char(ch, "You don't seem to have %s %s.\r\n", AN(arg), arg);
  else {
    if (GET_OBJ_TYPE(obj) == ITEM_LIGHT)
      perform_wear(ch, obj, WEAR_WIELD2);
    else {
      if (!CAN_WEAR(obj, ITEM_WEAR_HOLD) && GET_OBJ_TYPE(obj) != ITEM_WAND &&
      GET_OBJ_TYPE(obj) != ITEM_STAFF && GET_OBJ_TYPE(obj) != ITEM_SCROLL &&
	  GET_OBJ_TYPE(obj) != ITEM_POTION)
	send_to_char(ch, "You can't hold that.\r\n");
      else
	perform_wear(ch, obj, WEAR_WIELD2);
    }
  }
}



void perform_remove(struct char_data *ch, int pos)
{
  struct obj_data *obj;

  if (!(obj = GET_EQ(ch, pos)))
    log("SYSERR: perform_remove: bad pos %d passed.", pos);
  else if (OBJ_FLAGGED(obj, ITEM_NODROP))
    act("You can't remove $p, it must be CURSED!", FALSE, ch, obj, 0, TO_CHAR);
  else if (IS_CARRYING_N(ch) >= CAN_CARRY_N(ch))
    act("$p: you can't carry that many items!", FALSE, ch, obj, 0, TO_CHAR);
  else {
    if (!remove_otrigger(obj, ch))
       return;

    obj_to_char(unequip_char(ch, pos), ch);
    act("You stop using $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n stops using $p.", TRUE, ch, obj, 0, TO_ROOM);
  }
}



ACMD(do_remove)
{
  struct obj_data *obj;
  char arg[MAX_INPUT_LENGTH];
  int i, dotmode, found = 0, msg;

  one_argument(argument, arg);

  if (!*arg) {
    send_to_char(ch, "Remove what?\r\n");
    return;
  }
  /* lemme check for a board FIRST */
  for (obj = ch->carrying; obj; obj = obj->next_content) {
    if (GET_OBJ_TYPE (obj) == ITEM_BOARD) {
      found = 1;
      break;
    }
  }
  if (!obj) {
    for (obj = world[ch->in_room].contents; obj; obj = obj->next_content) {
      if (GET_OBJ_TYPE (obj) == ITEM_BOARD) {
	found = 1;
	break;
      }
    }
  }

  if (found) {
    if (!isdigit (*arg) || (!(msg = atoi (arg)))) {
      found = 0;
    } else {
      remove_board_msg (GET_OBJ_VNUM (obj), ch, msg);
    }
  }
  if (!found) {
  dotmode = find_all_dots(arg);

  if (dotmode == FIND_ALL) {
    found = 0;
      for (i = 0; i < NUM_WEARS; i++) {
      if (GET_EQ(ch, i)) {
	perform_remove(ch, i);
	found = 1;
      }
      }
      if (!found) {
      send_to_char(ch, "You're not using anything.\r\n");
      }
  } else if (dotmode == FIND_ALLDOT) {
      if (!*arg) {
      send_to_char(ch, "Remove all of what?\r\n");
      } else {
      found = 0;
	for (i = 0; i < NUM_WEARS; i++) {
	if (GET_EQ(ch, i) && CAN_SEE_OBJ(ch, GET_EQ(ch, i)) &&
	    isname(arg, GET_EQ(ch, i)->name)) {
	  perform_remove(ch, i);
	  found = 1;
	}
	}
	if (!found) {
	send_to_char(ch, "You don't seem to be using any %ss.\r\n", arg);
    }
      }
  } else {
      if ((i = get_obj_pos_in_equip_vis(ch, arg, NULL, ch->equipment)) < 0) {
      send_to_char(ch, "You don't seem to be using %s %s.\r\n", AN(arg), arg);
      } else {
      perform_remove(ch, i);
  }
    }
  }
}

ACMD(do_sac) 
{ 
   char arg[MAX_INPUT_LENGTH]; 
   struct obj_data *j, *jj, *next_thing2; 

  one_argument(argument, arg); 

  if (!*arg) { 
     send_to_char(ch, "Sacrifice what?\n\r"); 
     return; 
   } 

  if (!(j = get_obj_in_list_vis(ch, arg, NULL, world[IN_ROOM(ch)].contents)) && (!(j = get_obj_in_list_vis(ch, arg, NULL, ch->carrying)))) { 
     send_to_char(ch, "It doesn't seem to be here.\n\r"); 
     return; 
   } 

  if (!CAN_WEAR(j, ITEM_WEAR_TAKE)) { 
     send_to_char(ch, "You can't sacrifice that!\n\r"); 
     return; 
   } 

   act("$n sacrifices $p.", FALSE, ch, j, 0, TO_ROOM); 

  switch (rand_number(0, 5)) { 
    case 0: 
      send_to_char(ch, "You sacrifice %s to the Gods.\r\nYou receive one gold coin for your humility.\r\n", GET_OBJ_SHORT(j));
      GET_GOLD(ch) += 1; 
    break; 
    case 1: 
      send_to_char(ch, "You sacrifice %s to the Gods.\r\nThe Gods ignore your sacrifice.\r\n", GET_OBJ_SHORT(j));
    break; 
    case 2: 
      send_to_char(ch, "You sacrifice %s to the Gods.\r\nZizazat gives you %d experience points.\r\n", GET_OBJ_SHORT(j), (2*GET_OBJ_COST(j)));
      GET_EXP(ch) += (2*GET_OBJ_COST(j)); 
    break; 
    case 3: 
      send_to_char(ch, "You sacrifice %s to the Gods.\r\nYou receive %d experience points.\r\n", GET_OBJ_SHORT(j), GET_OBJ_COST(j));
      GET_EXP(ch) += GET_OBJ_COST(j); 
    break; 
    case 4: 
      send_to_char(ch, "Your sacrifice to the Gods is rewarded with %d gold coins.\r\n", GET_OBJ_COST(j)); 
      GET_GOLD(ch) += GET_OBJ_COST(j); 
    break; 
    case 5: 
      send_to_char(ch, "Your sacrifice to the Gods is rewarded with %d gold coins\r\n", (2*GET_OBJ_COST(j))); 
      GET_GOLD(ch) += (2*GET_OBJ_COST(j)); 
    break; 
  default: 
      send_to_char(ch, "You sacrifice %s to the Gods.\r\nYou receive one gold coin for your humility.\r\n", GET_OBJ_SHORT(j));
      GET_GOLD(ch) += 1; 
    break; 
  } 
   for (jj = j->contains; jj; jj = next_thing2) { 
     next_thing2 = jj->next_content;       /* Next in inventory */ 
     obj_from_obj(jj); 
      
     if (j->carried_by) 
       obj_to_room(jj, j->carried_by->in_room); 
     else if (j->in_room != NOWHERE) 
       obj_to_room(jj, j->in_room); 
     else 
       assert(FALSE); 
   } 
   extract_obj(j);
}

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
const int max_carry_load[] = {
  0,
  10,
  20,
  30,
  40,
  50,
  60,
  70,
  80,
  90,
  100,
  115,
  130,
  150,
  175,
  200,
  230,
  260,
  300,
  350,
  400,
  460,
  520,
  600,
  700,
  800,
  920,
  1040,
  1200,
  1400,
  1640,
};

/* Derived from the SRD under OGL, see ../doc/srd.txt for information */
int max_carry_weight(struct char_data *ch)
{
  int abil, total;
  abil = MAX(0, MIN(100, GET_STR(ch)));
  total = 1;
  while (abil > 30) {
    abil -= 10;
    total *= 4;
  }
  return total * max_carry_load[abil];
}

