/* ************************************************************************
*   File: act.wizard.c                                  Part of CircleMUD *
*  Usage: Player-level god commands and other goodies                     *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.wizard.c,v 1.21 2005/03/08 02:10:11 zizazat Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "spells.h"
#include "house.h"
#include "screen.h"
#include "constants.h"
#include "oasis.h"
#include "genzon.h"
#include "dg_scripts.h"
#include "assemblies.h"
#include "feats.h"

/*   external vars  */
extern FILE *player_fl;
extern struct attack_hit_type attack_hit_text[];
extern char *class_abbrevs[];
extern char *race_abbrevs[];
extern time_t boot_time;
extern int circle_shutdown, circle_reboot;
extern int circle_restrict;
extern int buf_switches, buf_largecount, buf_overflows;
extern int top_of_p_table;
extern socket_t mother_desc;
extern ush_int port;
extern struct player_index_element *player_table;
extern int top_guild;

/* for chars */
extern const char *pc_class_types[];
extern const char *pc_race_types[];
extern const char *class_names[];
extern const char *race_names[];
extern const char *material_names[];
extern const char *config_sect[];
extern char level_version[READ_SIZE];
extern int level_vernum;
extern const char *weapon_type[];
extern const char *armor_type[];
extern const char *crit_type[];
extern room_vnum freeres[];

/* extern functions */
int level_exp(int level);
void show_shops(struct char_data *ch, char *value);
void hcontrol_list_houses(struct char_data *ch);
void do_start(struct char_data *ch);
void appear(struct char_data *ch);
void reset_zone(zone_rnum zone);
void roll_real_abils(struct char_data *ch);
int parse_class(char arg);
int parse_race(char arg);
void run_autowiz(void);
int save_all(void);
void print_zone(struct char_data *ch, zone_vnum vnum);
struct char_data *find_char(int n);
SPECIAL(shop_keeper);
void Crash_rentsave(struct char_data * ch, int cost);
void show_guild(struct char_data * ch, char *arg);
int class_armor_bonus(struct char_data *ch);
int compute_armor_class(struct char_data *ch, struct char_data *att);
char *reduct_desc(struct damreduct_type *reduct);

/* local functions */
int perform_set(struct char_data *ch, struct char_data *vict, int mode, char *val_arg);
void perform_immort_invis(struct char_data *ch, int level);
ACMD(do_echo);
ACMD(do_send);
room_rnum find_target_room(struct char_data *ch, char *rawroomstr);
ACMD(do_at);
ACMD(do_goto);
ACMD(do_trans);
ACMD(do_teleport);
ACMD(do_vnum);
void do_stat_room(struct char_data *ch);
void do_stat_object(struct char_data *ch, struct obj_data *j);
void do_stat_character(struct char_data *ch, struct char_data *k);
ACMD(do_stat);
ACMD(do_shutdown);
void stop_snooping(struct char_data *ch);
ACMD(do_snoop);
ACMD(do_switch);
ACMD(do_return);
ACMD(do_load);
ACMD(do_vstat);
ACMD(do_purge);
ACMD(do_syslog);
ACMD(do_advance);
ACMD(do_restore);
void perform_immort_vis(struct char_data *ch);
ACMD(do_invis);
ACMD(do_gecho);
ACMD(do_poofset);
ACMD(do_dc);
ACMD(do_wizlock);
ACMD(do_date);
ACMD(do_last);
ACMD(do_force);
ACMD(do_wiznet);
ACMD(do_zreset);
ACMD(do_wizutil);
size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall);
ACMD(do_show);
ACMD(do_set);
void snoop_check(struct char_data *ch);
ACMD(do_saveall);
ACMD(do_wizupdate);

ACMD(do_echo)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes.. but what?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 4];

    if (subcmd == SCMD_EMOTE)
      snprintf(buf, sizeof(buf), "$n %s", argument);
    else
      strlcpy(buf, argument, sizeof(buf));

    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      act(buf, FALSE, ch, 0, 0, TO_CHAR);
  }
}


ACMD(do_send)
{
  char arg[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  struct char_data *vict;

  half_chop(argument, arg, buf);

  if (!*arg) {
    send_to_char(ch, "Send what to who?\r\n");
    return;
  }
  if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  send_to_char(vict, "%s\r\n", buf);
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "Sent.\r\n");
  else
    send_to_char(ch, "You send '%s' to %s.\r\n", buf, GET_NAME(vict));
}



