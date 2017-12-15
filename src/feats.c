/*****************************************************************************
** FEATS.C                                                                  **
** Source code for the Gates of Krynn Feats System.                         **
** Initial code by Paladine (Stephen Squires)                               **
** Created Thursday, September 5, 2002                                      **
**                                                                          **
*****************************************************************************/

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/feats.c,v 1.14 2004/12/30 04:34:48 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "spells.h"
#include "handler.h"
#include "db.h"
#include "constants.h"
#include "interpreter.h"
#include "dg_scripts.h"
#include "feats.h"

/* Local Functions */
void assign_feats(void);
void feato(int featnum, char *name, int in_game, int can_learn, int can_stack);
void list_feats_known(struct char_data *ch); 
void list_feats_available(struct char_data *ch); 
void list_feats_complete(struct char_data *ch); 
int compare_feats(const void *x, const void *y);
void sort_feats(void);	
int find_feat_num(char *name);

/* Global Variables and Structures */
struct feat_info feat_list[NUM_FEATS_DEFINED];
int feat_sort_info[MAX_FEATS + 1];
char buf3[MAX_STRING_LENGTH];
char buf4[MAX_STRING_LENGTH];

/* External variables and structures */
extern int spell_sort_info[SKILL_TABLE_SIZE+1];
extern struct spell_info_type spell_info[];

/* External functions*/
int count_metamagic_feats(struct char_data *ch);

void feato(int featnum, char *name, int in_game, int can_learn, int can_stack)
{
  feat_list[featnum].name = name;
  feat_list[featnum].in_game = in_game;
  feat_list[featnum].can_learn = can_learn;
  feat_list[featnum].can_stack = can_stack;
}

void free_feats(void)
{
  /* Nothing to do right now */
}

