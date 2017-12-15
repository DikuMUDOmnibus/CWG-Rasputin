#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/races.c,v 1.7 2004/12/26 18:14:25 fnord Exp $");

#include "structs.h"
#include "utils.h"

const char *race_names[] = {
  "human",
  "elf",
  "gnome",
  "dwarf",
  "half elf",
  "halfling",
  "drow",
  "half orc",
  "animal",
  "construct",
  "demon",
  "dragon",
  "fish",
  "giant",
  "goblin",
  "insect",
  "orc",
  "snake",
  "troll",
  "minotaur",
  "kobold",
  "mindflayer",
  "warhost",
  "faerie",
  "\n"
};

const char *race_abbrevs[] = {
  "Hum",
  "Elf",
  "Gno",
  "Dwa",
  "HEl",
  "Hlf",
  "Drw",
  "HOr",
  "Ani",
  "Con",
  "Dem",
  "Drg",
  "Fsh",
  "Gnt",
  "Gob",
  "Ict",
  "Orc",
  "Snk",
  "Trl",
  "Min",
  "Kob",
  "Mnd",
  "War",
  "Fae",
  "\n"
};

const char *pc_race_types[] = {
  "Human",
  "Elf",
  "Gnome",
  "Dwarf",
  "Half Elf",
  "Halfling",
  "Drow Elf",
  "Half Orc",
  "Animal",
  "Construct",
  "Demon",
  "Dragon",
  "Fish",
  "Giant",
  "Goblin",
  "Insect",
  "Orc",
  "Snake",
  "Troll",
  "Minotaur",
  "Kobold",
  "Mindflayer",
  "Warhost",
  "Faerie",
  "\n"
};

#define Y   TRUE
#define N   FALSE