/* take a string, and return an rnum.. used for goto, at, etc.  -je 4/6/93 */
room_rnum find_target_room(struct char_data *ch, char *rawroomstr)
{
  room_rnum location = NOWHERE;
  char roomstr[MAX_INPUT_LENGTH];

  one_argument(rawroomstr, roomstr);

  if (!*roomstr) {
    send_to_char(ch, "You must supply a room number or name.\r\n");
    return (NOWHERE);
  }

  if (isdigit(*roomstr) && !strchr(roomstr, '.')) {
    if ((location = real_room((room_vnum)atoi(roomstr))) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (NOWHERE);
    }
  } else {
    struct char_data *target_mob;
    struct obj_data *target_obj;
    char *mobobjstr = roomstr;
    int num;

    num = get_number(&mobobjstr);
    if ((target_mob = get_char_vis(ch, mobobjstr, &num, FIND_CHAR_WORLD)) != NULL) {
      if ((location = IN_ROOM(target_mob)) == NOWHERE) {
        send_to_char(ch, "That character is currently lost.\r\n");
        return (NOWHERE);
      }
    } else if ((target_obj = get_obj_vis(ch, mobobjstr, &num)) != NULL) {
      if (IN_ROOM(target_obj) != NOWHERE)
        location = IN_ROOM(target_obj);
      else if (target_obj->carried_by && IN_ROOM(target_obj->carried_by) != NOWHERE)
        location = IN_ROOM(target_obj->carried_by);
      else if (target_obj->worn_by && IN_ROOM(target_obj->worn_by) != NOWHERE)
        location = IN_ROOM(target_obj->worn_by);

      if (location == NOWHERE) {
        send_to_char(ch, "That object is currently not in a room.\r\n");
        return (NOWHERE);
      }
    }

    if (location == NOWHERE) {
      send_to_char(ch, "Nothing exists by that name.\r\n");
      return (NOWHERE);
    }
  }

  /* a location has been found -- if you're >= GRGOD, no restrictions. */
  if (GET_ADMLEVEL(ch) >= ADMLVL_GRGOD)
    return (location);

  if (ROOM_FLAGGED(location, ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (ROOM_FLAGGED(location, ROOM_PRIVATE) && world[location].people && world[location].people->next_in_room)
    send_to_char(ch, "There's a private conversation going on in that room.\r\n");
  else if (ROOM_FLAGGED(location, ROOM_HOUSE) && !House_can_enter(ch, GET_ROOM_VNUM(location)))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else
    return (location);

  return (NOWHERE);
}



ACMD(do_at)
{
  char command[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  room_rnum location, original_loc;

  half_chop(argument, buf, command);
  if (!*buf) {
    send_to_char(ch, "You must supply a room number or a name.\r\n");
    return;
  }

  if (!*command) {
    send_to_char(ch, "What do you want to do there?\r\n");
    return;
  }

  if ((location = find_target_room(ch, buf)) == NOWHERE)
    return;

  /* a location has been found. */
  original_loc = IN_ROOM(ch);
  char_from_room(ch);
  char_to_room(ch, location);
  command_interpreter(ch, command);

  /* check if the char is still there */
  if (IN_ROOM(ch) == location) {
    char_from_room(ch);
    char_to_room(ch, original_loc);
  }
}


ACMD(do_goto)
{
  char buf[MAX_STRING_LENGTH];
  room_rnum location;

  if ((location = find_target_room(ch, argument)) == NOWHERE)
    return;

  snprintf(buf, sizeof(buf), "$n %s", POOFOUT(ch) ? POOFOUT(ch) : "disappears in a puff of smoke.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  char_from_room(ch);
  char_to_room(ch, location);

  snprintf(buf, sizeof(buf), "$n %s", POOFIN(ch) ? POOFIN(ch) : "appears with an ear-splitting bang.");
  act(buf, TRUE, ch, 0, 0, TO_ROOM);

  look_at_room(IN_ROOM(ch), ch, 0);
  enter_wtrigger(&world[IN_ROOM(ch)], ch, -1);
}



ACMD(do_trans)
{
  char buf[MAX_INPUT_LENGTH];
  struct descriptor_data *i;
  struct char_data *victim;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to transfer?\r\n");
  else if (str_cmp("all", buf)) {
    if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (victim == ch)
      send_to_char(ch, "That doesn't make much sense, does it?\r\n");
    else {
      if ((GET_ADMLEVEL(ch) < GET_ADMLEVEL(victim)) && !IS_NPC(victim)) {
	send_to_char(ch, "Go transfer someone your own size.\r\n");
	return;
      }
      act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
      char_from_room(victim);
      char_to_room(victim, IN_ROOM(ch));
      act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
      act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
      look_at_room(IN_ROOM(victim), victim, 0);
      enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
    }
  } else {			/* Trans All */
    if (!ADM_FLAGGED(ch, ADM_TRANSALL)) {
      send_to_char(ch, "I think not.\r\n");
      return;
    }

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i->character && i->character != ch) {
	victim = i->character;
	if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
	  continue;
	act("$n disappears in a mushroom cloud.", FALSE, victim, 0, 0, TO_ROOM);
	char_from_room(victim);
	char_to_room(victim, IN_ROOM(ch));
	act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
	act("$n has transferred you!", FALSE, ch, 0, victim, TO_VICT);
        look_at_room(IN_ROOM(victim), victim, 0);
        enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
      }
    send_to_char(ch, "%s", CONFIG_OK);
  }
}



ACMD(do_teleport)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  room_rnum target;

  two_arguments(argument, buf, buf2);

  if (!*buf)
    send_to_char(ch, "Whom do you wish to teleport?\r\n");
  else if (!(victim = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (victim == ch)
    send_to_char(ch, "Use 'goto' to teleport yourself.\r\n");
  else if (GET_ADMLEVEL(victim) >= GET_ADMLEVEL(ch))
    send_to_char(ch, "Maybe you shouldn't do that.\r\n");
  else if (!*buf2)
    send_to_char(ch, "Where do you wish to send this person?\r\n");
  else if ((target = find_target_room(ch, buf2)) != NOWHERE) {
    send_to_char(ch, "%s", CONFIG_OK);
    act("$n disappears in a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    char_from_room(victim);
    char_to_room(victim, target);
    act("$n arrives from a puff of smoke.", FALSE, victim, 0, 0, TO_ROOM);
    act("$n has teleported you!", FALSE, ch, 0, (char *) victim, TO_VICT);
    look_at_room(IN_ROOM(victim), victim, 0);
    enter_wtrigger(&world[IN_ROOM(victim)], victim, -1);
  }
}



ACMD(do_vnum)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2 || (!is_abbrev(buf, "mob") && !is_abbrev(buf, "obj"))) {
    send_to_char(ch, "Usage: vnum { obj | mob } <name>\r\n");
    return;
  }
  if (is_abbrev(buf, "mob"))
    if (!vnum_mobile(buf2, ch))
      send_to_char(ch, "No mobiles by that name.\r\n");

  if (is_abbrev(buf, "obj"))
    if (!vnum_object(buf2, ch))
      send_to_char(ch, "No objects by that name.\r\n");
}


#define ZOCMD zone_table[zrnum].cmd[subcmd]
 
void list_zone_commands_room(struct char_data *ch, room_vnum rvnum)
{
  extern struct index_data **trig_index;
  zone_rnum zrnum = real_zone_by_thing(rvnum);
  room_rnum rrnum = real_room(rvnum), cmd_room = NOWHERE;
  int subcmd = 0, count = 0;

  if (zrnum == NOWHERE || rrnum == NOWHERE) {
    send_to_char(ch, "No zone information available.\r\n");
    return;
  }

  send_to_char(ch, "Zone commands in this room:@y\r\n");
  while (ZOCMD.command != 'S') {
    switch (ZOCMD.command) {
      case 'M':
      case 'O':
      case 'T':
      case 'V':
        cmd_room = ZOCMD.arg3;
        break;
      case 'D':
      case 'R':
        cmd_room = ZOCMD.arg1;
        break;
      default:
        break;
    }
    if (cmd_room == rrnum) {
      count++;
      /* start listing */
      switch (ZOCMD.command) {
        case 'M':
          send_to_char(ch, "%sLoad %s@y [@c%d@y], Max : %d\r\n",
                  ZOCMD.if_flag ? " then " : "",
                  mob_proto[ZOCMD.arg1].short_descr,
                  mob_index[ZOCMD.arg1].vnum, ZOCMD.arg2
                  );
          break;
        case 'G':
          send_to_char(ch, "%sGive it %s@y [@c%d@y], Max : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      ZOCMD.arg2
    	      );
          break;
        case 'O':
          send_to_char(ch, "%sLoad %s@y [@c%d@y], Max : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      ZOCMD.arg2
    	      );
          break;
        case 'E':
          send_to_char(ch, "%sEquip with %s@y [@c%d@y], %s, Max : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      equipment_types[ZOCMD.arg3],
    	      ZOCMD.arg2
    	      );
          break;
        case 'P':
          send_to_char(ch, "%sPut %s@y [@c%d@y] in %s@y [@c%d@y], Max : %d\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg1].short_description,
    	      obj_index[ZOCMD.arg1].vnum,
    	      obj_proto[ZOCMD.arg3].short_description,
    	      obj_index[ZOCMD.arg3].vnum,
    	      ZOCMD.arg2
    	      );
          break;
        case 'R':
          send_to_char(ch, "%sRemove %s@y [@c%d@y] from room.\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      obj_proto[ZOCMD.arg2].short_description,
    	      obj_index[ZOCMD.arg2].vnum
    	      );
          break;
        case 'D':
          send_to_char(ch, "%sSet door %s as %s.\r\n",
    	      ZOCMD.if_flag ? " then " : "",
    	      dirs[ZOCMD.arg2],
    	      ZOCMD.arg3 ? ((ZOCMD.arg3 == 1) ? "closed" : "locked") : "open"
    	      );
          break;
        case 'T':
          send_to_char(ch, "%sAttach trigger @c%s@y [@c%d@y] to %s\r\n",
            ZOCMD.if_flag ? " then " : "",
            trig_index[ZOCMD.arg2]->proto->name,
            trig_index[ZOCMD.arg2]->vnum,
            ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
              ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????"))));
          break;
        case 'V':
          send_to_char(ch, "%sAssign global %s:%d to %s = %s\r\n",
            ZOCMD.if_flag ? " then " : "",
            ZOCMD.sarg1, ZOCMD.arg2,
            ((ZOCMD.arg1 == MOB_TRIGGER) ? "mobile" :
              ((ZOCMD.arg1 == OBJ_TRIGGER) ? "object" :
                ((ZOCMD.arg1 == WLD_TRIGGER)? "room" : "????"))),
            ZOCMD.sarg2);
          break;
        default:
          send_to_char(ch, "<Unknown Command>\r\n");
          break;
      }
    } 
    subcmd++;  
  }
  send_to_char(ch, "@n");
  if (!count) 
    send_to_char(ch, "None!\r\n");

}
#undef ZOCMD

void do_stat_room(struct char_data *ch)
{
  char buf2[MAX_STRING_LENGTH];
  struct extra_descr_data *desc;
  struct room_data *rm = &world[IN_ROOM(ch)];
  int i, found, column;
  struct obj_data *j;
  struct char_data *k;

  send_to_char(ch, "Room name: @c%s@n\r\n", rm->name);

  sprinttype(rm->sector_type, sector_types, buf2, sizeof(buf2));
  send_to_char(ch, "Zone: [%3d], VNum: [@g%5d@n], RNum: [%5d], IDNum: [%5ld], Type: %s\r\n",
	  zone_table[rm->zone].number, rm->number, IN_ROOM(ch),
          (long) rm->number + ROOM_ID_BASE, buf2);

  sprintbitarray(rm->room_flags, room_bits, RF_ARRAY_MAX, buf2);
  send_to_char(ch, "SpecProc: %s, Flags: %s\r\n", rm->func == NULL ? "None" : "Exists", buf2);

  send_to_char(ch, "Description:\r\n%s", rm->description ? rm->description : "  None.\r\n");

  if (rm->ex_description) {
    send_to_char(ch, "Extra descs:");
    for (desc = rm->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [@c%s@n]", desc->keyword);
    send_to_char(ch, "\r\n");
  }

  send_to_char(ch, "Chars present:");
  column = 14;	/* ^^^ strlen ^^^ */
  for (found = FALSE, k = rm->people; k; k = k->next_in_room) {
    if (!CAN_SEE(ch, k))
      continue;

    column += send_to_char(ch, "%s @y%s@n(%s)", found++ ? "," : "", GET_NAME(k),
		!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB"));
    if (column >= 62) {
      send_to_char(ch, "%s\r\n", k->next_in_room ? "," : "");
      found = FALSE;
      column = 0;
    }
  }

  if (rm->contents) {
    send_to_char(ch, "Contents:@g");
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j = rm->contents; j; j = j->next_content) {
      if (!CAN_SEE_OBJ(ch, j))
	continue;

      column += send_to_char(ch, "%s %s", found++ ? "," : "", j->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "@n");
  }

  for (i = 0; i < NUM_OF_DIRS; i++) {
    char buf1[128];

    if (!rm->dir_option[i])
      continue;

    if (rm->dir_option[i]->to_room == NOWHERE)
      snprintf(buf1, sizeof(buf1), " @cNONE@n");
    else
      snprintf(buf1, sizeof(buf1), "@c%5d@n", GET_ROOM_VNUM(rm->dir_option[i]->to_room));

    sprintbit(rm->dir_option[i]->exit_info, exit_bits, buf2, sizeof(buf2));

    send_to_char(ch, "Exit @c%-5s@n:  To: [%s], Key: [%5d], Keywrd: %s, Type: %s\r\n  DC Lock: [%2d], DC Hide: [%2d], DC Skill: [%4s], DC Move: [%2d]\r\n%s",
        dirs[i], buf1,
        rm->dir_option[i]->key == NOTHING ? -1 : rm->dir_option[i]->key,
	rm->dir_option[i]->keyword ? rm->dir_option[i]->keyword : "None", buf2,
	rm->dir_option[i]->dclock, rm->dir_option[i]->dchide,
	rm->dir_option[i]->dcskill == 0 ? "None" : spell_info[rm->dir_option[i]->dcskill].name, rm->dir_option[i]->dcmove,
	rm->dir_option[i]->general_description ? rm->dir_option[i]->general_description : "  No exit description.\r\n");
  }

  /* check the room for a script */
  do_sstat_room(ch);
 
  list_zone_commands_room(ch, rm->number);
}



void do_stat_object(struct char_data *ch, struct obj_data *j)
{
  int i, found;
  obj_vnum vnum;
  struct obj_data *j2;
  struct extra_descr_data *desc;
  char buf[MAX_STRING_LENGTH];

  vnum = GET_OBJ_VNUM(j);
  send_to_char(ch, "Name: '%s', Aliases: %s, Size: %s\r\n",
               j->short_description ? j->short_description : "<None>", j->name,
               size_names[GET_OBJ_SIZE(j)]);

  sprinttype(GET_OBJ_TYPE(j), item_types, buf, sizeof(buf));
  send_to_char(ch, "VNum: [@g%5d@n], RNum: [%5d], Idnum: [%5ld], Type: %s, SpecProc: %s\r\n",
	vnum, GET_OBJ_RNUM(j), GET_ID(j), buf, GET_OBJ_SPEC(j) ? "Exists" : "None");

  send_to_char(ch, "Generation time: @g%s@nUnique ID: @g%lld@n\r\n",
    ctime(&j->generation), j->unique_id);

  send_to_char(ch, "Object Hit Points: [ @g%3d@n/@g%3d@n]\r\n",
   GET_OBJ_VAL(j, VAL_ALL_HEALTH), GET_OBJ_VAL(j, VAL_ALL_MAXHEALTH));

  send_to_char(ch, "Object Material: @y%s@n\r\n",
   material_names[GET_OBJ_MATERIAL(j)]);

  if (j->ex_description) {
    send_to_char(ch, "Extra descs:");
    for (desc = j->ex_description; desc; desc = desc->next)
      send_to_char(ch, " [@g%s@n]", desc->keyword);
  }

  sprintbitarray(GET_OBJ_WEAR(j), wear_bits, TW_ARRAY_MAX, buf);
  send_to_char(ch, "Can be worn on: %s\r\n", buf);

  sprintbitarray(GET_OBJ_PERM(j), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "Set char bits : %s\r\n", buf);

  sprintbitarray(GET_OBJ_EXTRA(j), extra_bits, EF_ARRAY_MAX, buf);
  send_to_char(ch, "Size: %s  Extra flags   : %s\r\n", size_names[GET_OBJ_SIZE(j)], buf);

  send_to_char(ch, "Weight: %d, Value: %d, Cost/day: %d, Timer: %d, Min Level: %d\r\n",
     GET_OBJ_WEIGHT(j), GET_OBJ_COST(j), GET_OBJ_RENT(j), GET_OBJ_TIMER(j), GET_OBJ_LEVEL(j));

  send_to_char(ch, "In room: %d (%s), ", GET_ROOM_VNUM(IN_ROOM(j)),
	IN_ROOM(j) == NOWHERE ? "Nowhere" : world[IN_ROOM(j)].name);

  /*
   * NOTE: In order to make it this far, we must already be able to see the
   *       character holding the object. Therefore, we do not need CAN_SEE().
   */
  send_to_char(ch, "In object: %s, ", j->in_obj ? j->in_obj->short_description : "None");
  send_to_char(ch, "Carried by: %s, ", j->carried_by ? GET_NAME(j->carried_by) : "Nobody");
  send_to_char(ch, "Worn by: %s\r\n", j->worn_by ? GET_NAME(j->worn_by) : "Nobody");

  switch (GET_OBJ_TYPE(j)) {
  case ITEM_LIGHT:
    if (GET_OBJ_VAL(j, VAL_LIGHT_HOURS) == -1)
      send_to_char(ch, "Hours left: Infinite\r\n");
    else
      send_to_char(ch, "Hours left: [%d]\r\n", GET_OBJ_VAL(j, VAL_LIGHT_HOURS));
    break;
  case ITEM_SCROLL:
    send_to_char(ch, "Spell: (Level %d) %s\r\n", GET_OBJ_VAL(j, VAL_SCROLL_LEVEL),
	    skill_name(GET_OBJ_VAL(j, VAL_SCROLL_SPELL1)));
    break;
  case ITEM_POTION:
    send_to_char(ch, "Spells: (Level %d) %s, %s, %s\r\n", GET_OBJ_VAL(j, VAL_POTION_LEVEL),
	    skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL1)), 
            skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL2)),
	    skill_name(GET_OBJ_VAL(j, VAL_POTION_SPELL3)));
    break;
  case ITEM_WAND:
  case ITEM_STAFF:
    send_to_char(ch, "Spell: %s at level %d, %d (of %d) charges remaining\r\n",
	    skill_name(GET_OBJ_VAL(j, VAL_STAFF_SPELL)), GET_OBJ_VAL(j, VAL_STAFF_LEVEL),
	    GET_OBJ_VAL(j, VAL_STAFF_CHARGES), GET_OBJ_VAL(j, VAL_STAFF_MAXCHARGES));
    break;
  case ITEM_WEAPON:
    send_to_char(ch, "Weapon Type: %s, Todam: %dd%d, Message type: %d\r\n",
	    weapon_type[GET_OBJ_VAL(j, VAL_WEAPON_SKILL)], 
            GET_OBJ_VAL(j, VAL_WEAPON_DAMDICE),
            GET_OBJ_VAL(j, VAL_WEAPON_DAMSIZE),
            GET_OBJ_VAL(j, VAL_WEAPON_DAMTYPE));
    send_to_char(ch, "Crit type: %s, Crit range: %d-20\r\n",
            crit_type[GET_OBJ_VAL(j, 6)], 20 - GET_OBJ_VAL(j, 8));
    break;
  case ITEM_ARMOR:
    send_to_char(ch, "Armor Type: %s, AC-apply: [%d]\r\n", armor_type[GET_OBJ_VAL(j, VAL_ARMOR_SKILL)], GET_OBJ_VAL(j, VAL_ARMOR_APPLYAC));
    send_to_char(ch, "Max dex bonus: %d, Armor penalty: %d, Spell failure: %d\r\n",
                 GET_OBJ_VAL(j, VAL_ARMOR_MAXDEXMOD), GET_OBJ_VAL(j, VAL_ARMOR_CHECK), GET_OBJ_VAL(j, VAL_ARMOR_SPELLFAIL));
    break;
  case ITEM_TRAP:
    send_to_char(ch, "Spell: %d, - Hitpoints: %d\r\n", GET_OBJ_VAL(j, VAL_TRAP_SPELL), GET_OBJ_VAL(j, VAL_TRAP_HITPOINTS));
    break;
  case ITEM_CONTAINER:
    sprintbit(GET_OBJ_VAL(j, VAL_CONTAINER_FLAGS), container_bits, buf, sizeof(buf));
    send_to_char(ch, "Weight capacity: %d, Lock Type: %s, Key Num: %d, Corpse: %s\r\n",
	    GET_OBJ_VAL(j, VAL_CONTAINER_CAPACITY), buf, GET_OBJ_VAL(j, VAL_CONTAINER_KEY),
	    YESNO(GET_OBJ_VAL(j, VAL_CONTAINER_CORPSE)));
    break;
  case ITEM_DRINKCON:
  case ITEM_FOUNTAIN:
    sprinttype(GET_OBJ_VAL(j, VAL_DRINKCON_LIQUID), drinks, buf, sizeof(buf));
    send_to_char(ch, "Capacity: %d, Contains: %d, Poisoned: %s, Liquid: %s\r\n",
	    GET_OBJ_VAL(j, VAL_DRINKCON_CAPACITY), GET_OBJ_VAL(j, VAL_DRINKCON_HOWFULL), YESNO(GET_OBJ_VAL(j, VAL_DRINKCON_POISON)), buf);
    break;
  case ITEM_NOTE:
    send_to_char(ch, "Tongue: %d\r\n", GET_OBJ_VAL(j, VAL_NOTE_LANGUAGE));
    break;
  case ITEM_KEY:
    /* Nothing */
    break;
  case ITEM_FOOD:
    send_to_char(ch, "Makes full: %d, Poisoned: %s\r\n", GET_OBJ_VAL(j, VAL_FOOD_FOODVAL), YESNO(GET_OBJ_VAL(j, VAL_FOOD_POISON)));
    break;
  case ITEM_MONEY:
    send_to_char(ch, "Coins: %d\r\n", GET_OBJ_VAL(j, VAL_MONEY_SIZE));
    break;
  default:
    send_to_char(ch, "Values 0-12: [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d] [%d]\r\n",
	    GET_OBJ_VAL(j, 0), GET_OBJ_VAL(j, 1),
	    GET_OBJ_VAL(j, 2), GET_OBJ_VAL(j, 3),
	    GET_OBJ_VAL(j, 4), GET_OBJ_VAL(j, 5),
	    GET_OBJ_VAL(j, 6), GET_OBJ_VAL(j, 7),
	    GET_OBJ_VAL(j, 8), GET_OBJ_VAL(j, 9),
	    GET_OBJ_VAL(j, 10), GET_OBJ_VAL(j, 11));
    break;
  }

  /*
   * I deleted the "equipment status" code from here because it seemed
   * more or less useless and just takes up valuable screen space.
   */

  if (j->contains) {
    int column;

    send_to_char(ch, "\r\nContents:@g");
    column = 9;	/* ^^^ strlen ^^^ */

    for (found = 0, j2 = j->contains; j2; j2 = j2->next_content) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", j2->short_description);
      if (column >= 62) {
	send_to_char(ch, "%s\r\n", j2->next_content ? "," : "");
	found = FALSE;
        column = 0;
      }
    }
    send_to_char(ch, "@n");
  }

  found = FALSE;
  send_to_char(ch, "Affections:");
  for (i = 0; i < MAX_OBJ_AFFECT; i++)
    if (j->affected[i].modifier) {
      sprinttype(j->affected[i].location, apply_types, buf, sizeof(buf));
      send_to_char(ch, "%s %+d to %s", found++ ? "," : "", j->affected[i].modifier, buf);
      switch (j->affected[i].location) {
      case APPLY_FEAT:
        send_to_char(ch, " (%s)", feat_list[j->affected[i].specific].name);
        break;
      case APPLY_SKILL:
        send_to_char(ch, " (%s)", spell_info[j->affected[i].specific].name);
        break;
      }
    }
  if (!found)
    send_to_char(ch, " None");

  send_to_char(ch, "\r\n");

  /* check the object for a script */
  do_sstat_object(ch, j);
}


void do_stat_character(struct char_data *ch, struct char_data *k)
{
  char buf[MAX_STRING_LENGTH];
  char buf2[MAX_STRING_LENGTH];
  int i, i2, column, found = FALSE;
  struct obj_data *j;
  struct follow_type *fol;
  struct affected_type *aff;
  struct damreduct_type *reduct;

  sprinttype(GET_SEX(k), genders, buf, sizeof(buf));
  send_to_char(ch, "%s %s '%s'  IDNum: [%5ld], In room [%5d], Loadroom : [%5d]\r\n",
	  buf, (!IS_NPC(k) ? "PC" : (!IS_MOB(k) ? "NPC" : "MOB")),
	  GET_NAME(k), IS_NPC(k) ? GET_ID(k) : GET_IDNUM(k), GET_ROOM_VNUM(IN_ROOM(k)), IS_NPC(k) ? NOWHERE : GET_LOADROOM(k));

  //if (IS_MOB(k))
    //send_to_char(ch, "Alias: %s, VNum: [%5d], RNum: [%5d]\r\n", k->name, GET_MOB_VNUM(k), GET_MOB_RNUM(k));

  if (IS_MOB(k)) {
    if (k->master_id > -1)
      sprintf(buf, ", Master: %s", get_name_by_id(k->master_id));
    else
      buf[0] = 0;
    send_to_char(ch, "Alias: %s, VNum: [%5d], RNum: [%5d]%s\r\n", k->name,
                 GET_MOB_VNUM(k), GET_MOB_RNUM(k), buf);
  } else

  send_to_char(ch, "Title: %s\r\n", k->title ? k->title : "<None>");

  send_to_char(ch, "L-Des: %s", k->long_descr ? k->long_descr : "<None>\r\n");
  if (CONFIG_ALLOW_MULTICLASS) {
    strncpy(buf, class_desc_str(k, 1, 0), sizeof(buf));
  } else {
    sprinttype(k->chclass, pc_class_types, buf, sizeof(buf));
  }
  sprinttype(k->race, pc_race_types, buf2, sizeof(buf2));
  send_to_char(ch, "Class: %s, Race: %s, Lev: [@y%2d(%dHD+%dcl+%d)@n], XP: [@y%7d@n]\r\n",
                   buf, buf2, GET_LEVEL(k), GET_HITDICE(k),
                   GET_CLASS_LEVEL(k), GET_LEVEL_ADJ(k), GET_EXP(k));

  if (!IS_NPC(k)) {
    char buf1[64], buf2[64];

    strlcpy(buf1, asctime(localtime(&(k->time.created))), sizeof(buf1));
    strlcpy(buf2, asctime(localtime(&(k->time.logon))), sizeof(buf2));
    buf1[10] = buf2[10] = '\0';

    send_to_char(ch, "Created: [%s], Last Logon: [%s], Played [%dh %dm], Age [%d]\r\n",
	    buf1, buf2, (int)k->time.played / 3600,
	    (int)((k->time.played % 3600) / 60), age(k)->year);

    send_to_char(ch, "Hometown: [%d], Align: [%4d], Ethic: [%4d]", k->hometown,
                 GET_ALIGNMENT(k), GET_ETHIC_ALIGNMENT(k));

    /*. Display OLC zone for immorts .*/
    if (GET_ADMLEVEL(k) >= ADMLVL_BUILDER) {
      if (GET_OLC_ZONE(k)==AEDIT_PERMISSION)
        send_to_char(ch, ", OLC[@cActions@n]");
      else if (GET_OLC_ZONE(k)==NOWHERE)
        send_to_char(ch, ", OLC[@cOFF@n]");
      else
        send_to_char(ch, ", OLC: [@c%d@n]", GET_OLC_ZONE(k));
    }
    send_to_char(ch, "\r\n");
  }
  send_to_char(ch, "Str: [@c%d@n]  Int: [@c%d@n]  Wis: [@c%d@n]  "
	  "Dex: [@c%d@n]  Con: [@c%d@n]  Cha: [@c%d@n]\r\n",
	  GET_STR(k), GET_INT(k), GET_WIS(k), GET_DEX(k), GET_CON(k), GET_CHA(k));

  send_to_char(ch, "Hit:[@g%d/%d+%d@n]  Mana:[@g%d/%d+%d@n]  Moves:[@g%d/%d+%d@n]  Ki: [@g%d/%d+%d@n]\r\n",
	  GET_HIT(k), GET_MAX_HIT(k), hit_gain(k),
	  GET_MANA(k), GET_MAX_MANA(k), mana_gain(k),
	  GET_MOVE(k), GET_MAX_MOVE(k), move_gain(k),
	  GET_KI(k), GET_MAX_KI(k), ki_gain(k));

  if (GET_ADMLEVEL(k))
    send_to_char(ch, "Admin Level: [@y%d - %s@n]\r\n", GET_ADMLEVEL(k), admin_level_names[GET_ADMLEVEL(k)]);

  send_to_char(ch, "Coins: [%9d], Bank: [%9d] (Total: %d)\r\n",
	  GET_GOLD(k), GET_BANK_GOLD(k), GET_GOLD(k) + GET_BANK_GOLD(k));

  send_to_char(ch, "Armor: [%d (%+dsz%+ddx%+dcl)], Accuracy: [%2d%+d], Damage: [%2d], Saving throws: [%d/%d/%d]\r\n",
	  compute_armor_class(k, NULL), get_size_bonus(get_size(k)) * 10,
          ability_mod_value(GET_DEX(k)) * 10, class_armor_bonus(k), GET_ACCURACY_MOD(k),
          get_size_bonus(get_size(k)), GET_DAMAGE_MOD(k), GET_SAVE_MOD(k, 0),
          GET_SAVE_MOD(k, 1), GET_SAVE_MOD(k, 2));

  send_to_char(ch, "Hit base: [%d(%+dbo%+dst)]\r\n", 
          GET_ACCURACY_BASE(k), GET_ACCURACY_MOD(k), ability_mod_value(GET_STR(k)));

  sprinttype(GET_POS(k), position_types, buf, sizeof(buf));
  send_to_char(ch, "Pos: %s, Fighting: %s", buf, FIGHTING(k) ? GET_NAME(FIGHTING(k)) : "Nobody");

  if (IS_NPC(k))
    send_to_char(ch, ", Attack type: %s", attack_hit_text[(int) k->mob_specials.attack_type].singular);

  if (k->desc) {
    sprinttype(STATE(k->desc), connected_types, buf, sizeof(buf));
    send_to_char(ch, ", Connected: %s", buf);
  }

  if (IS_NPC(k)) {
    sprinttype(k->mob_specials.default_pos, position_types, buf, sizeof(buf));
    send_to_char(ch, ", Default position: %s\r\n", buf);
    sprintbitarray(MOB_FLAGS(k), action_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "NPC flags: @c%s@n\r\n", buf);
  } else {
    send_to_char(ch, ", Idle Timer (in tics) [%d]\r\n", k->timer);

    sprintbitarray(PLR_FLAGS(k), player_bits, PM_ARRAY_MAX, buf);
    send_to_char(ch, "PLR: @c%s@n\r\n", buf);

    sprintbitarray(PRF_FLAGS(k), preference_bits, PR_ARRAY_MAX, buf);
    send_to_char(ch, "PRF: @g%s@n\r\n", buf);
  }

  if (IS_MOB(k))
    send_to_char(ch, "Mob Spec-Proc: %s, NPC Bare Hand Dam: %dd%d\r\n",
	    (mob_index[GET_MOB_RNUM(k)].func ? "Exists" : "None"),
	    k->mob_specials.damnodice, k->mob_specials.damsizedice);

  for (i = 0, j = k->carrying; j; j = j->next_content, i++);
  send_to_char(ch, "Carried: weight: %d, items: %d; Items in: inventory: %d, ", IS_CARRYING_W(k), IS_CARRYING_N(k), i);

  for (i = 0, i2 = 0; i < NUM_WEARS; i++)
    if (GET_EQ(k, i))
      i2++;
  send_to_char(ch, "eq: %d\r\n", i2);

  if (!IS_NPC(k))
    send_to_char(ch, "Hunger: %d, Thirst: %d, Drunk: %d\r\n", GET_COND(k, FULL), GET_COND(k, THIRST), GET_COND(k, DRUNK));

  column = send_to_char(ch, "Master is: %s, Followers are:", k->master ? GET_NAME(k->master) : "<none>");
  if (!k->followers)
    send_to_char(ch, " <none>\r\n");
  else {
    for (fol = k->followers; fol; fol = fol->next) {
      column += send_to_char(ch, "%s %s", found++ ? "," : "", PERS(fol->follower, ch));
      if (column >= 62) {
        send_to_char(ch, "%s\r\n", fol->next ? "," : "");
        found = FALSE;
        column = 0;
      }
    }
    if (column != 0)
      send_to_char(ch, "\r\n");
  }

  if (k->hit_breakdown[0] || k->hit_breakdown[1]) {
    if (k->hit_breakdown[0] && k->hit_breakdown[0][0] && k->dam_breakdown[0])
      send_to_char(ch, "Primary attack: @y%s.@n", k->hit_breakdown[0]);
      if (k->dam_breakdown[0])
        send_to_char(ch, "@y dam %s %s@n\r\n", k->dam_breakdown[0],
                     k->crit_breakdown[0] ? k->crit_breakdown[0] : "");
      else
        send_to_char(ch, "\r\n");
      send_to_char(ch, "Offhand attack: @y%s.@n", k->hit_breakdown[1]);
      if (k->dam_breakdown[1])
        send_to_char(ch, "@y dam %s %s@n\r\n", k->dam_breakdown[1],
                     k->crit_breakdown[1] ? k->crit_breakdown[1] : "");
      else
        send_to_char(ch, "\r\n");
  }

  if (k->damreduct)
    for (reduct = k->damreduct; reduct; reduct = reduct->next)
      send_to_char(ch, "Damage reduction: @y%s@n\r\n", reduct_desc(reduct));

  /* Showing the bitvector */
  sprintbitarray(AFF_FLAGS(k), affected_bits, AF_ARRAY_MAX, buf);
  send_to_char(ch, "AFF: @y%s@n\r\n", buf);

  /* Routine to show what spells a char is affected by */
  if (k->affected) {
    for (aff = k->affected; aff; aff = aff->next) {
      send_to_char(ch, "SPL: (%3dhr) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

      if (aff->modifier)
	send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
	if (aff->modifier)
	  send_to_char(ch, ", ");

	strcpy(buf, affected_bits[aff->bitvector]);
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }

  /* Routine to show what spells a char is affectedv by */
  if (k->affectedv) {
    for (aff = k->affectedv; aff; aff = aff->next) {
      send_to_char(ch, "SPL: (%3d rounds) @c%-21s@n ", aff->duration + 1, skill_name(aff->type));

      if (aff->modifier)
        send_to_char(ch, "%+d to %s", aff->modifier, apply_types[(int) aff->location]);

      if (aff->bitvector) {
        if (aff->modifier)
          send_to_char(ch, ", ");

        strcpy(buf, affected_bits[aff->bitvector]);
        send_to_char(ch, "sets %s", buf);
      }
      send_to_char(ch, "\r\n");
    }
  }

 /* check mobiles for a script */
 if (IS_NPC(k)) {
   do_sstat_character(ch, k);
   if (SCRIPT_MEM(k)) {
     struct script_memory *mem = SCRIPT_MEM(k);
     send_to_char(ch, "Script memory:\r\n  Remember             Command\r\n");
     while (mem) {
       struct char_data *mc = find_char(mem->id);
       if (!mc)
         send_to_char(ch, "  ** Corrupted!\r\n");
       else {
         if (mem->cmd)
           send_to_char(ch, "  %-20.20s%s\r\n",GET_NAME(mc),mem->cmd);
         else
           send_to_char(ch, "  %-20.20s <default>\r\n",GET_NAME(mc));
       }
     mem = mem->next;
     }
   }
 } else {
   /* this is a PC, display their global variables */
   if (k->script && k->script->global_vars) {
     struct trig_var_data *tv;
     char uname[MAX_INPUT_LENGTH];
     void find_uid_name(char *uid, char *name);

     send_to_char(ch, "Global Variables:\r\n");

     /* currently, variable context for players is always 0, so it is */
     /* not displayed here. in the future, this might change */
     for (tv = k->script->global_vars; tv; tv = tv->next) {
       if (*(tv->value) == UID_CHAR) {
         find_uid_name(tv->value, uname);
         send_to_char(ch, "    %10s:  [UID]: %s\r\n", tv->name, uname);
       } else
         send_to_char(ch, "    %10s:  %s\r\n", tv->name, tv->value);
     }
   }
 }
}


ACMD(do_stat)
{
  char buf1[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *victim;
  struct obj_data *object;

  half_chop(argument, buf1, buf2);

  if (!*buf1) {
    send_to_char(ch, "Stats on who or what or where?\r\n");
    return;
  } else if (is_abbrev(buf1, "room")) {
    do_stat_room(ch);
  } else if (is_abbrev(buf1, "mob")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which mobile?\r\n");
    else {
      if ((victim = get_char_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such mobile around.\r\n");
    }
  } else if (is_abbrev(buf1, "player")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which player?\r\n");
    } else {
      if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
      else
	send_to_char(ch, "No such player around.\r\n");
    }
  } else if (is_abbrev(buf1, "file")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which player?\r\n");
    else if ((victim = get_player_vis(ch, buf2, NULL, FIND_CHAR_WORLD)) != NULL)
	do_stat_character(ch, victim);
    else {
      CREATE(victim, struct char_data, 1);
      clear_char(victim);
      CREATE(victim->player_specials, struct player_special_data, 1);
      if (load_char(buf2, victim) >= 0) {
	char_to_room(victim, 0);
	if (GET_ADMLEVEL(victim) > GET_ADMLEVEL(ch))
	  send_to_char(ch, "Sorry, you can't do that.\r\n");
	else
	  do_stat_character(ch, victim);
	extract_char_final(victim);
      } else {
	send_to_char(ch, "There is no such player.\r\n");
	free_char(victim);
      }
    }
  } else if (is_abbrev(buf1, "object")) {
    if (!*buf2)
      send_to_char(ch, "Stats on which object?\r\n");
    else {
      if ((object = get_obj_vis(ch, buf2, NULL)) != NULL)
	do_stat_object(ch, object);
      else
	send_to_char(ch, "No such object around.\r\n");
    }
  } else if (is_abbrev(buf1, "zone")) {
    if (!*buf2) {
      send_to_char(ch, "Stats on which zone?\r\n");
      return;
    } else {
      print_zone(ch, atoi(buf2));
      return;
    }
  } else {
    char *name = buf1;
    int number = get_number(&name);

    if ((object = get_obj_in_equip_vis(ch, name, &number, ch->equipment)) != NULL)
      do_stat_object(ch, object);
    else if ((object = get_obj_in_list_vis(ch, name, &number, ch->carrying)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_ROOM)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_in_list_vis(ch, name, &number, world[IN_ROOM(ch)].contents)) != NULL)
      do_stat_object(ch, object);
    else if ((victim = get_char_vis(ch, name, &number, FIND_CHAR_WORLD)) != NULL)
      do_stat_character(ch, victim);
    else if ((object = get_obj_vis(ch, name, &number)) != NULL)
      do_stat_object(ch, object);
    else
      send_to_char(ch, "Nothing around by that name.\r\n");
  }
}


ACMD(do_shutdown)
{
  char arg[MAX_INPUT_LENGTH];

  if (subcmd != SCMD_SHUTDOWN) {
    send_to_char(ch, "If you want to shut something down, say so!\r\n");
    return;
  }
  one_argument(argument, arg);

  if (!*arg) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down.\r\n");
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2;
  } else if (!str_cmp(arg, "reboot")) {
    log("(GC) Reboot by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    touch(FASTBOOT_FILE);
    circle_shutdown = circle_reboot = 1;
  } else if (!str_cmp(arg, "die")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(KILLSCRIPT_FILE);
    circle_shutdown = 1;
  } else if (!str_cmp(arg, "now")) {
    log("(GC) Shutdown NOW by %s.", GET_NAME(ch));
    send_to_all("Rebooting.. come back in a minute or two.\r\n");
    circle_shutdown = 1;
    circle_reboot = 2; /* do not autosave olc */
  } else if (!str_cmp(arg, "pause")) {
    log("(GC) Shutdown by %s.", GET_NAME(ch));
    send_to_all("Shutting down for maintenance.\r\n");
    touch(PAUSE_FILE);
    circle_shutdown = 1;
  } else
    send_to_char(ch, "Unknown shutdown option.\r\n");
}


void snoop_check(struct char_data *ch)
{
  /*  This short routine is to ensure that characters that happen
   *  to be snooping (or snooped) and get advanced/demoted will
   *  not be snooping/snooped someone of a higher/lower level (and
   *  thus, not entitled to be snooping.
   */
  if (!ch || !ch->desc)
    return;
  if (ch->desc->snooping &&
     (GET_ADMLEVEL(ch->desc->snooping->character) >= GET_ADMLEVEL(ch))) {
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }

  if (ch->desc->snoop_by &&
     (GET_ADMLEVEL(ch) >= GET_ADMLEVEL(ch->desc->snoop_by->character))) {
    ch->desc->snoop_by->snooping = NULL;
    ch->desc->snoop_by = NULL;
  }
}

void stop_snooping(struct char_data *ch)
{
  if (!ch->desc->snooping)
    send_to_char(ch, "You aren't snooping anyone.\r\n");
  else {
    send_to_char(ch, "You stop snooping.\r\n");
    ch->desc->snooping->snoop_by = NULL;
    ch->desc->snooping = NULL;
  }
}


ACMD(do_snoop)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim, *tch;

  if (!ch->desc)
    return;

  one_argument(argument, arg);

  if (!*arg)
    stop_snooping(ch);
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such person around.\r\n");
  else if (!victim->desc)
    send_to_char(ch, "There's no link.. nothing to snoop.\r\n");
  else if (victim == ch)
    stop_snooping(ch);
  else if (victim->desc->snoop_by)
    send_to_char(ch, "Busy already. \r\n");
  else if (victim->desc->snooping == ch->desc)
    send_to_char(ch, "Don't be stupid.\r\n");
  else {
    if (victim->desc->original)
      tch = victim->desc->original;
    else
      tch = victim;

    if (GET_ADMLEVEL(tch) >= GET_ADMLEVEL(ch)) {
      send_to_char(ch, "You can't.\r\n");
      return;
    }
    send_to_char(ch, "%s", CONFIG_OK);

    if (ch->desc->snooping)
      ch->desc->snooping->snoop_by = NULL;

    ch->desc->snooping = victim->desc;
    victim->desc->snoop_by = ch->desc;
  }
}



ACMD(do_switch)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *victim;

  one_argument(argument, arg);

  if (ch->desc->original)
    send_to_char(ch, "You're already switched.\r\n");
  else if (!*arg)
    send_to_char(ch, "Switch with who?\r\n");
  else if (!(victim = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "No such character.\r\n");
  else if (ch == victim)
    send_to_char(ch, "Hee hee... we are jolly funny today, eh?\r\n");
  else if (victim->desc)
    send_to_char(ch, "You can't do that, the body is already in use!\r\n");
  else if (!(IS_NPC(victim) || ADM_FLAGGED(ch, ADM_SWITCHMORTAL)))
    send_to_char(ch, "You aren't holy enough to use a mortal's body.\r\n");
  else if (GET_ADMLEVEL(ch) < ADMLVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_GODROOM))
    send_to_char(ch, "You are not godly enough to use that room!\r\n");
  else if (GET_ADMLEVEL(ch) < ADMLVL_GRGOD && ROOM_FLAGGED(IN_ROOM(victim), ROOM_HOUSE)
		&& !House_can_enter(ch, GET_ROOM_VNUM(IN_ROOM(victim))))
    send_to_char(ch, "That's private property -- no trespassing!\r\n");
  else {
    send_to_char(ch, "%s", CONFIG_OK);

    ch->desc->character = victim;
    ch->desc->original = ch;

    victim->desc = ch->desc;
    ch->desc = NULL;
  }
}


ACMD(do_return)
{
  if (ch->desc && ch->desc->original) {
    send_to_char(ch, "You return to your original body.\r\n");

    /*
     * If someone switched into your original body, disconnect them.
     *   - JE 2/22/95
     *
     * Zmey: here we put someone switched in our body to disconnect state
     * but we must also NULL his pointer to our character, otherwise
     * close_socket() will damage our character's pointer to our descriptor
     * (which is assigned below in this function). 12/17/99
     */
    if (ch->desc->original->desc) {
      ch->desc->original->desc->character = NULL;
      STATE(ch->desc->original->desc) = CON_DISCONNECT;
    }

    /* Now our descriptor points to our original body. */
    ch->desc->character = ch->desc->original;
    ch->desc->original = NULL;

    /* And our body's pointer to descriptor now points to our descriptor. */
    ch->desc->character->desc = ch->desc;
    ch->desc = NULL;
  }
}



ACMD(do_load)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: load { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That is not a number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, IN_ROOM(ch));

    act("$n makes a quaint, magical gesture with one hand.", TRUE, ch,
	0, 0, TO_ROOM);
    act("$n has created $N!", FALSE, ch, 0, mob, TO_ROOM);
    act("You create $N.", FALSE, ch, 0, mob, TO_CHAR);
    load_mtrigger(mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    add_unique_id(obj);
    if (CONFIG_LOAD_INVENTORY)
      obj_to_char(obj, ch);
    else
      obj_to_room(obj, IN_ROOM(ch));
    act("$n makes a strange magical gesture.", TRUE, ch, 0, 0, TO_ROOM);
    act("$n has created $p!", FALSE, ch, obj, 0, TO_ROOM);
    act("You create $p.", FALSE, ch, obj, 0, TO_CHAR);
    load_otrigger(obj);
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}



ACMD(do_vstat)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  two_arguments(argument, buf, buf2);

  if (!*buf || !*buf2 || !isdigit(*buf2)) {
    send_to_char(ch, "Usage: vstat { obj | mob } <number>\r\n");
    return;
  }
  if (!is_number(buf2)) {
    send_to_char(ch, "That's not a valid number.\r\n");
    return;
  }

  if (is_abbrev(buf, "mob")) {
    struct char_data *mob;
    mob_rnum r_num;

    if ((r_num = real_mobile(atoi(buf2))) == NOBODY) {
      send_to_char(ch, "There is no monster with that number.\r\n");
      return;
    }
    mob = read_mobile(r_num, REAL);
    char_to_room(mob, 0);
    do_stat_character(ch, mob);
    extract_char(mob);
  } else if (is_abbrev(buf, "obj")) {
    struct obj_data *obj;
    obj_rnum r_num;

    if ((r_num = real_object(atoi(buf2))) == NOTHING) {
      send_to_char(ch, "There is no object with that number.\r\n");
      return;
    }
    obj = read_object(r_num, REAL);
    do_stat_object(ch, obj);
    extract_obj(obj);
  } else
    send_to_char(ch, "That'll have to be either 'obj' or 'mob'.\r\n");
}




/* clean a room of all mobiles and objects */
ACMD(do_purge)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  struct obj_data *obj;

  one_argument(argument, buf);

  /* argument supplied. destroy single object or char */
  if (*buf) {
    if ((vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)) != NULL) {
      if (!IS_NPC(vict) && (GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))) {
	send_to_char(ch, "Fuuuuuuuuu!\r\n");
	return;
      }
      act("$n disintegrates $N.", FALSE, ch, 0, vict, TO_NOTVICT);

      if (!IS_NPC(vict)) {
	mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s has purged %s.", GET_NAME(ch), GET_NAME(vict));
	if (vict->desc) {
	  STATE(vict->desc) = CON_CLOSE;
	  vict->desc->character = NULL;
	  vict->desc = NULL;
	}
      }
      extract_char(vict);
    } else if ((obj = get_obj_in_list_vis(ch, buf, NULL, world[IN_ROOM(ch)].contents)) != NULL) {
      act("$n destroys $p.", FALSE, ch, obj, 0, TO_ROOM);
      extract_obj(obj);
    } else {
      send_to_char(ch, "Nothing here by that name.\r\n");
      return;
    }

    send_to_char(ch, "%s", CONFIG_OK);
  } else {			/* no argument. clean out the room */
    int i;

    act("$n gestures... You are surrounded by scorching flames!",
	FALSE, ch, 0, 0, TO_ROOM);
    send_to_room(IN_ROOM(ch), "The world seems a little cleaner.\r\n");

    for (vict = world[IN_ROOM(ch)].people; vict; vict = vict->next_in_room) {
      if (!IS_NPC(vict))
        continue;

      /* Dump inventory. */
      while (vict->carrying)
        extract_obj(vict->carrying);

      /* Dump equipment. */
      for (i = 0; i < NUM_WEARS; i++)
        if (GET_EQ(vict, i))
          extract_obj(GET_EQ(vict, i));

      /* Dump character. */
      extract_char(vict);
    }

    /* Clear the ground. */
    while (world[IN_ROOM(ch)].contents)
      extract_obj(world[IN_ROOM(ch)].contents);
  }
}