void assign_feats(void)
{

  int i;

  // Initialize the list of feats.

  for (i = 0; i <= NUM_FEATS_DEFINED; i++) {
    feat_list[i].name = "Unused Feat";
    feat_list[i].in_game = FALSE;
    feat_list[i].can_learn = FALSE;
    feat_list[i].can_stack = FALSE;
  }

// Below are the various feat initializations.
// First parameter is the feat number, defined in feats.h
// Second parameter is the displayed name of the feat and argument used to train it
// Third parameter defines whether or not the feat is in the game or not, and thus can be learned and displayed
// Fourth parameter defines whether or not the feat can be learned through a trainer or whether it is
// a feat given automatically to certain classes or races.

feato(FEAT_ALERTNESS, "alertness", TRUE, TRUE, FALSE); 
feato(FEAT_ARMOR_PROFICIENCY_HEAVY, "heavy armor proficiency", TRUE, TRUE, FALSE); 
feato(FEAT_ARMOR_PROFICIENCY_LIGHT, "light armor proficiency", TRUE, TRUE, FALSE); 
feato(FEAT_ARMOR_PROFICIENCY_MEDIUM, "medium armor proficiency", TRUE, TRUE, FALSE); 
feato(FEAT_BLIND_FIGHT, "blind fighting", TRUE, TRUE, FALSE); 
feato(FEAT_BREW_POTION, "brew potion", TRUE, TRUE, FALSE); 
feato(FEAT_CLEAVE, "cleave", TRUE, TRUE, FALSE);
feato(FEAT_COMBAT_CASTING, "combat casting", TRUE, TRUE, FALSE); 
feato(FEAT_COMBAT_REFLEXES, "combat reflexes", TRUE, TRUE, FALSE);
feato(FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR, "craft magical arms and armor", TRUE, TRUE, FALSE); 
feato(FEAT_CRAFT_ROD, "craft rod", TRUE, TRUE, FALSE);
feato(FEAT_CRAFT_STAFF, "craft staff", TRUE, TRUE, FALSE); 
feato(FEAT_CRAFT_WAND, "craft wand", TRUE, TRUE, FALSE); 
feato(FEAT_CRAFT_WONDEROUS_ITEM, "craft wonderous item", FALSE, TRUE, FALSE); 
feato(FEAT_DEFLECT_ARROWS, "deflect arrows", FALSE, FALSE, FALSE); 
feato(FEAT_DODGE, "dodge", TRUE, TRUE, FALSE); 
feato(FEAT_EMPOWER_SPELL, "empower spell", TRUE, TRUE, FALSE); 
feato(FEAT_ENDURANCE, "endurance", TRUE, TRUE, FALSE);
feato(FEAT_ENLARGE_SPELL, "enlarge spell", FALSE, FALSE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD, "weapon proficiency - bastard sword", FALSE, TRUE, FALSE);
feato(FEAT_EXTEND_SPELL, "extend spell", TRUE, TRUE, FALSE);
feato(FEAT_EXTRA_TURNING, "extra turning", TRUE, TRUE, FALSE);
feato(FEAT_FAR_SHOT, "far shot", FALSE, FALSE, FALSE);
feato(FEAT_FORGE_RING, "forge ring", TRUE, TRUE, FALSE);
feato(FEAT_GREAT_CLEAVE, "great cleave", FALSE, FALSE, FALSE);
feato(FEAT_GREAT_FORTITUDE, "great fortitude", TRUE, TRUE, FALSE);
feato(FEAT_HEIGHTEN_SPELL, "heighten spell", TRUE, TRUE, FALSE);
feato(FEAT_IMPROVED_BULL_RUSH, "improved bull rush", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_CRITICAL, "improved critical", TRUE, TRUE, TRUE);
feato(FEAT_IMPROVED_DISARM, "improved disarm", TRUE, TRUE, FALSE);
feato(FEAT_IMPROVED_INITIATIVE, "improved initiative", TRUE, TRUE, FALSE);
feato(FEAT_IMPROVED_TRIP, "improved trip", TRUE, TRUE, FALSE);
feato(FEAT_IMPROVED_TWO_WEAPON_FIGHTING, "improved two weapon fighting", TRUE, TRUE, FALSE);
feato(FEAT_IMPROVED_UNARMED_STRIKE, "improved unarmed strike", FALSE, FALSE, FALSE);
feato(FEAT_IRON_WILL, "iron will", TRUE, TRUE, FALSE);
feato(FEAT_LEADERSHIP, "leadership", FALSE, FALSE, FALSE);
feato(FEAT_LIGHTNING_REFLEXES, "lightning reflexes", TRUE, TRUE, FALSE);
feato(FEAT_MARTIAL_WEAPON_PROFICIENCY, "martial weapon proficiency", TRUE, TRUE, FALSE);
feato(FEAT_MAXIMIZE_SPELL, "maximize spell", TRUE, TRUE, FALSE);
feato(FEAT_MOBILITY, "mobility", TRUE, TRUE, FALSE);
feato(FEAT_MOUNTED_ARCHERY, "mounted archery", FALSE, FALSE, FALSE);
feato(FEAT_MOUNTED_COMBAT, "mounted combat", FALSE, FALSE, FALSE);
feato(FEAT_POINT_BLANK_SHOT, "point blank shot", FALSE, FALSE, FALSE);
feato(FEAT_POWER_ATTACK, "power attack", TRUE, TRUE, FALSE);
feato(FEAT_PRECISE_SHOT, "precise shot", FALSE, FALSE, FALSE);
feato(FEAT_QUICK_DRAW, "quick draw", FALSE, FALSE, FALSE);
feato(FEAT_QUICKEN_SPELL, "quicken spell", TRUE, TRUE, FALSE);
feato(FEAT_RAPID_SHOT, "rapid shot", FALSE, FALSE, FALSE);
feato(FEAT_RIDE_BY_ATTACK, "ride by attack", FALSE, FALSE, FALSE);
feato(FEAT_RUN, "run", FALSE, FALSE, FALSE);
feato(FEAT_SCRIBE_SCROLL, "scribe scroll", TRUE, TRUE, FALSE);
feato(FEAT_SHOT_ON_THE_RUN, "shot on the run", FALSE, FALSE, FALSE);
feato(FEAT_SILENT_SPELL, "silent spell", TRUE, TRUE, FALSE);
feato(FEAT_SIMPLE_WEAPON_PROFICIENCY, "simple weapon proficiency", TRUE, TRUE, FALSE);
feato(FEAT_SKILL_FOCUS, "skill focus", TRUE, TRUE, TRUE);
feato(FEAT_SPELL_FOCUS, "spell focus", TRUE, TRUE, TRUE);
feato(FEAT_SPELL_MASTERY, "spell mastery", TRUE, TRUE, TRUE);
feato(FEAT_SPELL_PENETRATION, "spell penetration", TRUE, TRUE, FALSE);
feato(FEAT_SPIRITED_CHARGE, "spirited charge", FALSE, FALSE, FALSE);
feato(FEAT_SPRING_ATTACK, "spring attack", FALSE, FALSE, FALSE);
feato(FEAT_STILL_SPELL, "still spell", TRUE, TRUE, FALSE);
feato(FEAT_STUNNING_FIST, "stunning fist", TRUE, TRUE, FALSE);
feato(FEAT_SUNDER, "sunder", TRUE, TRUE, FALSE);
feato(FEAT_TOUGHNESS, "toughness", TRUE, TRUE, TRUE);
feato(FEAT_TRACK, "track", TRUE, TRUE, FALSE);
feato(FEAT_TRAMPLE, "trample", FALSE, FALSE, FALSE);
feato(FEAT_TWO_WEAPON_FIGHTING, "two weapon fighting", TRUE, TRUE, FALSE);
feato(FEAT_WEAPON_FINESSE, "weapon finesse", TRUE, TRUE, TRUE);
feato(FEAT_WEAPON_FOCUS, "weapon focus", TRUE, TRUE, TRUE);
feato(FEAT_WEAPON_SPECIALIZATION, "weapon specialization", TRUE, TRUE, TRUE);
feato(FEAT_WHIRLWIND_ATTACK, "whirlwind attack", TRUE, TRUE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_DRUID, "weapon proficiency - druids", FALSE, FALSE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_ROGUE, "weapon proficiency - rogues", FALSE, FALSE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_MONK, "weapon proficiency - monks", FALSE, FALSE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_WIZARD, "weapon proficiency - wizards", FALSE, FALSE, FALSE);
feato(FEAT_WEAPON_PROFICIENCY_ELF, "weapon proficiency - elves", FALSE, FALSE, FALSE);
feato(FEAT_ARMOR_PROFICIENCY_SHIELD, "shield armor proficiency", TRUE, FALSE, FALSE); 
feato(FEAT_SNEAK_ATTACK, "sneak attack", TRUE, FALSE, TRUE);
feato(FEAT_EVASION, "evasion", TRUE, FALSE, FALSE);
feato(FEAT_IMPROVED_EVASION, "improved evasion", TRUE, FALSE, FALSE);
feato(FEAT_ACROBATIC, "acrobatic", TRUE, TRUE, FALSE);
feato(FEAT_AGILE, "agile", TRUE, TRUE, FALSE);
feato(FEAT_ALERTNESS, "alertness", TRUE, TRUE, FALSE);
feato(FEAT_ANIMAL_AFFINITY, "animal affinity", TRUE, TRUE, FALSE);
feato(FEAT_ATHLETIC, "athletic", TRUE, TRUE, FALSE);
feato(FEAT_AUGMENT_SUMMONING, "augment summoning", FALSE, FALSE, FALSE);
feato(FEAT_COMBAT_EXPERTISE, "combat expertise", FALSE, FALSE, FALSE);
feato(FEAT_DECEITFUL, "deceitful", TRUE, TRUE, FALSE);
feato(FEAT_DEFT_HANDS, "deft hands", TRUE, TRUE, FALSE);
feato(FEAT_DIEHARD, "diehard", FALSE, FALSE, FALSE);
feato(FEAT_DILIGENT, "diligent", TRUE, TRUE, FALSE);
feato(FEAT_ESCHEW_MATERIALS, "eschew materials", FALSE, FALSE, FALSE);
feato(FEAT_EXOTIC_WEAPON_PROFICIENCY, "exotic weapon proficiency", FALSE, FALSE, FALSE);
feato(FEAT_GREATER_SPELL_FOCUS, "greater spell focus", FALSE, FALSE, TRUE);
feato(FEAT_GREATER_SPELL_PENETRATION, "greater spell penetration", FALSE, FALSE, FALSE);
feato(FEAT_GREATER_TWO_WEAPON_FIGHTING, "greater two weapon fighting", FALSE, FALSE, FALSE);
feato(FEAT_GREATER_WEAPON_FOCUS, "greater weapon focus", TRUE, TRUE, TRUE);
feato(FEAT_GREATER_WEAPON_SPECIALIZATION, "greater weapon specialization", TRUE, TRUE, TRUE);
feato(FEAT_IMPROVED_COUNTERSPELL, "improved counterspell", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_FAMILIAR, "improved familiar", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_FEINT, "improved feint", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_GRAPPLE, "improved grapple", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_OVERRUN, "improved overrun", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_PRECISE_SHOT, "improved precise shot", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_SHIELD_BASH, "improved shield bash", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_SUNDER, "improved sunder", FALSE, FALSE, FALSE);
feato(FEAT_IMPROVED_TURNING, "improved turning", FALSE, FALSE, FALSE);
feato(FEAT_INVESTIGATOR, "investigator", TRUE, TRUE, FALSE);
feato(FEAT_MAGICAL_APTITUDE, "magical aptitude", TRUE, TRUE, FALSE);
feato(FEAT_MANYSHOT, "manyshot", FALSE, FALSE, FALSE);
feato(FEAT_NATURAL_SPELL, "natural spell", FALSE, FALSE, FALSE);
feato(FEAT_NEGOTIATOR, "negotiator", TRUE, TRUE, FALSE);
feato(FEAT_NIMBLE_FINGERS, "nimble fingers", TRUE, TRUE, FALSE);
feato(FEAT_PERSUASIVE, "persuasive", TRUE, TRUE, FALSE);
feato(FEAT_RAPID_RELOAD, "rapid reload", FALSE, FALSE, FALSE);
feato(FEAT_SELF_SUFFICIENT, "self sufficient", TRUE, TRUE, FALSE);
feato(FEAT_STEALTHY, "stealthy", TRUE, TRUE, FALSE);
feato(FEAT_ARMOR_PROFICIENCY_TOWER_SHIELD, "tower shield armor proficiency", FALSE, FALSE, FALSE);
feato(FEAT_TWO_WEAPON_DEFENSE, "two weapon defense", FALSE, FALSE, FALSE);
feato(FEAT_WIDEN_SPELL, "widen spell", FALSE, FALSE, FALSE);
}