/* Original Race/Gender Breakout */
int race_ok_gender[NUM_SEX][NUM_RACES] = {
/*        H, E, G, D, H, H, D, A, C, D, D, F, G, G, I, O, S, T, H, M, K, M, W, F */
/* N */ { N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* M */ { Y, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N },
/* F */ { Y, Y, Y, Y, Y, Y, N, Y, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N }

};

const char *race_display[NUM_RACES] = {
  "@B1@W) Human\r\n",
  "@B2@W) @GElf\r\n",
  "@B3@W) @MGnome\r\n",
  "@B4@W) @BDwarf\r\n",
  "@B5@W) @RHalf Elf\r\n",
  "@B6@W) @CHalfling\r\n",
  "@B7@W) @YDrow Elf\r\n",
  "@B8@W) Half Orc\r\n",
  "@B9@W) @GAnimal\r\n",
  "@B10@W) @MConstruct\r\n",
  "@B11@W) @BDemon\r\n",
  "@B12@W) @RDragon\r\n",
  "@B13@W) @CFish\r\n",
  "@B14@W) @YGiant\r\n",
  "@B15@W) Goblin\r\n",
  "@B16@W) @GInsect\r\n",
  "@B17@W) @MOrc\r\n",
  "@B18@W) @BSnake\r\n",
  "@B19@W) @RTroll\r\n",
  "@B20@W) @CMinotaur\r\n",
  "@B21@W) @YKobold\r\n",
  "@B22@W) Mindflayer\r\n",
  "@B23@W) @GWarhost\r\n",
  "@B24@W) @MFaerie\r\n",
};

/*
 * The code to interpret a race letter (used in interpreter.c when a
 * new character is selecting a race).
 */
int parse_race(struct char_data *ch, int arg)
{
  int race = RACE_UNDEFINED;

  switch (arg) {
  case 1:  race = RACE_HUMAN      ; break;
  case 2:  race = RACE_ELF        ; break;
  case 3:  race = RACE_GNOME      ; break;
  case 4:  race = RACE_DWARF      ; break;
  case 5:  race = RACE_HALF_ELF   ; break;
  case 6:  race = RACE_HALFLING   ; break;
  case 7:  race = RACE_DROW_ELF   ; break;
  case 8:  race = RACE_HALF_ORC   ; break;
  case 9:  race = RACE_ANIMAL     ; break;
  case 10: race = RACE_CONSTRUCT  ; break;
  case 11: race = RACE_DEMON      ; break;
  case 12: race = RACE_DRAGON     ; break;
  case 13: race = RACE_FISH       ; break;
  case 14: race = RACE_GIANT      ; break;
  case 15: race = RACE_GOBLIN     ; break;
  case 16: race = RACE_INSECT     ; break;
  case 17: race = RACE_ORC        ; break;
  case 18: race = RACE_SNAKE      ; break;
  case 19: race = RACE_TROLL      ; break;
  case 20: race = RACE_MINOTAUR   ; break;
  case 21: race = RACE_KOBOLD     ; break;
  case 22: race = RACE_MINDFLAYER ; break;
  case 23: race = RACE_WARHOST    ; break;
  case 24: race = RACE_FAERIE     ; break;
  default: race = RACE_UNDEFINED  ; break;
  }
  if (race >= 0 && race < NUM_RACES)
    if (!race_ok_gender[(int)GET_SEX(ch)][race])
      race = RACE_UNDEFINED;

  return (race);
}


int racial_ability_mods[][6] = {
/*                      Str,Con,Int,Wis,Dex,Cha */
/* RACE_HUMAN       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_ELF         */ {  0, -2,  0,  0,  2,  0 },
/* RACE_GNOME       */ { -2,  2,  0,  0,  0,  0 },
/* RACE_DWARF       */ {  0,  2,  0,  0,  0, -2 },
/* RACE_HALF_ELF    */ {  0,  0,  0,  0,  0,  0 },
/* RACE_HALFLING    */ { -2,  0,  0,  0,  2,  0 },
/* RACE_DROW_ELF    */ {  0, -2,  2,  0,  2,  2 },
/* RACE_HALF_ORC    */ {  2,  0, -2,  0,  0, -2 },
/* RACE_ANIMAL      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_CONSTRUCT   */ {  0,  0,  0,  0,  0,  0 },
/* RACE_DEMON       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_DRAGON      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_FISH        */ {  0,  0,  0,  0,  0,  0 },
/* RACE_GIANT       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_GOBLIN      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_INSECT      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_ORC         */ {  0,  0,  0,  0,  0,  0 },
/* RACE_SNAKE       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_TROLL       */ {  0,  0,  0,  0,  0,  0 },
/* RACE_MINOTAUR    */ {  0,  0,  0,  0,  0,  0 },
/* RACE_KOBOLD      */ {  0,  0,  0,  0,  0,  0 },
/* RACE_MINDFLAYER  */ {  0,  0,  0,  0,  0,  0 },
/* RACE_WARHOST     */ {  0,  0,  0,  0,  0,  0 },
/* RACE_FAERIE      */ {  0,  0,  0,  0,  0,  0 },
{ 0, 0, 0, 0, 0}
};

void racial_ability_modifiers(struct char_data *ch)
{
  int chrace = 0;
  if (GET_RACE(ch) >= NUM_RACES || GET_RACE(ch) < 0) {
    log("SYSERR: Unknown race %d in racial_ability_modifiers", GET_RACE(ch));
  } else {
    chrace = GET_RACE(ch);
  }

  ch->real_abils.str += racial_ability_mods[chrace][0];
  ch->real_abils.con += racial_ability_mods[chrace][1];
  ch->real_abils.intel += racial_ability_mods[chrace][2];
  ch->real_abils.wis += racial_ability_mods[chrace][3];
  ch->real_abils.dex += racial_ability_mods[chrace][4];
}


/* Converted into metric units: cm and kg; SRD has english units. */
struct {
  int height[NUM_SEX];	/* cm */
  int heightdie;	/* 2d(heightdie) added to height */
  int weight[NUM_SEX];	/* kg */
  int weightfac;	/* added height * weightfac/100 added to weight */
} hw_info[NUM_RACES] = {
/* RACE_HUMAN      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ELF        */ { {135, 135, 135}, 15, {37, 39, 36}, 63},
/* RACE_GNOME      */ { {93, 91, 96}, 10, {17, 18, 16}, 18},
/* RACE_DWARF      */ { {111, 114, 109}, 10, {52, 59, 45}, 125},
/* RACE_HALF_ELF   */ { {137, 140, 135}, 20, {40, 45, 36}, 89},
/* RACE_HALFLING   */ { {78, 81, 76}, 10, {12, 14, 11}, 18},
/* RACE_DROW_ELF   */ { {135, 135, 135}, 15, {37, 39, 36}, 63},
/* RACE_HALF_ORC   */ { {141, 147, 135}, 30, {59, 68, 50}, 125},
/* RACE_ANIMAL     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_CONSTRUCT  */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_DEMON      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_DRAGON     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_FISH       */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_GIANT      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_GOBLIN     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_INSECT     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_ORC        */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_SNAKE      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_TROLL      */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MINOTAUR   */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_KOBOLD     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_MINDFLAYER */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_WARHOST    */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
/* RACE_FAERIE     */ { {141, 147, 135}, 26, {46, 54, 39}, 89},
};


void set_height_and_weight_by_race(struct char_data *ch)
{
  int race, sex, mod;

  race = GET_RACE(ch);
  sex = GET_SEX(ch);
  if (sex < SEX_NEUTRAL || sex >= NUM_SEX) {
    log("Invalid gender in set_height_and_weight_by_race: %d", sex);
    sex = SEX_NEUTRAL;
  }
  if (race <= RACE_UNDEFINED || race >= NUM_RACES) {
    log("Invalid gender in set_height_and_weight_by_race: %d", GET_SEX(ch));
    race = RACE_UNDEFINED + 1; /* first defined race */
  }

  mod = dice(2, hw_info[race].heightdie);
  GET_HEIGHT(ch) = hw_info[race].height[sex] + mod;
  mod *= hw_info[race].weightfac;
  mod /= 100;
  GET_WEIGHT(ch) = hw_info[race].weight[sex] + mod;
}


int invalid_race(struct char_data *ch, struct obj_data *obj)
{
  if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT)
    return FALSE;

  if (OBJ_FLAGGED(obj, ITEM_ANTI_HUMAN) && IS_HUMAN(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_ELF) && IS_ELF(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_DWARF) && IS_DWARF(ch))
    return (TRUE);

  if (OBJ_FLAGGED(obj, ITEM_ANTI_GNOME) && IS_GNOME(ch))
    return (TRUE);

  return (FALSE);
}