const char *logtypes[] = {
  "off", "brief", "normal", "complete", "\n"
};

ACMD(do_syslog)
{
  char arg[MAX_INPUT_LENGTH];
  int tp;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "Your syslog is currently %s.\r\n",
	logtypes[(PRF_FLAGGED(ch, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(ch, PRF_LOG2) ? 2 : 0)]);
    return;
  }
  if (((tp = search_block(arg, logtypes, FALSE)) == -1)) {
    send_to_char(ch, "Usage: syslog { Off | Brief | Normal | Complete }\r\n");
    return;
  }
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
  REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
  if (tp & 1) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
  if (tp & 2) SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);

  send_to_char(ch, "Your syslog is now %s.\r\n", logtypes[tp]);
}

#define EXE_FILE "bin/circle" /* maybe use argv[0] but it's not reliable */

/* (c) 1996-97 Erwin S. Andreasen <erwin@pip.dknet.dk> */
ACMD(do_copyover)
{
#ifdef CIRCLE_WINDOWS
  send_to_char(ch, "Copyover is not available for Windows.\r\n");
#else
  FILE *fp;
  struct descriptor_data *d, *d_next;
  char buf [100], buf2[100];
	
  fp = fopen (COPYOVER_FILE, "w");
	
  if (!fp) {
    send_to_char (ch, "Copyover file not writeable, aborted.\n\r");
    return;
  }
	
  /* Consider changing all saved areas here, if you use OLC */
  save_all();
  sprintf (buf, "\t\x1B[1;31m \007\007\007Time stops for a moment as %s folds space and time.\x1B[0;0m\r\n", GET_NAME(ch));
  /* For each playing descriptor, save its state */
  for (d = descriptor_list; d ; d = d_next) {
    struct char_data * och = d->character;
    d_next = d->next; /* We delete from the list , so need to save this */
    if (!d->character || d->connected > CON_PLAYING) {
      write_to_descriptor (d->descriptor, "\n\rSorry, we are rebooting. Come back in a few seconds.\n\r", d->comp);
      close_socket (d); /* throw'em out */
    } else {
      fprintf (fp, "%d %s %s %d\n", d->descriptor, GET_NAME(och), d->host, GET_ROOM_VNUM(IN_ROOM(och)));
      log("printing descriptor name and host of connected players");
      /* save och */
      Crash_rentsave(och, 0);
      save_char(och);
#ifdef HAVE_ZLIB_H
      if (d->comp->state == 2) {
        d->comp->state = 3; /* Code to use Z_FINISH for deflate */
      }
#endif /* HAVE_ZLIB_H */
      write_to_descriptor (d->descriptor, buf, d->comp);
      d->comp->state = 0;
#ifdef HAVE_ZLIB_H
      if (d->comp->stream) {
        deflateEnd(d->comp->stream);
        free(d->comp->stream);
        free(d->comp->buff_out);
        free(d->comp->buff_in);
      }
#endif /* HAVE_ZLIB_H */
    }
  }
	
  fprintf (fp, "-1\n");
  fclose (fp);

  /* Close reserve and other always-open files and release other resources
     since we are now using ASCII pfiles, closing the player_fl would crash
     the game, since it's no longer around, so I commented it out. I'll
     leave the code here, for historical reasons -spl
     fclose(player_fl); */

  /* exec - descriptors are inherited */
	
  sprintf (buf, "%d", port);
  sprintf (buf2, "-C%d", mother_desc);
  chdir ("..");
  execl (EXE_FILE, "circle", buf2, buf, (char *) NULL);
  /* Failed - sucessful exec will not return */
	
  perror ("do_copyover: execl");
  send_to_char (ch, "Copyover FAILED!\n\r");
	
  exit (1); /* too much trouble to try to recover! */
#endif
}

