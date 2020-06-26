
/*============================================================================
  
  libsolunar 
  
  solunaryearummary.h 

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/

#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <assert.h>
#include <math.h>
#include <libsolunar/moonephemera.h>
#include <libsolunar/solunaryearsummary.h>
#include <libsolunar/festival.h>
#include <klib/klog.h>

#define KLOG_CLASS "libsolunar.solunaryearsummary"

static void solunar_year_summary_calculate_dst (SolunarYearSummary *self,
             int year); // FWD
static void solunar_year_summary_calculate_moons (SolunarYearSummary *self,
    int year, int latitude); // FWD

static const int SEC_PER_DAY = 3600 * 24;

/*============================================================================
 
  SolunarYearSummary 

  ==========================================================================*/
struct _SolunarYearSummary
  {
  KList *list;
  int year;
  double latitude;
  char *tz;
  };

/*============================================================================
 
  solunar_year_summary_create 

  ==========================================================================*/
SolunarYearSummary *solunar_year_summary_create 
        (int year, double latitude, const char *tz)
  {
  KLOG_IN
  SolunarYearSummary *self = malloc (sizeof (SolunarYearSummary));
  memset (self, 0, sizeof (SolunarYearSummary));

  if (tz)
    self->tz = strdup (tz);
  self->latitude = latitude;
  self->year = year;

  self->list = klist_new_empty ((KListFreeFn) festival_destroy);

  klist_append (self->list, 
    festival_get_shrove_tuesday (year, tz));

  klist_append (self->list, 
    festival_get_ash_wednesday (year, tz));

  klist_append (self->list, 
    festival_get_mothering_sunday (year, tz));

  klist_append (self->list, 
    festival_get_palm_sunday (year, tz));

  klist_append (self->list, 
    festival_get_maundy_thursday (year, tz));

  klist_append (self->list, 
    festival_get_good_friday (year, tz));

  klist_append (self->list, 
    festival_get_easter_sunday (year, tz));

  klist_append (self->list, 
    festival_get_easter_monday (year, tz));

  klist_append (self->list, 
    festival_get_whitsun (year, tz));

  klist_append (self->list, 
    festival_get_vernal_equinox (year));

  klist_append (self->list, 
    festival_get_summer_solstice (year, latitude <= 0));

  klist_append (self->list, 
    festival_get_autumnal_equinox (year));

  klist_append (self->list, 
    festival_get_winter_solstice (year, latitude <= 0));

  solunar_year_summary_calculate_dst (self, year);
  solunar_year_summary_calculate_moons (self, year, latitude);

#ifdef __GLIBC__
  // klist_sort is only defined on glibc systems, because it uses a 
  //  specific glibc function :/
  klist_sort (self->list, festival_sort_fn, NULL);
#endif

  KLOG_OUT
  return self;
  }

/*============================================================================
 
  solunar_year_summary_destroy

  ==========================================================================*/
