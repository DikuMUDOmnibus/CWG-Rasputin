/*********************************
*     Memorization of spells     *
*********************************/

/*****************************************************************************+

A linked list takes care of the spells being memorized, then when it is
finished, it is saved using an array ch->player_specials->saved.spellmem[i].

Storage in playerfile:
ch->player_specials->saved.spellmem[i]

macros in utils.h:

GET_SPELLMEM(ch, i) returns the spell which is memorized in that slot.

defined in structs.h:

MAX_MEM is the total number of spell slots which can be held by a player.

void update_mem(void) is called in comm.c every tick to update the time
until a spell is memorized.

In spell_parser.c at the end of ACMD(do_cast), spell slots are checked when
a spell is cast, and the appropriate spell is removed from memory.

The number of available spell slots is determined by level and wisdom,

Expansion options:
Bard - compose
Priest - pray 
    
******************************************************************************/

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/memorization.c,v 1.14 2005/01/02 21:18:09 zizazat Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "interpreter.h"
#include "spells.h"
#include "handler.h"
#include "feats.h"

/* external functions */
int has_spellbook(struct char_data *ch, int spellnum);
int spell_in_book(struct obj_data *obj, int spellnum);
int spell_in_scroll(struct obj_data *obj, int spellnum);
int spell_in_domain(struct char_data *ch, int spellnum);

#define SINFO spell_info[spellnum]
void update_mem(struct char_data *ch, bool mem_all);
int findslotnum(struct char_data *ch, int spelllvl);
int find_freeslot(struct char_data *ch, int spelllvl);
int find_memspeed(struct char_data *ch, bool display);

extern struct descriptor_data *descriptor_list;
extern struct char_data *is_playing(char *vict_name);
extern struct char_data *character_list;

void memorize_remove(struct char_data * ch, struct memorize_node * mem);
void memorize_add(struct char_data * ch, int spellnum, int timer);
void displayslotnum(struct char_data *ch, int class);

void do_mem_display(struct char_data *ch)
{
  int speedfx, count, i, sortpos;
  struct memorize_node *mem, *next;
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  
  strcpy(buf, "@cSpells in memory:\r\n@n");
  //sprintf(buf + strlen(buf), "\r\n");
  strcpy(buf2, buf);

  /* List the memorized spells */
  for(i = 0; i < 10; i++)
  {    
    if((i >= 1) && (findslotnum(ch, i) < 1))
      break;
    sprintf(buf, "@c---@wLevel @R%d @wSpells@c---=============================@c---@w[@R%d @yslots@w]@c---@n\r\n", i, findslotnum(ch, i));
    count = 0;
    for(sortpos = 0; sortpos < GET_MEMCURSOR(ch); sortpos++)
    {
      if (strlen(buf2) >= MAX_STRING_LENGTH - 32) 
      {
	    strcat(buf2, "**OVERFLOW**\r\n");
	    break;
	  }
	  if ((GET_SPELLMEM(ch, sortpos) != 0) && (spell_info[GET_SPELLMEM(ch, sortpos)].spell_level == i))
	  {
	    count++;
	    sprintf(buf+strlen(buf), "@y%-22.22s@n", 
            spell_info[GET_SPELLMEM(ch,sortpos)].name);         
		if(count%3 == 0)
		  strcat(buf, "\r\n");
	  }
    }
    if(count%3 != 0)
      strcat(buf, "\r\n");
    sprintf(buf+strlen(buf), "@w(@r%d@y/@r%d@w)@n\r\n", count, findslotnum(ch, i));
    strcat(buf2, buf);
  }		

  /* here, list of spells being memorized and time till */
  /* memorization complete                              */ 
  speedfx = find_memspeed(ch, TRUE);

  sprintf(buf, "@c----------------------------------------------------------------@n\r\n");
  sprintf(buf + strlen(buf), "@cSpells being memorized:@n\r\n");
  count = 0;

  for (mem = ch->memorized; mem; mem = next) {
    next = mem->next;
    if ((mem->timer % speedfx) == 0) 
	  {
	    sprintf(buf+strlen(buf), "@y%-20.20s@w(@r%2dhr@w)@n ", spell_info[mem->spell].name, 
            (mem->timer/speedfx));
      }
	  else
      {
	    sprintf(buf+strlen(buf), "@y%-20.20s@w(@r%2dhr@w)@n ", spell_info[mem->spell].name, 
            ((mem->timer/speedfx)+1));
	  }
      count++;

	  if (count%2 == 0) 
        strcat(buf, "\r\n");
  }
  if(count%2 != 0)
    strcat(buf, "\r\n");
  if(count == 0)
    strcat(buf, "@w(@rNone@w)@n\r\n");
  strcat(buf2, buf);
  page_string(ch->desc, buf2, 1);
}