// The follwing function is used to check if the character satisfies the various prerequisite(s) (if any)
// of a feat in order to learn it.

int feat_is_available(struct char_data *ch, int featnum, int iarg, char *sarg)
{
  if (featnum > NUM_FEATS_DEFINED)
    return FALSE;

  if (HAS_FEAT(ch, featnum) && !feat_list[featnum].can_stack)
    return FALSE;

  switch (featnum) {

  case FEAT_ARMOR_PROFICIENCY_HEAVY:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
      return TRUE;
    return FALSE;

  case FEAT_ARMOR_PROFICIENCY_MEDIUM:
    if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
      return TRUE;
    return FALSE;

  case FEAT_DODGE:
    if (GET_DEX(ch) >= 13)
      return TRUE;
    return FALSE;

  case FEAT_MOBILITY:
    if (HAS_FEAT(ch, FEAT_DODGE))
      return TRUE;
    return FALSE;

  case FEAT_WEAPON_PROFICIENCY_BASTARD_SWORD:
    if (GET_BAB(ch) >= 1)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_DISARM:
    return TRUE;

  case FEAT_IMPROVED_TRIP:
    return TRUE;

  case FEAT_WHIRLWIND_ATTACK:
    return TRUE;

  case FEAT_STUNNING_FIST:
    return TRUE;
  
  case FEAT_POWER_ATTACK:
    if (GET_STR(ch) >= 13)
      return TRUE;
    return FALSE;

  case FEAT_CLEAVE:
    if (HAS_FEAT(ch, FEAT_POWER_ATTACK))
      return TRUE;
    return FALSE;

  case FEAT_SUNDER:
    if (HAS_FEAT(ch, FEAT_POWER_ATTACK))
      return TRUE;
    return FALSE;

  case FEAT_TWO_WEAPON_FIGHTING:
    if (GET_DEX(ch) >= 15)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_TWO_WEAPON_FIGHTING:
    if (GET_DEX(ch) >= 17 && HAS_FEAT(ch, FEAT_TWO_WEAPON_FIGHTING) && GET_BAB(ch) >= 6)
      return TRUE;
    return FALSE;

  case FEAT_IMPROVED_CRITICAL:
    if (GET_BAB(ch) < 8)
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_WEAPON_FINESSE:
  case FEAT_WEAPON_FOCUS:
    if (GET_BAB(ch) < 1)
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_WEAPON_SPECIALIZATION:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 4)
      return FALSE;
    if (!iarg || is_proficient_with_weapon(ch, iarg))
      return TRUE;
    return FALSE;

  case FEAT_GREATER_WEAPON_FOCUS:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 8)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (is_proficient_with_weapon(ch, iarg) && HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, iarg))
      return TRUE;
    return FALSE;

  case FEAT_GREATER_WEAPON_SPECIALIZATION:
    if (GET_CLASS_RANKS(ch, CLASS_FIGHTER) < 12)
      return FALSE;
    if (!iarg)
      return TRUE;
    if (is_proficient_with_weapon(ch, iarg) &&
        HAS_COMBAT_FEAT(ch, CFEAT_GREATER_WEAPON_FOCUS, iarg) &&
        HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_SPECIALIZATION, iarg) &&
        HAS_COMBAT_FEAT(ch, CFEAT_WEAPON_FOCUS, iarg))
      return TRUE;
    return FALSE;

  case FEAT_SPELL_FOCUS:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_SPELL_PENETRATION:
    if (GET_LEVEL(ch))
      return TRUE;
    return FALSE;

  case FEAT_BREW_POTION:
    if (GET_LEVEL(ch) >= 3)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_MAGICAL_ARMS_AND_ARMOR:
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_ROD:
    if (GET_LEVEL(ch) >= 9)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_STAFF:
    if (GET_LEVEL(ch) >= 12)
      return TRUE;
    return FALSE;

  case FEAT_CRAFT_WAND:
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    return FALSE;

  case FEAT_FORGE_RING:
    if (GET_LEVEL(ch) >= 5)
      return TRUE;
    return FALSE;

  case FEAT_SCRIBE_SCROLL:
    if (GET_LEVEL(ch) >= 1)
      return TRUE;
    return FALSE;

  case FEAT_EMPOWER_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_EXTEND_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_HEIGHTEN_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_MAXIMIZE_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_QUICKEN_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_SILENT_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_STILL_SPELL:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  case FEAT_EXTRA_TURNING:
    if (GET_CLASS_RANKS(ch, CLASS_CLERIC))
      return TRUE;
    return FALSE;

  case FEAT_SPELL_MASTERY:
    if (GET_CLASS_RANKS(ch, CLASS_WIZARD))
      return TRUE;
    return FALSE;

  default:
    return TRUE;

  }
}