const int race_def_sizetable[NUM_RACES + 1] =
{
/* HUMAN */	SIZE_MEDIUM,
/* ELF */	SIZE_MEDIUM,
/* GNOME */	SIZE_SMALL,
/* DWARF */	SIZE_MEDIUM,
/* HALF_ELF */	SIZE_MEDIUM,
/* HALFLING */	SIZE_SMALL,
/* DROW_ELF */	SIZE_MEDIUM,
/* HALF_ORC */	SIZE_MEDIUM,
/* ANIMAL */	SIZE_MEDIUM,
/* CONSTRUCT */	SIZE_MEDIUM,
/* DEMON */	SIZE_MEDIUM,
/* DRAGON */	SIZE_HUGE,
/* FISH */	SIZE_TINY,
/* GIANT */	SIZE_LARGE,
/* GOBLIN */	SIZE_MEDIUM,
/* INSECT */	SIZE_FINE,
/* ORC */	SIZE_LARGE,
/* SNAKE */	SIZE_MEDIUM,
/* TROLL */	SIZE_LARGE,
/* MINOTAUR */	SIZE_LARGE,
/* KOBOLD */	SIZE_MEDIUM,
/* MINDFLAYER */SIZE_MEDIUM,
/* WARHOST */	SIZE_MEDIUM,
/* FAERIE */	SIZE_TINY
};


int get_size(struct char_data *ch)
{
  int racenum;

  if (ch->size != SIZE_UNDEFINED)
    return ch->size;
  else {
    racenum = GET_RACE(ch);
    if (racenum < 0 || racenum >= NUM_RACES)
      return SIZE_MEDIUM;
    return (ch->size = race_def_sizetable[racenum]);
  }
}


const int size_bonus_table[NUM_SIZES] = {
/* XTINY */	8,
/* TINY */	4,
/* XSMALL */	2,
/* SMALL */	1,
/* MEDIUM */	0,
/* LARGE */	-1,
/* HUGE */	-2,
/* GIGANTIC */	-4,
/* COLOSSAL */	-8
};


const int get_size_bonus(int sz)
{
  if (sz < 0 || sz >= NUM_SIZES)
    sz = SIZE_MEDIUM;
  return size_bonus_table[sz];
}


const int wield_type(int chsize, const struct obj_data *weap)
{
  if (GET_OBJ_TYPE(weap) != ITEM_WEAPON) {
    return OBJ_FLAGGED(weap, ITEM_2H) ? WIELD_TWOHAND : WIELD_ONEHAND;
  } else if (chsize > GET_OBJ_SIZE(weap)) {
    return WIELD_LIGHT;
  } else if (chsize == GET_OBJ_SIZE(weap)) {
    return WIELD_ONEHAND;
  } else if (chsize == GET_OBJ_SIZE(weap) - 1) {
    return WIELD_TWOHAND;
  } else if (chsize < GET_OBJ_SIZE(weap) - 1) {
    return WIELD_NONE; /* It's just too big for you! */
  } else {
    log("unknown size vector in wield_type: chsize=%d, weapsize=%d", chsize, GET_OBJ_SIZE(weap));
    return WIELD_NONE;
  }
}
