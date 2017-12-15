/* ************************************************************************
*   File: utils.c                                       Part of CircleMUD *
*  Usage: various internal functions of a utility nature                  *
*                                                                         *
*  All rights reserved.  See license.doc for complete information.        *
*                                                                         *
*  Copyright (C) 1993, 94 by the Trustees of the Johns Hopkins University *
*  CircleMUD is based on DikuMUD, Copyright (C) 1990, 1991.               *
************************************************************************ */

#include "conf.h"
#include "sysdep.h"

CVSHEADER("$CVSHeader: cwg/rasputin/src/utils.c,v 1.9 2005/01/04 21:24:29 fnord Exp $");

#include "structs.h"
#include "utils.h"
#include "db.h"
#include "comm.h"
#include "screen.h"
#include "spells.h"
#include "handler.h"
#include "interpreter.h"
#include "feats.h"
#include "constants.h"


/* external globals */
extern struct time_data time_info;

/* local functions */
struct time_info_data *real_time_passed(time_t t2, time_t t1);
struct time_info_data *mud_time_passed(time_t t2, time_t t1);
void prune_crlf(char *txt);
int count_metamagic_feats(struct char_data *ch);


/* creates a random number in interval [from;to] */
int rand_number(int from, int to)
{
  /* error checking in case people call this incorrectly */
  if (from > to) {
    int tmp = from;
    from = to;
    to = tmp;
    log("SYSERR: rand_number() should be called with lowest, then highest. (%d, %d), not (%d, %d).", from, to, to, from);
  }

  /*
   * This should always be of the form:
   *
   *	((float)(to - from + 1) * rand() / (float)(RAND_MAX + from) + from);
   *
   * if you are using rand() due to historical non-randomness of the
   * lower bits in older implementations.  We always use circle_random()
   * though, which shouldn't have that problem. Mean and standard
   * deviation of both are identical (within the realm of statistical
   * identity) if the rand() implementation is non-broken.
   */
  return ((circle_random() % (to - from + 1)) + from);
}


/* simulates dice roll */
int dice(int num, int size)
{
  int sum = 0;

  if (size <= 0 || num <= 0)
    return (0);

  while (num-- > 0)
    sum += rand_number(1, size);

  return (sum);
}


/* Be wary of sign issues with this. */
int MIN(int a, int b)
{
  return (a < b ? a : b);
}

/* Be wary of sign issues with this. */
int MAX(int a, int b)
{
  return (a > b ? a : b);
}


char *CAP(char *txt)
{
  *txt = UPPER(*txt);
  return (txt);
}


#if !defined(HAVE_STRLCPY)
/*
 * A 'strlcpy' function in the same fashion as 'strdup' below.
 *
 * This copies up to totalsize - 1 bytes from the source string, placing
 * them and a trailing NUL into the destination string.
 *
 * Returns the total length of the string it tried to copy, not including
 * the trailing NUL.  So a '>= totalsize' test says it was truncated.
 * (Note that you may have _expected_ truncation because you only wanted
 * a few characters from the source string.)
 */
size_t strlcpy(char *dest, const char *source, size_t totalsize)
{
  strncpy(dest, source, totalsize - 1);	/* strncpy: OK (we must assume 'totalsize' is correct) */
  dest[totalsize - 1] = '\0';
  return strlen(source);
}
#endif


#if !defined(HAVE_STRDUP)
/* Create a duplicate of a string */
char *strdup(const char *source)
{
  char *new_z;

  CREATE(new_z, char, strlen(source) + 1);
  return (strcpy(new_z, source)); /* strcpy: OK */
}
#endif


/*
 * Strips \r\n from end of string.
 */
void prune_crlf(char *txt)
{
  int i = strlen(txt) - 1;

  while (txt[i] == '\n' || txt[i] == '\r')
    txt[i--] = '\0';
}


#ifndef str_cmp
/*
 * str_cmp: a case-insensitive version of strcmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different or we reach the end of both.
 */