ACMD(do_advance)
{
  struct char_data *victim;
  char name[MAX_INPUT_LENGTH], level[MAX_INPUT_LENGTH];
  int newlevel, oldlevel;

  two_arguments(argument, name, level);

  if (*name) {
    if (!(victim = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
      send_to_char(ch, "That player is not here.\r\n");
      return;
    }
  } else {
    send_to_char(ch, "Advance who?\r\n");
    return;
  }

  if (IS_NPC(victim)) {
    send_to_char(ch, "NO!  Not on NPC's.\r\n");
    return;
  }
  if (!*level || (newlevel = atoi(level)) <= 0) {
    send_to_char(ch, "That's not a level!\r\n");
    return;
  }
  if (newlevel >= CONFIG_LEVEL_CAP) {
    send_to_char(ch, "%d is the highest possible level.\r\n", CONFIG_LEVEL_CAP - 1);
    return;
  }
  if (newlevel == GET_LEVEL(victim)) {
    send_to_char(ch, "They are already at that level.\r\n");
    return;
  }
  oldlevel = GET_LEVEL(victim);
  if (newlevel < GET_LEVEL(victim)) {
    send_to_char(ch, "You cannot demote a player.\r\n");
  } else {
    act("$n makes some strange gestures.\r\n"
	"A strange feeling comes upon you, like a giant hand, light comes down\r\n"
	"from above, grabbing your body, which begins to pulse with colored\r\n"
        "lights from inside.\r\n\r\n"
	"Your head seems to be filled with demons from another plane as your\r\n"
        "body dissolves to the elements of time and space itself.\r\n\r\n"
	"Suddenly a silent explosion of light snaps you back to reality.\r\n\r\n"
	"You feel slightly different.", FALSE, ch, 0, victim, TO_VICT);
  }

  send_to_char(ch, "%s", CONFIG_OK);

  if (newlevel < oldlevel)
    log("(GC) %s demoted %s from level %d to %d.",
		GET_NAME(ch), GET_NAME(victim), oldlevel, newlevel);
  else
    log("(GC) %s has advanced %s to level %d (from %d)",
		GET_NAME(ch), GET_NAME(victim), newlevel, oldlevel);

  gain_exp_regardless(victim,
	 level_exp(newlevel) - GET_EXP(victim));
  save_char(victim);
}

ACMD(do_restore)
{
  char buf[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int i;

  one_argument(argument, buf);
  if (!*buf)
    send_to_char(ch, "Whom do you wish to restore?\r\n");
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (!IS_NPC(vict) && ch != vict && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
    send_to_char(ch, "They don't need your help.\r\n");
  else {
    GET_HIT(vict) = GET_MAX_HIT(vict);
    GET_MANA(vict) = GET_MAX_MANA(vict);
    GET_MOVE(vict) = GET_MAX_MOVE(vict);
    GET_KI(vict) = GET_MAX_KI(vict);

    if (!IS_NPC(vict) && GET_ADMLEVEL(ch) >= ADMLVL_GRGOD) {
      if (GET_ADMLEVEL(vict) >= ADMLVL_IMMORT)
        for (i = 1; i <= MAX_SKILLS; i++)
          SET_SKILL(vict, i, 100);

      if (GET_ADMLEVEL(vict) >= ADMLVL_GRGOD) {
	vict->real_abils.intel = 25;
	vict->real_abils.wis = 25;
	vict->real_abils.dex = 25;
	vict->real_abils.str = 25;
	vict->real_abils.con = 25;
	vict->real_abils.cha = 25;
      }
    }
    update_pos(vict);
    affect_total(vict);
    send_to_char(ch, "%s", CONFIG_OK);
    act("You have been fully healed by $N!", FALSE, vict, 0, ch, TO_CHAR);
  }
}


void perform_immort_vis(struct char_data *ch)
{
  GET_INVIS_LEV(ch) = 0;
}


void perform_immort_invis(struct char_data *ch, int level)
{
  struct char_data *tch;

  for (tch = world[IN_ROOM(ch)].people; tch; tch = tch->next_in_room) {
    if (tch == ch)
      continue;
    if (GET_ADMLEVEL(tch) >= GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) < level)
      act("You blink and suddenly realize that $n is gone.", FALSE, ch, 0,
	  tch, TO_VICT);
    if (GET_ADMLEVEL(tch) < GET_INVIS_LEV(ch) && GET_ADMLEVEL(tch) >= level)
      act("You suddenly realize that $n is standing beside you.", FALSE, ch, 0,
	  tch, TO_VICT);
  }

  GET_INVIS_LEV(ch) = level;
  send_to_char(ch, "Your invisibility level is %d.\r\n", level);
}
  

ACMD(do_invis)
{
  char arg[MAX_INPUT_LENGTH];
  int level;

  if (IS_NPC(ch)) {
    send_to_char(ch, "You can't do that!\r\n");
    return;
  }

  one_argument(argument, arg);
  if (!*arg) {
    if (GET_INVIS_LEV(ch) > 0)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, GET_ADMLEVEL(ch));
  } else {
    level = atoi(arg);
    if (level > GET_ADMLEVEL(ch))
      send_to_char(ch, "You can't go invisible above your own level.\r\n");
    else if (level < 1)
      perform_immort_vis(ch);
    else
      perform_immort_invis(ch, level);
  }
}


ACMD(do_gecho)
{
  struct descriptor_data *pt;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument)
    send_to_char(ch, "That must be a mistake...\r\n");
  else {
    for (pt = descriptor_list; pt; pt = pt->next)
      if (IS_PLAYING(pt) && pt->character && pt->character != ch)
	send_to_char(pt->character, "%s\r\n", argument);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "%s\r\n", argument);
  }
}


