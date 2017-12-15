/* ************************************************************************
*   File: act.comm.c                                    Part of CircleMUD *
*  Usage: Player-level communication commands                             *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/act.comm.c,v 1.6 2005/01/03 18:23:32 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "screen.h"
#include "improved-edit.h"
#include "dg_scripts.h"
#include "spells.h"
#include "boards.h"

/* local functions */
void perform_tell(struct char_data *ch, struct char_data *vict, char *arg);
int is_tell_ok(struct char_data *ch, struct char_data *vict);
ACMD(do_say);
ACMD(do_say_languages);
ACMD(do_say_original);
ACMD(do_gsay);
ACMD(do_tell);
ACMD(do_reply);
ACMD(do_respond);
ACMD(do_spec_comm);
ACMD(do_write);
ACMD(do_page);
ACMD(do_gen_comm);
ACMD(do_qcomm);

char const *languages[] =
{
  "common",
  "elven",
  "gnomish",
  "dwarven",
  "\n"
};

void list_languages(struct char_data *ch)
{
  int a = 0, i;

  send_to_char(ch, "Languages:\r\n[");
  for (i = MIN_LANGUAGES ; i <= MAX_LANGUAGES ; i++)
    if (GET_SKILL(ch, i))
      send_to_char(ch, "%s %s%s%s",
        a++ != 0 ? "," : "",
        SPEAKING(ch) == i ? "@r": "@n",
        languages[i-MIN_LANGUAGES], "@n");
  send_to_char(ch, "%s ]\r\n", a== 0 ? " None!" : "");
}

ACMD(do_languages)
{
  int i, found = FALSE;
  char arg[MAX_STRING_LENGTH];

  if (CONFIG_ENABLE_LANGUAGES) {
  one_argument(argument, arg);
  if (!*arg)
    list_languages(ch);
  else {
    for (i = MIN_LANGUAGES; i <= MAX_LANGUAGES; i++) {
      if ((search_block(arg, languages, FALSE) == i-MIN_LANGUAGES) && GET_SKILL(ch, i)) {
        SPEAKING(ch) = i;
        send_to_char(ch, "You now speak %s.\r\n", languages[i-MIN_LANGUAGES]);
        found = TRUE;
        break;
      }
    }
    if (!found) {
      send_to_char(ch, "You do not know of any such language.\r\n");
      return;
    }
  }
  } else {
    send_to_char(ch, "But everyone already understands everyone else!\r\n");
    return;
  }

  // Strictly speaking, I'm not sure these need to be here. --Ziz
  /* trigger check */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
}

void garble_text(char *string, int known, int lang)
{
  char letters[50] = "";
  /* Always up letters[13] to the largest size for letters you wish to
  use below. */
  int i, s;

  switch (lang) {
  case SKILL_LANG_DWARVEN:
    strcpy (letters, "hprstwxyz");
    s = 8;
    break;
  case SKILL_LANG_ELVEN:
    strcpy (letters, "aefhilnopstu");
    s = 11;
    break;
  default:
    strcpy (letters, "aehiopstuwxyz");
    s = 12;
    break;
  }

  for (i = 0; i < (int) strlen(string); ++i)
    if (isalpha(string[i]) && (!known))
      string[i] = letters[rand_number(0, s)];
}

ACMD(do_say)
{
  if (CONFIG_ENABLE_LANGUAGES)
    do_say_languages(ch, argument, 0, 0);
  else
    do_say_original(ch, argument, 0, 0);
}

