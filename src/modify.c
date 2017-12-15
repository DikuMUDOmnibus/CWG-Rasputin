/* ************************************************************************
*   File: modify.c                                      Part of CircleMUD *
*  Usage: Run-time modification of game variables                         *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/modify.c,v 1.4 2004/12/21 23:44:47 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "interpreter.h"
#include "handler.h"
#include "db.h"
#include "comm.h"
#include "spells.h"
#include "mail.h"
#include "boards.h"
#include "improved-edit.h"
#include "oasis.h"
#include "tedit.h"
#include "shop.h"
#include "guild.h"

void show_string(struct descriptor_data *d, char *input);

extern struct spell_info_type spell_info[];
extern const char *unused_spellname;	/* spell_parser.c */

/* local functions */
void smash_tilde(char *str);
ACMD(do_skillset);
char *next_page(char *str, struct char_data *ch);
int count_pages(char *str, struct char_data *ch);
void paginate_string(char *str, struct descriptor_data *d);
void playing_string_cleanup(struct descriptor_data *d, int action);
void exdesc_string_cleanup(struct descriptor_data *d, int action);
void trigedit_string_cleanup(struct descriptor_data *d, int terminator);

const char *string_fields[] =
{
  "name",
  "short",
  "long",
  "description",
  "title",
  "delete-description",
  "\n"
};


/* maximum length for text field x+1 */
int length[] =
{
  15,
  60,
  256,
  240,
  60
};


/* ************************************************************************
*  modification of malloc'ed strings                                      *
************************************************************************ */

/*
 * Put '#if 1' here to erase ~, or roll your own method.  A common idea
 * is smash/show tilde to convert the tilde to another innocuous character
 * to save and then back to display it. Whatever you do, at least keep the
 * function around because other MUD packages use it, like mudFTP.
 *   -gg 9/9/98
 */
void smash_tilde(char *str)
{
  /*
   * Erase any _line ending_ tildes inserted in the editor.
   * The load mechanism can't handle those, yet.
   * -- Welcor 04/2003
   */

   char *p = str;
   for (; *p; p++)
     if (*p == '~' && (*(p+1)=='\r' || *(p+1)=='\n' || *(p+1)=='\0'))
       *p=' ';
#if 1
  /*
   * Erase any ~'s inserted by people in the editor.  This prevents anyone
   * using online creation from causing parse errors in the world files.
   * Derived from an idea by Sammy <samedi@dhc.net> (who happens to like
   * his tildes thank you very much.), -gg 2/20/98
   */
    while ((str = strchr(str, '~')) != NULL)
      *str = ' ';
#endif
}

/*
 * Basic API function to start writing somewhere.
 *
 * 'data' isn't used in stock CircleMUD but you can use it to pass whatever
 * else you may want through it.  The improved editor patch when updated
 * could use it to pass the old text buffer, for instance.
 */
void string_write(struct descriptor_data *d, char **writeto, size_t len, long mailto, void *data)
{
  if (d->character && !IS_NPC(d->character))
    SET_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);

  if (using_improved_editor)
    d->backstr = (char *)data;
  else if (data)
    free(data); 

  d->str = writeto;
  d->max_str = len;
  d->mail_to = mailto;
}

/*
 * Add user input to the 'current' string (as defined by d->str).
 * This is still overly complex.
 */