int str_cmp(const char *arg1, const char *arg2)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: str_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; arg1[i] || arg2[i]; i++)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


#ifndef strn_cmp
/*
 * strn_cmp: a case-insensitive version of strncmp().
 * Returns: 0 if equal, > 0 if arg1 > arg2, or < 0 if arg1 < arg2.
 *
 * Scan until strings are found different, the end of both, or n is reached.
 */
int strn_cmp(const char *arg1, const char *arg2, int n)
{
  int chk, i;

  if (arg1 == NULL || arg2 == NULL) {
    log("SYSERR: strn_cmp() passed a NULL pointer, %p or %p.", arg1, arg2);
    return (0);
  }

  for (i = 0; (arg1[i] || arg2[i]) && (n > 0); i++, n--)
    if ((chk = LOWER(arg1[i]) - LOWER(arg2[i])) != 0)
      return (chk);	/* not equal */

  return (0);
}
#endif


/* log a death trap hit */
void log_death_trap(struct char_data *ch)
{
  mudlog(BRF, ADMLVL_IMMORT, TRUE, "%s hit death trap #%d (%s)", GET_NAME(ch), GET_ROOM_VNUM(IN_ROOM(ch)), world[IN_ROOM(ch)].name);
}


/*
 * New variable argument log() function.  Works the same as the old for
 * previously written code but is very nice for new code.
 */
void basic_mud_vlog(const char *format, va_list args)
{
  time_t ct = time(0);
  char *time_s = asctime(localtime(&ct));

  if (logfile == NULL) {
    puts("SYSERR: Using log() before stream was initialized!");
    return;
  }

  if (format == NULL)
    format = "SYSERR: log() received a NULL format.";

  time_s[strlen(time_s) - 1] = '\0';

  fprintf(logfile, "%-15.15s :: ", time_s + 4);
  vfprintf(logfile, format, args);
  fputc('\n', logfile);
  fflush(logfile);
}


/* So mudlog() can use the same function. */
void basic_mud_log(const char *format, ...)
{
  va_list args;

  va_start(args, format);
  basic_mud_vlog(format, args);
  va_end(args);
}


/* the "touch" command, essentially. */
int touch(const char *path)
{
  FILE *fl;

  if (!(fl = fopen(path, "a"))) {
    log("SYSERR: %s: %s", path, strerror(errno));
    return (-1);
  } else {
    fclose(fl);
    return (0);
  }
}


/*
 * mudlog -- log mud messages to a file & to online imm's syslogs
 * based on syslog by Fen Jul 3, 1992
 */
void mudlog(int type, int level, int file, const char *str, ...)
{
  char buf[MAX_STRING_LENGTH];
  struct descriptor_data *i;
  va_list args;

  if (str == NULL)
    return;	/* eh, oh well. */

  if (file) {
    va_start(args, str);
    basic_mud_vlog(str, args);
    va_end(args);
  }

  if (level < ADMLVL_IMMORT)
    level = ADMLVL_IMMORT;

  strcpy(buf, "[ ");	/* strcpy: OK */
  va_start(args, str);
  vsnprintf(buf + 2, sizeof(buf) - 6, str, args);
  va_end(args);
  strcat(buf, " ]\r\n");	/* strcat: OK */

  for (i = descriptor_list; i; i = i->next) {
    if (STATE(i) != CON_PLAYING || IS_NPC(i->character)) /* switch */
      continue;
    if (GET_ADMLEVEL(i->character) < level)
      continue;
    if (PLR_FLAGGED(i->character, PLR_WRITING))
      continue;
    if (type > (PRF_FLAGGED(i->character, PRF_LOG1) ? 1 : 0) + (PRF_FLAGGED(i->character, PRF_LOG2) ? 2 : 0))
      continue;

    send_to_char(i->character, "@g%s@n", buf);
  }
}



/*
 * If you don't have a 'const' array, just cast it as such.  It's safer
 * to cast a non-const array as const than to cast a const one as non-const.
 * Doesn't really matter since this function doesn't change the array though.
 */