ACMD(do_say_languages)
{
  skip_spaces(&argument);

  if(!*argument) {
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
    return;
  } else {
    char ibuf[MAX_INPUT_LENGTH];
    char obuf[MAX_INPUT_LENGTH];
    char cbuf[MAX_INPUT_LENGTH];
    char verb[10];
    struct char_data *tch;

    if (argument[strlen(argument)-1] == '!')
      strcpy(verb, "exclaim");
    else if(argument[strlen(argument)-1] == '?')
      strcpy(verb, "ask");
    else
      strcpy(verb, "say");

    strcpy(ibuf, argument);     /* preserve original text */

    if (!IS_NPC(ch))
      garble_text(ibuf, GET_SKILL(ch, SPEAKING(ch)) ? 1 : 100, SPEAKING(ch));
    else 
      garble_text(ibuf, 1, MIN_LANGUAGES);

    for (tch = world[ch->in_room].people; tch; tch = tch->next_in_room) {
      if (tch != ch && AWAKE(tch) && tch->desc) {
        strcpy(obuf, ibuf);     /* preserve the first garble */
        if (!IS_NPC(ch) && !IS_NPC(tch) && !GET_SKILL(tch, SPEAKING(ch)))
          garble_text(obuf, GET_SKILL(tch, SPEAKING(ch)) ? 1 : 100, SPEAKING(ch));
        else
          garble_text(ibuf, 1, MIN_LANGUAGES);
        if (CAN_SEE(tch, ch))
          strcpy(cbuf, GET_NAME(ch));
        else
          strcpy(cbuf, "Someone");
        if (!IS_NPC(ch) && !GET_SKILL(tch, SPEAKING(ch)))
	 send_to_char(tch, "%s %ss, in an unfamiliar tongue, '%s@n'\r\n", CAP(cbuf), verb, obuf);
        else
	 send_to_char(tch, "%s %ss '%s@n'\r\n", CAP(cbuf), verb, obuf);
      }
    }
    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else {
      delete_doubledollar(argument);
      send_to_char(ch, "@cYou %s, '%s@c'@n\r\n", verb, argument);
    }

    /* trigger check */
    speech_mtrigger(ch, argument);
    speech_wtrigger(ch, argument);
  }
}

ACMD(do_say_original)
{
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to say?\r\n");
  else {
    char buf[MAX_INPUT_LENGTH + 12];
    char verb[10];

    if (argument[strlen(argument)-1] == '!')
      strcpy(verb, "exclaim");
    else if(argument[strlen(argument)-1] == '?')
      strcpy(verb, "ask");
    else
      strcpy(verb, "say");

    snprintf(buf, sizeof(buf), "$n %ss, '%s@n'", verb, argument);
    act(buf, FALSE, ch, 0, 0, TO_ROOM);

    if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else {
      delete_doubledollar(argument);
      send_to_char(ch, "You %ss, '%s@n'\r\n", verb, argument);
    }
  /* trigger check */
  speech_mtrigger(ch, argument);
  speech_wtrigger(ch, argument);
  }
}

ACMD(do_gsay)
{
  struct char_data *k;
  struct follow_type *f;

  skip_spaces(&argument);

  if (!AFF_FLAGGED(ch, AFF_GROUP)) {
    send_to_char(ch, "But you are not the member of a group!\r\n");
    return;
  }
  if (!*argument)
    send_to_char(ch, "Yes, but WHAT do you want to group-say?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    if (ch->master)
      k = ch->master;
    else
      k = ch;

    snprintf(buf, sizeof(buf), "$n tells the group, '%s@n'", argument);

    if (AFF_FLAGGED(k, AFF_GROUP) && (k != ch))
      act(buf, FALSE, ch, 0, k, TO_VICT | TO_SLEEP);
    for (f = k->followers; f; f = f->next)
      if (AFF_FLAGGED(f->follower, AFF_GROUP) && (f->follower != ch))
	act(buf, FALSE, ch, 0, f->follower, TO_VICT | TO_SLEEP);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "You tell the group, '%s@n'\r\n", argument);
  }
}


