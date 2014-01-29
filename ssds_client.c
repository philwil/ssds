/*
 * ssds_client.c
 * small system data server
 * connects to the server
 * provides an functional interface for clients
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


/* local data */
struct db_field {
  char name[64];
  int id;
  char *data;
};

struct db_item {
  char name[64];
  int id;
  int num_fields;
  struct db_field **fields;
};

struct db_class {
  char name[64];
  int i;
  int id;
  int num_items;
  int num_fields;
  struct db_field **fields;
  struct db_item **items;
};

struct db_ssds {
  int num_classes;
  struct db_class **classes;
};

struct db_ssds ssds;

char *def_node_addr = "127.0.0.1";
int def_port_num     = 4321;
int port_num;
char *query_port_num=NULL;
char *query_node_addr=NULL;

int client_debug;

#define DEF_SYS_VERS "1.0.1"
char *sys_vers = DEF_SYS_VERS;

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

/*
 * char **argv;
 * char *myargv[16];
 * int argc;
 * argv = myargv;
 * argc = get_argc(lbuf, argv, 16);
 * argv[0]:etc
 */
int ssds_send_debug=0;
int ssds_send(int fd, char *sbuf, char *rbuf)
{
  int len;
  int res;
  char snum[8];

  len = strlen(sbuf);
  sprintf(snum, "%04d", len);
  memcpy((void*)&sbuf[4], (void *)&snum[0], 4);
  if(ssds_send_debug)
    printf(" ssds_send send (%s)\n",sbuf);
  res = write(fd, sbuf, len);
  if (res == len) {
    res = read(fd, rbuf, 8);
    if (res == 8) {
        memcpy((void *)&snum[0], (void *)&rbuf[4], 4);
        snum[4]=0;
        len=atoi(snum);
        if(ssds_send_debug){
	  printf(" Client len (%d) \n", len);
	  printf(" Client rbuf %c %c %c %c %x %x %x %x\n"
		 , rbuf[0], rbuf[1], rbuf[2], rbuf[3]
		 , rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	}
	res = read(fd, &rbuf[8], len-8);
	//rbuf[4]='1';
	//rbuf[5]='2';
	//rbuf[6]='3';
	//rbuf[7]='4';
        res += 8;
        rbuf[res]=0;
	if(ssds_send_debug)printf(" ssds_send read (%s)\n",rbuf);
    }
  } else {
    res = -1;
  }
  return res;
}

/*
 * char **argv;
 * char *myargv[16];
 * int argc;
 * argv = myargv;
 * argc = get_argc(lbuf, argv, 16);
 */
int get_arg_int(int argc, char **argv, char *key, int *num)
{
  int ret;
  int i;
  ret = -1;
  for (i = 0 ; i < argc; i++) {
    if (strcmp(argv[i],key) == 0) {
      ret = sscanf(argv[i+1],"%d", num);
      ret = 0;
      break;
    }
  }
  return ret;
}

int get_arg_char(int argc, char ** argv, char *key, char *sp, int len)
{
  int ret;
  int i;
  char *xsp;
  ret = 0;
  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], key) == 0) {
      xsp = argv[i+1];
      while(*xsp && len--) {
	ret ++;
	*sp++=*xsp++;
      }
      *sp = 0;
      break;
    }
  }
  return ret;
}

/*
 * gets the classses fields and items for each class
 */
int test_ssds(int fd)
{
  int ret;
  int res;
  int num;
  int i;
  int id;
  char **argv;
  char *myargv[16];
  int argc;
  int fields;
  int items;
  char myclass[32];
  char rbuf[132];
  char sbuf[132];

  argv = myargv;

  ret = 0;
  num = -1;

  sprintf(sbuf,"msg 5678:action:get_classes:");
  res = ssds_send(fd, sbuf, rbuf);

  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_int(argc, argv, "reply", &num);
  //printf(" Client num_classes is %d \n", num);

  for (i=0; i<num; i++) {
    sprintf(sbuf,"msg 5678:action:get_class_num:num:%d:",i);
    res = ssds_send(fd, sbuf, rbuf);
    argc = get_argc(rbuf, argv, 16);
    get_arg_char(argc, argv, "class", myclass, 32);
    ret = get_arg_int(argc, argv, "id", &id);
    if (ret == 0) {
      printf(" Client class name (%s) id %d \n", myclass, id);
      sprintf(sbuf,"msg 5678:action:get_fields:id:%d:",id);
      res = ssds_send(fd, sbuf, rbuf);
      argc = get_argc(rbuf, argv, 16);
      get_arg_char(argc, argv,"class", myclass, 32);
      ret = get_arg_int(argc, argv,"reply", &fields);

      sprintf(sbuf,"msg 5678:action:get_items:id:%d:", id);
      res = ssds_send(fd, sbuf, rbuf);
      argc = get_argc(rbuf, argv, 16);
      get_arg_char(argc, argv,"class", myclass, 32);
      ret = get_arg_int(argc, argv,"reply", &items);

      printf(" Client class (%s) id %d num fields %d num items %d\n",
	     myclass, id, fields, items);

    } else {
      printf(" Client class name NOT_FOUND \n");
    }
  }

  return ret;
}