void string_add(struct descriptor_data *d, char *str)
{
  int action;

  delete_doubledollar(str);
  smash_tilde(str);

  /* determine if this is the terminal string, and truncate if so */
  /* changed to only accept '@' at the beginning of line - J. Elson 1/17/94 */
  /* changed to only accept '@' if it's by itself - fnord 10/15/2004 */
  if ((action = (*str == '@' && !str[1])))
    *str = '\0';
  else
    if ((action = improved_editor_execute(d, str)) == STRINGADD_ACTION)
      return;

  if (action != STRINGADD_OK)
    /* Do nothing. */ ;
  else if (!(*d->str)) {
    if (strlen(str) + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(d->character, "String too long - Truncated.\r\n");
      strcpy(str + (d->max_str - 3), "\r\n");
      CREATE(*d->str, char, d->max_str);
      strcpy(*d->str, str);	/* strcpy: OK (size checked) */
      if (!using_improved_editor)
        action = STRINGADD_SAVE; 
    } else {
      CREATE(*d->str, char, strlen(str) + 3);
      strcpy(*d->str, str);	/* strcpy: OK (size checked) */
    }
  } else {
    if (strlen(str) + strlen(*d->str) + 3 > d->max_str) { /* \r\n\0 */
      send_to_char(d->character, "String too long.  Last line skipped.\r\n");
      if (!using_improved_editor)
        action = STRINGADD_SAVE; 
      else if (action == STRINGADD_OK)
        action = STRINGADD_ACTION;    /* No appending \r\n\0, but still let them save. */
    } else {
      RECREATE(*d->str, char, strlen(*d->str) + strlen(str) + 3); /* \r\n\0 */
      strcat(*d->str, str);	/* strcat: OK (size precalculated) */
    }
  }

  /*
   * Common cleanup code.
   */
  switch (action) {
    case STRINGADD_ABORT:
      switch (STATE(d)) {
        case CON_CEDIT:
        case CON_TEDIT:
        case CON_REDIT:
        case CON_MEDIT:
        case CON_OEDIT:
        case CON_IEDIT:
        case CON_EXDESC:
        case CON_TRIGEDIT:
          free(*d->str);
          *d->str = d->backstr;
          d->backstr = NULL;
          d->str = NULL;
          break;
      case CON_PLAYING:
	/* all CON_PLAYING are handled below in playing_string_cleanup */
	break;
		     
        default:
          log("SYSERR: string_add: Aborting write from unknown origin.");
          break;
      }
      break;
    case STRINGADD_SAVE:
      if (d->str && *d->str && **d->str == '\0') {
        free(*d->str);
        *d->str = strdup("Nothing.\r\n");
      }
      if (d->backstr)
        free(d->backstr);
      d->backstr = NULL;
      break;
    case STRINGADD_ACTION:
      break;
  }

  /* Ok, now final cleanup. */

  if (action == STRINGADD_SAVE || action == STRINGADD_ABORT) {
    int i;
    struct {
      int mode;
      void (*func)(struct descriptor_data *dsc, int todo);
    } cleanup_modes[] = {
      { CON_CEDIT  , cedit_string_cleanup },
      { CON_MEDIT  , medit_string_cleanup },
      { CON_OEDIT  , oedit_string_cleanup },
      { CON_REDIT  , redit_string_cleanup },
      { CON_TEDIT  , tedit_string_cleanup },
      { CON_TRIGEDIT, trigedit_string_cleanup },
      { CON_EXDESC , exdesc_string_cleanup },
      { CON_PLAYING, playing_string_cleanup },
      { -1, NULL }
    };

    for (i = 0; cleanup_modes[i].func; i++)
      if (STATE(d) == cleanup_modes[i].mode)
        (*cleanup_modes[i].func)(d, action);

    /* Common post cleanup code. */
    d->str = NULL;
      d->mail_to = 0;
    d->max_str = 0;
    if (d->character && !IS_NPC(d->character)) {
      REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_MAILING);
	  REMOVE_BIT_AR(PLR_FLAGS(d->character), PLR_WRITING);
	}
  } else if (action != STRINGADD_ACTION && strlen(*d->str) + 3 <= d->max_str) /* 3 = \r\n\0 */
     strcat(*d->str, "\r\n");
}

