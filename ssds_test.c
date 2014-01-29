/*
 * ssds_test.c
 * small system data server direct test
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

#include "ssds.h"

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
static int get_argc(char *buf, char *argv[], int max)
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


int test_argc(void)
{
  char **argv;
  char *myargv[16];
  int argc;
  int idx;
  char lbuf[132];

  strcpy(lbuf, "class:alarm:action:create_class:");
  strcpy(lbuf, "class:alarm:action:create_field:InputAlarmValue:float:");
  strcpy(lbuf, "class:alarm:action:create_field:MaxAlarmValue:float:");
  //  ssds_action(0,lbuf,sbuf, NULL);

  argv = myargv;
  printf("test_argc lbuf was (%s) \n", lbuf);
  argc = get_argc(lbuf, argv, 16);
  idx = 0;

  while (argc--) {
    printf(" %d (%s) \n", idx, argv[idx]);
    idx++;
  }
  return 0;
}

/* this is a mess */
struct ssds_client *get_ssds_client(int ix)
{
  struct ssds_client *cli = NULL;

  return cli;
}

#include "ssds_find_group.c.inc"
unsigned int get_field_bit(char *stype, int mask, int debug);
int test_get_field_bit(void)
{

   get_field_bit("bit_0.0", 1, 1);
   get_field_bit("bit_0FF00.0",0,1);
   get_field_bit("bit_0.4",1,1);
   get_field_bit("bit_30.4",1,1);
   return 0;
}


int ssds_init_x(void *, void *);
int main(int argc, char *argv[])
{
  void *dummy;
  void *item=NULL;
  char lbuf[160];
  char sbuf[160];
  int ssds;
  char *svalue;


  ssds_set_find_group(do_find_group);
  ssds = ssds_init_x(NULL,NULL);
  test_get_field_bit();
  return 0;

  //  dummy = ssds_find_class("Alarms");
  //dummy = ssds_find_class("Equations");
  //dummy = ssds_find_class("Inputs");
  //dummy = ssds_find_class("Outputs");
  //dummy = ssds_find_class("Alarms");
  //dummy = ssds_find_class("Equations");
  //dummy = ssds_find_class("Inputs");
  //dummy = ssds_find_class("Outputs");


  ssds_print_classes(1);
  //printf(" SDCS running set_val\n");

  //ssds_set_val(1,2,1,"221.3");
  //printf(" SDCS done set_val\n");
  dummy = ssds_find_class("Alarms");

  //dummy = NULL;
  if(dummy) {
    //ssds_find_field(dummy,"InputAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"MaxAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"MinAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"HystAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"AlarmStatus",SSDS_INT, SSDS_INT_LEN);
    //ssds_print_class(dummy);
    //item = ssds_find_item(dummy,"Alarm1");
    //item = ssds_find_item(dummy,"Alarm2");
    //ssds_print_items(dummy);
    //ssds_print_item(item);

    svalue = "121.3";  set_action_val_name(".1.1.1",svalue,strlen(svalue)+1);
    svalue = "125.4";  set_action_val_name(".1.1.2",svalue,strlen(svalue)+1);
    svalue = "15.5";   set_action_val_name(".1.1.3",svalue,strlen(svalue)+1);
    svalue = "5.6";    set_action_val_name(".1.1.4",svalue,strlen(svalue)+1);
    svalue = "1234567";set_action_val_name(".1.1.5",svalue,strlen(svalue)+1);

    svalue = "221.1";  set_action_val_name(".1.2.1",svalue,strlen(svalue)+1);
    svalue = "225.4";  set_action_val_name(".1.2.2",svalue,strlen(svalue)+1);
    svalue = "25.5";   set_action_val_name(".1.2.3",svalue,strlen(svalue)+1);
    svalue = "5.6";    set_action_val_name(".1.2.4",svalue,strlen(svalue)+1);
    svalue = "1234567";set_action_val_name(".1.2.5",svalue,strlen(svalue)+1);
    item = ssds_find_item(dummy,"Alarm1");
    ssds_print_item(item);
    item = ssds_find_item(dummy,"Alarm2");
    ssds_print_item(item);

  }

  //return 0;

  if(0) {
    strcpy(lbuf, "class:Alarms2:action:create_class:");
    ssds_action(ssds, lbuf, sbuf, NULL);


    strcpy(lbuf,
       "class:Alarms2:action:create_field:field:InputAlarmValue:type:float:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:action:create_field:field:MaxAlarmValue:type:float:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:action:create_field:field:MinAlarmValue:type:float:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:action:create_field:field:HystAlarmValue:type:float:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:action:create_field:field:AlarmStatus:type:int:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "class:Alarms2:action:create_item:item:Alarm1:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "class:Alarms2:action:create_item:item:Alarm2:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "class:Alarms2:action:create_item:item:Alarm3:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "class:Alarms2:action:create_item:item:Alarm4:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:item:Alarm1:action:set_field:field:InputAlarmValue"
     ":value:2100.3:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:item:Alarm1:action:set_field:field:MaxAlarmValue"
     ":value:2500.0:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf,
     "class:Alarms2:item:Alarm1:action:set_field:field:MinAlarmValue"
     ":value:500.0:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "oid:5 3 4:action:set_oid:value:0.504:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    strcpy(lbuf, "oid:5 3 5:action:set_oid:value:123:");
    ssds_action(ssds, lbuf, sbuf, NULL);

    dummy = ssds_find_class("Alarms2");
  }
  dummy = ssds_find_class("Alarms");
  if(dummy) {
    //ssds_find_field(dummy,"InputAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"MaxAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"MinAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"HystAlarmVal",SSDS_FLOAT, SSDS_FLOAT_LEN);
    //ssds_find_field(dummy,"AlarmStatus",SSDS_INT, SSDS_INT_LEN);

    //ssds_find_item(dummy,"Alarm2");
    //ssds_find_item(dummy,"Alarm3");
    //ssds_find_item(dummy,"Alarm4");

    //fval = 21.3;    ssds_set_val(1,1,1,(void *)&fval);
    //fval = 25.0;    ssds_set_val(1,1,2,(void *)&fval);
    //fval = 5.0;     ssds_set_val(1,1,3,(void *)&fval);
    //fval = 0.5;     ssds_set_val(1,1,4,(void *)&fval);
    //ival = 123;     ssds_set_val(1,1,5,(void *)&ival);
    item = ssds_find_item(dummy,"Alarm1");
  }

  ssds_print_class(dummy);
  ssds_print_items(dummy);
  ssds_print_item(item);

  test_argc();
  {
    int val;
    val =  ssds_get_def_size();
    printf(" SDCS def size = %d \n", val);
    val =  ssds_get_data_size();
    printf(" SDCS data size = %d \n", val);
  }
  return 0;
}
