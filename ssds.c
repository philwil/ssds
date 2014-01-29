/*
 * ssds.c
 * small system data server
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 */

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>

#include "ssds.h"

struct ssds *ssds;

int num_data;
int max_data=DATA_SIZE;
char ssds_data[DATA_SIZE];
char *data = &ssds_data[0];
int ssds_debug=2;
int num_defs;
int max_defs=DEFS_SIZE;
char defs[DEFS_SIZE];

static int debug=0;
char * ssds_file_name;

static
int set_action_val(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num);
static
int get_action_val(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num);
static
int get_action_val_bin(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num);

static
int set_action_val_bin(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num);

static
int get_action_val_name_any(char *saction, char *f_val, int len, int bin);

static
int get_sv_ftype(char*f_val, char *sact, int f_size);

static
int get_sv_flen(char*f_val, char *sact, int f_size, int f_type);
static
int get_sv_value(char*f_val, char *sact, int f_size);

static
char *get_item_data(struct ssds_field *pfield, struct ssds_item *pitem);

char *ssds_alloc_data(int size)
{
  char *ret;
  ret = NULL;
  if(ssds) {
    if ((size + ssds->num_data) < ssds->max_data) {
      ret = &ssds->data[ssds->num_data];
      ssds->num_data += size;
    }
  }
  return ret;
}

/* 
  alloc some space 
  will use the defs field
  thinks there should also be a data field

*/
char *ssds_alloc_def(int size)
{
  char *ret;
  ret = NULL;
  if(ssds) {
    if ((size + ssds->num_defs) < ssds->max_defs) {
      ret = &ssds->defs[ssds->num_defs];
      ssds->num_defs += size;
    }
  } else { // special case for initial def
    if ((size + num_defs) < max_defs) {
      ret = &defs[num_defs];
      num_defs += size;
    }
  }
  return ret;
}

int ssds_file_x(char *fname,struct ssds_client *cli)
{
  char fbuf[256];
  char sbuf[256];
  FILE *f;
  int lines;
  int lnum;
  int ret;

  lines = 0;
  lnum = 0;
  if(ssds->debug)
    printf(" SSDS File (%s) \n", fname);
  f = fopen(fname, "r");
  if (f) {
    while (fgets(fbuf, 132, f)) {
       printf(" got line %d <%s>", lnum, fbuf);
       lnum++;
      if(strlen(fbuf) > 1) {
	ret = ssds_action(0, fbuf, sbuf, NULL);
        printf(" line %d action ret %d\n", lnum, ret);

	lines++;
      }
    }
    printf(" Server found file (%s) \n", fname);
  } else {
    printf(" server No file (%s) \n", fname);
  }
  return lines;
}

char *ssds_name(struct ssds *ssds,char *fname)
{
  char *ret;
  ret = NULL;
  if(ssds) {
    if(fname) {
      strncpy(ssds->name, fname, 31);
      ssds->name[31]=0;
    }
    ret = ssds->name;
  }
  return ret;
}

char *ssds_vers(struct ssds *ssds,char *fname)
{
  char *ret;
  ret = NULL;
  if(ssds) {
    if(fname) {
      strncpy(ssds->vers, fname, 31);
      ssds->name[31]=0;
    }
    ret = ssds->vers;
  }
  return ret;
}

int ssds_init_x(char *fname, struct ssds_client *cli)
{
  if(!ssds) {
    ssds = (struct ssds *)ssds_alloc_def(sizeof(struct ssds));
    if(ssds) {
      strcpy(ssds->name,"SSDS");
      strcpy(ssds->vers,"0.0.0");
      ssds->self = ssds;
      ssds->type = SSDS_SSDS;

      ssds->debug = 0;

      ssds->num_classes = 0;
      ssds->first_class = NULL;
      ssds->last_class = NULL;

      ssds->num_items = 0;
      ssds->first_item = NULL;
      ssds->last_item = NULL;

      ssds->num_fields = 0;
      ssds->first_field = NULL;
      ssds->last_field = NULL;

      ssds->defs = defs;
      ssds->num_defs = num_defs;
      ssds->max_defs = max_defs;

      ssds->data = data;
      ssds->num_data = num_data;
      ssds->max_data = max_data;
      if(fname) {
	ssds_file_x(fname, cli);
      } else {
	ssds_file_x(SSDS_FILE, cli);
	ssds_file_x(SSDS_LOCAL_FILE, cli);
      }
    }
  }
  return 0;
}

int ssds_get_num_classes(void)
{
  int ret;

  ret = 0;
  if(ssds) {
    ret = ssds->num_classes;
  }
  return ret;
}


void *ssds_get_class(int num)
{
  void *ret;
  struct ssds_class *pclass;
  ret = NULL;
  if(ssds) {
    if (num<ssds->num_classes) {
      pclass = ssds->first_class;
      while(num--) {
	pclass = pclass->next_def;
      }
      ret = pclass;
    }
  }
  return ret;
}

void *ssds_get_class_num(int inum)
{
  void *ret;
  int num;

  struct ssds_class *pclass;

  ret = NULL;
  if(ssds) {
    num=ssds->num_classes;
    //printf(" num %d inum %d\n", num, inum);
    pclass = ssds->first_class;
    while(num--) {
      //printf("pclass %p next_def %p\n", pclass, pclass->next_def);
      if(pclass->num == inum) {
	ret = pclass;
	break;
      }
      pclass = pclass->next_def;
    }

  }
  //printf(" num %d inum %d ret %p\n", num, inum, ret);
  return ret;
}

void *ssds_get_class_id(int id)
{
  void *ret;
  struct ssds_class *pclass;
  int num;

  ret = NULL;
  if(ssds) {
    num = ssds->num_classes;
    pclass = ssds->first_class;
    while(num--) {
      if (pclass->id == id) {
	ret = pclass;
	break;
      }
      pclass = pclass->next_def;
    }
  }
  return ret;
}
static
int class_init(struct ssds_class *pclass, char *name, int id)
{
  struct ssds_class *lclass;

  if(pclass) {
    strncpy(pclass->name,name,NAME_LEN);
    pclass->type = SSDS_CLASS;
    pclass->self = pclass;
    pclass->num_fields = 0;
    pclass->num_items = 0;
    pclass->inuse = 1;
    pclass->num = ssds->num_classes;
    pclass->id = id;
    pclass->data_len = 0;
    pclass->first_item = NULL;
    pclass->last_item = NULL;
    pclass->first_field = NULL;
    pclass->last_field = NULL;
    pclass->next_def = NULL;
    pclass->next = NULL;
    if(ssds) {
      if(ssds->first_class) {
	lclass = ssds->last_class;
	lclass->next = pclass;
	lclass->next_def = pclass;
      } else {
	ssds->first_class=pclass;
      }
      ssds->last_class=pclass;
      ssds->num_classes++;
    }
  }
  return 0;
}

int field_init(struct ssds_class *pclass,
	       struct ssds_field *field,
	       char *name,
	       int id,
	       int type,
	       int len)
{
  int ret;
  int i;
  struct ssds_field *lfield;
  struct ssds_item *item;
  ret = -1;
  if(field) {
    if(0)
      printf(" SDCS Adding field (%s) to class (%s)\n", name, pclass->name);
    strncpy(field->name, name, NAME_LEN);
    field->id = id;
    field->self = field;
    field->idx = id;
    field->len = len;
    if(type>=0) {
      field->ftype = type;
    } else {
      field->ftype = SSDS_INT;
      field->len = 4;
    }
    pclass->data_len += field->len;

    field->class = pclass;
    field->ssds = ssds;
    field->next = NULL;
    field->next_def = NULL;

    lfield = pclass->last_field;
    if (pclass->first_field == NULL) {
      pclass->first_field = field;
    } else {
      //field->next = pclass->last_field;
      if(lfield) lfield->next = field;
    }
    pclass->last_field = field;
    field->num = pclass->num_fields;
    pclass->num_fields++;

    if(ssds) {
      if(ssds->first_field) {
	lfield = ssds->last_field;
	lfield->next_def = field;
      } else {
	ssds->first_field=field;
      }
      ssds->last_field=field;
      ssds->num_fields++;
    }
    if(pclass->num_items > 0) {
      item = pclass->first_item;
      if(0)printf("SDCS adding field (%s) (num %d) to %d items \n",
	     field->name, field->num, pclass->num_items);
      for(i=0;i<pclass->num_items;i++) {
	if(0)
	  printf("SDCS adding field (%s) num %d len %d to item (%s) \n",
		 field->name, field->num, field->len,item->name);
	item->data_ix[field->num] = ssds_alloc_data(field->len+8);
	item=item->next;
      }
    }
    ret = 0;
  }
  return ret;
}

int item_init(struct ssds_class *pclass,
	      struct ssds_item *item,
	      char *name,
	      int id)
{
  int ret;
  struct ssds_item *litem;
  struct ssds_field *field;
  ret = -1;
  int i;
  int numf;

  if(item) {
    strncpy(item->name, name, NAME_LEN);
    item->type = SSDS_ITEM;
    item->self = item;

    item->id = id;
    item->ix = id;
    item->ssds_class = pclass;
    item->ssds = ssds;
    item->next_def = NULL;
    ret = -1;
    item->idx = ssds->num_data;
    item->num = pclass->num_items;
    numf =  pclass->num_fields;
    if(numf>16) numf = 16;  // SDCS little bomb to limit fields
    field = pclass->first_field;
    for (i=0; i<numf; i++) {
      item->data_ix[i] = ssds_alloc_data(field->len+8);
      field = field->next;
    }
    ret = -2;
    litem = pclass->last_item;
    if(pclass->num_items == 0) {
      pclass->first_item = item;
      item->next = NULL;
    } else {
      item->next = NULL;
      if(litem) litem->next = item;
    }
    pclass->last_item = item;
    pclass->num_items++;
    ret = 0;
    if(ssds) {
      if(ssds->first_item) {
	litem = ssds->last_item;
	litem->next_def = item;
      } else {
	ssds->first_item=item;
      }
      ssds->last_item=item;
      ssds->num_items++;
    }
    ret = 0;
  }
  return ret;
}