void playing_string_cleanup(struct descriptor_data *d, int action)
{
  struct board_info *board;
  struct board_msg *fore,*cur,*aft;
  
  if (PLR_FLAGGED(d->character, PLR_MAILING)) {
    if (action == STRINGADD_SAVE && *d->str) {
      store_mail(d->mail_to, GET_IDNUM(d->character), *d->str);
      write_to_output(d, "Message sent!\r\n");
    } else {
      write_to_output(d, "Mail aborted.\r\n");
      free(*d->str);
      free(d->str);
    }
  }

  if(PLR_FLAGGED(d->character,PLR_WRITING)) {
    if (d->mail_to >= BOARD_MAGIC) {
      if (action == STRINGADD_ABORT) {
	/* find the message */
	board = locate_board(d->mail_to - BOARD_MAGIC);
	fore=cur=aft=NULL;
	for(cur = BOARD_MESSAGES(board);cur;cur = aft) {
	  aft=MESG_NEXT(cur);
	  if(cur->data == *d->str) {
	    if(BOARD_MESSAGES(board) == cur) {
	      if(MESG_NEXT(cur) != NULL) {
		BOARD_MESSAGES(board) = MESG_NEXT(cur);
	      } else {
		BOARD_MESSAGES(board) = NULL;
	      }
	    }
	    if(fore) {
	      MESG_NEXT(fore) = aft;
	    }
	    if(aft) {
	      MESG_PREV(aft) = fore;
	    }
	    free(cur->subject);
	    free(cur->data);
	    free(cur);
	    BOARD_MNUM(board)--;
	    write_to_output(d,"Post aborted.\r\n");
	    return;
	  }
	  fore=cur;
	}
	write_to_output(d,"Unable to find your message to delete it!\r\n");
      } else {
	write_to_output(d,"\r\nPost saved.\r\n");
	save_board(locate_board(d->mail_to - BOARD_MAGIC));
      }
    }
    
    /* hm... I wonder what happens when you can't finish writing a note */
    }
}

void exdesc_string_cleanup(struct descriptor_data *d, int action)
{
  if (action == STRINGADD_ABORT)
    write_to_output(d, "Description aborted.\r\n");
  
  write_to_output(d, CONFIG_MENU);
  STATE(d) = CON_MENU;
}


/* *********************************************************************
   *  Modification of character skills                                 *
   ********************************************************************* */

