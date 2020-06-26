/*============================================================================
  
  solunar
  
  programcontext.c

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#include <assert.h> 
#include <stdio.h> 
#include <time.h> 
#include <stdlib.h> 
#include <klib/klib.h> 
#include <string.h> 
#include <getopt.h> 
#include <libsolunar/libsolunar.h> 
#include "program_context.h" 

#define KLOG_CLASS "solunar.program_context"

void program_context_show_usage (FILE *f, const char *argv0); // FWD

#define PCP program_context_put
#define PCG program_context_get
#define PCPB program_context_put_boolean
#define PCGB program_context_get_boolean
#define PCPI program_context_put_integer
#define PCGI program_context_get_integer

/*============================================================================
  
  ProgramContext  

  ==========================================================================*/
struct _ProgramContext
  {
  KProps *props;
  int nonswitch_argc;
  char **nonswitch_argv;
  const SolCity *city;
  };

/*============================================================================
  
  program_context_create

  ==========================================================================*/
ProgramContext *program_context_new (void)
  {
  KLOG_IN
  ProgramContext *self = malloc (sizeof (ProgramContext));
  memset (self, 0, sizeof (ProgramContext));
  self->props = kprops_new_empty();
  KLOG_OUT
  return self;
  }

/*============================================================================
  
  program_context_destroy

  ==========================================================================*/
void program_context_destroy (ProgramContext *self)
  {
  KLOG_IN
  if (self)
    {
    if (self->props) kprops_destroy (self->props);
    for (int i = 0; i < self->nonswitch_argc; i++)
      free (self->nonswitch_argv[i]);
    free (self->nonswitch_argv);
    free (self);
    }
  KLOG_OUT
  }

/*============================================================================
  
  program_context_check_and_resolve

  ==========================================================================*/
BOOL program_context_check_and_resolve (ProgramContext *self)
  {
  KLOG_IN
  BOOL ret = TRUE;
  char *city = program_context_get (self, "city");
  if (city)
    {
    klog_debug (KLOG_CLASS, "City set in context is %s", city);
    KList *city_list = solcity_find_matching ((UTF8 *)city);
    if (city_list)
      {
      int cities = klist_length (city_list);
      if (cities == 1)
        {
	const SolCity *c = klist_get (city_list, 0);
        self->city = c;
        }
      else
        {
        klog_error (KLOG_CLASS, "Ambiguous city. Matches:");
        int to_list = cities;
        if (to_list > 5) to_list = 5; 
        for (int i = 0; i < to_list; i++)
          {
	  const SolCity *c = klist_get (city_list, i);
	  const char *full_city = solcity_get_name (c);
          klog_error (KLOG_CLASS, "%s", full_city);
          }
        if (to_list < cities) 
          klog_error (KLOG_CLASS, "and others");
        ret = FALSE;
        }
      klist_destroy (city_list);
      }
    else
      {
      klog_error (KLOG_CLASS, "No city matches '%s'", city);
      ret = FALSE;
      }
    }
  else
    klog_debug (KLOG_CLASS, "No city set in context");
 
  if (ret)
    {
    char *date = PCG (self, "date");
    if (date)
      {
      if (strcmp (date, "help") == 0)
        {
        printf ("Supported formats:\n");
        printf ("  \"jan 21\" or \"21 jan\"\n");
        printf ("  \"jan 21 2021\" or \"21 jan 2021\"\n"); 
        printf ("  \"2021-01-21\"\n");
        printf ("  Month names are not case-sensitive.\n");
        printf ("  Month names can be full or abbreviated.\n");
        printf ("  Use quotes around month: --date=\"jan 21\" or\n");
        printf ("  -d\"jan 21\". If no year is specified, the current\n");
        printf ("  year is used.\n");
        ret = FALSE;
        }
      else
        {
        // Don't worry about timezone here -- we're just checking the
        //   date can actually be parsed. We'll parse it properly later.
        time_t t = datetimeconv_parse_date (date, 0, 0, NULL);
        if (t == 0)
          {
          printf ("Invalid date '%s'.\n", date);
          printf ("'" NAME " --date=help' for format information.\n");
          ret = FALSE;
          }
        }
      free (date);
      }
    }

  if (city) free (city);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get

  Caller must free the string if it is non-null

  ========================================================================*/
char *program_context_get (const ProgramContext *self, 
                   const char *key)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  const KString *val = kprops_get_utf8 (self->props, (UTF8 *)key);
  char *ret = NULL;
  if (val) ret = (char *)kstring_to_utf8 (val);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_boolean

  ========================================================================*/
BOOL program_context_get_boolean (const ProgramContext *self, 
    const char *key, BOOL deflt)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  BOOL ret = kprops_get_boolean_utf8 (self->props, (UTF8 *)key, deflt);
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_city

  ========================================================================*/
const SolCity *program_context_get_city (const ProgramContext *self)
  {
  KLOG_IN
  assert (self != NULL);
  const SolCity *ret = self->city;
  KLOG_OUT
  return ret;
  }

/*==========================================================================

  program_context_get_integer

  ========================================================================*/
BOOL program_context_get_integer (const ProgramContext *self, 
    const char *key, BOOL deflt)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  BOOL ret = kprops_get_integer_utf8 (self->props, (UTF8 *)key, deflt);
  KLOG_OUT
  return ret;
  }