ACMD(do_poofset)
{
  char **msg;

  switch (subcmd) {
  case SCMD_POOFIN:    msg = &(POOFIN(ch));    break;
  case SCMD_POOFOUT:   msg = &(POOFOUT(ch));   break;
  default:    return;
  }

  skip_spaces(&argument);

  if (*msg)
    free(*msg);

  if (!*argument)
    *msg = NULL;
  else
    *msg = strdup(argument);

  send_to_char(ch, "%s", CONFIG_OK);
}



ACMD(do_dc)
{
  char arg[MAX_INPUT_LENGTH];
  struct descriptor_data *d;
  int num_to_dc;

  one_argument(argument, arg);
  if (!(num_to_dc = atoi(arg))) {
    send_to_char(ch, "Usage: DC <user number> (type USERS for a list)\r\n");
    return;
  }
  for (d = descriptor_list; d && d->desc_num != num_to_dc; d = d->next);

  if (!d) {
    send_to_char(ch, "No such connection.\r\n");
    return;
  }
  if (d->character && GET_ADMLEVEL(d->character) >= GET_ADMLEVEL(ch)) {
    if (!CAN_SEE(ch, d->character))
      send_to_char(ch, "No such connection.\r\n");
    else
      send_to_char(ch, "Umm.. maybe that's not such a good idea...\r\n");
    return;
  }

  /* We used to just close the socket here using close_socket(), but
   * various people pointed out this could cause a crash if you're
   * closing the person below you on the descriptor list.  Just setting
   * to CON_CLOSE leaves things in a massively inconsistent state so I
   * had to add this new flag to the descriptor. -je
   *
   * It is a much more logical extension for a CON_DISCONNECT to be used
   * for in-game socket closes and CON_CLOSE for out of game closings.
   * This will retain the stability of the close_me hack while being
   * neater in appearance. -gg 12/1/97
   *
   * For those unlucky souls who actually manage to get disconnected
   * by two different immortals in the same 1/10th of a second, we have
   * the below 'if' check. -gg 12/17/99
   */
  if (STATE(d) == CON_DISCONNECT || STATE(d) == CON_CLOSE)
    send_to_char(ch, "They're already being disconnected.\r\n");
  else {
    /*
     * Remember that we can disconnect people not in the game and
     * that rather confuses the code when it expected there to be
     * a character context.
     */
    if (STATE(d) == CON_PLAYING)
      STATE(d) = CON_DISCONNECT;
    else
      STATE(d) = CON_CLOSE;

    send_to_char(ch, "Connection #%d closed.\r\n", num_to_dc);
    log("(GC) Connection closed by %s.", GET_NAME(ch));
  }
}



ACMD(do_wizlock)
{
  char arg[MAX_INPUT_LENGTH];
  int value;
  const char *when;

  one_argument(argument, arg);
  if (*arg) {
    value = atoi(arg);
    if (value < 0 || value > CONFIG_LEVEL_CAP) {
      send_to_char(ch, "Invalid wizlock value.\r\n");
      return;
    }
    circle_restrict = value;
    when = "now";
  } else
    when = "currently";

  switch (circle_restrict) {
  case 0:
    send_to_char(ch, "The game is %s completely open.\r\n", when);
    break;
  case 1:
    send_to_char(ch, "The game is %s closed to new players.\r\n", when);
    break;
  default:
    send_to_char(ch, "Only level %d and above may enter the game %s.\r\n", circle_restrict, when);
    break;
  }
}


ACMD(do_date)
{
  char *tmstr;
  time_t mytime;
  int d, h, m;

  if (subcmd == SCMD_DATE)
    mytime = time(0);
  else
    mytime = boot_time;

  tmstr = (char *) asctime(localtime(&mytime));
  *(tmstr + strlen(tmstr) - 1) = '\0';

  if (subcmd == SCMD_DATE)
    send_to_char(ch, "Current machine time: %s\r\n", tmstr);
  else {
    mytime = time(0) - boot_time;
    d = mytime / 86400;
    h = (mytime / 3600) % 24;
    m = (mytime / 60) % 60;

    send_to_char(ch, "Up since %s: %d day%s, %d:%02d\r\n", tmstr, d, d == 1 ? "" : "s", h, m);
  }
}



ACMD(do_last)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict = NULL;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "For whom do you wish to search?\r\n");
    return;
  }
  CREATE(vict, struct char_data, 1);
  clear_char(vict);
  CREATE(vict->player_specials, struct player_special_data, 1);
  if (load_char(arg, vict) <  0) {
    send_to_char(ch, "There is no such player.\r\n");
    free_char(vict);
    return;
  }
  if ((GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch)) && (GET_ADMLEVEL(ch) < ADMLVL_IMPL)) {
    send_to_char(ch, "You are not sufficiently godly for that!\r\n");
    return;
  }

  send_to_char(ch, "[%5ld] [%2d %s %s] %-12s : %-18s : %-20s\r\n",
    GET_IDNUM(vict), (int) GET_LEVEL(vict),
    race_abbrevs[(int) GET_RACE(vict)], class_abbrevs[(int) GET_CLASS(vict)],
    GET_NAME(vict), vict->player_specials->host && *vict->player_specials->host
    ? vict->player_specials->host : "(NOHOST)",
    ctime(&vict->time.logon));
  free_char(vict);
}


ACMD(do_force)
{
  struct descriptor_data *i, *next_desc;
  struct char_data *vict, *next_force;
  char arg[MAX_INPUT_LENGTH], to_force[MAX_INPUT_LENGTH], buf1[MAX_INPUT_LENGTH + 32];

  half_chop(argument, arg, to_force);

  snprintf(buf1, sizeof(buf1), "$n has forced you to '%s'.", to_force);

  if (!*arg || !*to_force)
    send_to_char(ch, "Whom do you wish to force do what?\r\n");
  else if (!ADM_FLAGGED(ch, ADM_FORCEMASS) || (str_cmp("all", arg) && str_cmp("room", arg))) {
    if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
      send_to_char(ch, "%s", CONFIG_NOPERSON);
    else if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict))
      send_to_char(ch, "No, no, no!\r\n");
    else {
      send_to_char(ch, "%s", CONFIG_OK);
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced %s to %s", GET_NAME(ch), GET_NAME(vict), to_force);
      command_interpreter(vict, to_force);
    }
  } else if (!str_cmp("room", arg)) {
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced room %d to %s",
		GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), to_force);

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_force) {
      next_force = vict->next_in_room;
      if (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  } else { /* force all */
    send_to_char(ch, "%s", CONFIG_OK);
    mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s forced all to %s", GET_NAME(ch), to_force);

    for (i = descriptor_list; i; i = next_desc) {
      next_desc = i->next;

      if (STATE(i) != CON_PLAYING || !(vict = i->character) || (!IS_NPC(vict) && GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)))
	continue;
      act(buf1, TRUE, ch, NULL, vict, TO_VICT);
      command_interpreter(vict, to_force);
    }
  }
}



ACMD(do_wiznet)
{
  char buf1[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32],
	buf2[MAX_INPUT_LENGTH + MAX_NAME_LENGTH + 32];
  struct descriptor_data *d;
  char emote = FALSE;
  char any = FALSE;
  int level = ADMLVL_IMMORT;

  skip_spaces(&argument);
  delete_doubledollar(argument);

  if (!*argument) {
    send_to_char(ch, "Usage: wiznet <text> | #<level> <text> | *<emotetext> |\r\n        wiznet @<level> *<emotetext> | wiz @\r\n");
    return;
  }
  switch (*argument) {
  case '*':
    emote = TRUE;
  case '#':
    one_argument(argument + 1, buf1);
    if (is_number(buf1)) {
      half_chop(argument+1, buf1, argument);
      level = MAX(atoi(buf1), ADMLVL_IMMORT);
      if (level > GET_ADMLEVEL(ch)) {
	send_to_char(ch, "You can't wizline above your own level.\r\n");
	return;
      }
    } else if (emote)
      argument++;
    break;

  case '@':
    send_to_char(ch, "God channel status:\r\n");
    for (any = 0, d = descriptor_list; d; d = d->next) {
      if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(d->character) < ADMLVL_IMMORT)
        continue;
      if (!CAN_SEE(ch, d->character))
        continue;

      send_to_char(ch, "  %-*s%s%s%s\r\n", MAX_NAME_LENGTH, GET_NAME(d->character),
		PLR_FLAGGED(d->character, PLR_WRITING) ? " (Writing)" : "",
		PLR_FLAGGED(d->character, PLR_MAILING) ? " (Writing mail)" : "",
		PRF_FLAGGED(d->character, PRF_NOWIZ) ? " (Offline)" : "");
    }
    return;

  case '\\':
    ++argument;
    break;
  default:
    break;
  }
  if (PRF_FLAGGED(ch, PRF_NOWIZ)) {
    send_to_char(ch, "You are offline!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Don't bother the gods like that!\r\n");
    return;
  }
  if (level > ADMLVL_IMMORT) {
    snprintf(buf1, sizeof(buf1), "@c%s@c: <%d> %s%s@n\r\n", GET_NAME(ch), level, emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone: <%d> %s%s@n\r\n", level, emote ? "<--- " : "", argument);
  } else {
    snprintf(buf1, sizeof(buf1), "@c%s@c: %s%s@n\r\n", GET_NAME(ch), emote ? "<--- " : "", argument);
    snprintf(buf2, sizeof(buf1), "@cSomeone: %s%s@n\r\n", emote ? "<--- " : "", argument);
  }

  for (d = descriptor_list; d; d = d->next) {
    if ((STATE(d) == CON_PLAYING) && (GET_ADMLEVEL(d->character) >= level) &&
	(!PRF_FLAGGED(d->character, PRF_NOWIZ)) &&
	(!PLR_FLAGGED(d->character, PLR_WRITING | PLR_MAILING))
	&& (d != ch->desc || !(PRF_FLAGGED(d->character, PRF_NOREPEAT)))) {
      if (CAN_SEE(d->character, ch))
	send_to_char(d->character, "%s", buf1);
      else
	send_to_char(d->character, "%s", buf2);
    }
  }

  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
}