/*
 * given a name find/create a class
 */
void *ssds_find_class_new(char *name, int *new)
{
  void *ret;
  struct ssds_class *pclass;
  int i;

  ret = NULL;
  if(ssds) {
    if(new) *new=0;
    pclass = ssds->first_class;
    i = ssds->num_classes;
    while (i--) {
      if(strncmp(pclass->name, name, NAME_LEN) == 0) {
	if(ssds->debug)
	  printf(" SDCS found class (%s) id %d \n",name, pclass->id);

	ret = pclass;
	break;
      }
      pclass = pclass->next;
    }

    if(!ret) {
      if(ssds->debug)
	printf(" SDCS creating class (%s) id %d \n",name, ssds->num_classes+1);
      if(new) *new=1;
      pclass = (struct ssds_class *)ssds_alloc_def(sizeof(struct ssds_class));
      class_init(pclass, name, ssds->num_classes+1);
      ret = pclass;
    }
  }
  return ret;
}

void *ssds_find_class(char *name)
{
  return ssds_find_class_new(name, NULL);
}


// given a name and a class find/create a field
void *ssds_find_field_new(struct ssds_class *pclass, char *name, int f_type
		      , int len, int *new)
{
  void *ret;
  struct ssds_field *field;
  int i;

  if (new) *new = 0;
  ret = NULL;
  field = pclass->first_field;
  if(field) {
    i = pclass->num_fields;
    while(i--) {
      if(strncmp(field->name, name, NAME_LEN) == 0) {
	ret = field;
	break;
      }
      field = field->next;
    }
  }
  if(!ret) {
    if(ssds) {
      if (new)*new = 1;
      field = (struct ssds_field*)ssds_alloc_def(sizeof(struct ssds_field));
      field_init(pclass, field, name, pclass->num_fields+1, f_type, len);
      ret = field;
    }
  }
  return ret;
}
void *ssds_find_field(struct ssds_class *pclass, char *name, int f_type
		      , int len)
{
  return ssds_find_field_new(pclass, name, f_type, len, NULL);

}


/*
  given a name and a class find/create a item
*/
void *ssds_find_item_new(struct ssds_class *pclass, char *name, int *new)
{
  void *ret;
  struct ssds_item *item;
  int i;

  if (new)*new = 0;
  ret = NULL;
  if(ssds && pclass) {
    if(ssds->debug)
      printf(" ssds_find_item class %s.%d num_items %d name %s\n"
	     , pclass->name
	     , pclass->id
	     , pclass->num_items
	     , name);
    item = pclass->first_item;
    if(item) {
      i = pclass->num_items;
      while(i--) {
	if(ssds->debug)
	  printf("    item %s.%d\n"
		 , item->name
		 , item->id
		 );

	if(strncmp(item->name, name, NAME_LEN) == 0) {
	  ret = item;
	  break;
	}
	item=item->next;
      }
    }
    if(!ret) {
      if(ssds) {
	if (new)*new = 1;
	item = (struct ssds_item *)ssds_alloc_def(sizeof(struct ssds_item));
	item_init(pclass, item, name, ssds->num_items+1);
	ret = item;
      }
    }
  }
  return ret;
}


void *ssds_find_item(struct ssds_class *pclass, char *name)
{
  return ssds_find_item_new(pclass, name, NULL);
}

void *ssds_get_field_id(struct ssds_class *pclass, int id)
{
  struct ssds_field *field;
  void *ret;
  int num;

  ret = NULL;
  if(pclass) {
    num = pclass->num_fields;
    field = pclass->first_field;
    while(num--) {
      if(field->id == id) {
	ret = field;
	break;
      }
      field= field->next;
    }
  }
  return ret;
}

void *ssds_get_field_num(struct ssds_class *pclass, int fnum)
{
  struct ssds_field *field;
  void *ret;
  int num;

  ret = NULL;
  if(pclass) {
    num = pclass->num_fields;
    field = pclass->first_field;
    while(num--) {
      if(0)
	printf("ssds_get_field_num num %d fnum %d field->num %d\n",
	       num,fnum,field->num);
      if(field->num == fnum) {
	ret = field;
	break;
      }
      field= field->next;
    }
  }
  return ret;
}

int ssds_get_num_items(struct ssds_class *pclass, int c_num)
{

  int ret;

  ret = pclass->num_items;
  return ret;
}

void *ssds_get_item_id(struct ssds_class *pclass, int id)
{
  struct ssds_item *item;
  int num;
  void * ret;

  ret = NULL;
  if(pclass) {
    num = pclass->num_items;
    item = pclass->first_item;
    while(num--) {
      if(item->id == id) {
	ret = item;
	break;
      }
      item = item->next;
    }
  }
  return ret;
}

void *ssds_get_item_num(struct ssds_class *pclass, int inum)
{
  struct ssds_item *item;
  int num;
  void * ret;

  ret = NULL;
  if(pclass) {
    num = pclass->num_items;
    item = pclass->first_item;
    while(num--) {
      if(item->num == inum) {
	ret = item;
	break;
      }
      item = item->next;
    }
  }
  return ret;
}

void *xssds_get_item_num(struct ssds_class *pclass, int num)
{
  struct ssds_item *item=NULL;
  void * ret;

  ret = NULL;
  if(pclass) {
    item = pclass->first_item;
    if(num < pclass->num_items) {
      while(num--) {
	item = item->next;
      }

    }
  }
  return item;
}

// given a class get an field name by index
void *ssds_get_field_name(struct ssds_class *pclass, int id)
{
  struct ssds_field *field;
  void *ret;
  ret = NULL;

  field = ssds_get_field_id(pclass, id);
  if(field){
    ret = field->name;
  }
  return ret;
}

// given a class get an item name by index
void *ssds_get_item_name(struct ssds_class *pclass, int id)
{
  struct ssds_item *item;
  void *ret;

  ret = NULL;
  item = ssds_get_item_id(pclass, id);
  if(item){
    ret = item->name;
  }
  return ret;
}


int ssds_print_items(struct ssds_class *pclass)
{
  int i;
  struct ssds_item *item;

  printf(" SSDS class (%s.%d) has %d items\n",pclass->name,
	 pclass->id, pclass->num_items);

  item=pclass->first_item;
  for (i=0; i<pclass->num_items; i++) {
     printf(" item %d name (%s) id %d data_idx %d\n"
	    ,i,item->name, item->id, item->idx);
     item=item->next;
  }
  return 0;
}

int ssds_print_class(struct ssds_class *pclass)
{
  int i;
  struct ssds_field *field;

  printf(" SSDS class (%s.%d) has %d fields\n",
	 pclass->name, pclass->id, pclass->num_fields);

  field=pclass->first_field;
  for (i=0; i < pclass->num_fields; i++) {
     printf(" field %d name (%s) id %d len %d\n",i
	    , field->name, field->id, field->len);
     field=field->next;
  }

  return 0;
}

int ssds_print_classes(int depth)
{
  int i;
  struct ssds_class *pclass;

  if(ssds) {
    printf(" SSDS has %d classes \n", ssds->num_classes);
    pclass=ssds->first_class;
    for (i=0; i<ssds->num_classes; i++) {
      printf(" (%d) class name (%s) id %d \n"
	     , i, pclass->name, pclass->id);
      pclass=pclass->next_def;
    }
  }
  return 0;
}

int ssds_print_item(void *in_item)
{
  struct ssds_item *item = in_item;
  struct ssds_field *field;
  struct ssds_class *pclass;
  float *fval;
  int *ival;
  char *sval;
  int ret;
  int i;

  ret = -1;
  if(item) {
    pclass=item->ssds_class;
    printf(" Item (%s.%d) class (%s.%d) num_fields %d\n"
	   , item->name
	   , item->id
	   , pclass->name
	   , pclass->id
	   , pclass->num_fields );
    ret = 0;
    field=pclass->first_field;
    i = pclass->num_fields;

    while(i--) {
      switch (field->ftype) {

      case SSDS_INT:
	ival = (int*)get_item_data(field, item);
	printf("  Field (%s) num %d type %d (int) value %d\n",
	       field->name, field->num, field->ftype, *ival);
	break;

      case SSDS_FLOAT:
	fval = (float*)get_item_data(field, item);
	printf("  Field (%s) num %d type %d (float) value %f\n",
	       field->name, field->num, field->ftype, *fval);
	break;

      case SSDS_STR:
	sval = (char*)get_item_data(field, item);
	printf("  Field (%s) num %d type %d (str) value %s\n",
	       field->name, field->num, field->ftype, sval);
	break;

      case SSDS_BIT:
	sval = (char*)get_item_data(field, item);
	printf("  Field (%s) num %d type %d (str) value %s\n",
	       field->name, field->num, field->ftype, sval);
	break;

      default:
	printf("  Field (%s) num %d type %d value UNKNOWN\n",
	       field->name, field->num, field->ftype);
      }
      field = field->next;
    }
  }
  return ret;

}