int setup_ssds_client(int argc, char *argv[])
{

  char rbuf[132];
  char sbuf[132];

  int client_fd, client_len;
  struct sockaddr_in client_addr;
  char *node_addr = def_node_addr;
  int res;

  char *ipaddr;
  char *iport;

  port_num = def_port_num;
  ipaddr = getenv("SSDS_IPADDR");
  if(query_node_addr) ipaddr=query_node_addr;
  if(ipaddr) node_addr = ipaddr;

  iport = getenv("SSDS_PORT");
  if(query_port_num) iport=query_port_num;
  if(iport) port_num = atoi(iport);

  client_fd = socket(AF_INET,SOCK_STREAM,0);
  client_addr.sin_family = AF_INET;

  if (argc > 1)
    node_addr = argv[1];

  if (argc > 2)
    port_num = atoi(argv[2]);

  if(client_debug)
    printf("<!--SSDS Client attempting to connect to %s:%d-->\n",
	   node_addr, port_num);
  res = inet_aton(node_addr,&client_addr.sin_addr);

  if ( res == 0 ) {
    perror("inet_aton error");
    exit(1);
  }

  if(client_debug)
    printf("<!-- SSDS Client socket set up -->\n");
  client_addr.sin_port = htons(port_num);
  client_len = sizeof(client_addr);
  res=connect(client_fd,
	      ( struct sockaddr *)&client_addr,client_len);

  if (res == -1) {
    perror(" Client cannot connect\n");
    exit(1);
  }

  if(client_debug)
    printf("<!--SSDS Client connected -->\n");

  sprintf(sbuf, "msg 5678:action:hello:");
  res = ssds_send(client_fd, sbuf, rbuf);

  return client_fd;
}

int ssds_num_classes(int fd)
{
  int res;
  int num;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_classes:");
  res  = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  res  = get_arg_int(argc, argv, "reply", &num);
  //  printf(" Client num_classes is %d \n", num);

  return num;
}

int ssds_num_fields(int fd, int class_num)
{
  int res;
  int num;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_fields:class_num:%d:",class_num);
  res = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  res  = get_arg_int(argc, argv, "reply", &num);
  if(0)
    printf("ssds_client.c:ssds_num_fields rbuf (%s) argc %d num %d\n",
	   rbuf, argc, num);
  return num;
}

int ssds_get_name(int fd, char *sp, char *name)
{
  int res;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  if(name) {  // set_name
    sprintf(sbuf,"msg 5678:action:set_name:name:%s:", name);
    res = ssds_send(fd, sbuf, rbuf);
    argc = get_argc(rbuf, argv, 16);
    res  = get_arg_char(argc, argv, "reply", sp, 32);
    if(0)
      printf("ssds_client.c:ssds_get_name Set to (%s)\n",
	     name);
  } else {
    sprintf(sbuf,"msg 5678:action:get_name:");
    res = ssds_send(fd, sbuf, rbuf);
    argc = get_argc(rbuf, argv, 16);
    res  = get_arg_char(argc, argv, "name", sp, 32);
    if(0)
      printf("ssds_client.c:ssds_get_name Found (%s)\n",
	     sp);
  }
  return res;
}

int ssds_get_vers(int fd, char *sp, char *name)
{
  int res;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  if(name) {  // set_vers
    sprintf(sbuf,"msg 5678:action:set_vers:name:%s:", name);
    res = ssds_send(fd, sbuf, rbuf);
    argc = get_argc(rbuf, argv, 16);
    res  = get_arg_char(argc, argv, "reply", sp, 32);
    if(0)
      printf("ssds_client.c:ssds_get_vers Set to (%s)\n",
	     name);
  } else {
    sprintf(sbuf,"msg 5678:action:get_vers:");
    res = ssds_send(fd, sbuf, rbuf);
    argc = get_argc(rbuf, argv, 16);
    res  = get_arg_char(argc, argv, "name", sp, 32);
    if(0)
      printf("ssds_client.c:ssds_get_vers Found (%s)\n",
	     sp);
  }
  return res;
}

int ssds_num_items(int fd, int class_num)
{
  int res;
  int num;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_items:class_num:%d:",class_num);
  res = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  res  = get_arg_int(argc, argv, "reply", &num);

  return num;
}