ACMD(do_skillset)
{
  struct char_data *vict;
  char name[MAX_INPUT_LENGTH];
  char buf[MAX_INPUT_LENGTH], help[MAX_STRING_LENGTH];
  int skill, value, i=0, qend;

  argument = one_argument(argument, name);

  if (!*name) {			/* no arguments. print an informative text */
    send_to_char(ch, "Syntax: skillset <name> '<skill>' <value>\r\n"
		"Skill being one of the following:\r\n");
    for (qend = 0, i = 0; i < SKILL_TABLE_SIZE; i++) {
      if (spell_info[i].name == unused_spellname)	/* This is valid. */
	continue;
      send_to_char(ch, "%18s", spell_info[i].name);
      if (qend++ % 4 == 3)
	send_to_char(ch, "\r\n");
    }
    if (qend % 4 != 0)
      send_to_char(ch, "\r\n");
    return;
  }

  if (!(vict = get_char_vis(ch, name, NULL, FIND_CHAR_WORLD))) {
    send_to_char(ch, "%s", CONFIG_NOPERSON);
    return;
  }
  skip_spaces(&argument);

  /* If there is no chars in argument */
  if (!*argument) {
    i = snprintf(help, sizeof(help) - i, "\r\nSkills:\r\n");
    i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SKILL);
    i += snprintf(help + i, sizeof(help) - i, "\r\nSpells:\r\n");
    i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SPELL);
    if (CONFIG_ENABLE_LANGUAGES) {
      i += snprintf(help + i, sizeof(help) - i, "\r\nLanguages:\r\n");
      i += print_skills_by_type(vict, help + i, sizeof(help) - i, SKTYPE_SKILL | SKTYPE_LANG);
    }
    if (i >= sizeof(help))
      strcpy(help + sizeof(help) - strlen("** OVERFLOW **") - 1, "** OVERFLOW **"); /* strcpy: OK */
    page_string(ch->desc, help, TRUE);
    return;
  }

  if (*argument != '\'') {
    send_to_char(ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  /* Locate the last quote and lowercase the magic words (if any) */

  for (qend = 1; argument[qend] && argument[qend] != '\''; qend++)
    argument[qend] = LOWER(argument[qend]);

  if (argument[qend] != '\'') {
    send_to_char(ch, "Skill must be enclosed in: ''\r\n");
    return;
  }
  strcpy(help, (argument + 1));	/* strcpy: OK (MAX_INPUT_LENGTH <= MAX_STRING_LENGTH) */
  help[qend - 1] = '\0';
  if ((skill = find_skill_num(help)) <= 0) {
    send_to_char(ch, "Unrecognized skill.\r\n");
    return;
  }
  argument += qend + 1;		/* skip to next parameter */
  argument = one_argument(argument, buf);

  if (!*buf) {
    send_to_char(ch, "Learned value expected.\r\n");
    return;
  }
  value = atoi(buf);
  if (value < 0) {
    send_to_char(ch, "Minimum value for learned is 0.\r\n");
    return;
  }

  /*
   * find_skill_num() guarantees a valid spell_info[] index, or -1, and we
   * checked for the -1 above so we are safe here.
   */
  SET_SKILL(vict, skill, value);
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "skillset: %s changed %s's '%s' to %d.", GET_NAME(ch), GET_NAME(vict), spell_info[skill].name, value);
  send_to_char(ch, "You change %s's %s to %d.\r\n", GET_NAME(vict), spell_info[skill].name, value);
}



/*********************************************************************
* New Pagination Code
* Michael Buselli submitted the following code for an enhanced pager
* for CircleMUD.  All functions below are his.  --JE 8 Mar 96
*
*********************************************************************/

/* Traverse down the string until the begining of the next page has been
 * reached.  Return NULL if this is the last page of the string.
 */
char *next_page(char *str, struct char_data *ch)
{
  int col = 1, line = 1, spec_code = FALSE;

  for (;; str++) {
    /* If end of string, return NULL. */
    if (*str == '\0')
      return (NULL);

    /* If we're at the start of the next page, return this fact. */
    else if (line > (GET_PAGE_LENGTH(ch) - (PRF_FLAGGED(ch, PRF_COMPACT) ? 1 : 2)))
      return (str);

    /* Check for the begining of an ANSI color code block. */
    else if (*str == '\x1B' && !spec_code)
      spec_code = TRUE;

    /* Check for the end of an ANSI color code block. */
    else if (*str == 'm' && spec_code)
      spec_code = FALSE;

    /* Check for everything else. */
    else if (!spec_code) {
      /* Carriage return puts us in column one. */
      if (*str == '\r')
	col = 1;
      /* Newline puts us on the next line. */
      else if (*str == '\n')
	line++;

      /* We need to check here and see if we are over the page width,
       * and if so, compensate by going to the begining of the next line.
       */
      else if (col++ > PAGE_WIDTH) {
	col = 1;
	line++;
      }
    }
  }
}


/* Function that returns the number of pages in the string. */
int count_pages(char *str, struct char_data *ch)
{
  int pages;

  for (pages = 1; (str = next_page(str, ch)); pages++);
  return (pages);
}


/* This function assigns all the pointers for showstr_vector for the
 * page_string function, after showstr_vector has been allocated and
 * showstr_count set.
 */
void paginate_string(char *str, struct descriptor_data *d)
{
  int i;

  if (d->showstr_count)
    *(d->showstr_vector) = str;

  for (i = 1; i < d->showstr_count && str; i++)
    str = d->showstr_vector[i] = next_page(str, d->character);

  d->showstr_page = 0;
}