// arg splitters
/////////////////////////////////////////////////////////////////
static char *get_del(char *sp, char *argv[], int ix)
{
  while(*sp && (*sp == ':')) sp++;
  if (*sp) argv[ix] = sp;
  while((*sp) && (*sp != ':')) sp++;
  if (*sp == ':') {
    *sp=0;
    sp++;
  }
  return sp;
}
///////////////////////////////////////////////////////////////
int ssds_get_argc(char *buf, char *argv[], int max)
{
  char * sp;
  int ix;

  sp = buf;
  ix = 0;
  while (*sp && (ix < max)) {
    sp = get_del(sp, argv, ix);
    ix++;
  }
  return ix;
}

static
struct ssds_class *get_action_class(char *saction)
{
  int ret;
  int res;
  int id;
  struct ssds_class *pclass;
  char *sp;
  char *spn;
  char name[32];

  pclass = NULL;
  sp = saction;
  name[0]=0;

  while(*sp && (*sp != '.'))sp++;
  //if(*sp) printf("get_action_class 1 sp(%s)\n",sp);
  if(*sp == '.') sp++;
  spn = name;
  while(*sp && (*sp != '.') && (*sp != '=') && (*sp != '&')) {
    *spn++ = *sp++;
  }
  *spn = 0;
  //printf("get_action_class name (%s)\n",name);
  sp = name;
  ret = -1;
  if ((*sp == '0') && (strlen(name)== 1)) {
    ret = 0;
    pclass = ssds_get_class_num(ret);
    if(0)
      if(*sp)
	printf("get_action_class 3 sp(%s) name(%s) pclass %p cname (%s)\n"
	       , sp, name, pclass, pclass->name);
  } else

  if (*sp == '0' && (isdigit(*(sp+1))) ) {
    res = sscanf(name,"0%d", &ret);
    if(res)
      pclass = ssds_get_class_num(ret);
  }
  else if (isdigit(*sp)) {
    sscanf(name,"%d", &id);
    if(id>0)pclass = ssds_get_class_id(id);
  }
  else {
    if(strlen(name) > 0)
      pclass = ssds_find_class(name);
  }
  return pclass;
}

static
struct ssds_field *get_action_field(struct ssds_class *pclass
				    , char *saction, int num)
{
  int ret;
  int res;
  int id;
  struct ssds_field *pfield;
  char *sp;
  char *spn;
  char name[32];
  char f_type[32];
  int ftnum;
  int ftlen;

  pfield = NULL;

  if(!pclass) {
    pclass = get_action_class(saction);
  }

  if(pclass) {
    sp = saction;
    // skip class (num=1)and possibly item (num=2)
    while(num--) {
      while(*sp && (*sp != '.') && (*sp != '=') && (*sp != '&'))sp++;
      if(*sp == '.') sp++;
    }

    spn = name;
    while(*sp && (*sp != '.') && (*sp != '=') && (*sp != '&')) {
      *spn++ = *sp++;
    }
    *spn = 0;
    //printf("get_action_field name (%s) \n", name);
    sp = name;
    ret = -1;
    if ((*sp == '0') && (strlen(name)== 1)) {
      ret = 0;
      pfield = ssds_get_field_num(pclass, ret);
    } else
    if (*sp == '0' && (isdigit(*(sp+1))) ) {
      res = sscanf(sp,"0%d", &ret);
      if(res)
         pfield = ssds_get_field_num(pclass, ret);
    }
    else if (isdigit(*sp)) {
      res = sscanf(sp,"%d", &id);
      if(res)
        pfield = ssds_get_field_id(pclass, id);
    }
    else {
      ftnum = get_sv_ftype(f_type, saction, 32);
      ftlen = get_sv_flen(f_type, saction, 32, ftnum);
      if(strlen(name) > 0)
	pfield = ssds_find_field(pclass, name, ftnum, ftlen);
    }
  }
  return pfield;
}

static
struct ssds_item *get_action_item(struct ssds_class *pclass,
				  char *saction, int num)
{
  int ret;
  int res;
  int id;
  int inum;
  struct ssds_item *pitem;
  char *sp;
  char *spn;
  char name[32];

  pitem = NULL;

  name[0] = 0;
  spn=name;
  if(!pclass) {
    pclass = get_action_class(saction);
  }

  if(pclass) {
    sp = saction;
    // skip class (num=1)and possibly item (num=2)
    inum = num;
    while(inum--) {
      while(*sp && (*sp != '.'))sp++;
      if(*sp == '.') sp++;

      //printf("get_action_item sp left (%s) inum %d\n", sp, inum);
    }
    //printf("get_action_item sp left (%s) num %d\n", sp, num);

    while(*sp && (*sp != '.') && (*sp != '=') && (*sp != '&')) {
      *spn++ = *sp++;
    }
    *spn = 0;
    //printf("get_action_item name (%s) \n", name);
    sp=name;
    ret = -1;
    if ((*sp == '0') && (strlen(name)==1)) {
      ret = 0;
      pitem = ssds_get_item_num(pclass, ret);
    } else
    if (*sp == '0' && *(sp+1) && (isdigit(*(sp+1)))) {
      res = sscanf(sp,"0%d", &ret);
      if(res)
	pitem = ssds_get_item_num(pclass, ret);
    }
    else if (isdigit(*sp)) {
      res = sscanf(sp,"%d", &id);
      if(res)
	pitem = ssds_get_item_id(pclass, id);
    }
    else {
      if(strlen(name) > 0)
	pitem = ssds_find_item(pclass, name);
    }
  }
  return pitem;
}

static
char *get_item_data(struct ssds_field *pfield, struct ssds_item *pitem)
{
  char *data;
  data = pitem->data_ix[pfield->num];
  data += 8; // skip locks
  return data;
}

static
int get_action_val(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num)
{
  char *data;
  int *ival;
  unsigned int *uival;
  float *fval;
  int ret;
  int flen;

  flen=pfield->len;
  if(flen > (num-1)) flen=(num-1);
  ret = 0;
  data = get_item_data(pfield, pitem);

  switch(pfield->ftype) {

  case SSDS_INT:
    ival = (int *)data;
    ret = sprintf(f_val,"%d",*ival);
    break;

  case SSDS_FLOAT:
    fval = (float *)data;
    ret = sprintf(f_val,"%f",*fval);
    break;

  case SSDS_STR:
    memcpy(f_val, data, flen);
    ret = flen;
    break;

  case SSDS_BIT:
    uival = (unsigned int *)data;
    ret = sprintf(f_val,"%d",(*uival && pfield->bmask) >> pfield->bshift);
    break;

  default:
    ret = sprintf(f_val,"%s", "UNKNOWN" );
    break;
  }
  //printf("get_action_val return value (%s) \n",f_val);
  return ret;
}

static
int get_action_val_bin(struct ssds_item *pitem,
		       struct ssds_field *pfield,
		       char *f_val, int num)
{
  char *data;
  int ret;
  int flen;
  int i;

  flen=pfield->len;
  if(flen > (num-1)) flen=(num-1);
  data = get_item_data(pfield, pitem);

  ret = pfield->len;
  for (i=0;i<ret;i++) {
    f_val[i] = data[i];
  }
  return ret;
}
static
int set_action_val_bin(struct ssds_item *pitem,
		       struct ssds_field *pfield,
		       char *f_val, int num)
{
  char *data;
  int ret;
  int flen;
  int i;


  flen=pfield->len;
  if(flen > (num-1)) flen=(num-1);
  data = get_item_data(pfield, pitem);

  ret = pfield->len;
  for (i=0;i<ret;i++) {
    data[i]=f_val[i];
  }
  return ret;
}

static
int set_action_val(struct ssds_item *pitem,
		   struct ssds_field *pfield,
		   char *f_val, int num)
{
  char *data;
  int *ival;
  unsigned int *uival;
  float *fval;
  int ret;
  int flen;


  flen=pfield->len;
  if(flen > (num-1)) flen=(num-1);
  ret = 0;
  data = get_item_data(pfield, pitem);

  switch(pfield->ftype) {

  case SSDS_INT:
    ival = (int *)data;
    ret = sscanf(f_val,"%d",ival);
    break;

  case SSDS_FLOAT:
    fval = (float *)data;
    ret = sscanf(f_val,"%f",fval);
    break;

  case SSDS_STR:
    memcpy(data, f_val, flen);
    ret = flen;
    break;

  case SSDS_BIT:
    uival = (unsigned int *)data;

    ret = scanf(f_val,"%d",uival);
    ret = sprintf(f_val,"%d", (*uival && pfield->bmask) >> pfield->bshift);

    break;


  default:
        break;
  }
  //printf("set_action_val value (%s) \n",f_val);
  return ret;
}

//static
int (*find_group)(char *name, void *cli, int act);

//static
int ssds_find_group(char *name, void *cli, int act)
{
  int ret = 0;
  if (find_group)
    ret = find_group(name, cli, act);
  return ret;
}

int ssds_set_find_group(int (*pfind_group)(char *name, void *cli, int act))
{
  int ret = 0;
  find_group = pfind_group;
  return ret;
}

int get_action_val_name(char *saction, char *f_val, int len)
{
  return get_action_val_name_any(saction, f_val, len, 0);
}

int get_action_val_name_bin(char *saction, char *f_val, int len)
{
  return get_action_val_name_any(saction, f_val, len, 1);
}