int ssds_class_name(int fd, int class_num, char *sp)
{
  int res;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_fields:class_num:%d:",class_num);
  res = ssds_send(fd, sbuf, rbuf);
  if(client_debug)
    printf("ssds_class_name sent <%s> got <%s> res %d\n",&sbuf[8],rbuf,res);
  argc = get_argc(rbuf, argv, 16);
  res = get_arg_char(argc, argv,"class", sp, 32);
  return res;
}

int ssds_field_name(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class_num:%d:action:get_field_num:num:%d:",
	  class_num, field_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"field", sp, 32);
  return ret;
}

int ssds_field_id(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class_num:%d:action:get_field_num:field_num:%d:",
	  class_num, field_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"id", sp, 32);
  return ret;
}

int ssds_field_type(int fd, int class_num, int field_num, char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class_num:%d:action:get_field_num:field_num:%d:",
	  class_num, field_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"type", sp, 32);

  return ret;
}

int ssds_do_action(int fd, char *sp_action, char *rsp)
{
  int res;
  char **argv;
  char *myargv[16];
  char sbuf[256];
  int len;

  argv = myargv;
  len = strlen(sp_action);
  if(len > 240) {
    res = sprintf(rsp,"Action size %d too large \n",len);
    goto do_act_ret;
  }
  sprintf(sbuf,"msg 5678:%s",sp_action);
  if(client_debug) printf("ssds_do_action sending (%s)",sbuf);
  res = ssds_send(fd, sbuf, rsp);
  if(client_debug) printf("ssds_do_action reply len %d buf (%s)", res, rsp);
 do_act_ret:
  return res;
}


int ssds_class_id(int fd, int class_num, char *sp)
{
  int res;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_fields:class_num:%d:",class_num);
  res = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  res = get_arg_char(argc, argv,"id", sp, 32);
  return res;
}


int ssds_item_name(int fd, int class_num, int item_num, char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class_num:%d:action:get_item_num:num:%d:",
	  class_num, item_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"item", sp, 32);
  return ret;
}

int ssds_item_id(int fd, int class_num, int item_num, char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class_num:%d:action:get_item_num:num:%d:",
	  class_num, item_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"id", sp, 32);
  return ret;
}

int ssds_set_item_value(int fd, int class_num, int field_num, int item_num
		    , char *valsp, int len, char *rsp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];
  char *sp;

  argv = myargv;
  sp = sbuf;
  sp += sprintf(sp,
	  "msg 5678:class_num:%d:action:set_item_value:field_num:%d"
	  ":item_num:%d:value:",
	  class_num, field_num, item_num);

  while(len--){
    *sp++ = *valsp++;
  }
  *sp++ = ':';
  *sp = 0;
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"result", rsp, 32);
  return ret;
}


int ssds_cgi_new_field(int fd,
		       char *classsp, char *namesp, char *typesp,
		       char *valsp, char *idsp, char *lensp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  //  int len;
  char rbuf[256];
  char sbuf[256];

  argv = myargv;

  ret = sprintf(sbuf,
		"msg 5678:class:%s:action:create_field:field:%s:"
		"type:%s:len:%s:reply:yes:",
		classsp, namesp, typesp, lensp);
  if(strlen(idsp) > 0) {
    ret +=sprintf(&sbuf[ret],"id:%s:",idsp);
  }
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  return ret;
}

int ssds_cgi_new_item(int fd, char *classsp, char *namesp, char *idsp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  //  int len;
  char rbuf[256];
  char sbuf[256];

  argv = myargv;
  ret = sprintf(sbuf,
		"msg 5678:class:%s:action:create_item:item:%s:reply:yes:",
		classsp, namesp);
  if(strlen(idsp) > 0) {
    ret += sprintf(&sbuf[ret],"id:%s:",idsp);
  }
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  return ret;
}

int ssds_cgi_new_class(int fd, char *classsp, char *idsp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  //int len;
  char rbuf[256];
  char sbuf[256];

  argv = myargv;
  ret = sprintf(sbuf,"msg 5678:class:%s:action:create_class:reply:yes:"
		, classsp);
  if(strlen(idsp) > 0) {
    ret += sprintf(&sbuf[ret],"id:%s:",idsp);
  }
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  return ret;
}

/*
 * used for gv.0.0.0 etc
 */
int ssds_item_keyx(int fd, char *tok, char *sp, char *key)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  int len;
  char rbuf[256];
  char sbuf[256];

  argv = myargv;

  /* copy most of the token miss out leading and trailing '@' */
  /* lead chars skipped already */
  strcpy(rbuf, tok);
  //printf("<br> new rbuf 1 (%s)\n",rbuf);
  //return 0;
  len = strlen(rbuf);
  rbuf[len-2]=0;
  //printf("<br> new rbuf 2 (%s)\n",rbuf);
  //return 0;
  sprintf(sbuf,"msg 5678:action:%s:", rbuf);
  //printf("<br>action [%s] key [%s]", sbuf , key);
  ret = ssds_send(fd, sbuf, rbuf);
  //printf("<br>action rbuf [%s] key [%s]", rbuf , key);
  argc = get_argc(rbuf, argv, 16);
  /* copy the key back */

  ret = 0;
  ret = get_arg_char(argc, argv, key, sp, 32);
  //printf("<br>action rbuf [%s] key [%s] ret %d sp %s", rbuf , key, ret , sp);
  return ret;
}