/*============================================================================
  
  program_context_parse_command_line

  ==========================================================================*/
BOOL program_context_parse_command_line (ProgramContext *self, 
        int argc, char **argv)
  {
  KLOG_IN
  BOOL ret = TRUE;
  static struct option long_options[] =
    {
      {"ampm", no_argument, NULL, 'a'},
      {"full", no_argument, NULL, 'f'},
      {"help", no_argument, NULL, 'h'},
      {"city", required_argument, NULL, 'c'},
      {"json", no_argument, NULL, 'j'},
      {"date", required_argument, NULL, 'd'},
      {"list-cities", no_argument, NULL, 0},
      {"tz", required_argument, NULL, 't'},
      {"year", optional_argument, NULL, 'y'},
      {"log-level", required_argument, NULL, 0},
      {"latitude", required_argument, NULL, 'l'},
      {"longitude", required_argument, NULL, 'o'},
      {"version", no_argument, NULL, 'v'},
      {0, 0, 0, 0}
    };

   int opt;
   while (ret)
     {
     int option_index = 0;
     opt = getopt_long (argc, argv, "hvl:o:y::c:t:d:faj",
     long_options, &option_index);

     if (opt == -1) break;

     switch (opt)
       {
       case 0:
         if (strcmp (long_options[option_index].name, "log-level") == 0)
           PCPI (self, "log-level", atoi (optarg));
         else if (strcmp (long_options[option_index].name, "list-cities") == 0)
           PCPB (self, "list-cities", TRUE);
         else
           exit (-1);
         break;
       case '?': 
         PCPB (self, "show-usage", TRUE); break;
       case 'a': PCPB (self, "ampm", TRUE); break;
       case 'c': PCP (self, "city", optarg); break;
       case 'd': PCP (self, "date", optarg); break; 
       case 'f': PCPB (self, "full", TRUE); break;
       case 'h': PCPB (self, "show-usage", TRUE); break;
       case 'j': PCPB (self, "json", TRUE); break;
       case 'l': PCP (self, "latitude", optarg); break;
       case 'o': PCP (self, "longitude", optarg); break;
       case 't': PCP (self, "tz", optarg); break;
       case 'v': PCPB (self, "show-version", TRUE); break;
       case 'y': 
         PCPB (self, "days", TRUE); 
         if (optarg)
           PCPI (self, "days-year", atoi (optarg)); 
         break;
       default:
         ret = FALSE; 
       }
    }

  if (ret)
    {
    self->nonswitch_argc = argc - optind + 1;
    self->nonswitch_argv = malloc (self->nonswitch_argc * sizeof (char *));
    self->nonswitch_argv[0] = strdup (argv[0]);
    int j = 1;
    for (int i = optind; i < argc; i++)
      {
      self->nonswitch_argv[j] = strdup (argv[i]);
      j++;
      }
    }

  if (PCGB (self, "show-version", FALSE))
    {
    printf ("%s: %s version %s\n", argv[0], NAME, VERSION);
    printf ("Copyright (c)2020 Kevin Boone\n");
    printf ("Distributed under the terms of the GPL v3.0\n");
    ret = FALSE;
    }

   if (PCGB (self, "show-usage", FALSE))
    {
    program_context_show_usage (stdout, argv[0]);
    ret = FALSE;
    }

   if (PCGB (self, "list-cities", FALSE))
    {
    KList *city_list = solcity_find_matching ((UTF8 *)"");
    int cities = klist_length (city_list);
    for (int i = 0; i < cities; i++)
      {
      const SolCity *c = klist_get (city_list, i);
      const char *full_city = solcity_get_name (c);
      printf ("%s\n", full_city);
      }
    if (city_list) klist_destroy (city_list);
    ret = FALSE;
    }

  KLOG_OUT
  return ret;  
  }

