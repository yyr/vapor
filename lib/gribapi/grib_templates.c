/*
 * Copyright 2005-2014 ECMWF.
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 *
 * In applying this licence, ECMWF does not waive the privileges and immunities granted to it by
 * virtue of its status as an intergovernmental organisation nor does it submit to any jurisdiction.
 */

#include "grib_api_internal.h"
#ifdef _WINDOWS
#include <io.h>
#endif
#if HAVE_UNISTD_H
# include <unistd.h>
#endif

typedef struct grib_templates {
  const char*           name;
  const unsigned char* data;
  size_t               size;
} grib_templates;

#if 1
#include "grib_templates.h"

#define NUMBER(x) (sizeof(x)/sizeof(x[0]))

grib_handle* grib_internal_template(grib_context* c,const char* name)
{
  int i;
  for(i = 0; i < NUMBER(templates); i++)
    if(strcmp(name,templates[i].name) == 0)
      return grib_handle_new_from_message_copy(c,templates[i].data,templates[i].size);
  return NULL;
}
#else
grib_handle* grib_internal_template(grib_context* c,const char* name)
{
      return NULL;
}
#endif

static grib_handle* try_template(grib_context* c,const char* dir,const char* name)
{
  char path[1024];
  grib_handle *g = NULL;
  int err = 0;

  sprintf(path,"%s/%s.tmpl",dir,name);
#ifdef _WINDOWS
  if(_access(path,0) == 0)
#else
  if(access(path,F_OK) == 0)
#endif
  {
    FILE* f = fopen(path,"r");
    if(!f)
    {
      grib_context_log(c,GRIB_LOG_PERROR,"cannot open %s",path);
      return NULL;
    }
    g = grib_handle_new_from_file(c,f,&err);
    fclose(f);
  }

  return g;
}

static char* try_template_path(grib_context* c,const char* dir,const char* name)
{
  char path[1024];

  sprintf(path,"%s/%s.tmpl",dir,name);
#ifdef _WINDOWS
  if(_access(path,04) == 0)
#else
  if(access(path,R_OK) == 0)
#endif
  {
    return grib_context_strdup(c,path);
  }

  return NULL;
}

grib_handle* grib_external_template(grib_context* c,const char* name)
{
  const char *base = c->grib_samples_path;
  char buffer[1024];
  char *p = buffer;
  grib_handle *g = NULL;
  /* printf("GRIB_TEMPLATES_PATH=%s\n",base); */

  if(!base) return NULL;

  while(*base)
  {
    if(*base == ':')
    {
      *p = 0;
      g = try_template(c,buffer,name);
      if(g) return g;
      p = buffer;
      base++; /*advance past delimiter*/
    }
    *p++ = *base++;
  }

  *p = 0;
  return g = try_template(c,buffer,name);
}

char* grib_external_template_path(grib_context* c,const char* name)
{
  const char *base = c->grib_samples_path;
  char buffer[1024];
  char *p = buffer;
  char *g = NULL;

  if(!base) return NULL;

  while(*base)
  {
    if(*base == ':')
    {
      *p = 0;
      g = try_template_path(c,buffer,name);
      if(g) return g;
      p = buffer;
      base++;
    }
    *p++ = *base++;
  }

  *p = 0;
  return g = try_template_path(c,buffer,name);
}
