/***********************************************************************
 * Copyright (c) 2010-2016 Technische Universitaet Dresden             *
 *                                                                     *
 * This file is part of libadapt.                                      *
 *                                                                     *
 * libadapt is free software: you can redistribute it and/or modify    *
 * it under the terms of the GNU General Public License as published by*
 * the Free Software Foundation, either version 3 of the License, or   *
 * (at your option) any later version.                                 *
 *                                                                     *
 * This program is distributed in the hope that it will be useful,     *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of      *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the       *
 * GNU General Public License for more details.                        *
 *                                                                     *
 * You should have received a copy of the GNU General Public License   *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.*
 ***********************************************************************/

/*
 * dct.c
 *
 *  Created on: 09.01.2012
 *      Author: rschoene
 */

#include "dct.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>


/* give us the status of dct */
static int dct_enabled = 0;
/* give us the number of threads that was last set by
 * omp_dct_set_num_threads */
static int last_set_threads = 0;
/* max number of threads returned by omp_get_max_threads
 * use if threads_after or threads_before are zero
 * TODO: really needed? 
 *       only used in dct_read_from_config
 *       maybe possbile to move inside of the function
 * */
static int initial_num_threads = 0;
/* save the number of threads from the last dct_process_after call */
static int last_exit_threads = 0;

/* The OMP runtime does not have to support dynamic, but we do */
/* Therefor we have to disable their dynamic and return true */
/* If we're asked for a dynamic omp runtime */

/* look if we use dynamic or not */
int omp_get_dynamic(){
  return dct_enabled;
}

/* enable, disable dynamic */
void omp_set_dynamic(int enabled){
  dct_enabled = enabled;
}

/* how many threads activcavte we the last time */
int omp_dct_get_set_threads(){
  return last_set_threads;
}

/* Use union to avoid ugly casting
 * attribute for function member from orig omp functions
 * */
static union{
  void * vp;
  int (*function)(void);
}omp_dct_orig_get_thread_num;

static union{
  void * vp;
  int (*function)(void);
}omp_dct_orig_get_max_threads;

static union{
  void * vp;
  int (*function)(void);
}omp_dct_orig_get_num_threads;

static union{
  void * vp;
  int (*function)(void);
} omp_dct_orig_get_dynamic;

static union{
  void * vp;
  void (*function)(int);
} omp_dct_orig_set_dynamic;

static union{
  void * vp;
  void (*function)(int);
}omp_dct_orig_set_num_threads;

/* wrapper function from orig omp to dct  
 * */
int omp_dct_get_thread_num(void){
  if (omp_dct_orig_get_thread_num.vp)
    return omp_dct_orig_get_thread_num.function();
  return 0;
}

int omp_dct_get_max_threads(void){
  if (omp_dct_orig_get_max_threads.vp)
    return omp_dct_orig_get_max_threads.function();
  return 0;
}
int omp_dct_get_num_threads(void){
  if (omp_dct_orig_get_num_threads.vp)
    return omp_dct_orig_get_num_threads.function();
  return 0;
}
int omp_dct_get_dynamic_orig(void){
  if (omp_dct_orig_get_dynamic.vp)
    return omp_dct_orig_get_dynamic.function();
  return 0;
}
void omp_set_dynamic_orig(int dyn){
  if (omp_dct_orig_set_dynamic.vp)
    omp_dct_orig_set_dynamic.function(dyn);
}

void omp_dct_set_num_threads(int num){
  if (omp_dct_orig_set_num_threads.vp){
    last_set_threads = num;
    omp_dct_orig_set_num_threads.function(num);
  }
}

/* load all needed omp function with dlsym */
void load_omp_lib(){
  static int failed=0;
  if (failed) return;

  omp_dct_orig_get_thread_num.vp = dlsym(RTLD_DEFAULT, "omp_get_thread_num");
  if (omp_dct_orig_get_thread_num.vp == NULL)
    omp_dct_orig_get_thread_num.vp = dlsym(RTLD_NEXT, "omp_get_thread_num");
  if (omp_dct_orig_get_thread_num.vp==NULL){
    fprintf(stderr,"Error loading function %s (%s)\n","omp_get_thread_num",dlerror());
    failed=1;
    return;
  }

  omp_dct_orig_get_max_threads.vp = dlsym(RTLD_DEFAULT, "omp_get_max_threads");
  if (omp_dct_orig_get_max_threads.vp==NULL)
    omp_dct_orig_get_max_threads.vp = dlsym(RTLD_NEXT, "omp_get_max_threads");
  if (omp_dct_orig_get_max_threads.vp==NULL){
    fprintf(stderr,"Error loading function %s (%s)\n","omp_get_max_threads",dlerror());
    failed=1;
    return;
  }

  omp_dct_orig_get_num_threads.vp = dlsym(RTLD_DEFAULT, "omp_get_num_threads");
  if (omp_dct_orig_get_num_threads.vp==NULL)
    omp_dct_orig_get_num_threads.vp = dlsym(RTLD_NEXT, "omp_get_num_threads");
  if (omp_dct_orig_get_num_threads.vp==NULL){
    fprintf(stderr,"Error loading function %s (%s)\n","omp_get_num_threads",dlerror());
    failed=1;
    return;
  }

  omp_dct_orig_get_dynamic.vp = dlsym(RTLD_DEFAULT, "omp_get_dynamic");
  if (omp_dct_orig_get_dynamic.vp==NULL)
    omp_dct_orig_get_dynamic.vp = dlsym(RTLD_NEXT, "omp_get_dynamic");
  if (omp_dct_orig_get_dynamic.vp==NULL){
    fprintf(stderr,"Error loading function %s (%s)\n","omp_get_dynamic",dlerror());
    failed=1;
    return;
  }


  omp_dct_orig_set_num_threads.vp = dlsym(RTLD_DEFAULT, "omp_set_num_threads");
  if (omp_dct_orig_set_num_threads.vp==NULL)
    omp_dct_orig_set_num_threads.vp = dlsym(RTLD_NEXT, "omp_set_num_threads");
  if (omp_dct_orig_set_num_threads.vp==NULL){

    fprintf(stderr,"Error loading function %s (%s)\n","omp_set_num_threads",dlerror());
    failed=1;
    return;
  }

  omp_dct_orig_set_dynamic.vp = dlsym(RTLD_DEFAULT, "omp_set_dynamic");
  if (omp_dct_orig_set_dynamic.vp==NULL)
    omp_dct_orig_set_dynamic.vp = dlsym(RTLD_NEXT, "omp_set_dynamic");
  if (omp_dct_orig_set_dynamic.vp==NULL){
      fprintf(stderr,"Error loading function %s (%s)\n","omp_set_dynamic",dlerror());
      failed=1;
      return;
  }
}