/* The call that gets the paging ball rolling... */
void page_string(struct descriptor_data *d, char *str, int keep_internal)
{
  char actbuf[MAX_INPUT_LENGTH] = "";

  if (!d)
    return;

  if (!str || !*str)
    return;

  if ((GET_PAGE_LENGTH(d->character) < 5 || GET_PAGE_LENGTH(d->character) > 254))
    GET_PAGE_LENGTH(d->character) = PAGE_LENGTH;
  d->showstr_count = count_pages(str, d->character);
  CREATE(d->showstr_vector, char *, d->showstr_count);

  if (keep_internal) {
    d->showstr_head = strdup(str);
    paginate_string(d->showstr_head, d);
  } else
    paginate_string(str, d);

  show_string(d, actbuf);
}


/* The call that displays the next page. */
void show_string(struct descriptor_data *d, char *input)
{
  char buffer[MAX_STRING_LENGTH], buf[MAX_INPUT_LENGTH];
  int diff;

  any_one_arg(input, buf);

  /* Q is for quit. :) */
  if (LOWER(*buf) == 'q') {
    free(d->showstr_vector);
    d->showstr_vector = NULL;
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
    return;
  }
  /* R is for refresh, so back up one page internally so we can display
   * it again.
   */
  else if (LOWER(*buf) == 'r')
    d->showstr_page = MAX(0, d->showstr_page - 1);

  /* B is for back, so back up two pages internally so we can display the
   * correct page here.
   */
  else if (LOWER(*buf) == 'b')
    d->showstr_page = MAX(0, d->showstr_page - 2);

  /* Feature to 'goto' a page.  Just type the number of the page and you
   * are there!
   */
  else if (isdigit(*buf))
    d->showstr_page = MAX(0, MIN(atoi(buf) - 1, d->showstr_count - 1));

  else if (*buf) {
    send_to_char(d->character, "Valid commands while paging are RETURN, Q, R, B, or a numeric value.\r\n");
    return;
  }
  /* If we're displaying the last page, just send it to the character, and
   * then free up the space we used.
   */
  if (d->showstr_page + 1 >= d->showstr_count) {
    send_to_char(d->character, "%s", d->showstr_vector[d->showstr_page]);
    free(d->showstr_vector);
    d->showstr_vector = NULL;
    d->showstr_count = 0;
    if (d->showstr_head) {
      free(d->showstr_head);
      d->showstr_head = NULL;
    }
  }
  /* Or if we have more to show.... */
  else {
    diff = d->showstr_vector[d->showstr_page + 1] - d->showstr_vector[d->showstr_page];
    if (diff > MAX_STRING_LENGTH - 3) /* 3=\r\n\0 */
      diff = MAX_STRING_LENGTH - 3;
    strncpy(buffer, d->showstr_vector[d->showstr_page], diff);	/* strncpy: OK (size truncated above) */
    /*
     * Fix for prompt overwriting last line in compact mode submitted by
     * Peter Ajamian <peter@pajamian.dhs.org> on 04/21/2001
     */
    if (buffer[diff - 2] == '\r' && buffer[diff - 1]=='\n')
      buffer[diff] = '\0';
    else if (buffer[diff - 2] == '\n' && buffer[diff - 1] == '\r')
      /* This is backwards.  Fix it. */
      strcpy(buffer + diff - 2, "\r\n");	/* strcpy: OK (size checked) */
    else if (buffer[diff - 1] == '\r' || buffer[diff - 1] == '\n')
      /* Just one of \r\n.  Overwrite it. */
      strcpy(buffer + diff - 1, "\r\n");	/* strcpy: OK (size checked) */
    else
      /* Tack \r\n onto the end to fix bug with prompt overwriting last line. */
      strcpy(buffer + diff, "\r\n");	/* strcpy: OK (size checked) */
    send_to_char(d->character, "%s", buffer);
    d->showstr_page++;
  }
}
