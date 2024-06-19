/*============================================================================
  
  solunar2
  
  program.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <string.h> 
#include <errno.h> 
#include <klib/klib.h> 
#include <libsolunar/libsolunar.h> 
#include "program_context.h" 
#include "program.h" 

#define KLOG_CLASS "solunar.program"

#define HAS_OPTION(x) program_context_get_boolean(context,x,FALSE)
#define GET_INTEGER(x,y) program_context_get_integer(context,x,y)
#define GET(x) program_context_get(context,x)

BOOL program_get_lat (const ProgramContext *context, double *lat); // FWD
BOOL program_get_longt (const ProgramContext *context, double *longt); // FWD
char *program_get_tz (const ProgramContext *context); //FWD
static void program_format_day_summary (const ProgramContext *context, 
              const SolunarDaySummary *sds); // FWD

/*============================================================================
  
  program_format_year_summary

  ==========================================================================*/
void program_format_year_summary (const ProgramContext *context, 
      const SolunarYearSummary *sys, const char *tz)
  {
  KLOG_IN
  const KList *list = solunar_year_summary_get_festivals (sys);
  int l = klist_length (list); 
  for (int i = 0; i < l; i++)
    {
    Festival *f = klist_get (list, i);
    time_t date = festival_get_date (f);
    BOOL has_time = festival_has_time (f);
    const char *name = festival_get_name (f);
    char *s;
    if (has_time)
      s = datetimeconv_format_time ("%a %b %d %Y (%H:%M)", tz, date);
    else
      s = datetimeconv_format_time ("%a %b %d %Y        ", tz, date);
    
    printf ("%s %s\n", s, name);
    free (s);
    }

  KLOG_OUT
  }


/*============================================================================
  
  program_days

  Handle the --days option by printing a year summary

  ==========================================================================*/
int program_days (const ProgramContext *context)
  {
  KLOG_IN
  char *tz = program_get_tz (context);
  int days_year = GET_INTEGER("days-year",-1);
  if (days_year == -1)
    days_year = datetimeconv_get_current_year (tz); 
  double lat = 51.0; // Assume northern hemisphere if not given
  program_get_lat (context, &lat);

  BOOL json = HAS_OPTION ("json");

  SolunarYearSummary *sys = solunar_year_summary_create 
        (days_year, lat, tz);

  if (json)
    {
    KString *s = solunar_year_summary_to_json (sys);
    printf ("%S\n", kstring_cstr(s));
    kstring_destroy (s);
    }
  else
    program_format_year_summary (context, sys, tz);

  solunar_year_summary_destroy (sys);
  if (tz) free (tz);
  KLOG_OUT
  return 0;
  }

/*============================================================================
  
  program_day_summary

  ==========================================================================*/