int get_action_val_name_gr(char *saction, char *f_val,  int len)
{
  return get_action_val_name_any(saction, f_val, len, 2);
}


int get_action_len_name_gr(char *saction, int len)
{
  return get_action_val_name_any(saction, NULL, len, 0);
}

int get_action_len_name_gr_x(char *saction, int len)
{
  int ret;
  int ferr;
  struct ssds_class *dclass=NULL;
  struct ssds_field *dfield=NULL;
  struct ssds_item *ditem=NULL;

  ret = 0;
  ferr = 0;
  dclass = get_action_class(saction);
  if(dclass) {
    if(0)
      printf("get_action_len_name_gr: sa (%s) class (%s)\n"
	     ,saction,dclass->name);
    ditem = get_action_item(dclass, saction, 2);
    dfield = get_action_field(dclass, saction, 3);
  } else {
    ferr |= 4;
  }
  if(!ditem) ferr |= 1;
  if(!dfield)ferr |= 2;

  if(ditem && dclass && dfield){
    ret = dfield->len;
    if(0)
      printf("item->name %s field->name %s ret %d ferr %d \n",
	     ditem->name, dfield->name, ret, ferr);
  }
  if(ferr) ret = -ferr;
  return ret;
}

static
int get_action_val_name_any(char *saction, char *f_val, int len, int bin)
{
  int ret;
  int ferr;
  struct ssds_class *dclass=NULL;
  struct ssds_field *dfield=NULL;
  struct ssds_item *ditem=NULL;
  int mydebug = 0;
  ret = 0;
  ferr = 0;
  dclass = get_action_class(saction);
  if(dclass) {
    ditem = get_action_item(dclass, saction, 2);
    dfield = get_action_field(dclass, saction, 3);
  } else {
    ferr |= 4;
  }
  if(!ditem) ferr |= 1;
  if(!dfield)ferr |= 2;

  if(ditem && dclass && dfield){
    if(f_val) {
      if((bin&1) > 0) {
	ret = get_action_val_bin(ditem, dfield, f_val, len);
	if(mydebug) {
	  printf( "i->n (%s) f->n (%s) ret %d ferr %d \n",
		  ditem->name, dfield->name, ret, ferr);
	  printf("bin f_val (%02x %02x %02x %02x )\n",
		 f_val[0],f_val[1],f_val[2],f_val[3] );

	}

      } else {
	ret = get_action_val(ditem, dfield, f_val, len);
	if(mydebug) {
	  printf("i->n (%s) f->n (%s) ret %d ferr %d\n",
		 ditem->name, dfield->name, ret, ferr);
	  printf(" text f_val (%s)\n", f_val);

	}
      }
    } else {  /* no f_val */
      ret = dfield->len;
      if(mydebug) {
	printf("i->n (%s) f->n (%s) ret %d ferr %d \n",
	       ditem->name, dfield->name, ret, ferr);
      }
    }
  }
  if((bin&2) > 0) {
    if(ferr) ret = -ferr;
  }
  return ret;
}

static
int set_action_val_name_any(char *saction, char *f_val, int len, int bin)
{
  int ret;
  int ferr;
  struct ssds_class *dclass=NULL;
  struct ssds_field *dfield=NULL;
  struct ssds_item *ditem=NULL;

  ret = 0;
  ferr = 0;
  dclass = get_action_class(saction);
  if(dclass) {
    ditem = get_action_item(dclass, saction, 2);
    dfield = get_action_field(dclass, saction, 3);
  } else {
    ferr |= 4;
  }
  if(!ditem) {
    if(0)printf(" NOTE: no item in (%s) !!! dfield %p\n"
	   , saction, dfield);

    //    dfield =
    //(struct ssds_field *)ssds_find_field(dclass,sfield,-1,-1);

    ferr |= 1;
  }
  if(!dfield)ferr |= 2;

  if(ditem && dclass && dfield){
    if(bin) {
      ret = set_action_val_bin(ditem, dfield, f_val, len);
    } else {
      ret = set_action_val(ditem, dfield, f_val, 32);
    }
  }
  //  if(ferr) ret = -ferr;
  return ret;
}

int set_action_val_name_bin(char *saction, char *f_val, int len)
{
  return set_action_val_name_any(saction, f_val, len, 1);
}

int set_action_val_name(char *saction, char *f_val, int len)
{
  return set_action_val_name_any(saction, f_val, len, 0);
}


int get_str_val(char * stmp, char *dsp, char delim, int len)
{
  int ret;
  char *sp;
  int num;

  sp = stmp;
  *sp = 0;
  ret = 0;
  num = 0;
  while (*dsp && (*dsp == delim)) {
    dsp++;
    ret++;
  }
  while (*dsp && (*dsp != delim) && (num <(len-1))){
    ret++;
    num++;
    *sp++ = *dsp++;
  }
  *sp=0;
  return ret;
}

char *get_field_type(struct ssds_field * dfield, char *stype, int len)
{

  switch(dfield->ftype) {
  case SSDS_INT:
    sprintf(stype,"int");
    break;
  case SSDS_FLOAT:
    sprintf(stype,"float");
    break;
  case SSDS_STR:
    sprintf(stype,"string");
    break;
  case SSDS_BIT:
    sprintf(stype,"bit");
    break;
  default:
    sprintf(stype,"unknown");
    break;
  }
  return stype;
}

/*
 * decode mask from bit%<mask>.<shift>
 * assume % in char 3 look for "."
 * decode as hex char no need for 0x
 */


unsigned int get_field_bit(char *stype, int mask, int debug)
{
   unsigned int bmask;
   unsigned int bshift;
   unsigned int ret;
   char temp[64];
   char *sp;
   char *spt;
   int idx;

   sp = &stype[4];
   spt = temp;
   idx = 1;
   while ((*sp != 0 ) && ( *sp != '.') && ( idx < 64) ) {
     idx++;
     *spt++= *sp++;
   }
   *spt = 0;
   sscanf(temp,"%x",&bmask);
   if(debug) printf(" stype <%s> bmask <%x> \n", stype, bmask);
   sp++;
   spt = temp;
   bshift = 0;
   idx = 1;
   while ((*sp != 0 ) && ( idx < 64) ) {
     idx++;
     *spt++= *sp++;
   }
   *spt = 0;

   if ( idx > 1 ) {
      sscanf(temp,"%d",&bshift);
   }
   if(debug)printf("     bshift <%d> \n", bshift);
   if(mask) {
      ret = bmask;
   } else {
      ret = bshift;
   }
   return ret;
}


int set_field_type(struct ssds_field * dfield, char *stype, int len)
{

  if(strcmp(stype,"int")==0) {
    dfield->ftype = SSDS_INT;
    dfield->len = 4;
  } else if(strcmp(stype,"float")==0) {
    dfield->ftype = SSDS_FLOAT;
    dfield->len = 4;
  } else if(strcmp(stype,"string")==0) {
    dfield->ftype = SSDS_STR;
    dfield->len = len;
  } else if(strncmp(stype,"bit", 3)==0) {
    dfield->ftype = SSDS_BIT;

    dfield->len = 4;
    dfield->bmask = get_field_bit(stype, 1, 0);
    dfield->bshift = get_field_bit(stype, 0, 0);



  } else {
    dfield->ftype = -1;
    dfield->len = 0;
  }

  return dfield->ftype;
}


/* gets the class number from 00.00.00 */
int get_class_gen(char *str, int num)
{
  int ret;
  char stmp[32];
  char *sp;
  char *spc;
  char *spi;
  char *spf;
  strncpy(stmp,str,32);
  spc = stmp;
  sp = stmp;
  while (*sp && (*sp !='.') ) sp++;
  if(*sp == '.'){
    *sp=0; sp++; spi=sp;
  }
  while (*sp && (*sp !='.') ) sp++;
  if(*sp == '.'){
    *sp=0; sp++; spf=sp;
  }
  switch (num) {

  case 1:
    ret = atoi(spc);
    break;
  case 2:
    ret = atoi(spi);
    break;
  case 3:
    ret = atoi(spf);
    break;
  default:
    ret = -1;
    break;
  }
  return ret;
}

int get_class_num(char *str)
{
  return get_class_gen(str, 1);
}

int get_field_num(char *str)
{
  return get_class_gen(str, 2);
}

int get_item_num(char *str)
{
  return get_class_gen(str, 3);
}

static
int get_sv_value(char*f_val, char *sact, int f_size)
{
  int fnum;
  char *fvsp;
  char *fsp;

  fvsp=f_val;
  *fvsp=0;
  fsp=strstr(sact,"=");
  if(fsp) {
    fnum=f_size-1;
    fsp++;
    //printf("sv_value found fsp (%s)\n",fsp);

    while(fnum--
	  && (*fsp)
	  && (*fsp != ':')
	  && (*fsp != '&') )  {
      *fvsp++=*fsp++;
    }
    *fvsp =0;
  }
  fnum = strlen(f_val);
  //printf("sv_value found = (%s) len%d\n",f_val, fnum);
  return fnum;
}