/*****************************************************************************+

   Memorization time for each spell is equal to the level of the spell + 2
   multiplied by 5 at POS_STANDING.  A level one spell would take 
   (1+2)*5=15 hours(ticks) to memorize when standing.

   POS_SITTING and POS_RESTING simulate the reduction in time by multiplying
   the number subtracted from the MEMTIME each tick by 5.  
   
   A character who has 15 hours(ticks) to memorize a spell standing will see
   this on his display.  When he is sitting, he will have 15 hours in MEMTIME,
   but the display will divide by the value returned in find_memspeed to show
   a value of 15/5 --> 3 hours sitting time.  

   If a tick occurs while sitting, update_mem will subtract 5 hours of 
   "standing time" which is one hour of "sitting time" from the timer.
   
******************************************************************************/
int find_memspeed(struct char_data *ch, bool display)
{
  int speedfx = 0; 

  if (GET_POS(ch) < POS_RESTING || GET_POS(ch) == POS_FIGHTING) {
    if(display)
      return 1;
    return speedfx;
  } else {
    if ((GET_POS(ch) == POS_RESTING) || (GET_POS(ch) == POS_SITTING))
      speedfx = 5;
    if (GET_POS(ch) == POS_STANDING)
      speedfx = 5;
    if (GET_COND(ch, DRUNK) > 10)
      speedfx = speedfx - 1;
    if (GET_COND(ch, FULL) == 0)
      speedfx = speedfx - 1;
    if (GET_COND(ch, THIRST) == 0)
      speedfx = speedfx - 1;
    speedfx = MAX(speedfx, 0);
    if(display)
      speedfx = MAX(speedfx, 1);
    return speedfx;
  }
}

/********************************************/
/* called during a tick to count down till  */ 
/* memorized in comm.c.                     */
/********************************************/

void update_mem(struct char_data *ch, bool mem_all)
{
  struct memorize_node *mem, *next_mem;
  struct descriptor_data *d;
  struct char_data *i;
  int speedfx = 0;

  for (d = descriptor_list; d; d = d->next) {
    if(ch)
      i = ch;
    else if(d->original)
      i = d->original;
    else if(!(i = d->character))
      continue;
    speedfx = find_memspeed(i, FALSE);
    for (mem = i->memorized; mem; mem = next_mem) {
      next_mem = mem->next;
      if (speedfx < mem->timer && !mem_all) {
        mem->timer -= speedfx;
      } else {
        send_to_char(i, "You have finished memorizing %s.\r\n", 
          spell_info[mem->spell].name);

        GET_SPELLMEM(i, GET_MEMCURSOR(i)) = mem->spell;
        GET_MEMCURSOR(i)++;

        memorize_remove(i, mem);
      }
    }
    if(ch)
      return;
  }
}

/* remove a spell from a character's memorize(in progress) linked list */
void memorize_remove(struct char_data * ch, struct memorize_node * mem)
{
  struct memorize_node *temp;

  if (ch->memorized == NULL) {
    core_dump();
    return;
  }

  REMOVE_FROM_LIST(mem, ch->memorized, next);
  free(mem);
}