ACMD(do_zreset)
{
  char arg[MAX_INPUT_LENGTH];
  zone_rnum i;
  zone_vnum j;

  one_argument(argument, arg);
  if (!*arg) {
    send_to_char(ch, "You must specify a zone.\r\n");
    return;
  }
  if (*arg == '*') {
    for (i = 0; i <= top_of_zone_table; i++)
      reset_zone(i);
    send_to_char(ch, "Reset world.\r\n");
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset entire world.", GET_NAME(ch));
    return;
  } else if (*arg == '.')
    i = world[IN_ROOM(ch)].zone;
  else {
    j = atoi(arg);
    for (i = 0; i <= top_of_zone_table; i++)
      if (zone_table[i].number == j)
	break;
  }
  if (i <= top_of_zone_table) {
    reset_zone(i);
    send_to_char(ch, "Reset zone %d (#%d): %s.\r\n", i, zone_table[i].number, zone_table[i].name);
    mudlog(NRM, MAX(ADMLVL_GRGOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s reset zone %d (%s)", GET_NAME(ch), i, zone_table[i].name);
  } else
    send_to_char(ch, "Invalid zone number.\r\n");
}


/*
 *  General fn for wizcommands of the sort: cmd <player>
 */
ACMD(do_wizutil)
{
  char arg[MAX_INPUT_LENGTH];
  struct char_data *vict;
  int taeller;
  long result;

  one_argument(argument, arg);

  if (!*arg)
    send_to_char(ch, "Yes, but for whom?!?\r\n");
  else if (!(vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "There is no such player.\r\n");
  else if (IS_NPC(vict))
    send_to_char(ch, "You can't do that to a mob!\r\n");
  else if (GET_ADMLEVEL(vict) > GET_ADMLEVEL(ch))
    send_to_char(ch, "Hmmm...you'd better not.\r\n");
  else {
    switch (subcmd) {
    case SCMD_REROLL:
      send_to_char(ch, "Rerolled...\r\n");
      roll_real_abils(vict);
      log("(GC) %s has rerolled %s.", GET_NAME(ch), GET_NAME(vict));
      send_to_char(ch, "New stats: Str %d, Int %d, Wis %d, Dex %d, Con %d, Cha %d\r\n",
	      GET_STR(vict), GET_INT(vict), GET_WIS(vict),
	      GET_DEX(vict), GET_CON(vict), GET_CHA(vict));
      break;
    case SCMD_PARDON:
      if (!PLR_FLAGGED(vict, PLR_THIEF) &&  !PLR_FLAGGED(vict, PLR_KILLER)) {
	send_to_char(ch, "Your victim is not flagged.\r\n");
	return;
      }
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_THIEF);
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_KILLER);
      send_to_char(ch, "Pardoned.\r\n");
      send_to_char(vict, "You have been pardoned by the Gods!\r\n");
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s pardoned by %s", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_NOTITLE:
      result = PLR_TOG_CHK(vict, PLR_NOTITLE);
      mudlog(NRM, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Notitle %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Notitle %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_SQUELCH:
      result = PLR_TOG_CHK(vict, PLR_NOSHOUT);
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) Squelch %s for %s by %s.",
		ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      send_to_char(ch, "(GC) Squelch %s for %s by %s.\r\n", ONOFF(result), GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_FREEZE:
      if (ch == vict) {
	send_to_char(ch, "Oh, yeah, THAT'S real smart...\r\n");
	return;
      }
      if (PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Your victim is already pretty cold.\r\n");
	return;
      }
      SET_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      GET_FREEZE_LEV(vict) = GET_ADMLEVEL(ch);
      send_to_char(vict, "A bitter wind suddenly rises and drains every erg of heat from your body!\r\nYou feel frozen!\r\n");
      send_to_char(ch, "Frozen.\r\n");
      act("A sudden cold wind conjured from nowhere freezes $n!", FALSE, vict, 0, 0, TO_ROOM);
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      break;
    case SCMD_THAW:
      if (!PLR_FLAGGED(vict, PLR_FROZEN)) {
	send_to_char(ch, "Sorry, your victim is not morbidly encased in ice at the moment.\r\n");
	return;
      }
      if (GET_FREEZE_LEV(vict) > GET_ADMLEVEL(ch)) {
	send_to_char(ch, "Sorry, a level %d God froze %s... you can't unfreeze %s.\r\n",
		GET_FREEZE_LEV(vict), GET_NAME(vict), HMHR(vict));
	return;
      }
      mudlog(BRF, MAX(ADMLVL_GOD, GET_INVIS_LEV(ch)), TRUE, "(GC) %s un-frozen by %s.", GET_NAME(vict), GET_NAME(ch));
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_FROZEN);
      send_to_char(vict, "A fireball suddenly explodes in front of you, melting the ice!\r\nYou feel thawed.\r\n");
      send_to_char(ch, "Thawed.\r\n");
      act("A sudden fireball conjured from nowhere thaws $n!", FALSE, vict, 0, 0, TO_ROOM);
      break;
    case SCMD_UNAFFECT:
      if (vict->affected || AFF_FLAGS(vict) || vict->affectedv) {
        while (vict->affected)
          affect_remove(vict, vict->affected);
          for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
          AFF_FLAGS(ch)[taeller] = 0;
        while (vict->affectedv)
          affectv_remove(vict, vict->affectedv);
          for(taeller=0; taeller < AF_ARRAY_MAX; taeller++)
          AFF_FLAGS(ch)[taeller] = 0;
	send_to_char(vict, "There is a brief flash of light!\r\nYou feel slightly different.\r\n");
	send_to_char(ch, "All spells removed.\r\n");
      } else {
	send_to_char(ch, "Your victim does not have any affections!\r\n");
	return;
      }
      break;
    default:
      log("SYSERR: Unknown subcmd %d passed to do_wizutil (%s)", subcmd, __FILE__);
      break;
    }
    save_char(vict);
  }
}


/* single zone printing fn used by "show zone" so it's not repeated in the
   code 3 times ... -je, 4/6/93 */

/* FIXME: overflow possible */
size_t print_zone_to_buf(char *bufptr, size_t left, zone_rnum zone, int listall)
{
  size_t tmp;
  
  if (listall) {
    int i, j, k, l, m, n, o;
    extern int top_of_trigt;
    extern struct index_data **trig_index;
    int count_shops(shop_vnum low, shop_vnum high);
    int count_guilds(shop_vnum low, shop_vnum high);

    tmp = snprintf(bufptr, left,
	"%3d %-30.30s By: %-10.10s Age: %3d; Reset: %3d (%1d); Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name, zone_table[zone].builders,
	zone_table[zone].age, zone_table[zone].lifespan,
	zone_table[zone].reset_mode,
	zone_table[zone].bot, zone_table[zone].top);
        i = j = k = l = m = n = o = 0;
        
        for (i = 0; i < top_of_world; i++)
          if (world[i].number >= zone_table[zone].bot && world[i].number <= zone_table[zone].top)
            j++;

        for (i = 0; i < top_of_objt; i++)
          if (obj_index[i].vnum >= zone_table[zone].bot && obj_index[i].vnum <= zone_table[zone].top)
            k++;
        
        for (i = 0; i < top_of_mobt; i++)
          if (mob_index[i].vnum >= zone_table[zone].bot && mob_index[i].vnum <= zone_table[zone].top)
            l++;

        m = count_shops(zone_table[zone].bot, zone_table[zone].top);
        o = count_guilds(zone_table[zone].bot, zone_table[zone].top);

        for (i = 0; i < top_of_trigt; i++)
          if (trig_index[i]->vnum >= zone_table[zone].bot && trig_index[i]->vnum <= zone_table[zone].top)
            n++;

	tmp += snprintf(bufptr + tmp, left - tmp,
                        "       Zone stats:\r\n"
                        "       ---------------\r\n"
                        "         Rooms:    %2d\r\n"
                        "         Objects:  %2d\r\n"
                        "         Mobiles:  %2d\r\n"
                        "         Shops:    %2d\r\n"
                        "         Triggers: %2d\r\n"
                        "         Guilds:   %2d\r\n", 
                          j, k, l, m, n, o);
        
    return tmp;
  } 
  
  return snprintf(bufptr, left,
	"%3d %-30.30s By: %-10.10s Range: %5d-%5d\r\n",
	zone_table[zone].number, zone_table[zone].name, zone_table[zone].builders,
	zone_table[zone].bot, zone_table[zone].top);
}


ACMD(do_show)
{
  int i, j, k, l, con, level_table;                /* i, j, k to specifics? */
  size_t len, nlen;
  zone_rnum zrn;
  zone_vnum zvn;
  int low, high;
  byte self = FALSE;
  struct char_data *vict = NULL;
  struct obj_data *obj;
  struct descriptor_data *d;
  struct affected_type *aff;
  char field[MAX_INPUT_LENGTH], value[MAX_INPUT_LENGTH], *strp,
	arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
  extern int top_of_trigt;

  struct show_struct {
    const char *cmd;
    const char level;
  } fields[] = {
    { "nothing",	0  },				/* 0 */
    { "zones",		ADMLVL_IMMORT },		/* 1 */
    { "player",		ADMLVL_GOD },
    { "rent",		ADMLVL_GRGOD },
    { "stats",		ADMLVL_IMMORT },
    { "errors",		ADMLVL_IMPL },			/* 5 */
    { "death",		ADMLVL_GOD },
    { "godrooms",	ADMLVL_IMMORT },
    { "shops",		ADMLVL_IMMORT },
    { "houses",		ADMLVL_GOD },
    { "snoop",		ADMLVL_GRGOD },			/* 10 */
    { "assemblies",     ADMLVL_IMMORT },
    { "guilds",         ADMLVL_GOD },
    { "levels",         ADMLVL_GRGOD },
    { "uniques",        ADMLVL_GRGOD },
    { "affect",         ADMLVL_GRGOD },			/* 15 */
    { "affectv",        ADMLVL_GRGOD },
    { "\n", 0 }
  };

  skip_spaces(&argument);

  if (!*argument) {
    send_to_char(ch, "Show options:\r\n");
    for (j = 0, i = 1; fields[i].level; i++)
      if (fields[i].level <= GET_ADMLEVEL(ch))
	send_to_char(ch, "%-15s%s", fields[i].cmd, (!(++j % 5) ? "\r\n" : ""));
    send_to_char(ch, "\r\n");
    return;
  }

  strcpy(arg, two_arguments(argument, field, value));	/* strcpy: OK (argument <= MAX_INPUT_LENGTH == arg) */

  for (l = 0; *(fields[l].cmd) != '\n'; l++)
    if (!strncmp(field, fields[l].cmd, strlen(field)))
      break;

  if (GET_ADMLEVEL(ch) < fields[l].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return;
  }
  if (!strcmp(value, "."))
    self = TRUE;
  buf[0] = '\0';

  switch (l) {
  /* show zone */
  case 1:
    /* tightened up by JE 4/6/93 */
    if (self)
      print_zone_to_buf(buf, sizeof(buf), world[IN_ROOM(ch)].zone, 1);
    else if (*value && is_number(value)) {
      for (zvn = atoi(value), zrn = 0; zone_table[zrn].number != zvn && zrn <= top_of_zone_table; zrn++);
      if (zrn <= top_of_zone_table)
	print_zone_to_buf(buf, sizeof(buf), zrn, 1);
      else {
	send_to_char(ch, "That is not a valid zone.\r\n");
	return;
      }
    } else
      for (len = zrn = 0; zrn <= top_of_zone_table; zrn++) {
	nlen = print_zone_to_buf(buf + len, sizeof(buf) - len, zrn, 0);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show player */
  case 2:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }

    CREATE(vict, struct char_data, 1);
    clear_char(vict);
    CREATE(vict->player_specials, struct player_special_data, 1);
    if (load_char(value, vict) < 0) {
      send_to_char(ch, "There is no such player.\r\n");
      free_char(vict);
      return;
    }
    send_to_char(ch, "Player: %-12s (%s) [%2d %s %s]\r\n", GET_NAME(vict),
      genders[(int) GET_SEX(vict)], GET_LEVEL(vict), class_abbrevs[(int)
      GET_CLASS(vict)], race_abbrevs[(int) GET_RACE(vict)]);
    send_to_char(ch, "Au: %-8d  Bal: %-8d  Exp: %-8d  Align: %-5d  Ethic: %-5d\r\n",
                 GET_GOLD(vict), GET_BANK_GOLD(vict), GET_EXP(vict),
                 GET_ALIGNMENT(vict), GET_ETHIC_ALIGNMENT(vict));
    if (CONFIG_ALLOW_MULTICLASS)
      send_to_char(ch, "Class ranks: %s\r\n", class_desc_str(vict, 1, 0));

    /* ctime() uses static buffer: do not combine. */
    send_to_char(ch, "Started: %-20.16s  ", ctime(&vict->time.created));
    send_to_char(ch, "Last: %-20.16s  Played: %3dh %2dm\r\n",
      ctime(&vict->time.logon),
      (int) (vict->time.played / 3600),
      (int) (vict->time.played / 60 % 60));
    free_char(vict);
    break;

  /* show rent */
  case 3:
    if (!*value) {
      send_to_char(ch, "A name would help.\r\n");
      return;
    }
    Crash_listrent(ch, value);
    break;

  /* show stats */
  case 4:
    i = 0;
    j = 0;
    k = 0;
    con = 0;
    for (vict = character_list; vict; vict = vict->next) {
      if (IS_NPC(vict))
	j++;
      else if (CAN_SEE(ch, vict)) {
	i++;
	if (vict->desc)
	  con++;
      }
    }
    for (obj = object_list; obj; obj = obj->next)
      k++;
    send_to_char(ch,
	"Current stats:\r\n"
	"  %5d players in game  %5d connected\r\n"
	"  %5d registered\r\n"
	"  %5d mobiles          %5d prototypes\r\n"
	"  %5d objects          %5d prototypes\r\n"
	"  %5d rooms            %5d zones\r\n"
        "  %5d triggers\r\n"
	"  %5d large bufs\r\n"
	"  %5d buf switches     %5d overflows\r\n",
	i, con,
	top_of_p_table + 1,
	j, top_of_mobt + 1,
	k, top_of_objt + 1,
	top_of_world + 1, top_of_zone_table + 1,
	top_of_trigt + 1,
	buf_largecount,
	buf_switches, buf_overflows
	);
    break;

  /* show errors */
  case 5:
    len = strlcpy(buf, "Errant Rooms\r\n------------\r\n", sizeof(buf));
    for (i = 0, k = 0; i <= top_of_world; i++)
      for (j = 0; j < NUM_OF_DIRS; j++) {
      	if (!W_EXIT(i,j))
      	  continue;
        if (W_EXIT(i,j)->to_room == 0) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (void   ) [%5d] %-40s (%s)\r\n", ++k, GET_ROOM_VNUM(i), world[i].name, dirs[j]);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
        if (W_EXIT(i,j)->to_room == NOWHERE && !W_EXIT(i,j)->general_description) {
	    nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: (Nowhere) [%5d] %-40s (%s)\r\n", ++k, GET_ROOM_VNUM(i), world[i].name, dirs[j]);
          if (len + nlen >= sizeof(buf) || nlen < 0)
            break;
          len += nlen;
        }
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show death */
  case 6:
    len = strlcpy(buf, "Death Traps\r\n-----------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_DEATH)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show godrooms */
  case 7:
    len = strlcpy(buf, "Godrooms\r\n--------------------------\r\n", sizeof(buf));
    for (i = 0, j = 0; i <= top_of_world; i++)
      if (ROOM_FLAGGED(i, ROOM_GODROOM)) {
        nlen = snprintf(buf + len, sizeof(buf) - len, "%2d: [%5d] %s\r\n", ++j, GET_ROOM_VNUM(i), world[i].name);
        if (len + nlen >= sizeof(buf) || nlen < 0)
          break;
        len += nlen;
      }
    page_string(ch->desc, buf, TRUE);
    break;

  /* show shops */
  case 8:
    show_shops(ch, value);
    break;

  /* show houses */
  case 9:
    hcontrol_list_houses(ch);
    break;

  /* show snoop */
  case 10:
    i = 0;
    send_to_char(ch, "People currently snooping:\r\n--------------------------\r\n");
    for (d = descriptor_list; d; d = d->next) {
      if (d->snooping == NULL || d->character == NULL)
	continue;
      if (STATE(d) != CON_PLAYING || GET_ADMLEVEL(ch) < GET_ADMLEVEL(d->character))
	continue;
      if (!CAN_SEE(ch, d->character) || IN_ROOM(d->character) == NOWHERE)
	continue;
      i++;
      send_to_char(ch, "%-10s - snooped by %s.\r\n", GET_NAME(d->snooping->character), GET_NAME(d->character));
    }
    if (i == 0)
      send_to_char(ch, "No one is currently snooping.\r\n");
    break;
  /* show assembly */
  case 11:
    assemblyListToChar(ch);
    break;
  /* show guilds */
  case 12:
    show_guild(ch, value);
    break;

  /* show level tables */
  case 13:
    if (!*value) {
      send_to_char(ch, "Show Level Options:\r\n"
                       "  Version    - Level configuration table version\r\n"
                       "  Experience - Experience points needed to level\r\n"
                       "  Basehit    - Hit base values\r\n"
		       "  Fortitude  - Fortitude saving throws\r\n"
		       "  Reflex     - Reflex saving throws\r\n"
		       "  Will       - Will saving throws\r\n");
    } else if ((level_table = search_block(value, config_sect, FALSE)) == -1) {
      send_to_char(ch, "Invalid level table option.\r\n");
    } else {
      switch (level_table) {
        case 0: send_to_char(ch, "%s (code: %d)\r\n", level_version, level_vernum); break;
        case 1: /* Experience table */
          send_to_char(ch, 
              "Level Exp\r\n"
              "----- --------------------\r\n");
	  for (i = 0; i < CONFIG_LEVEL_CAP; i++) {
	    send_to_char(ch, "@c  %3d@n", i);
            send_to_char(ch, " %10d\r\n", level_exp(i));
	  }
	  break;
        case 3:  /* fort */
        case 4:  /* reflex */
        case 5:  /* will */
          send_to_char(ch, "Level");
          for (i = 0; i < NUM_CLASSES; i++)
            send_to_char(ch, "  %s", class_abbrevs[i]);
          send_to_char(ch, "\n-----");
          while (i--)
            send_to_char(ch, " ---");
          send_to_char(ch, "\n");
	  for (i = 0; i < LVL_EPICSTART; i++) {
	    send_to_char(ch, "@c  %3d@n", i);
            for (j = 0; j < NUM_CLASSES; j++)
              send_to_char(ch, " %3d", saving_throw_lookup(0, j, level_table - 3, i));
            send_to_char(ch, "\r\n");
	  }
	  break;
        case 6:
          send_to_char(ch, "Level");
          for (i = 0; i < NUM_CLASSES; i++)
            send_to_char(ch, "  %s", class_abbrevs[i]);
          send_to_char(ch, "\n-----");
          while (i--)
            send_to_char(ch, " ---");
          send_to_char(ch, "\n");
	  for (i = 0; i < LVL_EPICSTART; i++) {
	    send_to_char(ch, "@c  %3d@n", i);
            for (j = 0; j < NUM_CLASSES; j++)
              send_to_char(ch, " %3d", base_hit(0, j, i));
            send_to_char(ch, "\r\n");
	  }
	  break;
        default: send_to_char(ch, "Not yet implemented.\r\n"); break;
      }
    }
    break;

  case 14:
    if (value && *value) {
      if (sscanf(value, "%d-%d", &low, &high) != 2) {
        if (sscanf(value, "%d", &low) != 1) {
          send_to_char(ch, "Usage: show uniques, show uniques [vnum], or show uniques [low-high]\r\n");
          return;
        } else {
          high = low;
        }
      }
    } else {
      low = -1;
      high = 9999999;
    }
    strp = sprintuniques(low, high);
    page_string(ch->desc, strp, TRUE);
    free(strp);
    break;

  case 15:
  case 16:
    if (!*value) {
      low = 1;
      vict = (l == 15) ? affect_list : affectv_list;
    } else {
      low = 0;
      if (! (vict = get_char_world_vis(ch, value, NULL))) {
        send_to_char(ch, "Cannot find that character.\r\n");
        return;
      }
    }
    k = MAX_STRING_LENGTH;
    CREATE(strp, char, k);
    strp[0] = j = 0;
    if (!vict) {
      send_to_char(ch, "None.\r\n");
      return;
    }
    do {
      if ((k - j) < (MAX_INPUT_LENGTH * 8)) {
        k *= 2;
        RECREATE(strp, char, k);
      }
      j += snprintf(strp + j, k - j, "Name: %s\r\n", GET_NAME(vict));
      if (l == 15)
        aff = vict->affected;
      else
        aff = vict->affectedv;
      for (; aff; aff = aff->next) {
        j += snprintf(strp + j, k - j, "SPL: (%3d%s) @c%-21s@n ", aff->duration + 1,
                      (l == 15) ? "hr" : "rd", skill_name(aff->type));

        if (aff->modifier)
          j += snprintf(strp + j, k - j, "%+d to %s", aff->modifier,
                        apply_types[(int) aff->location]);

        if (aff->bitvector) {
          if (aff->modifier)
            j += snprintf(strp + j, k - j, ", ");

          strcpy(field, affected_bits[aff->bitvector]);
          j += snprintf(strp + j, k - j, "sets %s", field);
        }
        j += snprintf(strp + j, k - j, "\r\n");
      }
      if (l == 15)
        vict = vict->next_affect;
      else
        vict = vict->next_affectv;
    } while (low && vict);
    page_string(ch->desc, strp, TRUE);
    free(strp);
    break;

  /* show what? */
  default:
    send_to_char(ch, "Sorry, I don't understand that.\r\n");
    break;
  }
}


/***************** The do_set function ***********************************/

#define PC   1
#define NPC  2
#define BOTH 3

#define MISC	0
#define BINARY	1
#define NUMBER	2

#define SET_OR_REMOVE(flagset, flags) { \
	if (on) SET_BIT_AR(flagset, flags); \
	else if (off) REMOVE_BIT_AR(flagset, flags); }

#define RANGE(low, high) (value = MAX((low), MIN((high), (value))))


/* The set options available */
  struct set_struct {
    const char *cmd;
    const char level;
    const char pcnpc;
    const char type;
  } set_fields[] = {
   { "brief",		ADMLVL_GOD, 	PC, 	BINARY },  /* 0 */
   { "invstart", 	ADMLVL_GOD, 	PC, 	BINARY },  /* 1 */
   { "title",		ADMLVL_GOD, 	PC, 	MISC },
   { "nosummon", 	ADMLVL_GRGOD, 	PC, 	BINARY },
   { "maxhit",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "maxmana", 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },  /* 5 */
   { "maxmove", 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "hit", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "mana",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "move",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "align",		ADMLVL_GOD, 	BOTH, 	NUMBER },  /* 10 */
   { "str",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "stradd",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "int", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "wis", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "dex", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },  /* 15 */
   { "con", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "cha",		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "ac", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "gold",		ADMLVL_GOD, 	BOTH, 	NUMBER },
   { "bank",		ADMLVL_GOD, 	PC, 	NUMBER },  /* 20 */
   { "exp", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "accuracy", 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "damage", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "invis",		ADMLVL_IMPL, 	PC, 	NUMBER },
   { "nohassle", 	ADMLVL_GRGOD, 	PC, 	BINARY },  /* 25 */
   { "frozen",		ADMLVL_FREEZE, 	PC, 	BINARY },
   { "practices", 	ADMLVL_GRGOD, 	PC, 	NUMBER },
   { "lessons", 	ADMLVL_GRGOD, 	PC, 	NUMBER },
   { "drunk",		ADMLVL_GRGOD, 	BOTH, 	MISC },
   { "hunger",		ADMLVL_GRGOD, 	BOTH, 	MISC },    /* 30 */
   { "thirst",		ADMLVL_GRGOD, 	BOTH, 	MISC },
   { "killer",		ADMLVL_GOD, 	PC, 	BINARY },
   { "thief",		ADMLVL_GOD, 	PC, 	BINARY },
   { "level",		ADMLVL_IMPL, 	BOTH, 	NUMBER },
   { "room",		ADMLVL_IMPL, 	BOTH, 	NUMBER },  /* 35 */
   { "roomflag", 	ADMLVL_GRGOD, 	PC, 	BINARY },
   { "siteok",		ADMLVL_GRGOD, 	PC, 	BINARY },
   { "deleted", 	ADMLVL_IMPL, 	PC, 	BINARY },
   { "class",		ADMLVL_GRGOD, 	BOTH, 	MISC },
   { "nowizlist", 	ADMLVL_GOD, 	PC, 	BINARY },  /* 40 */
   { "quest",		ADMLVL_GOD, 	PC, 	BINARY },
   { "loadroom", 	ADMLVL_GRGOD, 	PC, 	MISC },
   { "color",		ADMLVL_GOD, 	PC, 	BINARY },
   { "idnum",		ADMLVL_IMPL, 	PC, 	NUMBER },
   { "passwd",		ADMLVL_IMPL, 	PC, 	MISC },    /* 45 */
   { "nodelete", 	ADMLVL_GOD, 	PC, 	BINARY },
   { "sex", 		ADMLVL_GRGOD, 	BOTH, 	MISC },
   { "age",		ADMLVL_GRGOD,	BOTH,	NUMBER },
   { "height",		ADMLVL_GOD,	BOTH,	NUMBER },
   { "weight",		ADMLVL_GOD,	BOTH,	NUMBER },  /* 50 */
   { "olc",		ADMLVL_GRGOD,	PC,	MISC },
   { "race",		ADMLVL_GRGOD,	PC,	MISC },
   { "trains",		ADMLVL_GRGOD,	PC,	NUMBER },
   { "feats",		ADMLVL_GRGOD,	PC,	NUMBER },
   { "ethic",		ADMLVL_GOD, 	BOTH, 	NUMBER },  /* 55 */
   { "maxki",	 	ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "ki", 		ADMLVL_GRGOD, 	BOTH, 	NUMBER },
   { "adminlevel",	ADMLVL_IMPL,	PC,	NUMBER },
   { "\n", 0, BOTH, MISC }
  };


int perform_set(struct char_data *ch, struct char_data *vict, int mode,
		char *val_arg)
{
  int i, on = 0, off = 0, value = 0;
  room_rnum rnum;
  room_vnum rvnum;

  /* Check to make sure all the levels are correct */
  if (GET_ADMLEVEL(ch) != ADMLVL_IMPL) {
    if (!IS_NPC(vict) && GET_ADMLEVEL(ch) <= GET_ADMLEVEL(vict) && vict != ch) {
      send_to_char(ch, "Maybe that's not such a great idea...\r\n");
      return (0);
    }
  }
  if (GET_ADMLEVEL(ch) < set_fields[mode].level) {
    send_to_char(ch, "You are not godly enough for that!\r\n");
    return (0);
  }

  /* Make sure the PC/NPC is correct */
  if (IS_NPC(vict) && !(set_fields[mode].pcnpc & NPC)) {
    send_to_char(ch, "You can't do that to a beast!\r\n");
    return (0);
  } else if (!IS_NPC(vict) && !(set_fields[mode].pcnpc & PC)) {
    send_to_char(ch, "That can only be done to a beast!\r\n");
    return (0);
  }

  /* Find the value of the argument */
  if (set_fields[mode].type == BINARY) {
    if (!strcmp(val_arg, "on") || !strcmp(val_arg, "yes"))
      on = 1;
    else if (!strcmp(val_arg, "off") || !strcmp(val_arg, "no"))
      off = 1;
    if (!(on || off)) {
      send_to_char(ch, "Value must be 'on' or 'off'.\r\n");
      return (0);
    }
    send_to_char(ch, "%s %s for %s.\r\n", set_fields[mode].cmd, ONOFF(on), GET_NAME(vict));
  } else if (set_fields[mode].type == NUMBER) {
    value = atoi(val_arg);
    send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
  } else
    send_to_char(ch, "%s", CONFIG_OK);

  switch (mode) {
  case 0:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_BRIEF);
    break;
  case 1:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_INVSTART);
    break;
  case 2:
    set_title(vict, val_arg);
    send_to_char(ch, "%s's title is now: %s\r\n", GET_NAME(vict), GET_TITLE(vict));
    break;
  case 3:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_SUMMONABLE);
    send_to_char(ch, "Nosummon %s for %s.\r\n", ONOFF(!on), GET_NAME(vict));
    break;
  case 4:
    vict->max_hit = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 5:
    vict->max_mana = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 6:
    vict->max_move = RANGE(1, 5000);
    affect_total(vict);
    break;
  case 7:
    vict->hit = RANGE(-9, vict->max_hit);
    affect_total(vict);
    break;
  case 8:
    vict->mana = RANGE(0, vict->max_mana);
    affect_total(vict);
    break;
  case 9:
    vict->move = RANGE(0, vict->max_move);
    affect_total(vict);
    break;
  case 10:
    GET_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;
  case 11:
    RANGE(0, 100);
    vict->real_abils.str = value;
    affect_total(vict);
    break;
  case 12:
    send_to_char(ch, "Setting str_add does nothing now.\r\n");
    /* vict->real_abils.str_add = RANGE(0, 100);
    if (value > 0)
      vict->real_abils.str = 18;
    affect_total(vict);
       break; */
  case 13:
    RANGE(0, 100);
    vict->real_abils.intel = value;
    affect_total(vict);
    break;
  case 14:
    RANGE(0, 100);
    vict->real_abils.wis = value;
    affect_total(vict);
    break;
  case 15:
    RANGE(0, 100);
    vict->real_abils.dex = value;
    affect_total(vict);
    break;
  case 16:
    RANGE(0, 100);
    vict->real_abils.con = value;
    affect_total(vict);
    break;
  case 17:
    RANGE(0, 100);
    vict->real_abils.cha = value;
    affect_total(vict);
    break;
  case 18:
    vict->armor = RANGE(-100, 500);
    affect_total(vict);
    break;
  case 19:
    GET_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 20:
    GET_BANK_GOLD(vict) = RANGE(0, 100000000);
    break;
  case 21:
    vict->exp = RANGE(0, 50000000);
    break;
  case 22:
    vict->accuracy_mod = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 23:
    vict->damage_mod = RANGE(-20, 20);
    affect_total(vict);
    break;
  case 24:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    GET_INVIS_LEV(vict) = RANGE(0, GET_ADMLEVEL(vict));
    break;
  case 25:
    if (GET_ADMLEVEL(ch) < ADMLVL_IMPL && ch != vict) {
      send_to_char(ch, "You aren't godly enough for that!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_NOHASSLE);
    break;
  case 26:
    if (ch == vict && on) {
      send_to_char(ch, "Better not -- could be a long winter!\r\n");
      return (0);
    }
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_FROZEN);
    break;
  case 27:
  case 28:
    if (GET_CLASS_LEVEL(vict))
      GET_PRACTICES(vict, GET_CLASS(vict)) = RANGE(0, 1000);
    else
      GET_RACE_PRACTICES(vict) = RANGE(0, 1000);
    break;
  case 29:
  case 30:
  case 31:
    if (!str_cmp(val_arg, "off")) {
      GET_COND(vict, (mode - 29)) = -1; /* warning: magic number here */
      send_to_char(ch, "%s's %s now off.\r\n", GET_NAME(vict), set_fields[mode].cmd);
    } else if (is_number(val_arg)) {
      value = atoi(val_arg);
      RANGE(0, 24);
      GET_COND(vict, (mode - 29)) = value; /* and here too */
      send_to_char(ch, "%s's %s set to %d.\r\n", GET_NAME(vict), set_fields[mode].cmd, value);
    } else {
      send_to_char(ch, "Must be 'off' or a value from 0 to 24.\r\n");
      return (0);
    }
    break;
  case 32:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_KILLER);
    break;
  case 33:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_THIEF);
    break;
  case 34:
    if (!IS_NPC(vict) && value >= CONFIG_LEVEL_CAP) {
      send_to_char(ch, "You can't do that.\r\n");
      return (0);
    }
    value = MAX(0, value);
    vict->level = value;
    break;
  case 35:
    if ((rnum = real_room(value)) == NOWHERE) {
      send_to_char(ch, "No room exists with that number.\r\n");
      return (0);
    }
    if (IN_ROOM(vict) != NOWHERE)	/* Another Eric Green special. */
      char_from_room(vict);
    char_to_room(vict, rnum);
    break;
  case 36:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_ROOMFLAGS);
    break;
  case 37:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_SITEOK);
    break;
  case 38:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_DELETED);
    break;
  case 39:
    if ((i = search_block(val_arg, class_names, FALSE)) < 0) {
      send_to_char(ch, "That is not a class.\r\n");
      return (0);
    }
    value = GET_CLASS_RANKS(vict, GET_CLASS(vict));
    /* GET_CLASS_RANKS(vict, GET_CLASS(vict)) = 0; */
    GET_CLASS(vict) = i;
    /* GET_CLASS_RANKS(vict, i) = value; */
    break;
  case 40:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NOWIZLIST);
    break;
  case 41:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_QUEST);
    break;
  case 42:
    if (!str_cmp(val_arg, "off")) {
      REMOVE_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
      GET_LOADROOM(vict) = NOWHERE;
    } else if (is_number(val_arg)) {
      rvnum = atoi(val_arg);
      if (real_room(rvnum) != NOWHERE) {
        SET_BIT_AR(PLR_FLAGS(vict), PLR_LOADROOM);
	GET_LOADROOM(vict) = rvnum;
	send_to_char(ch, "%s will enter at room #%d.\r\n", GET_NAME(vict), GET_LOADROOM(vict));
      } else {
	send_to_char(ch, "That room does not exist!\r\n");
	return (0);
      }
    } else {
      send_to_char(ch, "Must be 'off' or a room's virtual number.\r\n");
      return (0);
    }
    break;
  case 43:
    SET_OR_REMOVE(PRF_FLAGS(vict), PRF_COLOR);
    break;
  case 44:
    if (GET_IDNUM(ch) != 1 || !IS_NPC(vict))
      return (0);
    GET_IDNUM(vict) = value;
    break;
  case 45:
    if (GET_IDNUM(ch) > 1) {
      send_to_char(ch, "Please don't use this command, yet.\r\n");
      return (0);
    }
    if (GET_ADMLEVEL(vict) >= ADMLVL_GRGOD) {
      send_to_char(ch, "You cannot change that.\r\n");
      return (0);
    }
    strncpy(GET_PASSWD(vict), CRYPT(val_arg, GET_NAME(vict)), MAX_PWD_LENGTH);	/* strncpy: OK (G_P:MAX_PWD_LENGTH) */
    *(GET_PASSWD(vict) + MAX_PWD_LENGTH) = '\0';
    send_to_char(ch, "Password changed to '%s'.\r\n", val_arg);
    break;
  case 46:
    SET_OR_REMOVE(PLR_FLAGS(vict), PLR_NODELETE);
    break;
  case 47:
    if ((i = search_block(val_arg, genders, FALSE)) < 0) {
      send_to_char(ch, "Must be 'male', 'female', or 'neutral'.\r\n");
      return (0);
    }
    GET_SEX(vict) = i;
    break;
  case 48:	/* set age */
    if (value < 2 || value > 20000) {	/* Arbitrary limits. */
      send_to_char(ch, "Ages 2 to 20000 accepted.\r\n");
      return (0);
    }
    /*
     * NOTE: May not display the exact age specified due to the integer
     * division used elsewhere in the code.  Seems to only happen for
     * some values below the starting age (17) anyway. -gg 5/27/98
     */
    vict->time.birth = time(0) - (value * SECS_PER_MUD_YEAR);
    break;

  case 49:	/* Blame/Thank Rick Glover. :) */
    GET_HEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 50:
    GET_WEIGHT(vict) = value;
    affect_total(vict);
    break;

  case 51:
    if (is_abbrev(val_arg, "socials") || is_abbrev(val_arg, "actions"))
      GET_OLC_ZONE(vict) = AEDIT_PERMISSION;
    else if (is_abbrev(val_arg, "off"))
      GET_OLC_ZONE(vict) = NOWHERE;
    else if (!is_number(val_arg)) {
      send_to_char(ch, "Value must be either 'socials', 'actions', 'off' or a zone number.\r\n");
      return (0);
    } else
      GET_OLC_ZONE(vict) = atoi(val_arg);
    break;

  case 52:
    if ((i = search_block(val_arg, race_names, FALSE )) < 0) {
      send_to_char(ch, "That is not a race.\r\n");
      return (0);
    }
    GET_RACE(vict) = i;
    break;

  case 53:
    GET_TRAINS(vict) = RANGE(0, 500);
    break;

  case 54:
    GET_FEAT_POINTS(vict) = RANGE(0, 500);
    break;

  case 55:
    GET_ETHIC_ALIGNMENT(vict) = RANGE(-1000, 1000);
    affect_total(vict);
    break;

  case 56:
    vict->max_ki = RANGE(1, 5000);
    affect_total(vict);
    break;

  case 57:
    vict->ki = RANGE(0, vict->max_ki);
    affect_total(vict);
    break;

  case 58:
    if (GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)) {
      send_to_char(ch, "Permission denied.\r\n");
      return (0);
    }
    if (value < ADMLVL_NONE || value > GET_ADMLEVEL(ch)) {
      send_to_char(ch, "You can't set it to that.\r\n");
      return (0);
    }
    if (GET_ADMLEVEL(vict) == value)
      return (1);
    admin_set(vict, value);
    break;

  default:
    send_to_char(ch, "Can't set that!\r\n");
    return (0);
  }

  return (1);
}