int is_proficient_with_armor(const struct char_data *ch, int armor_type)
{
  switch (armor_type) {
    case ARMOR_TYPE_LIGHT:
      if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_LIGHT))
        return TRUE;
    break;
    case ARMOR_TYPE_MEDIUM:
      if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_MEDIUM))
        return TRUE;
    break;
    case ARMOR_TYPE_HEAVY:
      if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_HEAVY))
        return TRUE;
    break;
    case ARMOR_TYPE_SHIELD:
      if (HAS_FEAT(ch, FEAT_ARMOR_PROFICIENCY_SHIELD))
        return TRUE;
    break;
  }
  return FALSE;
}

int is_proficient_with_weapon(const struct char_data *ch, int weapon_type)
{
  switch (weapon_type) {
  case WEAPON_TYPE_UNARMED:
    return 1;
  case WEAPON_TYPE_DAGGER:
  case WEAPON_TYPE_MACE:
  case WEAPON_TYPE_SICKLE:
  case WEAPON_TYPE_SPEAR:
  case WEAPON_TYPE_STAFF:
  case WEAPON_TYPE_CROSSBOW:
  case WEAPON_TYPE_SLING:
  case WEAPON_TYPE_THROWN:
  case WEAPON_TYPE_CLUB:
    if (HAS_FEAT(ch, FEAT_SIMPLE_WEAPON_PROFICIENCY))
      return TRUE;
    break;
  case WEAPON_TYPE_SHORTBOW:
  case WEAPON_TYPE_LONGBOW:
  case WEAPON_TYPE_HAMMER:
  case WEAPON_TYPE_LANCE:
  case WEAPON_TYPE_FLAIL:
  case WEAPON_TYPE_LONGSWORD:
  case WEAPON_TYPE_SHORTSWORD:
  case WEAPON_TYPE_GREATSWORD:
  case WEAPON_TYPE_RAPIER:
  case WEAPON_TYPE_SCIMITAR:
  case WEAPON_TYPE_POLEARM:
  case WEAPON_TYPE_BASTARD_SWORD:
  case WEAPON_TYPE_AXE:
    if (HAS_FEAT(ch, FEAT_MARTIAL_WEAPON_PROFICIENCY))
      return TRUE;
    break;
  default:
    return FALSE;
    break;
  }
  return FALSE;
}
  