int program_day_summary (const ProgramContext *context)
  {
  KLOG_IN
  int ret = 0;
  char *tz = program_get_tz (context);
  double lat, longt;
  BOOL has_lat = program_get_lat (context, &lat);
  BOOL has_longt = program_get_longt (context, &longt);
  const SolCity *c = program_context_get_city (context);
  // Note: c may be NULL here
  BOOL json = HAS_OPTION ("json");

  const char *city = NULL;
  if (c) 
    city = solcity_get_name (c); 

  if (has_lat && has_longt)
    {
    if (!tz)
      {
      klog_warn (KLOG_CLASS, "Using system timezone");
      }

    char *date = GET ("date");
    time_t d;
    if (date)
      {
      // We checked earlier that the date parsed OK
      d = datetimeconv_parse_date (date, 0, 0, tz);
      free (date);
      }
    else
      {
      d = time (NULL);
      d = datetimeconv_make_time_on_day (d, 12, 
                0, 0, tz);
      }

    SolunarDaySummary *sds = solunar_day_summary_create 
      (d, lat, longt, city, tz);
    if (json)
      {
      KString *s = solunar_day_summary_to_json (sds);
      printf ("%S\n", kstring_cstr(s));
      kstring_destroy (s);
      }
    else
      program_format_day_summary (context, sds); 

    solunar_day_summary_destroy (sds);
    }
  else
    {
    klog_error (KLOG_CLASS, 
  "No location specified. Specify a city using the --city switch, or\n"
  "  latitute and longitude in degrees using --lat and --long. If you\n"
  "  specify latitude and longitude, you'll need to specify a timezone\n"
  "  as well. These settings can also be placed in $HOME/.solunar.rc");
    ret = EINVAL;
    }

  if (tz) free (tz);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_format_day_summary

  ==========================================================================*/
static void program_format_day_summary (const ProgramContext *context, 
              const SolunarDaySummary *sds)
  {
  KLOG_IN

  BOOL full = HAS_OPTION ("full");

  const char *display_city = solunar_day_summary_get_city (sds);
  const char *tz_city = solunar_day_summary_get_tz_city (sds);
  if (display_city)
    printf ("%s\n", display_city);
  double display_lat = solunar_day_summary_get_latitude (sds);
  double display_long = solunar_day_summary_get_longitude (sds);
  if (display_lat > 0)
    printf ("%.1fN, ", display_lat);
  else
    printf ("%.1fS, ", -display_lat);
  if (display_long > 0)
    printf ("%.1fE", display_long);
  else
    printf ("%.1fW", -display_long);
  printf ("\n");
  time_t date = solunar_day_summary_get_date (sds); 
  char *s = datetimeconv_format_time ("long_date", tz_city, date);
  printf ("%s\n", s);
  free (s);

  printf ("Sun:\n");

  const char *clocktime = "24hr";
  if (HAS_OPTION ("ampm")) clocktime = "12hr";

  time_t t;
  if (full)
    {
    t = solunar_day_summary_get_start_astronomical_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  Start of astronomical twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No astronomical twilight\n");

    t = solunar_day_summary_get_start_nautical_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  Start of nautical twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No nautical twilight\n");

    t = solunar_day_summary_get_start_civil_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  Start of civil twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No civil twilight\n");
    }

  t = solunar_day_summary_get_sunrise (sds);
  if (t)
    {
    char *s = datetimeconv_format_time (clocktime, tz_city, t);
    printf ("  Sunrise %s\n", s);
    free (s);
    }
  else
    printf ("  No sunrise\n");

  if (full)
    {
    t = solunar_day_summary_get_high_noon (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  High noon %s\n", s);
      free (s);
      double a = solunar_day_summary_get_sun_max_altitude (sds);
      printf ("  Sun's altitude at high noon %g deg\n", a);
      }
    else
      printf ("  No high noon\n");
      }

  t = solunar_day_summary_get_sunset (sds);
  if (t)
    {
    char *s = datetimeconv_format_time (clocktime, tz_city, t);
    printf ("  Sunset %s\n", s);
    free (s);
    }
  else
    printf ("  No sunset\n");

  if (full)
    {
    t = solunar_day_summary_get_end_civil_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  End of civil twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No civil twilight\n");

    t = solunar_day_summary_get_end_nautical_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  End of nautical twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No nautical twilight\n");

    t = solunar_day_summary_get_end_astronomical_twilight (sds);
    if (t)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, t);
      printf ("  End of astronomical twilight %s\n", s);
      free (s);
      }
    else
      printf ("  No astronomical twilight\n");
    }

  printf ("Moon:\n");

  int nrises = solunar_day_summary_get_n_rises (sds);
  if (nrises > 0)
    {
    printf ("  Moonrise "); 
    for (int i = 0; i < nrises; i++)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, 
	 solunar_day_summary_get_moon_rise (sds, i));
      printf ("%s  ", s); 
      free (s);
      }
    printf ("\n"); 
    }
  else
    printf ("  No moonrises on this day\n");

  int nsets = solunar_day_summary_get_n_sets (sds);
  if (nsets > 0)
    {
    printf ("  Moonset "); 
    for (int i = 0; i < nsets; i++)
      {
      char *s = datetimeconv_format_time (clocktime, tz_city, 
	 solunar_day_summary_get_moon_set (sds, i));
      printf ("%s ", s); 
      free (s);
      }
    printf ("\n"); 
    }
  else
    printf ("  No moonsets on this day\n");

  double moon_age = solunar_day_summary_get_moon_age (sds);
  double moon_phase = solunar_day_summary_get_moon_phase (sds);
  double moon_distance = solunar_day_summary_get_moon_distance (sds);
  const char *moon_phase_name 
	   = solunar_day_summary_get_moon_phase_name (sds);

  printf ("  moon phase %.2f, %s\n", moon_phase, moon_phase_name);
  int moon_flags = solunar_day_summary_get_moon_flags (sds);
  if (moon_flags) 
    {
    if (moon_flags & MOONFLAG_PERIGEE)
      printf ("  perigee");
    if (moon_flags & MOONFLAG_SUPERMOON)
      printf (", supermoon");
    printf ("\n");
    }
  if (full)
    {
    printf ("  moon age %.1f days since new\n", moon_age);
    printf ("  moon distance %g km\n", moon_distance); 
    }

  KLOG_OUT
  }

