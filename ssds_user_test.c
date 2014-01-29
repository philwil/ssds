/*
 * ssds_user_test.c
 *
 * basic system client test
 * server must be running
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 */
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

#include "ssds_user.h"

extern int cl_db;

struct ssds_connect pccon;

extern char *def_node_addr;
extern int port_num;

int main(int argc, char *argv[])
{
  int pcfd;
  int i;
  int num;
  int glen;

  float *fp;
  unsigned char data[256];
  char res[256];
  char stmp[256];
  int ret;

  pccon.ipaddr =def_node_addr;
  pccon.port =DEF_PORT_NUM;

  if(argc>1) {
    pccon.ipaddr =argv[1];
  }

  if(argc>2) {
    pccon.port =atoi(argv[2]);
  }

  pcfd = ssds_open(&pccon);
  if (pcfd < 0) {
    exit(-1);
  }

  ssds_get_name(pcfd, res, 32);
  printf("SSDS name (%s) \n", res);
  ssds_get_vers(pcfd, res, 32);
  printf("SSDS vers (%s) \n", res);
  //cl_db = 1;
  num = ssds_get_num_classes(pcfd);
  printf("SSDS getting num_classes (%d) \n", num);
  //cl_db = 1;
  for (i = 0; i < num; i++) {
    sprintf(stmp,"0%d",i);
    ssds_get_class_name(pcfd,stmp,res,32);
    printf("SSDS class (%d) (%s) \n", i,res );
  }
  //cl_db = 1;
  ssds_get_item_value(pcfd, "0.0.0" , res, 32);
  printf("SSDS get item 0.0.0 value (%s) \n", res );

  //cl_db = 1;

  /* must set all fields before creating any items */
  /* possibly we should lock / unlock class */
  /* this will create the fields */
  printf("SSDS Setting Fields  \n");

  ssds_set_field_type(pcfd, "MyAlarms.OutputAlarmValue", "float", res, 32);
  ssds_set_field_type(pcfd, "MyAlarms.InputAlarmValue", "float", res, 32);
  ssds_set_field_type(pcfd, "MyAlarms.HighAlarmValue", "float", res, 32);
cl_db = 1;
  ssds_set_field_type(pcfd, "MyAlarms.LowAlarmValue", "float", res, 32);
  printf("SSDS Field Type  (%s) \n", res );


  /* this will create an item MyAlarm1 */

cl_db = 1;
  ssds_get_item_value(pcfd, "MyAlarms.MyAlarm1.OutputAlarmValue", res, 32);
  printf("SSDS create item result (%s) \n", res );
  ssds_get_item_value(pcfd, "MyAlarms.MyAlarm2.OutputAlarmValue", res, 32);

  /* add a field to a class after defining an item */
  cl_db = 1;
  ssds_set_field_type(pcfd, "MyAlarms.MidAlarmValue", "float", res, 32);

  /* set a value as text */
  ssds_set_item_value(pcfd,
		      "MyAlarms.MyAlarm1.OutputAlarmValue", "23.45", res, 32);
  printf("SSDS set item value from text (%s) \n", res );

  /* set a field and default value as text */
  cl_db = 1;
  ssds_set_item_value(pcfd,
		      "MyAlarms..TestAlarmValue[float]", "123.45", res, 32);
  printf("SSDS set item value from text (%s) \n", res );

  /* recover a value as text*/
  ssds_get_item_value(pcfd, "MyAlarms.MyAlarm1.OutputAlarmValue", res, 32);

  printf("SSDS get item value as text (%s) \n", res );

  /* set binary values */
  {
    float fvalue;
    //cl_db = 1;
    fvalue = 567.89;
    ssds_set_item_value_bin(pcfd, "MyAlarms.MyAlarm1.OutputAlarmValue",
			    (char *)&fvalue, 4,
			    res, 32);
    printf("SSDS set item value as binary (%s) \n", res );
  }

  /* moving on to groups */

  /* first create a group, note that this is additive*/
  //cl_db = 1;
  ssds_create_group(pcfd, "MyGroup", res , 32);
  printf("SSDS create group  (%s) \n", res );

  /* then add some items to the group the "-" will reset the group */
  ssds_add_group_item(pcfd, "MyGroup", "-MyAlarms.MyAlarm1.OutputAlarmValue",
		  res , 32);
  ssds_add_group_item(pcfd, "MyGroup", "MyAlarms.MyAlarm1.InputAlarmValue",
		  res , 32);
  ssds_add_group_item(pcfd, "MyGroup", "MyAlarms.MyAlarm1.HighAlarmValue",
		  res , 32);
  ssds_add_group_item(pcfd, "MyGroup", "MyAlarms.MyAlarm1.LowAlarmValue",
		  res , 32);
  printf("SSDS add group items (%s) \n", res );


  cl_db = 1;
  /* get the group data size */
  ret = ssds_get_group_len(pcfd, "0AlarmGroup", res , 132);
  glen = ret;
  printf("SSDS get group len %d (%s) glen %d\n", ret, res, glen);
  cl_db = 1;
  /* get the group data size */
  ret = ssds_get_group_len(pcfd, "0AlarmGroup2", res , 132);
  glen = ret;
  printf("SSDS get group len %d (%s) glen %d should be zero (new group)\n"
	 , ret, res, glen);

  /* get the group data size */
  ret = ssds_get_group_len(pcfd, "MyGroup", res , 132);
  glen = ret;
  printf("SSDS get group len %d (%s) glen %d\n", ret, res, glen);

  //cl_db = 1;
  ssds_get_group_data(pcfd, "MyGroup", res , 256);
  printf("SSDS get group data (%s) \n", res );

  if(1) {
    for (i =0; i < glen; i++) {
      data[i]=0;
    }
    //cl_db = 1;
    /* get the group data as binary */
    ret = ssds_get_group_data_bin(pcfd, "MyGroup", (char *)data, glen,
				  res , 132);
    printf("SSDS get group data bin ret %d res (%s) \n", ret, res );

    /* lets check it out first as hex numbers*/
    for (i =0; i < glen; i+=4) {
      printf(" idx [%02d] %02x %02x %02x %02x \n"
	     , i, data[i], data[i+1], data[i+2], data[i+3]);
    }
    /* then as float numbers */
    printf("printing  data as floats\n");
    for (i =0; i < glen; i+=4) {
      fp = (float*)&data[i];
      printf("[%02d] %f\n",i,*fp);
    }

  }
  /* next we set up floating point data */
  printf("setting float data \n");
  for (i =0; i < glen; i+=4) {
    fp = (float*)&data[i];
    *fp = *fp + 1.25;
    printf("[%02d] %f\n",i,*fp);
  }

  printf("sending group data\n");

  for (i =0; i < glen; i+=4) {
    printf("[%02d] %02x %02x %02x %02x \n",
	   i, data[i+0], data[i+1], data[i+2], data[i+3] );
  }
  //cl_db = 1;
  ret = ssds_set_group_data_bin(pcfd, "MyGroup", (char*)data, glen, res, 132);
  printf("SSDS set group data bin (%s) \n", res );

  close(pcfd);

  return 0;
}