int compare_feats(const void *x, const void *y)
{
  int   a = *(const int *)x,
        b = *(const int *)y;
  
  return strcmp(feat_list[a].name, feat_list[b].name);
}

void sort_feats(void)
{
  int a;

  /* initialize array, avoiding reserved. */
  for (a = 1; a <= NUM_FEATS_DEFINED; a++)
    feat_sort_info[a] = a;

  qsort(&feat_sort_info[1], NUM_FEATS_DEFINED, sizeof(int), compare_feats);
}

void list_feats_known(struct char_data *ch) 
{
  int i, sortpos;
  int none_shown = TRUE;
  int temp_value;
  int added_hp = 0;
  char buf [MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];

  if (!GET_FEAT_POINTS(ch))
    strcpy(buf, "\r\nYou cannot learn any feats right now.\r\n");
  else
    sprintf(buf, "\r\nYou can learn %d feat%s right now.\r\n",
            GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"));

    
    // Display Headings
    sprintf(buf + strlen(buf), "\r\n");
    sprintf(buf + strlen(buf), "@WFeats Known@n\r\n");
    sprintf(buf + strlen(buf), "@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@n\r\n");
    sprintf(buf + strlen(buf), "\r\n");

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) {

    if (strlen(buf2) > MAX_STRING_LENGTH -32)
      break;

    i = feat_sort_info[sortpos];
    if (HAS_FEAT(ch, i)  && feat_list[i].in_game) {
      if (i == FEAT_SKILL_FOCUS) {
        sprintf(buf, "%-20s (+%d points overall)\r\n", feat_list[i].name, HAS_FEAT(ch, i) * 2);
        strcat(buf2, buf);
        none_shown = FALSE;
      }
    else if (i == FEAT_TOUGHNESS) {
      temp_value = HAS_FEAT(ch, FEAT_TOUGHNESS);
      added_hp = temp_value * 3;
      sprintf(buf, "%-20s (+%d hp)\r\n", feat_list[i].name, added_hp);
      strcat(buf2, buf);  
      none_shown = FALSE;
    } else {
        sprintf(buf, "%-20s\r\n", feat_list[i].name);
        strcat(buf2, buf);        /* The above, ^ should always be safe to do. */
        none_shown = FALSE;
      }
    }
  }

  if (none_shown) {
    sprintf(buf, "You do not know any feats at this time.\r\n");
    strcat(buf2, buf);
  }
   
  page_string(ch->desc, buf2, 1);
}

