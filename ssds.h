/*
 * ssds.h
 * small system data server
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 */

#ifndef _SSDS_H
#define _SSDS_H

#include <netinet/in.h>
#include <arpa/inet.h>

#define MAX_CLASSES 16
#define MAX_FIELDS 256
#define MAX_ITEMS 256
#define MAX_DATA (1024 * 1024)
#define DEFS_SIZE (1024 * 16)
#define DATA_SIZE (1024 * 16)


#define NUM_ITEMS 16
#define NUM_FIELDS 16
#define NAME_LEN 32
#define SSDS_INVALID   -1
#define SSDS_INVALID_LEN   -1

#define SSDS_INT   1
#define SSDS_FLOAT 2
#define SSDS_STR   3
#define SSDS_BIT   4
#define SSDS_SSDS  1000
#define SSDS_CLASS 1001
#define SSDS_FIELD 1002
#define SSDS_ITEM  1003

#define SSDS_FILE "/etc/ssds.conf"
#define SSDS_LOCAL_FILE "./ssds.conf"



#define SSDS_INT_LEN (sizeof(int))
#define SSDS_FLOAT_LEN (sizeof(float))

// data contains all the fields for an entry
struct ssds_item {
  struct ssds_item *next;
  struct ssds_item *next_def;
  int type;
  void *self;
  char name[NAME_LEN];
  int id;
  int num;
  int ix;
  int idx;  //index into data
  char *data_ix[16];
  struct ssds *ssds;
  struct ssds_class *ssds_class;

};

// defines a field for a class
// all fields in a class must be defined before
// the first item or instance is set up
//
struct ssds_field {
  struct ssds_field *next;
  struct ssds_field *next_def;
  int type;
  void *self;
  char name[NAME_LEN];
  int ftype;
  unsigned int bmask;  // bitmask ie 0x00000F00
  int bshift;          // bit shift to get value (8 in this case)
  int id;
  int num;
  int ix;
  int idx;
  int len;

  struct ssds *ssds;
  struct ssds_class *class;

};

// A class
struct ssds_class {
  struct ssds_class *next;
  struct ssds_class *next_def;
  int type;
  void *self;
  char name[NAME_LEN];
  int inuse;
  int id;
  int num;
  int ix;
  int data_len;
  struct ssds *ssds;

  int num_items;
  struct ssds_item *first_item;
  struct ssds_item *last_item;

  int num_fields;
  struct ssds_field *first_field;
  struct ssds_field *last_field;

};


/*
 * The DataBase
 */
struct ssds {
  struct ssds *next;
  int type;
  void *self;
  char name[NAME_LEN];
  char vers[NAME_LEN];

  int debug;

  int num_classes;
  struct ssds_class *first_class;
  struct ssds_class *last_class;

  int num_items;
  struct ssds_item *first_item;
  struct ssds_item *last_item;

  int num_fields;
  struct ssds_field *first_field;
  struct ssds_field *last_field;

  int num_defs;
  int max_defs;
  char *defs;

  int num_data;
  int max_data;
  char *data;
};


#define RXBUF_LEN 512
#define TXBUF_LEN 512

/* max number of clients we can talk to */
#define NUM_CLIENTS 32

/* max number of groups defined by each client */
#define NUM_GROUPS 32
#define NUM_GENTS 32


#define GROUP_FIND_NAME 1
#define GROUP_ADD_ITEMS 2
#define GROUP_SET_ITEMS 3
#define GROUP_GET_ITEMS 4
#define GROUP_SEE_ITEMS 5
#define GROUP_GET_ITEMS_BIN 6
#define GROUP_SET_ITEMS_BIN 7
#define GROUP_LEN_ITEMS_BIN 8
#define PRINT_GROUP 9

#define SSDS_DEF_RBUF_LEN 256
#define SSDS_DEF_SBUF_LEN 256

struct cli_group_entry {
  char name[64];
  short f_len;
  short f_type;
  char *data;
};

struct cli_group {
  char name[64];
  int num_entries;
  int max_entries;
  struct cli_group_entry entries[NUM_GENTS];
};


struct ssds_client {
  struct ssds_client *next;
  struct ssds_client *next_def;
  int type;
  int fd;
  int idx;
  int ix;
  int is_auto;
  char ipaddr[32];
  int port;
  char rxbuf[RXBUF_LEN];
  int rxbuf_len;
  char txbuf[RXBUF_LEN];
  int txbuf_len;
  struct sockaddr_in cli_addr;
  int num_groups;
  int max_groups;
  struct cli_group *curr_group;
  struct cli_group groups[NUM_GROUPS];
};

int ssds_init(char *fname);
char * ssds_alloc_data(int size);
char * ssds_alloc_def(int size);

void *ssds_find_class(char *name);
void *ssds_get_class(int num);
int ssds_get_num_classes(void);

void *ssds_find_field(struct ssds_class *pclass, char *name, int f_type, int len);

int ssds_get_num_fields(struct ssds_class *pclass);
void *ssds_get_field_name(struct ssds_class *pclass, int num);
void *ssds_get_field(struct ssds_class *pclass, int num);

void *ssds_find_item(struct ssds_class *pclass, char *name);
void *ssds_get_item(struct ssds_class *pclass, int num);
void *ssds_get_item_name(struct ssds_class *pclass, int num);
void *ssds_get_item_data(struct ssds_class *pclass, int i_num, int f_num);

int ssds_print_class(struct ssds_class *pclass);
int ssds_print_items(struct ssds_class *pclass);

//int ssds_set_val(int c_num, int i_num, int f_num, char *data);
void *ssds_get_val(int c_num, int i_num, int f_num);
int ssds_action(int ssds_fd, char *lbuf, char *sbuf, void *cli);
int ssds_print_classes(int depth);

int ssds_set_find_group(int (*pfind_group)(char *name, void *cli, int act));
int get_str_val(char * stmp, char *dsp, char delim, int len);
char *ssds_name(struct ssds *ssds,char *fname);
int set_action_val_name_bin(char *f_val, char *saction, int len);
int set_action_val_name_bin(char *f_val, char *saction, int len);


int get_action_len_name_gr(char *saction, int len);

int get_action_val_name_gr(char *f_val, char *saction, int len);

int get_action_val_name_bin(char *f_val, char *saction, int len);

int set_action_val_name_bin(char *f_val, char *saction, int len);
int set_action_val_name(char *saction, char *f_val, int len);
int ssds_print_item(void *in_item);
int ssds_get_data_size(void);
int ssds_get_def_size(void);


#endif