size_t sprintbit(bitvector_t bitvector, const char *names[], char *result, size_t reslen)
{
  size_t len = 0;
  int nlen;
  long nr;

  *result = '\0';

  for (nr = 0; bitvector && len < reslen; bitvector >>= 1) {
    if (IS_SET(bitvector, 1)) {
      nlen = snprintf(result + len, reslen - len, "%s ", *names[nr] != '\n' ? names[nr] : "UNDEFINED");
      if (len + nlen >= reslen || nlen < 0)
        break;
      len += nlen;
    }

    if (*names[nr] != '\n')
      nr++;
  }

  if (!*result)
    len = strlcpy(result, "NOBITS ", reslen);

  return (len);
}


size_t sprinttype(int type, const char *names[], char *result, size_t reslen)
{
  int nr = 0;

  while (type && *names[nr] != '\n') {
    type--;
    nr++;
  }

  return strlcpy(result, *names[nr] != '\n' ? names[nr] : "UNDEFINED", reslen);
}


void sprintbitarray(int bitvector[], const char *names[], int maxar, char *result)
{
  int nr, teller, found = FALSE;

  *result = '\0';

  for(teller = 0; teller < maxar && !found; teller++)
    for (nr = 0; nr < 32 && !found; nr++) {
	  if (IS_SET_AR(bitvector, (teller*32)+nr)) {
        if (*names[(teller*32)+nr] != '\n') {
          if (*names[(teller*32)+nr] != '\0') {

    strcat(result, names[(teller*32)+nr]);

    strcat(result, " ");
          }
		} else {

  strcat(result, "UNDEFINED ");
        }
	  }
      if (*names[(teller*32)+nr] == '\n')
        found = TRUE;
    }

  if (!*result)
    strcpy(result, "NOBITS ");
}


/* Calculate the REAL time passed over the last t2-t1 centuries (secs) */
struct time_info_data *real_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_REAL_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_REAL_HOUR * now.hours;

  now.day = (secs / SECS_PER_REAL_DAY);	/* 0..34 days  */
  /* secs -= SECS_PER_REAL_DAY * now.day; - Not used. */

  now.month = -1;
  now.year = -1;

  return (&now);
}



/* Calculate the MUD time passed over the last t2-t1 centuries (secs) */
struct time_info_data *mud_time_passed(time_t t2, time_t t1)
{
  long secs;
  static struct time_info_data now;

  secs = t2 - t1;

  now.hours = (secs / SECS_PER_MUD_HOUR) % 24;	/* 0..23 hours */
  secs -= SECS_PER_MUD_HOUR * now.hours;

  now.day = (secs / SECS_PER_MUD_DAY) % 30;	/* 0..29 days  */
  secs -= SECS_PER_MUD_DAY * now.day;

  now.month = (secs / SECS_PER_MUD_MONTH) % 12;	/* 0..11 months */
  secs -= SECS_PER_MUD_MONTH * now.month;

  now.year = (secs / SECS_PER_MUD_YEAR);	/* 0..XX? years */

  return (&now);
}


time_t mud_time_to_secs(struct time_info_data *now)
{
  time_t when = 0;

  when += now->year  * SECS_PER_MUD_YEAR;
  when += now->month * SECS_PER_MUD_MONTH;
  when += now->day   * SECS_PER_MUD_DAY;
  when += now->hours * SECS_PER_MUD_HOUR;

  return (time(NULL) - when);
}

struct time_info_data *age(struct char_data *ch)
{
  static struct time_info_data player_age;

  player_age = *mud_time_passed(time(0), ch->time.birth);