/*============================================================================
  
  program_get_longt

  Get the longitude from the command line if given, else from the cityy
  if given, else return FALSE.

  If no longitude is specified, the supplied *longt arg is not overwritten
  ==========================================================================*/
BOOL program_get_longt (const ProgramContext *context, double *longt)
  {
  KLOG_IN
  BOOL ret = FALSE;

  char *s_longt = GET("longitude");
  if (s_longt)
    {
    double v;
    if (numberformat_read_double (s_longt, &v, FALSE))
      {
      *longt = v;
      ret = TRUE; 
      }
    else
      {
      klog_error (KLOG_CLASS, "Invalid longitude: %s", s_longt);
      ret = FALSE; 
      }
    free (s_longt);
    ret = TRUE;
    }

  if (!ret)
    {
    const SolCity *c = program_context_get_city (context);
    if (c)
      {
      *longt = solcity_get_longitude (c);
      ret = TRUE; 
      }
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_get_lat

  Get the latitute from the command line if given, else from the cityy
  if given, else return FALSE.

  If no latitute is specified, the supplied *lat arg is not overwritten
  ==========================================================================*/

BOOL program_get_lat (const ProgramContext *context, double *lat)
  {
  KLOG_IN
  BOOL ret = FALSE;

  char *s_lat = GET("latitude");
  if (s_lat)
    {
    double v;
    if (numberformat_read_double (s_lat, &v, FALSE))
      {
      *lat = v;
      ret = TRUE; 
      }
    else
      {
      klog_error (KLOG_CLASS, "Invalid latitude: %s", s_lat);
      ret = FALSE; 
      }
    free (s_lat);
    ret = TRUE;
    }

  if (!ret)
    {
    const SolCity *c = program_context_get_city (context);
    if (c)
      {
      *lat = solcity_get_latitude (c);
      ret = TRUE; 
      }
    }

  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_get_tz

  Get the timezone. Use the value on the command-line (or RC file) if
  given, else the value in the city, if given, else NULL. Note that if
  the return value is not NULL, the caller must free it.

  ==========================================================================*/
char *program_get_tz (const ProgramContext *context)
  {
  KLOG_IN
  char *ret = NULL;
  klog_debug (KLOG_CLASS, "Getting timezone for calculation");

  ret = program_context_get (context, "tz");  

  // If the specified timezone is sys, we must return NULL, so the program
  //   behaves as if no timezone was set. If we don't make this rather
  //   ugly check, the the city timezone will take effect even when the
  //   user has specified the sys timezone.
  // Throughout the program we use NULL to indicate the default timezone,
  //   but there's no way to specify NULL on the shell command-line ;)
  if (ret)
    {
    if (strstr (ret, "sys") || strstr (ret, "system"))
      {
      free (ret);
      ret = NULL;
      goto done; // ugh
      }
    }

  if (!ret)
    {
    const SolCity *c = program_context_get_city (context);
    if (c)
      {
      ret = strdup (solcity_get_tz_name (c));
      }
    }

  KLOG_IN
  done:
  return ret;
  }

/*============================================================================
  
  program_log_handler

  ==========================================================================*/
void program_log_handler (KLogLevel level, const char *cls, 
                  void *user_data, const char *msg)
  {
  if (level == KLOG_ERROR)
    fprintf (stderr, "%s: %s\n", NAME, msg);
  else
    fprintf (stderr, "%s %s: %s\n", klog_level_to_utf8 (level), 
      cls, msg);
  }



/*============================================================================
  
  program_run 

  ==========================================================================*/
int program_run (const ProgramContext *context)
  {
  KLOG_IN
  int ret = 0;
  klog_set_handler (program_log_handler);

  if (HAS_OPTION ("days"))
    {
    ret = program_days (context);
    }
  else
    {
    ret = program_day_summary (context);
    }

  KLOG_OUT
  return ret;
  }