void perform_tell(struct char_data *ch, struct char_data *vict, char *arg)
{
  char buf[MAX_STRING_LENGTH];

  snprintf(buf, sizeof(buf), "@r$n tells you, '%s@r'@n", arg);
  act(buf, FALSE, ch, 0, vict, TO_VICT | TO_SLEEP);

  if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else {
    snprintf(buf, sizeof(buf), "@rYou tell $N, '%s@r'@n", arg);
    act(buf, FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  }

  if (!IS_NPC(vict) && !IS_NPC(ch))
    GET_LAST_TELL(vict) = GET_IDNUM(ch);
}

int is_tell_ok(struct char_data *ch, struct char_data *vict)
{
  if (ch == vict)
    send_to_char(ch, "You try to tell yourself something.\r\n");
  else if (!IS_NPC(ch) && PRF_FLAGGED(ch, PRF_NOTELL))
    send_to_char(ch, "You can't tell other people while you have notell on.\r\n");
  else if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF))
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
  else if (!IS_NPC(vict) && !vict->desc)        /* linkless */
    act("$E's linkless at the moment.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if (PLR_FLAGGED(vict, PLR_WRITING))
    act("$E's writing a message right now; try again later.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else if ((!IS_NPC(vict) && PRF_FLAGGED(vict, PRF_NOTELL)) || ROOM_FLAGGED(IN_ROOM(vict), ROOM_SOUNDPROOF))
    act("$E can't hear you.", FALSE, ch, 0, vict, TO_CHAR | TO_SLEEP);
  else
    return (TRUE);

  return (FALSE);
}

/*
 * Yes, do_tell probably could be combined with whisper and ask, but
 * called frequently, and should IMHO be kept as tight as possible.
 */
ACMD(do_tell)
{
  struct char_data *vict = NULL;
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Who do you wish to tell what??\r\n");
  else if (!(vict = get_player_vis(ch, buf, NULL, FIND_CHAR_WORLD)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (is_tell_ok(ch, vict))
    perform_tell(ch, vict, buf2);
}


ACMD(do_reply)
{
  struct char_data *tch = character_list;

  if (IS_NPC(ch))
    return;

  skip_spaces(&argument);

  if (GET_LAST_TELL(ch) == NOBODY)
    send_to_char(ch, "You have nobody to reply to!\r\n");
  else if (!*argument)
    send_to_char(ch, "What is your reply?\r\n");
  else {
    /*
     * Make sure the person you're replying to is still playing by searching
     * for them.  Note, now last tell is stored as player IDnum instead of
     * a pointer, which is much better because it's safer, plus will still
     * work if someone logs out and back in again.
     */
				     
    /*
     * XXX: A descriptor list based search would be faster although
     *      we could not find link dead people.  Not that they can
     *      hear tells anyway. :) -gg 2/24/98
     */
    while (tch != NULL && (IS_NPC(tch) || GET_IDNUM(tch) != GET_LAST_TELL(ch)))
      tch = tch->next;

    if (tch == NULL)
      send_to_char(ch, "They are no longer playing.\r\n");
    else if (is_tell_ok(ch, tch))
      perform_tell(ch, tch, argument);
  }
}


ACMD(do_spec_comm)
{
  char buf[MAX_INPUT_LENGTH], buf2[MAX_INPUT_LENGTH];
  struct char_data *vict;
  const char *action_sing, *action_plur, *action_others;

  switch (subcmd) {
  case SCMD_WHISPER:
    action_sing = "whisper to";
    action_plur = "whispers to";
    action_others = "$n whispers something to $N.";
    break;

  case SCMD_ASK:
    action_sing = "ask";
    action_plur = "asks";
    action_others = "$n asks $N a question.";
    break;

  default:
    action_sing = "oops";
    action_plur = "oopses";
    action_others = "$n is tongue-tied trying to speak with $N.";
    break;
  }

  half_chop(argument, buf, buf2);

  if (!*buf || !*buf2)
    send_to_char(ch, "Whom do you want to %s.. and what??\r\n", action_sing);
  else if (!(vict = get_char_vis(ch, buf, NULL, FIND_CHAR_ROOM)))
    send_to_char(ch, "%s", CONFIG_NOPERSON);
  else if (vict == ch)
    send_to_char(ch, "You can't get your mouth close enough to your ear...\r\n");
  else {
    char buf1[MAX_STRING_LENGTH];

    snprintf(buf1, sizeof(buf1), "$n %s you, '%s'", action_plur, buf2);
    act(buf1, FALSE, ch, 0, vict, TO_VICT);

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else
      send_to_char(ch, "You %s %s, '%s'\r\n", action_sing, GET_NAME(vict), buf2);
    act(action_others, FALSE, ch, 0, vict, TO_NOTVICT);
  }
}


/*
 * buf1, buf2 = MAX_OBJECT_NAME_LENGTH
 *	(if it existed)
 */
ACMD(do_write)
{
  extern struct index_data *obj_index;
  struct obj_data *paper, *pen = NULL, *obj;
  char *papername, *penname;
  char buf1[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  /* before we do anything, lets see if there's a board involved. */
  for (obj = ch->carrying; obj;obj=obj->next_content) {
    if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
      break;
    }
  }
  
  if(!obj) {
    for (obj = world[ch->in_room].contents; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	break;
      }
    }
  }
  
  if(obj) {                /* then there IS a board! */
    write_board_message(GET_OBJ_VNUM(obj),ch,argument);
    act ("$n begins to write a note on $p.", TRUE, ch, obj, 0, TO_ROOM);
    return;
  }
  
  papername = buf1;
  penname = buf2;

  two_arguments(argument, papername, penname);

  if (!ch->desc)
    return;

  if (!*papername) {		/* nothing was delivered */
    send_to_char(ch, "Write?  With what?  ON what?  What are you trying to do?!?\r\n");
    return;
  }
  if (*penname) {		/* there were two arguments */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", papername);
      return;
    }
    if (!(pen = get_obj_in_list_vis(ch, penname, NULL, ch->carrying))) {
      send_to_char(ch, "You have no %s.\r\n", penname);
      return;
    }
  } else {		/* there was one arg.. let's see what we can find */
    if (!(paper = get_obj_in_list_vis(ch, papername, NULL, ch->carrying))) {
      send_to_char(ch, "There is no %s in your inventory.\r\n", papername);
      return;
    }
    if (GET_OBJ_TYPE(paper) == ITEM_PEN) {	/* oops, a pen.. */
      pen = paper;
      paper = NULL;
    } else if (GET_OBJ_TYPE(paper) != ITEM_NOTE) {
      send_to_char(ch, "That thing has nothing to do with writing.\r\n");
      return;
    }
    /* One object was found.. now for the other one. */
    if (!GET_EQ(ch, WEAR_WIELD2)) {
      send_to_char(ch, "You can't write with %s %s alone.\r\n", AN(papername), papername);
      return;
    }
    if (!CAN_SEE_OBJ(ch, GET_EQ(ch, WEAR_WIELD2))) {
      send_to_char(ch, "The stuff in your hand is invisible!  Yeech!!\r\n");
      return;
    }
    if (pen)
      paper = GET_EQ(ch, WEAR_WIELD2);
    else
      pen = GET_EQ(ch, WEAR_WIELD2);
  }


  /* ok.. now let's see what kind of stuff we've found */
  if (GET_OBJ_TYPE(pen) != ITEM_PEN)
    act("$p is no good for writing with.", FALSE, ch, pen, 0, TO_CHAR);
  else if (GET_OBJ_TYPE(paper) != ITEM_NOTE)
    act("You can't write on $p.", FALSE, ch, paper, 0, TO_CHAR);
  else {
    char *backstr = NULL;
 
    /* Something on it, display it as that's in input buffer. */
    if (paper->action_description) {
      backstr = strdup(paper->action_description);
      send_to_char(ch, "There's something written on it already:\r\n");
      send_to_char(ch, "%s", paper->action_description);
    }
 
    /* we can write - hooray! */
    act("$n begins to jot down a note.", TRUE, ch, 0, 0, TO_ROOM);
    SET_BIT_AR(GET_OBJ_EXTRA(paper),ITEM_UNIQUE_SAVE);
    send_editor_help(ch->desc);
    string_write(ch->desc, &paper->action_description, MAX_NOTE_LENGTH, 0, backstr);
  }
}



ACMD(do_page)
{
  struct descriptor_data *d;
  struct char_data *vict;
  char buf2[MAX_INPUT_LENGTH], arg[MAX_INPUT_LENGTH];

  half_chop(argument, arg, buf2);

  if (IS_NPC(ch))
    send_to_char(ch, "Monsters can't page.. go away.\r\n");
  else if (!*arg)
    send_to_char(ch, "Whom do you wish to page?\r\n");
  else {
    char buf[MAX_STRING_LENGTH];

    snprintf(buf, sizeof(buf), "\007\007*$n* %s", buf2);
    if (!str_cmp(arg, "all")) {
      if (ADM_FLAGGED(ch, ADM_TELLALL)) {
	for (d = descriptor_list; d; d = d->next)
	  if (STATE(d) == CON_PLAYING && d->character)
	    act(buf, FALSE, ch, 0, d->character, TO_VICT);
      } else
	send_to_char(ch, "You will never be godly enough to do that!\r\n");
      return;
    }
    if ((vict = get_char_vis(ch, arg, NULL, FIND_CHAR_WORLD)) != NULL) {
      act(buf, FALSE, ch, 0, vict, TO_VICT);
      if (PRF_FLAGGED(ch, PRF_NOREPEAT))
	send_to_char(ch, "%s", CONFIG_OK);
      else
	act(buf, FALSE, ch, 0, vict, TO_CHAR);
    } else
      send_to_char(ch, "There is no such person in the game!\r\n");
  }
}


/**********************************************************************
 * generalized communication func, originally by Fred C. Merkel (Torg) *
  *********************************************************************/

ACMD(do_gen_comm)
{
  struct descriptor_data *i;
  char color_on[24];
  char buf1[MAX_INPUT_LENGTH];

  /* Array of flags which must _not_ be set in order for comm to be heard */
  int channels[] = {
    0,
    PRF_DEAF,
    PRF_NOGOSS,
    PRF_NOAUCT,
    PRF_NOGRATZ,
    0
  };

  /*
   * com_msgs: [0] Message if you can't perform the action because of noshout
   *           [1] name of the action
   *           [2] message if you're not on the channel
   *           [3] a color string.
   */
  const char *com_msgs[][4] = {
    {"You cannot holler!!\r\n",
      "holler",
      "",
      "@[8]"},

    {"You cannot shout!!\r\n",
      "shout",
      "Turn off your noshout flag first!\r\n",
      "@[9]"},

    {"You cannot gossip!!\r\n",
      "gossip",
      "You aren't even on the channel!\r\n",
      "@[10]"},

    {"You cannot auction!!\r\n",
      "auction",
      "You aren't even on the channel!\r\n",
      "@[11]"},

    {"You cannot congratulate!\r\n",
      "congrat",
      "You aren't even on the channel!\r\n",
      "@[12]"}
  };

  /* to keep pets, etc from being ordered to shout */
  if (!ch->desc)
    return;

  if (PLR_FLAGGED(ch, PLR_NOSHOUT)) {
    send_to_char(ch, "%s", com_msgs[subcmd][0]);
    return;
  }
  if (ROOM_FLAGGED(IN_ROOM(ch), ROOM_SOUNDPROOF)) {
    send_to_char(ch, "The walls seem to absorb your words.\r\n");
    return;
  }

  if (subcmd == SCMD_GOSSIP && *argument == '*') { 
    subcmd = SCMD_GEMOTE; 
  } 

  if (subcmd == SCMD_GEMOTE) { 
    ACMD(do_gmote); 
    if (*argument == '*') 
      do_gmote(ch, argument + 1, 0, 1); 
    else 
      do_gmote(ch, argument, 0, 1); 

    return; 
  } 

  /* level_can_shout defined in config.c */
  if (GET_LEVEL(ch) < CONFIG_LEVEL_CAN_SHOUT) {
    send_to_char(ch, "You must be at least level %d before you can %s.\r\n", CONFIG_LEVEL_CAN_SHOUT, com_msgs[subcmd][1]);
    return;
  }
  /* make sure the char is on the channel */
  if (PRF_FLAGGED(ch, channels[subcmd])) {
    send_to_char(ch, "%s", com_msgs[subcmd][2]);
    return;
  }
  /* skip leading spaces */
  skip_spaces(&argument);

  /* make sure that there is something there to say! */
  if (!*argument) {
    send_to_char(ch, "Yes, %s, fine, %s we must, but WHAT???\r\n", com_msgs[subcmd][1], com_msgs[subcmd][1]);
    return;
  }
  if (subcmd == SCMD_HOLLER) {
    if (GET_MOVE(ch) < CONFIG_HOLLER_MOVE_COST) {
      send_to_char(ch, "You're too exhausted to holler.\r\n");
      return;
    } else
      GET_MOVE(ch) -= CONFIG_HOLLER_MOVE_COST;
  }
  /* set up the color on code */
  strlcpy(color_on, com_msgs[subcmd][3], sizeof(color_on));

  /* first, set up strings to be given to the communicator */
  if (PRF_FLAGGED(ch, PRF_NOREPEAT))
    send_to_char(ch, "%s", CONFIG_OK);
  else
    send_to_char(ch, "%sYou %s, '%s%s'@n\r\n", color_on, com_msgs[subcmd][1], argument, color_on);

  snprintf(buf1, sizeof(buf1), "%s$n %ss, '%s%s'@n", color_on, com_msgs[subcmd][1], argument, color_on);

  /* now send all the strings out */
  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) == CON_PLAYING && i != ch->desc && i->character &&
	!PRF_FLAGGED(i->character, channels[subcmd]) &&
	!PLR_FLAGGED(i->character, PLR_WRITING) &&
	!ROOM_FLAGGED(IN_ROOM(i->character), ROOM_SOUNDPROOF)) {

      if (subcmd == SCMD_SHOUT &&
	  ((world[IN_ROOM(ch)].zone != world[IN_ROOM(i->character)].zone) ||
	   !AWAKE(i->character)))
	continue;

      act(buf1, FALSE, ch, 0, i->character, TO_VICT | TO_SLEEP);
    }
  }
}