int ssds_item_key(int fd, char *tok, char *sp, char *key)
{
  return ssds_item_keyx(fd, &tok[2], sp, key);
}

int ssds_item_value(int fd, int class_num, int field_num, int item_num
		    , char *sp)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,
	  "msg 5678:class_num:%d:action:get_item_value:field_num:%d"
	  ":item_num:%d:",
	  class_num, field_num, item_num);

  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  ret = get_arg_char(argc, argv,"value", sp, 32);
  return ret;
}

int ssds_class_data(struct db_class *db_class, int fd, int num)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_class_num:num:%d:",num);
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  get_arg_char(argc, argv,"class", db_class->name, 32);
  ret = get_arg_int(argc, argv,"id", &db_class->id);
  ret += get_arg_int(argc, argv,"items", &db_class->num_items);
  ret += get_arg_int(argc, argv,"fields", &db_class->num_fields);
  if (ret == 0) {
    printf(" Client class name (%s) id %d \n",db_class->name, db_class->id);
  }

  return ret;
}


int ssds_field_data(struct db_class *db_class, int fd, int num)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];
  struct db_field * field;

  field = db_class->fields[num];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class:%s:action:get_field_num:num:%d:",
	  db_class->name,num);
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  get_arg_char(argc, argv,"field", field->name, 32);
  ret = get_arg_int(argc, argv,"id", &field->id);
  if (ret == 0) {
    printf(" ..  Client field data class name (%s) id %d field %s (id %d)\n",
	   db_class->name, db_class->id,
	   field->name, field->id);
  }
  return ret;
}

int ssds_item_data(struct db_class *db_class, int fd, int num)
{
  int ret;
  char **argv;
  char *myargv[16];
  int argc;
  char rbuf[132];
  char sbuf[132];
  struct db_item * item;

  item = db_class->items[num];

  argv = myargv;
  sprintf(sbuf,"msg 5678:class:%s:action:get_item_num:num:%d:",
	  db_class->name, num);
  ret = ssds_send(fd, sbuf, rbuf);
  argc = get_argc(rbuf, argv, 16);
  get_arg_char(argc, argv,"item", item->name, 32);
  ret = get_arg_int(argc, argv,"id", &item->id);
  if (ret == 0) {
    printf(" ..  Client item data class name (%s) id %d item %s (id %d)\n",
	   db_class->name, db_class->id,
	   item->name, item->id);
  }

  return ret;
}


int fill_ssds(int client_fd)
{
  struct db_class *pclass;
  struct db_item *item;
  int i,j,k;
  int ret;
  int res;

  ret = 0;
  ssds.num_classes = ssds_num_classes(client_fd);
  printf("SDCS num_classes %d\n", ssds.num_classes);

  ssds.classes = malloc(ssds.num_classes * sizeof(struct db_class *));

  for (i = 0; i < ssds.num_classes; i++ ) {
    pclass =
      ssds.classes[i] =(struct db_class *)malloc(sizeof(struct db_class));
    res = ssds_class_data(ssds.classes[i], client_fd, i);
    printf("...[%d] Class %s id %d num_items %d num_fields %d\n"
	   , i, pclass->name, pclass->id
	   , pclass->num_items, pclass->num_fields);
    pclass->items = malloc(pclass->num_items * sizeof(struct db_item*));
    pclass->fields = malloc(pclass->num_fields * sizeof(struct db_field*));
    for (j = 0; j < pclass->num_fields; j++) {
      pclass->fields[j] = malloc(sizeof(struct db_field));
      res = ssds_field_data(pclass, client_fd, j);
    }
    for (j = 0 ; j < pclass->num_items; j++) {
      item = pclass->items[j] = (struct db_item*)malloc(sizeof(struct db_item));
      res = ssds_item_data(pclass, client_fd, j);
      item->num_fields = pclass->num_fields;
      item->fields=malloc(pclass->num_fields * sizeof(struct db_field*));
      for (k = 0 ; k < pclass->num_fields; k++) {
	item->fields[k]= malloc(sizeof(struct db_field));
	item->fields[k]->id=pclass->fields[k]->id;
      }
    }
  }
  return ret;
}

#ifdef CLIENT_TEST

int main(int argc, char *argv[])
{

   int client_fd;

   client_fd = setup_ssds_client(argc, argv);
   test_ssds(client_fd);
   fill_ssds(client_fd);

   close(client_fd);

   return 0;
}

#endif
