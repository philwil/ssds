#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>


#define DEF_SYS_VERS "1.0.1"

char *sys_vers = DEF_SYS_VERS;


int ssds_num_classes(int fd)
{
  return 4;
}

int ssds_num_fields(int fd, int class_num)
{
  if(class_num == 0)
    return 3;
  if(class_num == 1)
    return 4;
  if(class_num == 2)
    return 2;
  if(class_num == 3)
    return 6;
  return 8;
}

char *get_system_name(void)
{
  static char name[32];
  sprintf(name, "SSDS SYSTEM");
  return name;
}

char *get_system_vers(void)
{
  static char name[32];
  sprintf(name, "version %s", sys_vers);
  return name;
}
int ssds_class_name(int fd, int class_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"CNAM%d", class_num);
  return ret;
}

char *get_item_class_name(int class_num, int item_num)
{
  static char name[32];
  sprintf(name,"ICNAM%d.%d", class_num, item_num);
  return name;
}

int ssds_class_id(int fd, int class_num, char * sp)
{
  int ret;
  ret = sprintf(sp,"%d", class_num+1);
  return ret;
}

int ssds_field_name(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"FNAM%d.%d", class_num, field_num);
  return ret;
}


int ssds_field_id(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"%d", field_num+1);
  return ret;
}

int ssds_field_type(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"FTYPE%d", class_num, field_num);
  return ret;
}

int ssds_field_value(int fd, int class_num, int field_num, int item_num
		     , char *sp)
{
  int ret;
  ret = sprintf(sp,"%d", class_num *100+(field_num+1)*10 +item_num);
  return ret;
}

char *get_item_field_name(int class_num, int field_num, int item_num)
{
  static char name[32];
  sprintf(name,"I_FNAM%d.%d.%d", class_num, field_num, item_num);
  return name;
}


char *get_item_field_id(int class_num, int field_num, int item_num)
{
  static char name[32];
  sprintf(name,"%d.%d.%d", class_num, field_num+1,item_num);
  return name;
}

char *get_item_field_value(int class_num, int field_num, int item_num)
{
  static char name[32];
  sprintf(name,"%d", class_num *100+(field_num+1)*10 +item_num);
  return name;
}

char *get_item_field_type(int class_num, int field_num, int item_num)
{
  static char name[32];
  sprintf(name,"I_FTYPE%d.%d.%d", class_num, field_num, item_num);
  return name;
}

int ssds_item_name(int fd, int class_num, int item_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"INAM%d.%d", class_num, item_num);
  return ret;
}

int ssds_item_id(int fd, int class_num, int item_num, char *sp)
{
  int ret;
  ret = sprintf(sp,"%d", item_num+1);
  return ret;
}

int setup_ssds_client(void)
{
  return 2;
}

int ssds_num_items(int fd, int class_num)
{
  if(class_num == 0)
    return 5;
  if(class_num == 1)
    return 5;
  if(class_num == 2)
    return 2;
  if(class_num == 3)
    return 2;
  return 2;
}
