#ifndef _SSDS_USER
#define _SSDS_USER

/*
 * ssds_user.h
 * small system data server
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 */


#define DATA_SIZE (1024*16)
#define DEF_SIZE (1024*16)

#define NAME_LEN 32

#define NUM_CLASSES 256
#define NUM_ITEMS 1024
#define NUM_FIELDS 1024
#define NUM_CLASS_FIELDS 16
#define NUM_CLASS_ITEMS 256

#define DEF_NODE_ADDR "127.0.0.1"
#define DEF_PORT_NUM 4321

#define SSDS_DEF_RBUF_LEN 256
#define SSDS_DEF_SBUF_LEN 256


struct ssds_connect {
  char *name;
  char *ipaddr;
  int port;
  int fd;
};

struct ssds_user {
  char name[NAME_LEN];
  int num;
  int id;
  int fd;
  int num_classes;
  int num_items;
  int num_fields;

  int cur_classes;
  int cur_items;
  int cur_fields;

  int max_classes;
  int max_items;
  int max_fields;

  int *classes[NUM_CLASSES];
  int *items[NUM_ITEMS];
  int *fields[NUM_FIELDS];

};

struct ssds_class {
  char name[NAME_LEN];
  int num;
  int id;
  int num_fields;
  int num_items;
  int *fields[NUM_CLASS_FIELDS];
  int *items[NUM_CLASS_ITEMS];
};

struct ssds_field {
  char name[NAME_LEN];
  int num;
  int id;
  int len;
  int ftype;
  int offset;
};

struct ssds_item {
  char name[NAME_LEN];
  int num;
  int id;
  int class;
  char *data;
};


//char *data[DATA_SIZE];
//char *defs[DEF_SIZE];

int ssds_open(struct ssds_connect *pccon);
int ssds_get_name(int pcfd, char *res, int len);
int ssds_get_vers(int pcfd, char *res, int len);

int ssds_get_num_classes(int pcfd);
int ssds_get_class_name(int pcfd, char *class_def , char *res, int len);
int ssds_get_class_num(int pcfd, char *class_def);
int ssds_get_class_id(int pcfd, char *class_def);
int ssds_get_num_class_fields(int pcfd, char *class_def);

int ssds_get_field_name(int pcfd, char *class_def , char *res, int len);
int ssds_get_field_num(int pcfd, char *class_def);
int ssds_get_field_id(int pcfd, char *class_def);
int ssds_set_field_type(int pcfd, char *class_def, char *type_def, char *res, int len );
int ssds_set_field_len(int pcfd, char *class_def, int len);

int ssds_get_num_class_items(int pcfd, char *class_def);
int ssds_get_item_name(int pcfd, char *class_def , char *res, int len);
int ssds_get_item_num(int pcfd, char *class_def);
int ssds_get_item_id(int pcfd, char *class_def);


int ssds_get_item_value(int pcfd, char *full_def, char *reply, int len);
int ssds_get_item_value_bin(int pcfd, char *full_def, char *reply, int len);
int ssds_set_item_value(int pcfd, char *full_def, char * val, char *reply, int len);
int ssds_set_item_value_bin(int pcfd, char *full_def, char * val, int vlen, char *reply, int len);


int ssds_create_group(int pcfd, char *list_name, char *res , int rlen);
int ssds_add_group_item(int pcfd,char *list_name, char *idef, char *res, int rlen);
int ssds_get_group_data(int pcfd, char *list_name, char *res, int rlen);
int ssds_get_group_len(int pcfd, char *list_name, char *res , int rlen);

int ssds_set_group_data_bin(int pcfd, char *list_name, char*data
			    , int dlen, char *res, int rlen);

int ssds_get_group_data_bin(int pcfd, char *list_name,
			    char *data, int dlen, char *res, int rlen);

#endif