  return (&player_age);
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool circle_follow(struct char_data *ch, struct char_data *victim)
{
  struct char_data *k;

  for (k = victim; k; k = k->master) {
    if (k == ch)
      return (TRUE);
  }

  return (FALSE);
}



/* Called when stop following persons, or stopping charm */
/* This will NOT do if a character quits/dies!!          */
void stop_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master == NULL) {
    core_dump();
    return;
  }

    act("You stop following $N.", FALSE, ch, 0, ch->master, TO_CHAR);
    act("$n stops following $N.", TRUE, ch, 0, ch->master, TO_NOTVICT);
  if (!(DEAD(ch->master) || (ch->master->desc && STATE(ch->master->desc) == CON_MENU)))
    act("$n stops following you.", TRUE, ch, 0, ch->master, TO_VICT);

  if (ch->master->followers->follower == ch) {	/* Head of follower-list? */
    k = ch->master->followers;
    ch->master->followers = k->next;
    free(k);
  } else {			/* locate follower who is not head of list */
    for (k = ch->master->followers; k->next->follower != ch; k = k->next);

    j = k->next;
    k->next = j->next;
    free(j);
  }

  ch->master = NULL;
}


int num_followers_charmed(struct char_data *ch)
{
  struct follow_type *lackey;
  int total = 0;

  /* Summoned creatures don't count against total */
  for (lackey = ch->followers; lackey; lackey = lackey->next)
    if (AFF_FLAGGED(lackey->follower, AFF_CHARM) &&
        !AFF_FLAGGED(lackey->follower, AFF_SUMMONED) &&
        lackey->follower->master == ch)
      total++;

  return (total);
}


/* Called when a character that follows/is followed dies */
void die_follower(struct char_data *ch)
{
  struct follow_type *j, *k;

  if (ch->master)
    stop_follower(ch);

  for (k = ch->followers; k; k = j) {
    j = k->next;
    stop_follower(k->follower);
  }
}



/* Do NOT call this before having checked if a circle of followers */
/* will arise. CH will follow leader                               */
void add_follower(struct char_data *ch, struct char_data *leader)
{
  struct follow_type *k;

  if (ch->master) {
    core_dump();
    return;
  }

  ch->master = leader;

  CREATE(k, struct follow_type, 1);

  k->follower = ch;
  k->next = leader->followers;
  leader->followers = k;

  act("You now follow $N.", FALSE, ch, 0, leader, TO_CHAR);
  if (IN_ROOM(ch) != NOWHERE && IN_ROOM(leader) != NOWHERE && CAN_SEE(leader, ch)) {
    act("$n starts following you.", TRUE, ch, 0, leader, TO_VICT);
  act("$n starts to follow $N.", TRUE, ch, 0, leader, TO_NOTVICT);
  }
}


/*
 * get_line reads the next non-blank line off of the input stream.
 * The newline character is removed from the input.  Lines which begin
 * with '*' are considered to be comments.
 *
 * Returns the number of lines advanced in the file. Buffer given must
 * be at least READ_SIZE (256) characters large.
 */
int get_line(FILE *fl, char *buf)
{
  char temp[READ_SIZE];
  int lines = 0;
  int sl;

  do {
    if (!fgets(temp, READ_SIZE, fl))
      return (0);
    lines++;
  } while (*temp == '*' || *temp == '\n' || *temp == '\r');

  /* Last line of file doesn't always have a \n, but it should. */
  sl = strlen(temp);
  while (sl > 0 && (temp[sl - 1] == '\n' || temp[sl - 1] == '\r'))
    temp[--sl] = '\0';

  strcpy(buf, temp); /* strcpy: OK, if buf >= READ_SIZE (256) */
  return (lines);
}