/* add a spell to a character's memorize(in progress) linked list */
void memorize_add(struct char_data * ch, int spellnum, int timer)
{
  struct memorize_node * mem;

  CREATE(mem, struct memorize_node, 1);
  mem->timer = timer;
  mem->spell = spellnum;
  mem->next = ch->memorized;
  ch->memorized = mem;
}

/********************************************/
/*  type is forget, memorize, or stop       */
/*  message 0 for no message, 1 for message */
/********************************************/

void do_mem_spell(struct char_data *ch, char *arg, int type, int message)
{
  char *s;
  int spellnum, i;
  struct memorize_node *mem, *next;
  struct obj_data *obj;
  bool found = FALSE, domain = FALSE;

  if (message == 1) {
    s = strtok(arg, "\0");
    if (s == NULL) {
      if (type == SCMD_MEMORIZE)
        send_to_char(ch, "Memorize what spell?!?\r\n");
      if (type == SCMD_STOP)
        send_to_char(ch, "Stop memorizing what spell?!?\r\n");
      if (type == SCMD_FORGET)
        send_to_char(ch, "Forget what spell?!?\r\n");
      return;
    }
    spellnum = find_skill_num(s);
  } else
    spellnum = atoi(arg);

  switch(type) {

/************* SCMD_MEMORIZE *************/

  case SCMD_MEMORIZE:
  if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
    send_to_char(ch, "Memorize what?!?\r\n");
    return;
  }

  /* check for spellbook stuff */

  if (IS_ARCANE(ch)) {
  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
      if (spell_in_book(obj, spellnum)) {
        found = TRUE;
        break;
      }
      continue;
    }
    if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
      if (spell_in_scroll(obj, spellnum)) {
        found = TRUE;
        send_to_char(ch, "The @gmagical energy@n of the scroll leaves the paper and enters your @rmind@n!\r\n");
        send_to_char(ch, "With the @gmagical energy@n transfered from the scroll, the scroll withers to dust!\r\n");
        obj_from_char(obj);
        break;
      }
      continue;
    }
  }

  if (!found) {
    send_to_char(ch, "You don't seem to have %s in your spellbook.\r\n", spell_info[spellnum].name);
    return;
  }

  } else if (IS_DIVINE(ch)) {
    for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
      if (OBJ_FLAGGED(obj, ITEM_BLESS)) {
        found = TRUE;
        if (spell_in_domain(ch, spellnum)) {
          domain = TRUE;
          break;
        }
      continue;
      }
    }
    if (!found) {
      send_to_char(ch, "You do not possess an item of divine favor.\r\n");
      return;
    }
    if (!domain) {
      send_to_char(ch, "You are not granted that spell by your Diety!\r\n");
      return;
    }
  }

  //if (!IS_ARCANE(ch) && GET_SKILL(ch, spellnum) <= 0) {
  //  send_to_char(ch, "You are unfamiliar with that spell.\r\n");
  //  return;
  //}

  /*if spell is practiced and there is an open spell slot*/
  if (find_freeslot(ch, spell_info[spellnum].spell_level) >= 1) {	  
    memorize_add(ch, spellnum, ((spell_info[spellnum].spell_level + 2) * 5));	
    if (message == 1) {
      send_to_char(ch, "You start memorizing %s.\r\n", spell_info[spellnum].name);
      return;
    }
  } else {
    if (message) 
      send_to_char(ch, "All of your level %d spell slots are currently filled\r\n", 
        spell_info[spellnum].spell_level);
    else
      /* if automem toggle is on */
      send_to_char(ch, "You cannot auto-rememorize because all of your level %d spell slots are currently filled.\r\n", 
        spell_info[spellnum].spell_level);
  }
  break;

/************* SCMD_STOP *************/

  case SCMD_STOP:
    if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
      send_to_char(ch, "Stop memorizing what?!?\r\n");
      return;
    }
    
    for (mem = ch->memorized; mem; mem = next) {
      if (mem->spell == spellnum) {
        send_to_char(ch, "You stop memorizing %s.\r\n", spell_info[spellnum].name);
        memorize_remove(ch, mem);
        return;
      }
      next = mem->next;
    }

    send_to_char(ch, "%s is not being memorized.\r\n", spell_info[spellnum].name);
    break;