/* initial everything for dct if there is not already the first function
 * loaded */
int init_dct_information(){
  if (omp_dct_orig_get_thread_num.vp != NULL){
    return 1;
  }
  load_omp_lib();
#ifdef VERBOSE
  fprintf(stderr, "Load OpenMP libary \n");
#endif
  return 0;
}

/* get the information from our config file */
int dct_read_from_config(void * vp,struct config_t * cfg, char * buffer, char * prefix){
  config_setting_t *setting;
  struct dct_information * info = vp;
  info->threads_before = -1;
  info->threads_after = -1;
  init_dct_information();
  initial_num_threads = omp_dct_get_max_threads();

  /* search settings for threads_before declarations */
  sprintf(buffer, "%s.%s_threads_before",
      prefix, DCT_CONFIG_STRING);
  setting = config_lookup(cfg, buffer);
  if (setting)
  {
    /* set variable to value in config */
    info->threads_before = config_setting_get_int(setting);
#ifdef VERBOSE
    fprintf(stderr, "%s = %" PRId32 "\n",buffer,info->threads_before);
#endif
    if (info->threads_before == 0)
      /* by zero no config option was found */
      info->threads_before = initial_num_threads;
    omp_set_dynamic(1);
  }
  else
  {
      /* without a setting,set it to the default value */
      info->threads_before = -1;
  }
  /* the same for threas_after */
  sprintf(buffer, "%s.%s_threads_after",
      prefix, DCT_CONFIG_STRING);
  setting = config_lookup(cfg, buffer);
  if (setting)
  {
    info->threads_after = config_setting_get_int(setting);
#ifdef VERBOSE
    fprintf(stderr, "%s = %" PRId32 "\n",buffer,info->threads_after);
#endif
    if (info->threads_after == 0)
      info->threads_after = initial_num_threads;
    omp_set_dynamic(1);
  }
  else
  {
      info->threads_after = -1;
  }

  if (omp_get_dynamic())
  {
#ifdef VERBOSE
    fprintf(stderr, "Enable DCT and disable original dynamic \n");
#endif
    /* disable dynamic from omp because we want to use our
     * implementation */
    /* ISSUE: dct settings for region before were not applied with this line */
    /* omp_set_dynamic_orig(0); */
    return 1;
  }
  else
    return 0;
}

/* change the number of threads before the function will called like configured */
int dct_process_before(void * vp,int ignore){
  struct dct_information * info = vp;
  if (omp_get_dynamic())
    if (info->threads_before > 0) {
        /* then we get a number of threads from the config so use it */
#ifdef VERBOSE
      fprintf(stderr,"Adapting threads before to %d\n",info->threads_before);
#endif

      omp_dct_set_num_threads(info->threads_before);

#ifdef VERBOSE
      fprintf(stderr,"Actual max number of threads: %d\n", omp_dct_get_max_threads());
#endif
    }
#ifdef VERBOSE
  if ( !omp_get_dynamic() )
    fprintf(stderr, "DCT not enabled for setting before \n");
#endif
  return 0;
}

/* change the number of threads after the function will called like configured */
int dct_process_after(void * vp,int ignore){
  struct dct_information * info = vp;
  if (omp_get_dynamic())
    if (info->threads_after > 0) {
#ifdef VERBOSE
      fprintf(stderr,"Adapting threads after to %d\n",info->threads_after);
#endif

      omp_dct_set_num_threads(info->threads_after);

#ifdef VERBOSE
      fprintf(stderr,"Actual max number of threads: %d\n", omp_dct_get_max_threads());
#endif
      /* set last_exit_threads to the number of the latest change of the
       * threads afterwards */
      last_exit_threads = info->threads_after;
    }
#ifdef VERBOSE
  if ( !omp_get_dynamic() )
    fprintf(stderr, "DCT not enabled for setting after \n");
#endif
  return 0;
}

/* it would called in adapt.c because the dct exit doesn't work in all
 * compilers and repeat the last dct_process_after operation */
void omp_dct_repeat_exit(){
  if (omp_get_dynamic())
    if (last_exit_threads){
      omp_dct_set_num_threads(last_exit_threads);
      /* we don't want to repeat it again */
      last_exit_threads = 0;
    }
}