int get_filename(char *filename, size_t fbufsize, int mode, const char *orig_name)
{
  const char *prefix, *middle, *suffix;
  char name[PATH_MAX], *ptr;

  if (orig_name == NULL || *orig_name == '\0' || filename == NULL) {
    log("SYSERR: NULL pointer or empty string passed to get_filename(), %p or %p.",
		orig_name, filename);
    return (0);
  }

  switch (mode) {
  case CRASH_FILE:
    prefix = LIB_PLROBJS;
    suffix = SUF_OBJS;
    break;
  case ALIAS_FILE:
    prefix = LIB_PLRALIAS;
    suffix = SUF_ALIAS;
    break;
  case ETEXT_FILE:
    prefix = LIB_PLRTEXT;
    suffix = SUF_TEXT;
    break;
  case SCRIPT_VARS_FILE:
    prefix = LIB_PLRVARS;
    suffix = SUF_MEM;
    break;
  case NEW_OBJ_FILES:
    prefix = LIB_PLROBJS;
    suffix = "new";
    break;
  case PLR_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PLR;
    break;
  case PET_FILE:
    prefix = LIB_PLRFILES;
    suffix = SUF_PET;
    break;
  default:
    return (0);
  }

  strlcpy(name, orig_name, sizeof(name));
  for (ptr = name; *ptr; ptr++)
    *ptr = LOWER(*ptr);

  switch (LOWER(*name)) {
  case 'a':  case 'b':  case 'c':  case 'd':  case 'e':
    middle = "A-E";
    break;
  case 'f':  case 'g':  case 'h':  case 'i':  case 'j':
    middle = "F-J";
    break;
  case 'k':  case 'l':  case 'm':  case 'n':  case 'o':
    middle = "K-O";
    break;
  case 'p':  case 'q':  case 'r':  case 's':  case 't':
    middle = "P-T";
    break;
  case 'u':  case 'v':  case 'w':  case 'x':  case 'y':  case 'z':
    middle = "U-Z";
    break;
  default:
    middle = "ZZZ";
    break;
  }

  snprintf(filename, fbufsize, "%s%s"SLASH"%s.%s", prefix, middle, name, suffix);
  return (1);
}


int num_pc_in_room(struct room_data *room)
{
  int i = 0;
  struct char_data *ch;

  for (ch = room->people; ch != NULL; ch = ch->next_in_room)
    if (!IS_NPC(ch))
      i++;

  return (i);
}

/*
 * This function (derived from basic fork(); abort(); idea by Erwin S.
 * Andreasen) causes your MUD to dump core (assuming you can) but
 * continue running.  The core dump will allow post-mortem debugging
 * that is less severe than assert();  Don't call this directly as
 * core_dump_unix() but as simply 'core_dump()' so that it will be
 * excluded from systems not supporting them. (e.g. Windows '95).
 *
 * You still want to call abort() or exit(1) for
 * non-recoverable errors, of course...
 *
 * XXX: Wonder if flushing streams includes sockets?
 */
extern FILE *player_fl;
void core_dump_real(const char *who, int line)
{
  log("SYSERR: Assertion failed at %s:%d!", who, line);

#if 0	/* By default, let's not litter. */
#if defined(CIRCLE_UNIX)
  /* These would be duplicated otherwise...make very sure. */
  fflush(stdout);
  fflush(stderr);
  fflush(logfile);
  fflush(player_fl);
  /* Everything, just in case, for the systems that support it. */
  fflush(NULL);

  /*
   * Kill the child so the debugger or script doesn't think the MUD
   * crashed.  The 'autorun' script would otherwise run it again.
   */
  if (fork() == 0)
    abort();
#endif
#endif
}


/*
 * Rules (unless overridden by ROOM_DARK):
 *
 * Inside and City rooms are always lit.
 * Outside rooms are dark at sunset and night.
 */
int room_is_dark(room_rnum room)
{
  if (!VALID_ROOM_RNUM(room)) {
    log("room_is_dark: Invalid room rnum %d. (0-%d)", room, top_of_world);
    return (FALSE);
  }

  if (world[room].light)
    return (FALSE);

  if (ROOM_FLAGGED(room, ROOM_DARK))
    return (TRUE);

  if (SECT(room) == SECT_INSIDE || SECT(room) == SECT_CITY)
    return (FALSE);

  if (weather_info.sunlight == SUN_SET || weather_info.sunlight == SUN_DARK)
    return (TRUE);

  return (FALSE);
}