ACMD(do_qcomm)
{
  if (!PRF_FLAGGED(ch, PRF_QUEST)) {
    send_to_char(ch, "You aren't even part of the quest!\r\n");
    return;
  }
  skip_spaces(&argument);

  if (!*argument)
    send_to_char(ch, "%c%s?  Yes, fine, %s we must, but WHAT??\r\n", UPPER(*CMD_NAME), CMD_NAME + 1, CMD_NAME);
  else {
    char buf[MAX_STRING_LENGTH];
    struct descriptor_data *i;

    if (PRF_FLAGGED(ch, PRF_NOREPEAT))
      send_to_char(ch, "%s", CONFIG_OK);
    else if (subcmd == SCMD_QSAY) {
      snprintf(buf, sizeof(buf), "You quest-say, '%s'", argument);
      act(buf, FALSE, ch, 0, argument, TO_CHAR);
    } else
      act(argument, FALSE, ch, 0, argument, TO_CHAR);

    if (subcmd == SCMD_QSAY)
      snprintf(buf, sizeof(buf), "$n quest-says, '%s'", argument);
    else
      strlcpy(buf, argument, sizeof(buf));

    for (i = descriptor_list; i; i = i->next)
      if (STATE(i) == CON_PLAYING && i != ch->desc && PRF_FLAGGED(i->character, PRF_QUEST))
	act(buf, 0, ch, 0, i->character, TO_VICT | TO_SLEEP);
  }
}

ACMD(do_respond) {
  int found=0,mnum=0;
  struct obj_data *obj;
  char number[MAX_STRING_LENGTH];
  
  if(IS_NPC(ch)) {
    send_to_char(ch,"As a mob, you never bothered to learn to read or write.\r\n");
    return;
  }
  
  for (obj = ch->carrying; obj;obj=obj->next_content) {
    if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
      found=1;
      break;
    }
  }
  if(!obj) {
    for (obj = world[ch->in_room].contents; obj;obj=obj->next_content) {
      if(GET_OBJ_TYPE(obj) == ITEM_BOARD) {
	found=1;
	break;
      }
    }
  }
  if (obj) {
    argument = one_argument(argument, number);
    if (!*number) {
      send_to_char(ch,"Respond to what?\r\n");
      return;
    }
    if (!isdigit(*number) || (!(mnum = atoi(number)))) {
      send_to_char(ch,"You must type the number of the message you wish to reply to.\r\n");
      return;
    }
    board_respond(GET_OBJ_VNUM(obj), ch, mnum);
  }
  
  /* No board in the room? Send generic message -spl */
  if (found == 0) {
    send_to_char(ch,"Sorry, you may only reply to messages posted on a board.\r\n");
  }
}