static
int get_sv_ftype(char*f_val, char *sact, int f_size)
{
  int fnum;
  char *fvsp;
  char *fsp;

  fvsp=f_val;
  *fvsp=0;
  fsp=strstr(sact,"[");
  if(fsp) {
    fnum=f_size-1;
    fsp++;
    //printf("sv_type found fsp (%s)\n",fsp);

    while(fnum--
	  && (*fsp)
	  && (*fsp != ':')
	  && (*fsp != '&')
	  && (*fsp != ']')
	  ) {
      *fvsp++=*fsp++;
    }
    *fvsp =0;
  }
  if(strcmp(f_val,"float") == 0) {
    fnum = SSDS_FLOAT;
  } else
  if(strcmp(f_val,"int") == 0) {
    fnum = SSDS_INT;
  } else
  if(strncmp(f_val,"str",3) == 0) {
    fnum = SSDS_STR;
  } else
  if(strncmp(f_val,"bit",3) == 0) {
    fnum = SSDS_BIT;
  } else {
    fnum = SSDS_INVALID;
  }
  //printf("sv_ftype found = (%s) type %d\n",f_val, fnum);
  return fnum;
}

static
int get_sv_flen(char*f_val, char *sact, int f_size, int f_type)
{
  int fnum;
  char *fvsp;
  char *fsp;

  fvsp=f_val;
  *fvsp=0;
  fsp=strstr(sact,"[");
  if(fsp) {
    fsp=strstr(fsp,":");
  }
  if(fsp) {
    fnum=f_size-1;
    fsp++;
    //printf("sv_flen found fsp (%s)\n",fsp);

    while(fnum--
	  && (*fsp)
	  && (*fsp != ':')
	  && (*fsp != '&')
	  && (*fsp != ']')
	  ) {
      *fvsp++=*fsp++;
    }
    *fvsp =0;
  }

  fnum = -1;

  if(f_type == SSDS_FLOAT) {
    fnum = SSDS_FLOAT_LEN;
  } else
  if(f_type == SSDS_INT) {
    fnum = SSDS_INT_LEN;
  } else
  if(f_type == SSDS_BIT) {
    fnum = SSDS_INT_LEN;
  } else
  if(f_type == SSDS_STR) {
    if(strlen(f_val)>0) {
      fnum=atoi(f_val);
    } else {
      fnum = 32;
    }
  }
  //printf("sv_flen found sact (%s)  (%s) len %d\n",sact, f_val, fnum);
  return fnum;
}

int sdds_next_field_id(struct ssds_class *xc)
{
  int xid = 1;
  int i;
  struct ssds_field *xf;
  xf = xc->first_field;
  for(i=0; i< xc->num_fields; i++) {
    if (xf->id > xid) xid=xf->id+1;
    xf = xf->next;
  }
  return xid;
}

int sdds_next_item_id(struct ssds_class *xc)
{
  int xid = 1;
  int i;
  struct ssds_item *xi;
  xi = xc->first_item;
  for(i=0; i< xc->num_items; i++) {
    if (xi->id > xid) xid=xi->id+1;
    xi = xi->next;
  }
  return xid;
}

int sdds_next_class_id(void)
{
  int xid = 1;
  int i;
  int num;
  struct ssds_class *xc;

  if(ssds) {
    num = ssds->num_classes;
    xc = ssds->first_class;
    for(i=0; i< num; i++) {
      if (xc->id > xid) xid=xc->id+1;
      xc = xc->next;
    }
  }
  return xid;
}



struct ssds_client *get_ssds_client(int ix);
/*
 * char **argv;
 * char *myargv[16];
 * int argc;
 * argv = myargv;
 * argc = ssds_get_argc(lbuf, argv, 16);
 * argv[0]:etc
 */