ACMD(do_set)
{
  struct char_data *vict = NULL, *cbuf = NULL;
  char field[MAX_INPUT_LENGTH], name[MAX_INPUT_LENGTH], buf[MAX_INPUT_LENGTH];
  int mode, len, player_i = 0, retval;
  char is_file = 0, is_player = 0;

  half_chop(argument, name, buf);

  if (!strcmp(name, "file")) {
    is_file = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "player")) {
    is_player = 1;
    half_chop(buf, name, buf);
  } else if (!str_cmp(name, "mob"))
    half_chop(buf, name, buf);

  half_chop(buf, field, buf);

  if (!*name || !*field) {
    send_to_char(ch, "Usage: set <victim> <field> <value>\r\n");
    return;
  }

  /* find the target */
  if (!is_file) {
    if (is_player) {
      if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such player.\r\n");
	return;
      }
    } else { /* is_mob */
      if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
	send_to_char(ch, "There is no such creature.\r\n");
	return;
      }
    }
  } else if (is_file) {
    /* try to load the player off disk */
    CREATE(cbuf, struct char_data, 1);
    clear_char(cbuf);
    CREATE(cbuf->player_specials, struct player_special_data, 1);
    if ((player_i = load_char(name, cbuf)) > -1) {
      if (GET_ADMLEVEL(cbuf) >= GET_ADMLEVEL(ch)) {
	free_char(cbuf);
	send_to_char(ch, "Sorry, you can't do that.\r\n");
	return;
      }
      vict = cbuf;
    } else {
      free_char(cbuf);
      send_to_char(ch, "There is no such player.\r\n");
      return;
    }
  }

  /* find the command in the list */
  len = strlen(field);
  for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++)
    if (!strcmp(field, set_fields[mode].cmd))
      break;
  if (*(set_fields[mode].cmd) == '\n') 
    for (mode = 0; *(set_fields[mode].cmd) != '\n'; mode++) 
      if (!strncmp(field, set_fields[mode].cmd, len)) 
        break; 

  /* perform the set */
  retval = perform_set(ch, vict, mode, buf);

  /* save the character if a change was made */
  if (retval) {
    if (!is_file && !IS_NPC(vict))
      save_char(vict);
    if (is_file) {
      GET_PFILEPOS(cbuf) = player_i;
      save_char(cbuf);
      send_to_char(ch, "Saved in file.\r\n");
    }
  }

  /* free the memory if we allocated it earlier */
  if (is_file)
    free_char(cbuf);
}

