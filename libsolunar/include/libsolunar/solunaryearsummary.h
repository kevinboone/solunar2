/*============================================================================
  
  libsolunar 
  
  solunaryearsummary.h

  Copyright (c)1990-2020 Kevin Boone. Distributed under the terms of the
  GNU Public Licence, v3.0

  ==========================================================================*/
#pragma once

#include <klib/klib.h>

struct _SolunarYearSummary;
typedef struct _SolunarYearSummary SolunarYearSummary;

BEGIN_DECLS

extern SolunarYearSummary *solunar_year_summary_create 
            (int year, double latitude, const char *tz);

extern void solunar_year_summary_destroy (SolunarYearSummary *self);

/** Get the festivals as a KList of Festival objects. */
const KList *solunar_year_summary_get_festivals 
        (const SolunarYearSummary *self);

double solunar_year_summary_get_latitude
        (const SolunarYearSummary *self);

/** Get the timezone for which this summary was created. It might be
 * NULL. */
const char *solunar_year_summary_get_timezone
        (const SolunarYearSummary *self);

extern KString *solunar_year_summary_to_json 
            (const SolunarYearSummary *self);
extern KString *solunar_year_summary_to_string 
            (const SolunarYearSummary *self);

END_DECLS