void list_feats_available(struct char_data *ch) 
{
  char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int i, sortpos;
  int none_shown = TRUE;

  if (!GET_FEAT_POINTS(ch))
    strcpy(buf, "\r\nYou cannot learn any feats right now.\r\n");
  else
    sprintf(buf, "\r\nYou can learn %d feat%s right now.\r\n",
            GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"));
    
    // Display Headings
    sprintf(buf + strlen(buf), "\r\n");
    sprintf(buf + strlen(buf), "@WFeats Available to Learn@n\r\n");
    sprintf(buf + strlen(buf), "@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@n\r\n");
    sprintf(buf + strlen(buf), "\r\n");

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) {
    i = feat_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n"); 
      break;   
    }
    if (feat_is_available(ch, i, 0, NULL) && feat_list[i].in_game && feat_list[i].can_learn) {
      sprintf(buf, "%-20s\r\n", feat_list[i].name);
      strcat(buf2, buf);        /* The above, ^ should always be safe to do. */
      none_shown = FALSE;
    }
  }

  if (none_shown) {
    sprintf(buf, "There are no feats available for you to learn at this point.\r\n");
    strcat(buf2, buf);
  }
   
  page_string(ch->desc, buf2, 1);
}
void list_feats_complete(struct char_data *ch) 
{

	char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH];
  int i, sortpos;
  int none_shown = TRUE;

  if (!GET_FEAT_POINTS(ch))
    strcpy(buf, "\r\nYou cannot learn any feats right now.\r\n");
  else
    sprintf(buf, "\r\nYou can learn %d feat%s right now.\r\n",
            GET_FEAT_POINTS(ch), (GET_FEAT_POINTS(ch) == 1 ? "" : "s"));

    
    // Display Headings
    sprintf(buf + strlen(buf), "\r\n");
    sprintf(buf + strlen(buf), "@WComplete Feat List@n\r\n");
    sprintf(buf + strlen(buf), "@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@B~@R~@n\r\n");
    sprintf(buf + strlen(buf), "\r\n");

  strcpy(buf2, buf);

  for (sortpos = 1; sortpos <= NUM_FEATS_DEFINED; sortpos++) {
    i = feat_sort_info[sortpos];
    if (strlen(buf2) >= MAX_STRING_LENGTH - 32) {
      strcat(buf2, "**OVERFLOW**\r\n"); 
      break;   
    }
//	sprintf(buf, "%s : %s\r\n", feat_list[i].name, feat_list[i].in_game ? "In Game" : "Not In Game");
//	strcat(buf2, buf);
    if (feat_list[i].in_game) {
      sprintf(buf, "%-20s\r\n", feat_list[i].name);
      strcat(buf2, buf);        /* The above, ^ should always be safe to do. */
      none_shown = FALSE;
    }
  }

  if (none_shown) {
    sprintf(buf, "There are currently no feats in the game.\r\n");
    strcat(buf2, buf);
  }
   
  page_string(ch->desc, buf2, 1);
}

