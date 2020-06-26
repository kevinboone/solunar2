/*============================================================================
  
  solunar
  
  main.c

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
#include "program.h" 
#include "program_context.h" 

#define KLOG_CLASS "solunar.main"

/*============================================================================
  
  main 

  ==========================================================================*/
int main (int argc, char **argv)
  {
  KLOG_IN

  int ret = 0;

  ProgramContext *context = program_context_new();

  klog_set_log_level (KLOG_INFO);
 
  program_context_read_rc_files (context);
  if (program_context_parse_command_line (context, argc, argv))
    {
    if (program_context_check_and_resolve (context))
      {
      int log_level = 
        program_context_get_integer (context, "log-level", KLOG_INFO);
      klog_set_log_level (log_level);
      ret = program_run (context);
      }
    else
      ret = EINVAL;
    }
  else
    ret = EINVAL;
  
  program_context_destroy (context);

  exit (ret);
  }