/*============================================================================
  
  program_context_put

  ==========================================================================*/
void program_context_put (ProgramContext *self, const char *name, 
       const char *value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  KString *temp = kstring_new_from_utf8 ((UTF8 *)value);
  kprops_add_utf8 (self->props, (UTF8 *)name, temp);
  kstring_destroy (temp);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_put_boolean

  ==========================================================================*/
void program_context_put_boolean (ProgramContext *self, const char *name, 
       BOOL value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  kprops_put_boolean_utf8 (self->props, (UTF8 *)name, value);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_put_integer

  ==========================================================================*/
void program_context_put_integer (ProgramContext *self, const char *name, 
       int value)
  {
  KLOG_IN
  assert (self != NULL);
  assert (self->props != NULL);
  kprops_put_integer_utf8 (self->props, (UTF8 *)name, value);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_rc_file

  ==========================================================================*/
void program_context_read_rc_file (ProgramContext *self, const KPath *path)
  {
  KLOG_IN
  klog_debug (KLOG_CLASS, "Reading RC file %S", 
     kstring_cstr ((KString *)path));

  kprops_from_file (self->props, path);

  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_rc_files

  ==========================================================================*/
void program_context_read_rc_files (ProgramContext *self)
  {
  KLOG_IN
  program_context_read_system_rc_file (self);
  program_context_read_user_rc_file (self);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_system_rc_file

  // TODO Windows. 

  ==========================================================================*/
void program_context_read_system_rc_file (ProgramContext *self)
  {
  KLOG_IN
  KPath *path = kpath_new_from_utf8 ((UTF8*) ("/etc/" NAME ".rc"));
  klog_debug (KLOG_CLASS, 
     "System RC file is %S", kstring_cstr ((KString *)path));
  program_context_read_rc_file (self, path);
  kpath_destroy (path);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_read_user_rc_file
  
  // TODO Windows. 

  ==========================================================================*/
void program_context_read_user_rc_file (ProgramContext *self)
  {
  KLOG_IN
  KPath *path = kpath_new_home();
  kpath_append_utf8 (path, (UTF8 *) ("." NAME ".rc"));
  klog_debug (KLOG_CLASS, 
     "User RC file is %S", kstring_cstr ((KString *)path));
  program_context_read_rc_file (self, path);
  kpath_destroy (path);
  KLOG_OUT
  }

/*============================================================================
  
  program_context_show_usage
  
  ==========================================================================*/
void program_context_show_usage (FILE *fout, const char *argv0)
  {
  KLOG_IN
  fprintf (fout, "Usage: %s [options]\n", argv0);
  fprintf (fout, "  -a,--ampm                show AM/PM times\n");
  fprintf (fout, "  -c,--city=[name]         set city\n");
  fprintf (fout, "  -d,--date=[date,help]    set date, or see format\n");
  fprintf (fout, "  -f,--full                show more results\n");
  fprintf (fout, "     --help                show this message\n");
  fprintf (fout, "     --list-cities         list cities\n");
  fprintf (fout, "     --log-level=[0..5]    log level (default 2)\n");
  fprintf (fout, "  -l,--latitude=[degrees]  set latitude\n");
  fprintf (fout, "  -o,--longitude=[degrees] set longitude\n");
  fprintf (fout, "  -t,--tz=[timezone]       set timezone\n");
  fprintf (fout, "  -v,--version             show version\n");
  fprintf (fout, "  -y,--year=[year]         show year summary\n");
  KLOG_OUT
  }