int count_metamagic_feats(struct char_data *ch)
{
  int count = 0;                // Number of Metamagic Feats Known

  if (HAS_FEAT(ch, FEAT_STILL_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_SILENT_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_QUICKEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_MAXIMIZE_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_HEIGHTEN_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EXTEND_SPELL))
    count++;

  if (HAS_FEAT(ch, FEAT_EMPOWER_SPELL))
    count++;

  return count;
}

/* General use directory functions & structures. Required due to */
/* various differences between directory handling code on        */
/* different OS'es.  Needs solid testing though.                 */
/* Added by Dynamic Boards v2.4 - PjD (dughi@imaxx.net)          */

#include <sys/types.h>
#include <sys/stat.h>

#ifdef CIRCLE_WINDOWS
#include <direct.h>
/* I shouldn't need to include the following line, right? */
#include <windows.h>

int xdir_scan(char *dir_name, struct xap_dir *xapdirptest) {
  HANDLE dirhandle;
  WIN32_FIND_DATA wtfd;
  int i, total = 0;
  struct xap_dir *xapdirp;

  xapdirp = xapdirptest;

  xapdirp->current = -1;
  xapdirp->total = -1;

  dirhandle = FindFirstFile(dir_name, &wtfd);

  if(dirhandle == INVALID_HANDLE_VALUE) {
    return -1;
  }

  (xapdirp->total)++;
  while(FindNextFile(dirhandle, &wtfd)) {
    (xapdirp->total)++;
  }

  if(GetLastError() != ERROR_NO_MORE_FILES) {
    xapdirp->total = -1;
    return -1;
  }

  FindClose(dirhandle);
  dirhandle = FindFirstFile(dir_name, &wtfd);

  xapdirp->namelist = (char **) malloc(sizeof(char *) * total);

  i = 0;
  while(FindNextFile(dirhandle, &wtfd) != 0) {
    xapdirp->namelist[i] = strdup(wtfd.cFileName);
    i++;
  }
  FindClose(dirhandle);

  xapdirp->current=0;
  return xapdirp->total;
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i];
  }

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1];
}

#else
#include <dirent.h>
#include <unistd.h>

int xdir_scan(char *dir_name, struct xap_dir *xapdirp) {
  xapdirp->total = scandir(dir_name,&(xapdirp->namelist),0,alphasort);
  xapdirp->current = 0;

  return(xapdirp->total);
}

char *xdir_get_name(struct xap_dir *xd,int i) {
  return xd->namelist[i]->d_name;
}

char *xdir_get_next(struct xap_dir *xd) {
  if(++(xd->current) >= xd->total) {
    return NULL;
  }
  return xd->namelist[xd->current-1]->d_name;
}

#endif

void xdir_close(struct xap_dir *xd) {
  int i;
  for(i=0;i < xd->total;i++) {
    free(xd->namelist[i]);
  }
  free(xd->namelist);
  xd->namelist = NULL;
  xd->current = xd->total = -1;
}

int xdir_get_total(struct xap_dir *xd) {
  return xd->total;
}