ACMD(do_saveall)
{
 if (GET_ADMLEVEL(ch) < ADMLVL_BUILDER)
    send_to_char (ch, "You are not holy enough to use this privelege.\n\r");
 else {
    save_all();
    send_to_char(ch, "World files saved.\n\r");
 }
}

ACMD(do_players)
{
  int i, count = 0;
  char buf[MAX_STRING_LENGTH];
  *buf = 0; 

  send_to_char(ch, "Player List-------------------------------------------------\r\n");

  for (i = 0; i <= top_of_p_table; i++) {
    sprintf(buf, "%s  %-20.20s", buf, (player_table + i)->name);
    count++;
    if (count == 3) {
      count = 0;
      strcat(buf, "\r\n");
    }
  }
  page_string(ch->desc, buf, TRUE);
}

ACMD(do_peace)
{
  struct char_data *vict, *next_v;
  send_to_room(ch->in_room, "Everything is quite peaceful now.\r\n");

    for (vict = world[IN_ROOM(ch)].people; vict; vict = next_v) {
      next_v = vict->next_in_room;  
      if (GET_ADMLEVEL(vict) >= GET_ADMLEVEL(ch)) 
        continue;
      stop_fighting(vict);  
      GET_POS(vict) = POS_SITTING; 
    }
    stop_fighting(ch);
    GET_POS(ch) = POS_STANDING;  
}

ACMD(do_wizupdate)
{
       run_autowiz();
       send_to_char(ch, "Wizlists updated.\n\r");
}

ACMD(do_raise)
{
  room_rnum rm;
  struct affected_type *af, *next_af;
  struct char_data *vict = NULL;
  char name[MAX_INPUT_LENGTH];

  one_argument(argument, name);

  if (!(vict = get_player_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "There is no such player.\r\n");
    return;
  }

  if (IS_NPC(vict)) {
    send_to_char(ch, "Sorry, only players get spirits.\r\n");
    return;
  }

  if (!AFF_FLAGGED(vict, AFF_SPIRIT)) {
    send_to_char(ch, "But they aren't even dead!\r\n");
    return;
  }

  send_to_char(ch, "@yYou return %s from the @Bspirit@y world, to the world of the living!\r\n", GET_NAME(vict));
  send_to_char(vict, "@yYour @Bspirit@y has been returned to the world of the living by %s!\r\n", GET_NAME(ch));

  for (af = vict->affected; af; af = next_af) {
    next_af = af->next;
    if (af->location == APPLY_NONE && af->type == -1 &&
        (af->bitvector == AFF_SPIRIT || af->bitvector == AFF_ETHEREAL))
      affect_remove(vict, af);
  }

  if (GET_HIT(vict) < 1)
    GET_HIT(vict) = 1;

  if ((rm = real_room(freeres[ALIGN_TYPE(vict)])) == NOWHERE)
    rm = real_room(CONFIG_MORTAL_START);

  if (rm != NOWHERE) {
    char_from_room(vict);
    char_to_room(vict, rm);
    look_at_room(IN_ROOM(vict), vict, 0);
  }

  act("$n's body forms in a pool of @Bblue light@n.", TRUE, vict, 0, 0, TO_ROOM);
}