/************* SCMD_FORGET *************/

  case SCMD_FORGET:
    if ((spellnum < 1) || (spellnum > SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
      send_to_char(ch, "Forget what?!?\r\n");
      return;
    }
    for (i = 0; i < GET_MEMCURSOR(ch); i++) {
      if(GET_SPELLMEM(ch, i) == spellnum) {
        GET_MEMCURSOR(ch)--;
        GET_SPELLMEM(ch, i) = GET_SPELLMEM(ch, GET_MEMCURSOR(ch));
	if (message) 
	  send_to_char(ch, "You forget the spell, %s.\r\n", spell_info[spellnum].name);
        return;
      }
    }
    send_to_char(ch, "%s is not memorized.\r\n", spell_info[spellnum].name);
    break;

/***********************************/

  }
}

/* returns the number of slots memorized if single is 0 
   returns 1 at the first occurance of the spell if memorized
   and single is 1 (true). 
*/
int is_memorized(struct char_data *ch, int spellnum, int single)
{
  int memcheck = 0, i;
  for (i = 0; i < GET_MEMCURSOR(ch); i++){
    if (GET_SPELLMEM(ch, i) == spellnum){
      memcheck++;
      if(single)
        return 1;
    }
  }
  return memcheck;
}

/**************************************************/
/* returns 1 if slot of spelllvl is open 0 if not */
/**************************************************/

int find_freeslot(struct char_data *ch, int spelllvl)
{
  struct memorize_node *mem, *next;
  int i, memcheck = 0;

  /* checked the memorized array */
  for (i = 0;i < GET_MEMCURSOR(ch); i++) {
    if (spell_info[GET_SPELLMEM(ch, i)].spell_level == spelllvl) {
      memcheck++;
    }
  }

  /* check the memorize linked list */
  for (mem = ch->memorized; mem; mem = next) {
    if (spell_info[mem->spell].spell_level == spelllvl) {  
      memcheck++;
    }
    next = mem->next;
  }

  if (memcheck < findslotnum(ch, spelllvl)) {
    return (1);
  } else {
    return 0;
  }
}

ACMD(do_memorize)
{
  char arg[MAX_INPUT_LENGTH];

  if(IS_NPC(ch))
    return;

  one_argument(argument, arg); 

  switch (subcmd) {
  case SCMD_MEMORIZE:
    if (!*arg) {
      do_mem_display(ch);
    } else {
      if(!*argument)
        send_to_char(ch, "Memorize what spell?\r\n");
      else
        do_mem_spell(ch, argument, SCMD_MEMORIZE, 1);
    }
    break;
  case SCMD_STOP:
    if(!*argument) 
      send_to_char(ch, "Stop memorizing what?!?\r\n");
    else
      do_mem_spell(ch, argument, SCMD_STOP, 1);
    break;
  case SCMD_FORGET:
    if(!*argument) 
      send_to_char(ch, "Forget what spell?\r\n");
    else
      do_mem_spell(ch, argument, SCMD_FORGET, 1);
    break;
  case SCMD_WHEN_SLOT:
    displayslotnum(ch, GET_CLASS(ch));
    break;
  default:
    log("SYSERR: Unknown subcmd %d passed to do_memorize (%s)", subcmd, __FILE__);
    break;
  }
}

/********************************************/
/*          Spell Levels                    */
/*  0   1   2   3   4   5   6   7   8   9   */
/********************************************/
  
          /* Wizard */