int insure_directory(char *path, int isfile) {
  char *chopsuey = strdup(path);
  char *p;
  char *temp;
#ifdef CIRCLE_WINDOWS
  struct _stat st;
#else
  struct stat st;
#endif

  extern int errno;

  // if it's a file, remove that, we're only checking dirs;
  if(isfile) {
    if(!(p=strrchr(path,'/'))) {
      free(chopsuey);
      return 1;
    }
    *p = '\0';
  }

  // remove any trailing /'s

  while(chopsuey[strlen(chopsuey)-1] == '/') {
    chopsuey[strlen(chopsuey) -1 ] = '\0';
  }

  // check and see if it's already a dir

  #ifndef S_ISDIR
  #define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
  #endif

  #ifdef CIRCLE_WINDOWS
  if(!_stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
  #else
  if(!stat(chopsuey,&st) && S_ISDIR(st.st_mode)) {
  #endif
    free(chopsuey);
    return 1;
  }

  temp = strdup(chopsuey);
  if((p = strrchr(temp,'/')) != NULL) {
    *p = '\0';
  }
  if(insure_directory(temp,0) &&

#ifdef CIRCLE_WINDOWS
     !_mkdir(chopsuey)) {
#else
     !mkdir(chopsuey, S_IRUSR | S_IWRITE | S_IEXEC | S_IRGRP | S_IXGRP |
            S_IROTH | S_IXOTH)) {
#endif
    free(temp);
    free(chopsuey);
    return 1;
  }

  if(errno == EEXIST &&
#ifdef CIRCLE_WINDOWS
     !_stat(temp,&st)
#else
     !stat(temp,&st)
#endif
     && S_ISDIR(st.st_mode)) {
    free(temp);
    free(chopsuey);
    return 1;
  } else {
    free(temp);
    free(chopsuey);
    return 1;
  }
}


int default_admin_flags_mortal[] =
  { -1 };

int default_admin_flags_immortal[] =
  { ADM_SEEINV, ADM_SEESECRET, ADM_FULLWHERE, ADM_NOPOISON, ADM_WALKANYWHERE,
    ADM_NODAMAGE, ADM_NOSTEAL, -1 };

int default_admin_flags_builder[] =
  { -1 };

int default_admin_flags_god[] =
  { ADM_ALLSHOPS, ADM_TELLALL, ADM_KNOWWEATHER, ADM_MONEY, ADM_EATANYTHING,
    ADM_NOKEYS, -1 };

int default_admin_flags_grgod[] =
  { ADM_TRANSALL, ADM_FORCEMASS, ADM_ALLHOUSES, -1 };

int default_admin_flags_impl[] =
  { ADM_SWITCHMORTAL, ADM_INSTANTKILL, ADM_CEDIT, -1 };

int *default_admin_flags[ADMLVL_IMPL + 1] = {
  default_admin_flags_mortal,
  default_admin_flags_immortal,
  default_admin_flags_builder,
  default_admin_flags_god,
  default_admin_flags_grgod,
  default_admin_flags_impl
};

void admin_set(struct char_data *ch, int value)
{
  void run_autowiz(void);
  int i;
  int orig = GET_ADMLEVEL(ch);

  if (GET_ADMLEVEL(ch) == value)
    return;
  if (GET_ADMLEVEL(ch) < value) { /* Promotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s promoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) < value) {
      GET_ADMLEVEL(ch)++;
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        SET_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
    }
    run_autowiz();
    if (orig < ADMLVL_IMMORT && value >= ADMLVL_IMMORT) {
      SET_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
      SET_BIT_AR(PRF_FLAGS(ch), PRF_AUTOEXIT);
    }
    if (GET_ADMLEVEL(ch) >= ADMLVL_IMMORT) {
      for (i = 0; i < 3; i++)
        GET_COND(ch, i) = (char) -1;
      SET_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
    }
    return;
  }
  if (GET_ADMLEVEL(ch) > value) { /* Demotion */
    mudlog(BRF, MAX(ADMLVL_IMMORT, GET_INVIS_LEV(ch)), TRUE,
           "%s demoted from %s to %s", GET_NAME(ch), admin_level_names[GET_ADMLEVEL(ch)],
           admin_level_names[value]);
    while (GET_ADMLEVEL(ch) > value) {
      for (i = 0; default_admin_flags[GET_ADMLEVEL(ch)][i] != -1; i++)
        REMOVE_BIT_AR(ADM_FLAGS(ch), default_admin_flags[GET_ADMLEVEL(ch)][i]);
      GET_ADMLEVEL(ch)--;
    }
    run_autowiz();
    if (orig >= ADMLVL_IMMORT && value < ADMLVL_IMMORT) {
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG1);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_LOG2);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_NOHASSLE);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_HOLYLIGHT);
      REMOVE_BIT_AR(PRF_FLAGS(ch), PRF_ROOMFLAGS);
    }
    return;
  }
}