int find_feat_num(char *name)
{  
  int index, ok;
  char *temp, *temp2;
  char first[256], first2[256];
   
  for (index = 1; index <= NUM_FEATS_DEFINED; index++) {
    if (is_abbrev(name, feat_list[index].name))
      return (index);
    
    ok = TRUE;
    /* It won't be changed, but other uses of this function elsewhere may. */
    temp = any_one_arg((char *)feat_list[index].name, first);
    temp2 = any_one_arg(name, first2);
    while (*first && *first2 && ok) {
      if (!is_abbrev(first2, first))
        ok = FALSE;
      temp = any_one_arg(temp, first);
      temp2 = any_one_arg(temp2, first2);
    }
  
    if (ok && !*first2)
      return (index);
  }
    
  return (-1);
}

ACMD(do_feats)
{
  char arg[80];

  one_argument(argument, arg);

  if (is_abbrev(arg, "known") || !*arg) {
    send_to_char(ch, "Syntax is \"feats <available | complete | known>\".\r\n");
    list_feats_known(ch);
  } else if (is_abbrev(arg, "available")) {
    list_feats_available(ch);
  } else if (is_abbrev(arg, "complete")) {
    list_feats_complete(ch);
  }
}

int feat_to_subfeat(int feat)
{
  switch (feat) {
  case FEAT_IMPROVED_CRITICAL:
    return CFEAT_IMPROVED_CRITICAL;
  case FEAT_WEAPON_FINESSE:
    return CFEAT_WEAPON_FINESSE;
  case FEAT_WEAPON_FOCUS:
    return CFEAT_WEAPON_FOCUS;
  case FEAT_WEAPON_SPECIALIZATION:
    return CFEAT_WEAPON_SPECIALIZATION;
  case FEAT_GREATER_WEAPON_FOCUS:
    return CFEAT_GREATER_WEAPON_FOCUS;
  case FEAT_GREATER_WEAPON_SPECIALIZATION:
    return CFEAT_GREATER_WEAPON_SPECIALIZATION;
  case FEAT_SPELL_FOCUS:
    return CFEAT_SPELL_FOCUS;
  case FEAT_GREATER_SPELL_FOCUS:
    return CFEAT_GREATER_SPELL_FOCUS;
  default:
    return -1;
  }
}