int spell_lvlmax_table[NUM_CLASSES][21][10] = {
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  2, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 4,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2,  1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 4,  3,  3,  2, -1, -1, -1, -1, -1, -1 }, 
  { 4,  4,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 4,  4,  3,  3,  2, -1, -1, -1, -1, -1 },
  { 4,  4,  4,  3,  2,  1, -1, -1, -1, -1 },
  { 4,  4,  4,  3,  3,  2, -1, -1, -1, -1 }, /* lvl 10 */
  { 4,  4,  4,  4,  3,  2,  1, -1, -1, -1 }, 
  { 4,  4,  4,  4,  3,  3,  2, -1, -1, -1 },
  { 4,  4,  4,  4,  4,  3,  2,  1, -1, -1 },
  { 4,  4,  4,  4,  4,  3,  3,  2, -1, -1 },
  { 4,  4,  4,  4,  4,  4,  3,  2,  1, -1 }, /* lvl 15 */
  { 4,  4,  4,  4,  4,  4,  3,  3,  2, -1 },
  { 4,  4,  4,  4,  4,  4,  4,  3,  2,  1 },
  { 4,  4,  4,  4,  4,  4,  4,  3,  3,  2 },
  { 4,  4,  4,  4,  4,  4,  4,  4,  3,  3 },
  { 4,  4,  4,  4,  4,  4,  4,  4,  4,  4 }}, /* lvl 20 */

          /* Cleric */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  2, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 4,  3, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 4,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  4,  3, -1, -1, -1, -1, -1, -1, -1 },
  { 5,  4,  3,  2, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 5,  4,  4,  3, -1, -1, -1, -1, -1, -1 },
  { 6,  5,  4,  3,  2, -1, -1, -1, -1, -1 },
  { 6,  5,  4,  4,  3, -1, -1, -1, -1, -1 },
  { 6,  5,  5,  4,  3,  2, -1, -1, -1, -1 },
  { 6,  5,  5,  4,  4,  3, -1, -1, -1, -1 }, /* lvl 10 */
  { 6,  6,  5,  5,  4,  3,  2, -1, -1, -1 },
  { 6,  6,  5,  5,  4,  4,  3, -1, -1, -1 },
  { 6,  6,  6,  5,  5,  4,  3,  2, -1, -1 },
  { 6,  6,  6,  5,  5,  4,  4,  3, -1, -1 },
  { 6,  6,  6,  6,  5,  5,  4,  3,  2, -1 }, /* lvl 15 */
  { 6,  6,  6,  6,  5,  5,  4,  4,  3, -1 },
  { 6,  6,  6,  6,  6,  5,  5,  4,  3,  2 },
  { 6,  6,  6,  6,  6,  5,  5,  4,  4,  3 },
  { 6,  6,  6,  6,  6,  6,  5,  5,  4,  4 },
  { 6,  6,  6,  6,  6,  6,  5,  5,  5,  5 }}, /* lvl 20 */

          /* Rogue */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Fighter */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Monk */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Paladin */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  0, -1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  0, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1,  1,  1,  0, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1, -1, -1, -1, -1, -1, -1 },
  {-1,  1,  1,  1,  0, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  0, -1, -1, -1, -1, -1 },
  {-1,  2,  1,  1,  1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1,  2,  2,  1,  1, -1, -1, -1, -1, -1 },
  {-1,  2,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  2,  2,  1, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  2, -1, -1, -1, -1, -1 },
  {-1,  3,  3,  3,  3, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Expert */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Adept */
 {{ 0,  0, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  { 3,  1, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2, -1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2,  0, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  2,  1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  { 3,  2,  1, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2, -1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  0, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  1, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  2,  1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  { 3,  3,  3,  2, -1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  0, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  2,  1, -1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2, -1, -1, -1, -1, -1 }, /* lvl 15 */
  { 3,  3,  3,  3,  2,  0, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2,  1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  2,  1, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  3,  2, -1, -1, -1, -1 },
  { 3,  3,  3,  3,  3,  2, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Commoner */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Aristocrat */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */

          /* Warrior */
 {{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 0  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, 
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 5  */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 10 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }, /* lvl 15 */
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 },
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1 }}, /* lvl 20 */
};

int spell_lvlmax(int whichclass, int chlevel, int slevel)
{
  if (chlevel < 1)
    return -1;
  if (chlevel > 20)
    chlevel = 20;
  if (slevel < 0)
    slevel = 0;
  if (slevel > 9)
    slevel = 9;
  return spell_lvlmax_table[whichclass][chlevel][slevel];
}

/***********************************************************************/

/********************************************/
/* bonus spells based on class attribute    */
/********************************************/

int spell_bonus(int attr, int lvl) {
  if (lvl < 1)
    return 0;
  if (attr < 12)
    return 0;
  lvl = ((ability_mod_value(attr) - lvl) / 4) + 1;
  if (lvl < 0)
    return 0;
  return lvl;
}

/*****************************************************************/
/* Returns the number of spell slots for the character and spell */
/* level requested                                               */
/*****************************************************************/

int findslotnum(struct char_data *ch, int spelllvl)
{
  int spelllvl_num, spell_mod, stat_mod, slevel = 0;

  slevel = (GET_LEVEL(ch));
  slevel = MIN(slevel, 30);

  if (GET_LEVEL(ch) > 0)
    slevel = MAX(slevel, 1);

  /* check ability for bonus spell slots (depends on class) */
  switch (GET_CLASS(ch)) {
  case CLASS_WIZARD:
    stat_mod = spell_bonus(GET_INT(ch), spelllvl);
  break;
  case CLASS_CLERIC:
    stat_mod = spell_bonus(GET_WIS(ch), spelllvl);
  break;
  default:
    stat_mod = spell_bonus(GET_INT(ch), spelllvl);
  break;
  }

  spell_mod = spell_lvlmax(GET_CLASS(ch), slevel, spelllvl);

  /* check to see if they have any slots for that level and type */
  if (spell_mod == -1)
    return 0;

  /* spell level modifying EQ */
  spell_mod += GET_SPELL_LEVEL(ch, spelllvl) + stat_mod;

  /* max of 10 slots per spell lvl */  
  spelllvl_num = MAX(0, MIN(10, spell_mod)); 

  return spelllvl_num;
}

void displayslotnum(struct char_data *ch, int class)
{
  char buf[MAX_INPUT_LENGTH];
  char slot_buf[MAX_STRING_LENGTH * 10];
  int i, j, tmp;

  snprintf(slot_buf, sizeof(slot_buf), "test\r\n");

  for(class = 0; class < NUM_CLASSES; class++) {
    snprintf(buf, sizeof(buf), "\r\n/* %s */\r\n\r\n", pc_class_types[class]);
    strcat(slot_buf, buf);
    for(i = 0; i < LVL_EPICSTART; i++) {
      strcat(slot_buf, "  { ");
      for(j = 0; j < MAX_SPELL_LEVEL; j++) {
        tmp = spell_lvlmax(class, i, j);
        if (tmp > -1)
          snprintf(buf, sizeof(buf), " %d%s ", tmp, (j==(MAX_SPELL_LEVEL - 1)) ? "" : ",");
        else
          snprintf(buf, sizeof(buf), " -%s ", (j==(MAX_SPELL_LEVEL - 1)) ? "" : ",");
        strcat(slot_buf, buf);
      }
      strcat(slot_buf, " },");
      if (!(i % 5)) {
        snprintf(buf, sizeof(buf), " /* lvl %d */", i);
        strcat(slot_buf, buf);
      }
      strcat(slot_buf, "\r\n");
    }
  }
  page_string(ch->desc, slot_buf, 1);
}

ACMD(do_scribe)
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char *s, buf[READ_SIZE];
  int i, spellnum;
  struct obj_data *obj;
  bool found = FALSE;

  half_chop(argument, arg1, arg2);

  if (!*arg1 || !*arg2) {
    send_to_char(ch, "Usually you scribe SOMETHING.\r\n");
    return;
  }

  if (!IS_ARCANE(ch) && GET_ADMLEVEL(ch) <= ADMLVL_IMMORT) {
    send_to_char(ch, "You really aren't qualified to do that...\r\n");
    return;
  }

  if (!HAS_FEAT(ch, FEAT_SCRIBE_SCROLL)) {
    send_to_char(ch, "You haven't had the proper instruction yet!\r\n");
    return;
  }

  if ((strcmp(arg1, "scroll") && strcmp(arg1, "spellbook"))) {
    send_to_char(ch, "It really doesn't work that way.\r\n");
    return;
  }

  s = strtok(arg2, "\0");
  spellnum = find_skill_num(s);

  if ((spellnum < 1) || (spellnum >= SKILL_TABLE_SIZE) || !IS_SET(skill_type(spellnum), SKTYPE_SPELL)) {
    send_to_char(ch, "Strange, there is no such spell.\r\n");
    return;
  }

  for (obj = ch->carrying; obj && !found; obj = obj->next_content) {
    if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK || GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
      found = TRUE;
      break;
    }
  }

    if (found) {
      if (GET_OBJ_TYPE(obj) == ITEM_SPELLBOOK) {
        /* check if spell is already in book */
        if (spell_in_book(obj, spellnum)) {
          send_to_char(ch, "You already have the spell '%s' in this spellbook.\r\n", spell_info[spellnum].name);
          return;
        }
        if (!obj->sbinfo) {
          CREATE(obj->sbinfo, struct obj_spellbook_spell, SPELLBOOK_SIZE);
          memset((char *) obj->sbinfo, 0, SPELLBOOK_SIZE * sizeof(struct obj_spellbook_spell));
        }
        for (i=0; i < SPELLBOOK_SIZE; i++) {
          if (obj->sbinfo[i].spellname == 0) {
            break;
          } else {
            continue;
          }
          send_to_char(ch, "Your spellbook is full!\r\n");
          return;
        }

        if (!is_memorized(ch, spellnum, TRUE)) {
          send_to_char(ch, "You must have the spell committed to memory before you can scribe it!\r\n");
          return;
        }

        obj->sbinfo[i].spellname = spellnum;
        obj->sbinfo[i].pages = MAX(1, spell_info[spellnum].spell_level * 2);
        send_to_char(ch, "You scribe the spell '%s' into your spellbook, which takes up %d pages.\r\n", spell_info[spellnum].name, obj->sbinfo[i].pages);
        send_to_char(ch, "The magical energy committed for the spell '%s' has been expended.\r\n", spell_info[spellnum].name);
        sprintf(buf, "%d", spellnum);
        do_mem_spell(ch, buf, SCMD_FORGET, 0);
        if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE)) {
          SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
        }
      } else if (GET_OBJ_TYPE(obj) == ITEM_SCROLL) {
          if (GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) > 0) {
            send_to_char(ch, "The scroll has a spell enscribed on it!\r\n");
            return;
          }
        GET_OBJ_VAL(obj, VAL_SCROLL_LEVEL) = GET_LEVEL(ch);
        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL1) = spellnum;
        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL2) = -1;
        GET_OBJ_VAL(obj, VAL_SCROLL_SPELL3) = -1;
        sprintf(buf, "a scroll of '%s'", spell_info[spellnum].name);
        obj->short_description = strdup(buf);
        send_to_char(ch, "You scribe the spell '%s' onto the scroll.\r\n", spell_info[spellnum].name);
        send_to_char(ch, "The magical energy committed for the spell '%s' has been expended.\r\n", spell_info[spellnum].name);
        sprintf(buf, "%d", spellnum);
        do_mem_spell(ch, buf, SCMD_FORGET, 0);
        if (!OBJ_FLAGGED(obj, ITEM_UNIQUE_SAVE)) {
          SET_BIT_AR(GET_OBJ_EXTRA(obj), ITEM_UNIQUE_SAVE);
        }
      } else {
        send_to_char(ch, "But you don't have anything suitable for scribing!\r\n");
        return;
      }
    }
    return;
}