int ssds_action(int fd, char *lbuf, char *sbuf, void *cli)
{
  int i;
  int ret;
  int num;
  int id;
  int f_num;
  int c_num;
  int i_num;
  int l_start;
  struct ssds_class *dclass=NULL;
  struct ssds_field *dfield=NULL;
  struct ssds_item *ditem=NULL;
  char f_val[32];

  char * sclass;
  char * sfield;
  char * saction;
  char * stype;
  char * svalue;
  char * soid;
  char * sgroup;
  char * sname;
  char * sitem;
  char * slen;
  char * snum;
  char * sfnum;
  char * scnum;
  char * sinum;
  char * sid;
  char * sdata;
  char * srep;

  sclass  = NULL;
  sdata  = NULL;
  sfield  = NULL;
  saction = NULL;
  soid    = NULL;
  sgroup  = NULL;
  sname   = NULL;
  sitem   = NULL;
  svalue  = NULL;
  stype   = NULL;
  slen    = NULL;
  snum    = NULL;
  scnum   = NULL;
  sfnum   = NULL;
  sinum   = NULL;
  sid     = NULL;
  srep     = NULL;

  char *myargv[32];
  char **argv;
  int argc;

  if(cli==NULL){
    cli = get_ssds_client(0);
  }

  argv = myargv;
  l_start = 0;
  argc = ssds_get_argc(&lbuf[l_start], argv, 32);
  if(ssds_debug>0) {
    printf("ssds_action argc = %d\n", argc);
    for (i=0; i<argc; i++) {
      printf("ssds_action argv[%d] = (%s)\n", i, argv[i]);
    }
  }


  if(strcmp(argv[argc-1], "data") == 0) { //this is special for bin
    sdata = argv[argc-1];
    printf(" Special for sdata at end %p\n", sdata);
  }
  for (i=0; i<argc-1; i++) {

    if(strcmp(argv[i], "class") == 0) {
      sclass = argv[i+1];i++;
    } else if(strcmp(argv[i], "data") == 0) { //this is special for bin
      sdata = argv[i+1];i++;
    } else if(strcmp(argv[i], "action") == 0) {
      saction = argv[i+1];i++;
    } else if(strcmp(argv[i], "field") == 0) {
      sfield = argv[i+1];i++;
    } else if(strcmp(argv[i], "type") == 0) {
      stype = argv[i+1];i++;
    } else if(strcmp(argv[i], "value") == 0) {
      svalue = argv[i+1];i++;
    } else if(strcmp(argv[i], "oid") == 0) {
      soid = argv[i+1];i++;
    } else if(strcmp(argv[i], "group") == 0) {
      sgroup = argv[i+1];i++;
    } else if(strcmp(argv[i], "name") == 0) {
      sname = argv[i+1];i++;
    } else if(strcmp(argv[i], "item") == 0) {
      sitem = argv[i+1];i++;
    } else if(strcmp(argv[i], "items") == 0) {
      sitem = argv[i+1];i++;
    } else if(strcmp(argv[i], "len") == 0) {
      slen = argv[i+1];i++;
    } else if(strcmp(argv[i], "num") == 0) {
      snum = argv[i+1];i++;
      sscanf(snum,"%d",&num);
    } else if(strcmp(argv[i], "class_num") == 0) {
      scnum = argv[i+1];i++;
      sscanf(scnum,"%d",&c_num);
    } else if(strcmp(argv[i], "field_num") == 0) {
      sfnum = argv[i+1];i++;
      sscanf(sfnum,"%d",&f_num);
    } else if(strcmp(argv[i], "item_num") == 0) {
      sinum = argv[i+1];i++;
      sscanf(sinum,"%d",&i_num);
    } else if(strcmp(argv[i], "reply") == 0) {
      srep = argv[i+1];i++;
    } else if(strcmp(argv[i], "id") == 0) {
      sid = argv[i+1];i++;
      sscanf(sid,"%d", &id);
    }
  }
  // try this as a default
  if(!saction) {
    saction=argv[0];
  }
  ret = 0;
  if(!ssds) {
    struct ssds_client *cli = NULL;
    if(1 || debug)printf("SDCS setting up ssds 1\n");
    ssds_init_x(ssds_file_name, cli);
  }

  if(ssds && ssds->debug)
    printf("SDCS action (%s) \n", saction);
  if(!saction) {
    ret = 0;
    printf(" No Action Found in command\n");
    ret += sprintf(&sbuf[ret], "rep 5555:reply:no action found:");

    for (i=0; i<argc; i++) {
      printf("ssds_action command [%d] = (%s)\n", i, argv[i]);
    }
    goto do_action_ret;
  } else {
   if(0) printf("SDCS action (%s) \n", saction);
  }

  if(strcmp(saction,"set_oid") == 0) {
    if(ssds->debug)
      printf("SDCS set_oid %p (%s) %p (%s)\n",soid, soid,
	     svalue, svalue);
    if (soid) {
      if(svalue) {
	int oid1, oid2, oid3, oids;
	char mtmp[32];
	oids = sscanf(soid,"%d %d %d",&oid1, &oid2, &oid3);
	if(ssds->debug)
	  printf(" SDCS oid1 %d oid2 %d oid3 %d\n"
		 ,oid1, oid2, oid3);
        // use same code as sv.
        sprintf(mtmp,".%d.%d.%d", oid1, oid2, oid3);
	set_action_val_name(mtmp, svalue, strlen(svalue)+1);
	//ssds_set_val(oid1, oid2, oid3, svalue);
      }
    }
  } else if (strcmp(saction, "gc") == 0) {
    ret = sprintf(sbuf, "rep 5555:action:%s:num_classes:%d:",
		  saction, ssds->num_classes);
  } else if (strncmp(saction, "gc.",3) == 0) {
    dclass = get_action_class(saction);
    if(dclass){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:name:%s:id:%d:c_num:%d:"
		    "num_fields:%d:num_items:%d:",
		    saction, dclass->name, dclass->id,
		    dclass->num, dclass->num_fields, dclass->num_items);

    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:no class found:",
		    saction);
    }
  } else if (strncmp(saction, "gf.",3) == 0) {
    dclass = get_action_class(saction);
    dfield = get_action_field(dclass, saction, 2);
    if(dfield && dclass){
      char stmp[32];
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "c_name:%s:c_id:%d:c_num:%d:"
		    "num_fields:%d:num_items:%d:"
		    "f_name:%s:f_id:%d:f_num:%d:f_type:%s:f_tid:%d:f_len:%d:",
		    saction,
		    dclass->name, dclass->id, dclass->num,
		    dclass->num_fields, dclass->num_items,
		    dfield->name, dfield->id, dfield->num,
		    get_field_type(dfield, stmp, 32)
		    , dfield->ftype, dfield->len
		    );
    } else
    if(dclass){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "c_name:%s:c_id:%d:c_num:%d:"
		    "num_fields:%d:num_items:%d:"
		    ,saction
		    ,dclass->name, dclass->id, dclass->num
		    ,dclass->num_fields, dclass->num_items
		    );

    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:no class or field found:",
		    saction);
    }
  } else if (strncmp(saction, "gi.",3) == 0) {
    dclass = get_action_class(saction);
    if(dclass){
       ditem = get_action_item(dclass, saction, 2);

    } else {
      printf("saction (%s) no class\n", saction);
    }
    if(ditem && dclass){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "c_name:%s:c_id:%d:c_num:%d:"
		    "num_fields:%d:num_items:%d:"
		    "i_name:%s:i_id:%d:i_num:%d:",
		    saction,
		    dclass->name, dclass->id, dclass->num,
		    dclass->num_fields, dclass->num_items,
		    ditem->name, ditem->id, ditem->num
		    );

    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:no class or field found:",
		    saction);
    }
  } else if (strncmp(saction, "gvf.",4) == 0) {
    int ferr;
    dclass = get_action_class(saction);

    ferr = 0;
    ret = 0;
    ditem = NULL;
    dfield = NULL;
    f_val[0]=0;

    if(dclass) {
      //printf(" gvf. found class %s\n", dclass->name);
      ditem = get_action_item(dclass, saction, 2);
      dfield = get_action_field(dclass, saction, 3);
    }
    if(ditem){
      //printf(" gvf. found item %s\n",ditem->name);
    }
    if(dfield){
      //printf(" gvf. found field %s\n",dfield->name);
    }
    if(ditem && dfield){
      ret = get_action_val(ditem, dfield, f_val, 32);
    } else {
      ferr = 0;
      if(!ditem) ferr |= 1;
      if(!dfield) ferr |= 2;
      if(!dclass) ferr |= 4;
    }

    if(ditem && dclass && dfield && (ret>0)){
      char stype[32];
      //printf(" gvf. found value %s\n", f_val);
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "c_name:%s:c_id:%d:c_num:%d:"
		    "i_name:%s:i_id:%d:i_num:%d:"
		    "f_name:%s:f_id:%d:f_num:%d:f_type:%s:f_tid:%d:f_len:%d:"
                    "value:%s:",
		    saction,
		    dclass->name, dclass->id, dclass->num,
		    //dclass->num_fields, dclass->num_items,
		    ditem->name, ditem->id, ditem->num,
		    dfield->name, dfield->id, dfield->num,
		    get_field_type(dfield, stype, 32), dfield->ftype
		    , dfield->len , f_val);

    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:%s %s %s found ferr %d:",
		    saction
		    , (ferr & 4)? "no class ":""
		    , (ferr & 2)? "no field ":""
		    , (ferr & 1)? "no item " :""
		    , ferr
		    );
    }
  } else if (strncmp(saction, "sf.",3) == 0) {
    int ferr;
    dclass = get_action_class(saction);

    ferr = 0;
    ret = 0;
    ditem = NULL;
    dfield = NULL;
    f_val[0]=0;

    if(dclass) {
      //printf(" sf. found class %s\n", dclass->name);
      dfield = get_action_field(dclass, saction, 2);
      //ditem = get_action_item(dclass, saction, 3);
    }
    if(ditem){
      //printf(" sf. found item %s\n",ditem->name);
    }
    if(dfield){
      //printf(" sf. found field %s\n",dfield->name);
    }

    ferr = 0;
    //if(!ditem) ferr |= 1;
    if(!dfield) ferr |= 2;
    if(!dclass) ferr |= 4;


    if(dclass && dfield){
      if(stype) {
	set_field_type(dfield, stype, 32);
      }
      //printf(" sf. found field (%s.%s)\n", dclass->name, dfield->name);
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "c_name:%s:c_id:%d:c_num:%d:"
		    "num_fields:%d:"
		    "f_name:%s:f_id:%d:f_num:%d:f_type:%s:f_tid:%d:",
		    saction,
		    dclass->name, dclass->id, dclass->num,
		    dclass->num_fields, //dclass->num_items,
		    //ditem->name, ditem->id, ditem->num,
		    dfield->name, dfield->id, dfield->num,
		    get_field_type(dfield, stype, 32), dfield->ftype
		    );

    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:%s %s %s found ferr %d:",
		    saction
		    , (ferr & 4)? "no class ":""
		    , (ferr & 2)? "no field ":""
		    , (ferr & 1)? "no item " :""
		    , ferr
		    );
    }
  } else if (strncmp(saction, "gn.",3) == 0) {
    int ferr;

    dclass = get_action_class(saction);
    ferr = 7;
    ret = 0;
    ditem = NULL;
    dfield = NULL;

    if(dclass) {
      ferr &= ~4;
      //printf(" gn. found class %s\n", dclass->name);
      ditem = get_action_item(dclass, saction, 2);
      dfield = get_action_field(dclass, saction, 3);
    }
    if(ditem){
      ferr &= ~2;
      //printf(" gn. found item %s\n", ditem->name);
    }
    if(dfield){
      ferr &= ~1;
      //printf(" gn. found field %s\n",dfield->name);
    }
    if(ditem && dclass && dfield ){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "name:%s.%s.%s:"
		    "num:0%d.0%d.0%d:"
		    "id:%d.%d.%d:",
		    saction
		    ,dclass->name, ditem->name, dfield->name
		    ,dclass->num, ditem->num, dfield->num
		    ,dclass->id, ditem->id, dfield->id
		    );


    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:name:not found:result:%s %s %s found ferr %d:",
		    saction
		    , (ferr & 4)? "no class ":""
		    , (ferr & 2)? "no field ":""
		    , (ferr & 1)? "no item " :""
		    , ferr
		    );
    }
  } else if (
	     (strncmp(saction, "gv.",3) == 0)
	     || (strncmp(saction, "get_value.",10) == 0) ) {
    int ferr;

    f_val[0]=0;
    ret = get_action_val_name(saction, f_val, 32);
    if(ret>0){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "value:%s:",
		    saction, f_val);
    } else {
      ferr = -ret;
      ret = sprintf(sbuf, "rep 5555:action:%s:result:%s %s %s found ferr %d:",
		    saction
		    , (ferr & 4)? "no class ":""
		    , (ferr & 2)? "no field ":""
		    , (ferr & 1)? "no item " :""
		    , ferr
		    );
    }

  } else if ((strncmp(saction, "sv.",3) == 0)
	     || (strncmp(saction, "set_value.",10) == 0)){
    int fnum;
    fnum = get_sv_value(f_val, saction, 32);
    ret = 0;
    if(fnum > 0){
      ret = set_action_val_name(saction, f_val, fnum+1);
    }

    if(ret>0){
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:result:OK:"
		    "value:%s:",
		    saction, f_val);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:result:no class, item or field found:",
		    saction);
    }

  } else if (strncmp(saction, "svb.",4) == 0) {


    char *fsp;
    char *fdsp;
    int flen;
    int mylen;

    dclass = get_action_class(saction);
    if(dclass) {
      //ferr &= ~4;
      //printf(" svb. found class (%s)\n", dclass->name);
      ditem = get_action_item(dclass, saction, 2);
      dfield = get_action_field(dclass, saction, 3);
    }
    if(ditem){
      //ferr &= ~2;
      //printf(" svb. found item (%s)\n", ditem->name);
      //printf(" svb. found item data (%p)\n", ditem->data);
    }
    if(dfield){
      //ferr &= ~1;
      //printf(" svb. found field (%s)\n",dfield->name);

      sscanf(slen,"%d",&mylen);
      //printf("svb found len = (%s) len %d\n" ,slen, mylen);
    }

    if(sdata) {
      //printf("svb data (%p) \n", sdata);
    } else {
      printf("NO svb data  \n");
    }

    if(sdata) {
      fsp = get_item_data(dfield, ditem);
      fdsp = sdata;
      flen = mylen;
      //fnum = 8.76;
      //printf("Before Float value is %f len %d\n", fnum, flen);
      while(flen--) {
	//printf( " %02x ", *fdsp);
	*fsp++ = *fdsp++;
      }
      //printf("After Float value is %f\n",fnum);
    }
    f_val[0]=0;
    ret = get_action_val_name(saction, f_val, 32);
    ret = sprintf(sbuf,
		  "rep 5555:action:%s:result:OK:value:%s:",
		  saction, f_val);
  } else if (strcmp(saction, "make_group") == 0) {

    if(ssds_debug > 1) {
      printf("ssds find_group <%s> \n", sname);
    }
    if(sname) {
      int res;      int res2;
      res2 = -1;

      if(ssds_debug > 1) {
          printf("ssds find_group <%x> \n", (unsigned int)find_group);
      }
      if ((unsigned int)find_group == 0 ) {
      }
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(ssds_debug > 1) {
          printf("ssds find_group <%s> res %d \n", sname, res);
      }
      if(sitem) {
	if(res>=0) {
	  res2 = find_group(sitem, cli, GROUP_ADD_ITEMS);
	}
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:result:OK:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      } else {
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:result:OK:name:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, res, res2);

      }
    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }

  } else if (strcmp(saction, "show_group") == 0) {
    if(sname ) {
      int res;      int res2=0;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
	ret = sprintf(sbuf,
          "rep 5555:action:%s:reply:OK:name:%s:items:"
		      ,saction, sname);
	res2 = find_group(&sbuf[ret], cli, GROUP_SEE_ITEMS);
	if(0)
	  printf("see_items ret %d res %d res2 %d\n",ret,res,res2);
        if(res2>0) ret += res2;
      } else {

	ret = sprintf(sbuf,
		      "rep 5555:action:%s:reply:Error:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }

  } else if (strcmp(saction, "get_group") == 0) {

    /* special case of a group in the server */
    //if(sid) {
    //  cli = get_ssds_client(0);
    //}
    if(sname) {
      int res;      int res2=0;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
	if(sid) {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:reply:OK:name:%s:id:%s:items:"
			,saction, sname, sid);
	} else {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:reply:OK:name:%s:items:"
			,saction, sname);
	}

	res2 = find_group(&sbuf[ret], cli, GROUP_GET_ITEMS);
	//printf("get_group items ret %d res %d res2 %d\n",ret,res,res2);
        if(res2>0) ret += res2;
      } else {
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:reply:Error:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }
  } else if (strcmp(saction, "get_group_len") == 0) {
    //if(sid) {
      //printf(" changing to server cli from %p ", cli);
      //cli = get_ssds_client(0);
      //printf(" to %p \n", cli);
      //find_group(sname, cli, PRINT_GROUP);
      //}
    if(sname) {
      int res;      int res2=0;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
	res2 = find_group(NULL, cli, GROUP_LEN_ITEMS_BIN);
	if(0)printf("get_group_len ret %d res %d res2 %d\n",ret,res,res2);

	if(sid) {
	  ret = sprintf(sbuf,
		   "rep 5555:action:%s:reply:OK:name:%s:id:%s:len:%d:res:%d:"
			,saction, sname, sid, res2, res);
	} else {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:reply:OK:name:%s:len:%d:"
			,saction, sname, res2);
	}
      } else {
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:reply:Error:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }
  } else if (strcmp(saction, "get_group_bin") == 0) {
    //if(sid) {
    //  cli = get_ssds_client(0);
    //}
    if(sname) {
      int res;      int res2=0;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
	res2 = find_group(NULL, cli, GROUP_LEN_ITEMS_BIN);
	ret = sprintf(sbuf, "bin 5555:len:%d:data:",res2);
        res2 = 0;
	res2 = find_group(&sbuf[ret], cli, GROUP_GET_ITEMS_BIN);
        if(res2>0) {
	  ret += res2;
	}
	if(0)printf("get_group_bin ret %d res %d res2 %d\n",ret,res,res2);
      } else {
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:reply:Error:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }
  } else if (strcmp(saction, "set_group_bin") == 0) {
    //if(sid) {
    //  cli = get_ssds_client(0);
    //}
    if(sname && sdata) {
      int res;      int res2=0;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
        /* copy binary items */
	if(0) {
	  printf("set_group_bin data\n");
	  for (i = 0; i < 16; i+=4) {
	    printf("[%03d] %02x %02x %02x %02x\n",
		   i, sdata[i],sdata[i+1],sdata[i+2],sdata[i+3]);
	  }
	}
	res2 = find_group(sdata, cli, GROUP_SET_ITEMS_BIN);
	ret = sprintf(sbuf, "rep 5555:result:OK:len:%d:",res2);

      } else {
	ret = sprintf(sbuf,
		      "rep 5555:action:%s:reply:Error:name:%s:items:%s:"
		      "res:%d:res2:%d:",
		      saction, sname, sitem, res, res2);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }
  } else if (strcmp(saction, "set_group") == 0) {
    //if(sid) {
    //  cli = get_ssds_client(0);
    //}
    if(sname) {
      int res;      int res2;
      res = find_group(sname, cli, GROUP_FIND_NAME);
      if(res>=0) {
	res2 = find_group(sitem, cli, GROUP_SET_ITEMS);
	if(res2>=0) {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:reply:OK:name:%s:item:%s:"
			,saction, sname, sitem);

	  //printf("set_group items ret %d res %d res2 %d\n",ret,res,res2);

	} else {

	  ret = sprintf(sbuf,
			"rep 5555:action:%s:reply:ERROR:name:%s:items:%s:"
			"res:%d:res2:%d:",
			saction, sname, sitem, res, res2);
	}
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:reply:"
		    "Error give group name and items:",
		    saction);
    }


  } else if (strcmp(saction, "hello") == 0) {
    ret = sprintf(sbuf, "rep 5555:reply:hello client:");
    if(!ssds) {
      struct ssds_client *cli = NULL;

      if(1 || debug)printf("SDCS setting up ssds 2\n");
      ssds_init_x(ssds_file_name, cli);
    }
  } else if (strcmp(saction, "set_name") == 0) {
    if(sname) {
      ssds_name(ssds, sname);
      ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK:name:%s:"
		    ,saction,sname);
    }
  } else if (strcmp(saction, "get_name") == 0) {

    char *svers;
    sname = ssds_name(ssds, NULL);
    svers = ssds_vers(ssds, NULL);
    ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK:name:%s:vers:%s:"
		  ,saction, sname, svers);
  } else if (strcmp(saction, "set_vers") == 0) {
    if(sname) {
      ssds_vers(ssds, sname);
      ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK:name:%s:"
		    ,saction,sname);
    }
  } else if (strcmp(saction, "get_vers") == 0) {

    sname = ssds_vers(ssds, NULL);
    ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK:name:%s:"
		  ,saction,sname);

  } else if (strcmp(saction,"get_classes") == 0) {
    ret = sprintf(sbuf, "rep 5555:action:%s:reply:%d:",
		  saction, ssds->num_classes);
  } else if (strcmp(saction,"num_classes") == 0) {
    ret = sprintf(sbuf, "rep 5555:action:%s:reply:%d:",
		  saction, ssds->num_classes);
  } else if (strcmp(saction,"get_items") == 0) {
    dclass = NULL;
    if(sclass) {
      dclass = ssds_find_class(sclass);
    } else if (scnum) {
      dclass = ssds_get_class_num(c_num);
    } else if (sid) {
      dclass = ssds_get_class_id(id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name or id:",
		      saction);
    }
    if(dclass) {
      ret = sprintf(sbuf, "rep 5555:action:%s:class:%s:reply:%d:",
		      saction,dclass->name, dclass->num_items);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:class not found:",
		      saction);
    }
  } else if ((strcmp(saction,"get_fields") == 0)
	     || (strcmp(saction,"num_fields") == 0)) {
    dclass = NULL;
    if(sclass) {
      dclass = ssds_find_class(sclass);
    } else if (scnum) {
      dclass = ssds_get_class_num(c_num);
    } else if (sid) {
      dclass = ssds_get_class_id(id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name or id:",
		      saction);
    }
    if(dclass) {
      ret = sprintf(sbuf, "rep 5555:action:%s:class:%s:reply:%d:id:%d:",
		    saction, dclass->name, dclass->num_fields,
		    dclass->id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:class not found:",
		      saction);
    }
  } else if ((strcmp(saction,"get_items") == 0)
	     || (strcmp(saction,"num_items") == 0)) {
    dclass = NULL;
    if(sclass) {
      dclass = ssds_find_class(sclass);
    } else if (scnum) {
      dclass = ssds_get_class_num(c_num);
    } else if (sid) {
      dclass = ssds_get_class_id(id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name or id:",
		      saction);
    }
    if(dclass) {
      ret = sprintf(sbuf, "rep 5555:action:%s:class:%s:reply:%d:",
		      saction,dclass->name, dclass->num_items);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:class not found:",
		      saction);
    }
  } else if (strcmp(saction,"get_class_num") == 0) {

    dclass = NULL;
    if(scnum) {
      dclass = ssds_get_class_num(c_num);
    } else if(snum) {
      dclass = ssds_get_class_num(num);
    }

    if(dclass) {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:class:%s:id:%d:items:%d:fields:%d",
		    saction, dclass->name, dclass->id
		    , dclass->num_items, dclass->num_fields);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:num:%s:error:no class:",
		    saction, snum);
    }
  } else if (strcmp(saction,"get_field_num") == 0) {

    if(snum || sfnum) {
      dclass = NULL;
      if(sclass) {
	dclass = ssds_find_class(sclass);
      } else if(scnum) {
	dclass = ssds_get_class_num(c_num);
      } else if (sid) {
	dclass = ssds_get_class_id(id);
      } else {
	ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name or id:",
		      saction);
      }
      if(dclass) {
        if(snum) {
	  dfield = ssds_get_field_num(dclass, num);
	} else if (sfnum) {
	  dfield = ssds_get_field_num(dclass, f_num);
	}
	if(dfield) {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:class:%s:field:%s:id:%d:type:%d:",
			saction,dclass->name, dfield->name
			,dfield->id, dfield->ftype);
	} else {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:class:%s:num:%d:error:field not found:",
			saction,dclass->name,num);
	}
      } else {
	ret = sprintf(sbuf, "rep 5555:action:%s:error:class not found:",
		      saction);
      }
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no field number:",
		    saction);
    }
  } else if (strcmp(saction,"get_item_num") == 0) {
    if(snum) {
      dclass = NULL;
      if(sclass) {
	dclass = ssds_find_class(sclass);
      } else if (scnum) {
	dclass = ssds_get_class_num(c_num);
      } else if (sid) {
	dclass = ssds_get_class_id(id);
      } else {
	ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name or id:",
		      saction);
      }
      if(dclass) {
        if(sinum) {
	  ditem = ssds_get_item_num(dclass, i_num);
	} else {
	  ditem = ssds_get_item_num(dclass, num);
	}

	if(ditem) {
	  ret = sprintf(sbuf, "rep 5555:action:%s:class:%s:item:%s:id:%d:",
			saction,dclass->name, ditem->name, ditem->id);
	} else {
	  ret = sprintf(sbuf,
			"rep 5555:action:%s:class:%s:num:%d:error:item not found:",
			saction, dclass->name,num);
	}
      } else {
	ret = sprintf(sbuf, "rep 5555:action:%s:error:class not found:",
		      saction);
      }
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no item number:",
		    saction);
    }
  } else if (strcmp(saction,"get_item_value") == 0) {
    dclass = NULL;
    if(sclass) {
      dclass = ssds_find_class(sclass);
    } else if (scnum) {
      //printf(" calling ssds_get_class_num(%d)\n",c_num);
      dclass = ssds_get_class_num(c_num);
      //printf(" called ssds_get_class_num(%p)\n",dclass);
    } else if (sid) {
      dclass = ssds_get_class_id(id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name,num or id:",
		    saction);
    }
    ditem = NULL;
    if(dclass) {
      if(sinum) {
	ditem = ssds_get_item_num(dclass, i_num);
      } else {
	ditem = ssds_get_item_num(dclass, num);
      }
    }
    dfield = NULL;
    if(ditem && dclass) {
      if(sfnum) {
	dfield = ssds_get_field_num(dclass, f_num);
      } else {
	dfield = ssds_get_field_num(dclass, num);
      }
    }
    if(dfield && ditem) {
      char *data;
      int ftype;
      int flen;

      data = get_item_data(dfield, ditem);
      ftype=dfield->ftype;
      flen=dfield->len;

      ret = sprintf(sbuf,
		 "rep 5555:action:%s:class:%s:item:%s:id:%d:field_id:%d:type:%d:value:",
		    saction, dclass->name, ditem->name, ditem->id,
		    dfield->id, dfield->ftype);
      ret += get_action_val(ditem, dfield, &sbuf[ret], 32);
      ret += sprintf(&sbuf[ret],":");
    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:num:%d:error:item not found:",
		    saction,num);
    }

  } else if (strcmp(saction,"set_item_value") == 0) {
    dclass = NULL;
    if(sclass) {
      dclass = ssds_find_class(sclass);
    } else if (scnum) {
      dclass = ssds_get_class_num(c_num);

    } else if (sid) {
      dclass = ssds_get_class_id(id);
    } else {
      ret = sprintf(sbuf, "rep 5555:action:%s:error:no class name,num or id:",
		    saction);
    }
    ditem = NULL;
    if(dclass) {
      if(sinum) {
	ditem = ssds_get_item_num(dclass, i_num);
      } else {
	ditem = ssds_get_item_num(dclass, num);
	i_num = num;
      }
    }
    dfield = NULL;
    if(ditem && dclass) {
      if(sfnum) {
	dfield = ssds_get_field_num(dclass, f_num);
      } else {

	dfield = ssds_get_field_num(dclass, num);
	f_num = num;
      }
    }
    if(dfield && ditem && svalue) {
      ret = set_action_val(ditem, dfield, svalue, strlen(svalue));

      if(ret >0) {
	ret = sprintf(sbuf,
		 "rep 5555:action:%s:class_num:%d:item_num:%d:field_num:%d:result:OK:",
		    saction, c_num, i_num, f_num);
      } else {
	ret = sprintf(sbuf,
		 "rep 5555:action:%s:class_num:%d:item_num:%d:field_num:%d:result:FAIL:",
		    saction, c_num, i_num, f_num);
      }

    } else {
      ret = sprintf(sbuf,
		    "rep 5555:action:%s:class:%s:num:%d:error:item not found:",
		    saction, dclass->name,num);
    }

  } else if (strcmp(saction,"set_field") == 0) {
    if(sclass) {
      if(sfield) {
	if(sitem) {
	  if(svalue) {
	    //int cnt;
            //float fval;
            //int ival;
            if(sclass) {
	      dclass = (struct ssds_class *)ssds_find_class(sclass);
	    } else if(scnum) {
	      dclass = (struct ssds_class *)ssds_get_class_num(c_num);
	    }

            if(sfield) {
	      dfield =
		(struct ssds_field *)ssds_find_field(dclass,sfield,-1,-1);
	    } else if(sfnum) {
	      dfield =
		(struct ssds_field *)ssds_get_field_num(dclass, f_num);
	    }
	    if(sitem) {
	      ditem = (struct ssds_item *)ssds_find_item(dclass,sitem);
	    } else if(sinum) {
	      ditem = (struct ssds_item *)ssds_get_item_num(dclass, i_num);
	    }
            if(dclass && ditem && dfield) {
	      set_action_val(ditem, dfield, svalue, strlen(svalue));
	    }
	  }
	}
      }
    }
  } else if (strcmp(saction,"create_class") == 0) {
    int new;
    if(sclass) {
      dclass = (struct ssds_class *)ssds_find_class_new(sclass, &new);
    } else if (scnum) {
      dclass = (struct ssds_class *)ssds_get_class_num(c_num);
    }
    if(dclass) {
      if(sid) {
	dclass->id = id;
      } else {
	if(new) dclass->id = sdds_next_class_id();
      }
    }
    if(srep) {
      ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK::", saction);
    }
  } else if (strcmp(saction,"create_field") == 0) {
    int new=0;
    if(ssds->debug)
      printf("SDCS class (%s) create_field (%s) type (%s)\n"
	     , sclass, sfield, stype);
    if(sclass) {
      if(sfield) {
	if(stype) {
	  int ftype;
	  int flen;
          unsigned int bmask;
          unsigned int bshift;

          if(sclass) {
	    dclass = (struct ssds_class *)ssds_find_class_new(sclass, &new);
          } else if(scnum) {
	    dclass = (struct ssds_class *)ssds_get_class_num(c_num);
	  }
	  if(ssds->debug)
	    printf("SDCS dclass (%p) (%s) \n", dclass, dclass->name);
          flen=32;
	  if (strcmp(stype, "float") == 0) {
	    ftype = SSDS_FLOAT;
	    flen = SSDS_FLOAT_LEN;
	  } else if (strcmp(stype, "int") == 0) {
	    ftype = SSDS_INT;
	    flen = SSDS_INT_LEN;
	  } else if (strcmp(stype, "string") == 0) {
	    ftype = SSDS_STR;
	    if(slen) sscanf(slen,"%d",&flen);
	  } else if (strncmp(stype, "bit", 3) == 0) {
	    ftype = SSDS_BIT;
            bmask = get_field_bit(stype, 1, 0);
            bshift = get_field_bit(stype, 0, 0);
	  } else if (strcmp(stype, "str") == 0) {
	    ftype = SSDS_STR;
	    if(slen)sscanf(slen,"%d",&flen);
	  } else {
	    ftype = SSDS_INVALID;
	    flen = SSDS_INVALID_LEN;
	  }

          if(sfield) {
	    if(ssds->debug)
	      printf("SDCS dclass (%s) find field (%s)\n", dclass->name
		     , sfield);

	    dfield = (struct ssds_field *)ssds_find_field_new(dclass
							      ,sfield
							      ,ftype
							      ,flen
							      ,&new);
	    if(ssds->debug)
	      printf("SDCS find field done\n");
	  } else if(sfnum) {
	    dfield = (struct ssds_field *)ssds_get_field_num(dclass
							     ,f_num);
	  }
	  if(dfield) {
	    if(sid) {
	      dfield->id = id;
	    } else {
	      if(new)dfield->id = sdds_next_field_id(dclass);
	    }
	  }
	  if(ssds->debug)
	    printf("SDCS dfield (%p) (%s) \n", dfield, dfield->name);
	}
      }
    }
    if(srep) {
      ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK::",
		    saction);
    }
  } else if (strcmp(saction,"create_item") == 0) {
    dclass = NULL;
    if(sclass) {
      dclass = (struct ssds_class *)ssds_find_class(sclass);
    } else if(scnum) {
      dclass = (struct ssds_class *)ssds_get_class_num(c_num);
    }
    if(dclass) {
      int new=0;
      if(sitem) {
	ditem = (struct ssds_item *)ssds_find_item_new(dclass, sitem, &new);
      } else if(sinum) {
	ditem = (struct ssds_item *)ssds_get_item_num(dclass, i_num);
      }
      if(ditem) {
	if(sid) {
	  ditem->id = id;
	} else {
	  if(new)
	    ditem->id = sdds_next_item_id(dclass);
	}
      }
    }
    if(srep) {
      ret = sprintf(sbuf, "rep 5555:action:%s:reply:OK::", saction);
    }
  } else {
    ret = sprintf(sbuf, "rep 5555:action:%s:error:not understood:",
		  saction);
  }
 do_action_ret:
  return ret;
}

int ssds_get_data_size(void)
{
  int ret;
  ret =-1;
  if(ssds) {
    ret = ssds->num_data;
  }
  return ret;
}

int ssds_get_def_size(void)
{
  int ret;
  ret =-1;
  if(ssds) {
    ret = ssds->num_defs;
  }
  return ret;
}