void solunar_year_summary_destroy (SolunarYearSummary *self)
  {
  KLOG_IN
  if (self)
    {
    if (self->list) klist_destroy (self->list);
    if (self->tz) free (self->tz);
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
 
  solunar_year_summary_calculate_dst

  ==========================================================================*/
static void solunar_year_summary_calculate_dst (SolunarYearSummary *self,
    int year)
  {
  KLOG_IN
  int days_in_year = 365; // DST probably never changes on the last day
                          //  in a leap year
  // We use a base time of 3:00 AM, as most locations switch over
  //  DST before this (usually 1-2 AM)
  // Start the test on the last day of the previous year, to deal with
  //  cases where the year starts in DST (e.g., Australia)
  time_t soy = datetimeconv_maketime (year - 1, 12, 31, 
         3, 0, 0, self->tz);
  BOOL last_dst = FALSE;
  for (int i = 0; i < days_in_year; i++)
    {
    struct tm tm;

    datetimeconv_localtime (&soy, &tm, self->tz);

    if (tm.tm_year + 1900 == year)
      {
      if (last_dst && !tm.tm_isdst)
        {
        Festival *f = festival_new (soy, FALSE, 
           "Daylight saving ends");
        klist_append (self->list, f);
        }
      if (!last_dst && tm.tm_isdst)
        {
        Festival *f = festival_new (soy, FALSE, 
           "Daylight saving starts");
        klist_append (self->list, f);
        }
      }
    last_dst = tm.tm_isdst;
    soy += SEC_PER_DAY;
    }

  KLOG_OUT
  }


/*============================================================================
 
  solunar_year_summary_get_festivals

  ==========================================================================*/
static void solunar_year_summary_calculate_moons (SolunarYearSummary *self,
    int year, int latitude)
  {
  KLOG_IN
  int days_in_year = 365; //TODO
  time_t soy = datetimeconv_maketime (year, 1, 1, 
         12, 0, 0, self->tz);
  for (int i = 0; i < days_in_year; i++)
    {
    double phase, distance, age;
    const char *dummys;
    int moon_flags;
    moonephemera_get_moon_state (latitude, 0, 
               soy, &dummys, 
               &phase, &age, &distance, &moon_flags);

     if (moon_flags & MOONFLAG_SUPERMOON)
       {
        Festival *f = festival_new (soy, FALSE, 
           "Supermoon");
        klist_append (self->list, f);
       }

    soy += SEC_PER_DAY;
    }
  KLOG_OUT
  }


/*============================================================================
 
  solunar_year_summary_get_festivals

  ==========================================================================*/
const KList *solunar_year_summary_get_festivals 
        (const SolunarYearSummary *self)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->list != NULL);
  KList *ret = self->list;
  KLOG_OUT
  return ret;
  }

/*============================================================================
 
  solunar_year_summary_get_latitude

  ==========================================================================*/
double solunar_year_summary_get_latitude
        (const SolunarYearSummary *self)
  {
  KLOG_IN
  assert (self != NULL);
  double ret = self->latitude;
  KLOG_OUT
  return ret;
  }

/*============================================================================
 
  solunar_year_summary_get_timezone

  ==========================================================================*/
const char *solunar_year_summary_get_timezone
        (const SolunarYearSummary *self)
  {
  KLOG_IN
  assert (self != NULL);
  const char *ret = self->tz;
  KLOG_OUT
  return ret;
  }


/*============================================================================
 
  solunar_year_summary_to_json

  ==========================================================================*/
KString *solunar_year_summary_to_json (const SolunarYearSummary *self)
  {
  KLOG_IN
  assert (self != NULL);
  KString *json = kstring_new_empty();
  int l = klist_length (self->list); 
  kstring_append_utf8 (json, (UTF8 *)"[");
  for (int i = 0; i < l; i++)
    {
    Festival *f = klist_get (self->list, i);
    kstring_append_utf8 (json, (UTF8 *)"{");
    const char *name = festival_get_name (f);
    time_t date = festival_get_date (f);

    struct tm tm;
    localtime_r (&date, &tm);  

    KString *ds = kstring_new_empty();
    kstring_append_printf (ds, "%04d-%02d-%02d", 
      tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday); 
        
    if (festival_has_time (f))
      {
      kstring_append_printf (ds, " (%02d:%02d)", tm.tm_hour, tm.tm_min);
      }

    kstring_append_printf (json, "\"name\":\"%s\",", name); 
    kstring_append_utf8 (json, (UTF8 *)"\"date\":\"");
    kstring_append (json, ds); 
    kstring_append_utf8 (json, (UTF8 *)"\"");
    kstring_destroy (ds);
    kstring_append_utf8 (json, (UTF8 *)"}");
    if (i != l - 1)
      kstring_append_utf8 (json, (UTF8 *)",\n");
    }

  kstring_append_utf8 (json, (UTF8 *)"]\n");
  KLOG_OUT
  return json;
  }

/*============================================================================
 
  solunar_year_summary_to_string

  ==========================================================================*/
KString *solunar_year_summary_to_string (const SolunarYearSummary *self)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->list != NULL);
  KString *s = kstring_new_empty();
  
  int l = klist_length (self->list); 
  for (int i = 0; i < l; i++)
    {
    Festival *f = klist_get (self->list, i);
    KString *ss = festival_to_string (f, self->tz);
    kstring_append (s, ss); 
    kstring_append_utf8 (s, (UTF8 *)"\n"); 
    kstring_destroy (ss);
    }

  KLOG_OUT
  return s;
  }

