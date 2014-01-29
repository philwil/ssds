/*
 * ssds_user.c
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 * user interface
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#include "ssds_user.h"

char *def_node_addr = DEF_NODE_ADDR;
int port_num = DEF_PORT_NUM;


int client_debug = 0;


char *data[DATA_SIZE];
char *defs[DEF_SIZE];


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

int ssds_send_len(int fd, char *sbuf, int slen, char *rbuf, int rlen)
{
  int len;
  int mlen;
  int res;
  int ret;
  char snum[8];

  sprintf(snum, "%04d", slen);
  memcpy((void*)&sbuf[4], (void *)&snum[0], 4);
  //printf("sending len %04d \n", slen);
  res = write(fd, sbuf, slen);
  if (res == slen) {
    res = read(fd, rbuf, 8);
    if (res == 8) {
        memcpy((void *)&snum[0], (void *)&rbuf[4], 4);
        snum[4]=0;
        mlen=atoi(snum);
        if(0){
	  printf(" Client len (%d) \n", len);
	  printf(" Client rbuf %c %c %c %c %x %x %x %x\n"
		 , rbuf[0], rbuf[1], rbuf[2], rbuf[3]
		 , rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
	}
        len=mlen;
        if(len > rlen) len=rlen;
	res = read(fd, &rbuf[8], len-8);
        res += 8;
        rbuf[res]=0;

        /* we have to throw the rest away */
        while(mlen > rlen) {
	  ret = read(fd, snum, 8);
	  if(ret >0) {
	    mlen -= ret;
	  }
	}
	//rbuf[4]='1';
	//rbuf[5]='2';
	//rbuf[6]='3';
	//rbuf[7]='4';

    }
  } else {
    res = -1;
  }
  return res;
}

int ssds_send(int fd, char *sbuf, char *rbuf)
{
  int len;

  len = strlen(sbuf);
  return ssds_send_len(fd, sbuf, len, rbuf , SSDS_DEF_RBUF_LEN);
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

int get_arg_char(int argc, char **argv, char *key, char *sp, int len)
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

int get_arg_bin(int argc, char **argv, char *key, char *sp, int len)
{
  int ret;
  int i;
  char *xsp;
  ret = 0;
  for (i = 0; i < argc; i++) {
    if (strcmp(argv[i], key) == 0) {
      xsp = argv[i+1];
      while(len--) {
	ret ++;
	*sp++=*xsp++;
      }
      *sp = 0;
      break;
    }
  }
  return ret;
}

int ssds_open(struct ssds_connect *pccon)
{
  //int setup_ssds_client(int argc, char *argv[])
   char rbuf[SSDS_DEF_RBUF_LEN];
   char sbuf[SSDS_DEF_SBUF_LEN];

   int client_fd, client_len;
   struct sockaddr_in client_addr;
   char *node_addr = def_node_addr;
   int res;
   char *ipaddr;
   char *iport;

   ipaddr = getenv("SSDS_IPADDR");
   if(ipaddr) node_addr = ipaddr;
   iport = getenv("SSDS_PORT");
   if(iport) port_num = atoi(iport);


   client_fd = socket(AF_INET,SOCK_STREAM,0);
   client_addr.sin_family = AF_INET;

   //   if (argc > 1)
   //   node_addr = argv[1];

   //if (argc > 2)
   //   port_num = atoi(argv[2]);

   if(pccon){
     node_addr = pccon->ipaddr;
     port_num = pccon->port;
   }

   if(client_debug)
     printf("<!--SSDS Client attempting to connect to %s:%d-->\n",
	    node_addr, port_num);
   res = inet_aton(node_addr, &client_addr.sin_addr);

   if (res == 0) {
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

/* class def is
    0
    0x num
    1  id
    name
    TODO local cache
    // uses number
    ssds_get_class_num(pcfd,"02");
    // uses ID
    ssds_get_class_num(pcfd,"2");
    // uses Name (will also create the class
    ssds_get_class_num(pcfd,"MyName.myvar.myfield");
*/

int cl_db;

int ssds_get_gen_num(int pcfd, char *sdef, char *skey, char *rkey)
{
  int num;
  int res;
  int ret;
  int argc;
  char **argv;
  char *myargv[32];
  char rbuf[256];
  char sbuf[256];
  int i;

  argv = myargv;
  if(sdef) {
    sprintf(sbuf,"msg 5678:action:%s.%s:", skey, sdef);
  } else {
    sprintf(sbuf,"msg 5678:action:%s:", skey);
  }
  if(cl_db) {
    printf("sending (%s) \n", sbuf);
  }
  res = ssds_send(pcfd, sbuf, rbuf);
  if (res < 0 ) {
    printf(" Error in send\n");
  } 
  if(cl_db) {
    printf("got (%s) \n", rbuf);
  }

  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i = 0; i<argc;i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  ret = get_arg_int(argc, argv, rkey, &num);
  if(cl_db) {
    printf("ret %d key (%s) num %d\n", ret, rkey, num);
  }

  if(ret >= 0) {
    ret = num;
  }
  if(cl_db)cl_db--;

  return ret;
}

int ssds_get_gen_name(int pcfd, char *sdef, char *skey, char *rkey
		      , char *res, int len)
{
  int ret;
  int argc;
  char **argv;
  char *myargv[32];
  char rbuf[256];
  char sbuf[256];
  int i;

  argv = myargv;
  if(sdef) {
    sprintf(sbuf,"msg 5678:action:%s.%s:",skey, sdef);
  } else {
    sprintf(sbuf,"msg 5678:action:%s:",skey);
  }
  if(cl_db) {
    printf("sending (%s) \n", sbuf);
  }
  ret = ssds_send(pcfd, sbuf, rbuf);
  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }

  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i = 0; i<argc;i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, rkey, res, len);

  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }

  if(cl_db) cl_db--;

  return ret;
}

int ssds_get_num_classes(int pcfd)
{
  return ssds_get_gen_num(pcfd, NULL, "num_classes", "reply");
}

int ssds_get_name(int pcfd, char *res, int len)
{
  return ssds_get_gen_name(pcfd, NULL, "get_name", "name", res, len);
}

int ssds_get_vers(int pcfd, char *res, int len)
{
  return ssds_get_gen_name(pcfd, NULL, "get_name", "vers", res, len);
}

int ssds_get_class_name(int pcfd, char *class_def, char *res, int len)
{
  return ssds_get_gen_name(pcfd, class_def, "gc", "name", res, len);
}

int ssds_get_class_num(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gc", "c_num");
}


int ssds_get_class_id(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gc", "c_id");
}

int ssds_get_num_class_fields(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gc", "num_fields");
}

int ssds_get_field_name(int pcfd, char *full_def, char *res, int len)
{
  return ssds_get_gen_name(pcfd, full_def, "gf", "f_name", res, len);
}

int ssds_get_field_type(int pcfd, char *full_def, char *res, int len)
{
  return ssds_get_gen_name(pcfd, full_def, "gf", "f_type", res, len);
}


int ssds_get_field_num(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gf", "f_num");
}


int ssds_get_field_id(int pcfd,char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gf", "f_id");
}


int ssds_set_field_type(int pcfd, char *class_def, char *type_def, char *res, int len )
{
  char stmp[128];
  sprintf(stmp,"%s:type:%s",class_def,type_def);
  return ssds_get_gen_name(pcfd, stmp, "sf", "f_type", res, len);
}

int ssds_set_field_len(int pcfd,char *class_def, int len)
{
  return -1;
}

int ssds_get_num_class_items(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gc", "num_items");
}

int ssds_get_item_name(int pcfd,char *class_def, char *res, int len)
{
  return ssds_get_gen_name(pcfd, class_def, "gi", "i_name", res, len);
}

int ssds_get_item_num(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gi", "i_num");
}

int ssds_get_item_id(int pcfd, char *class_def)
{
  return ssds_get_gen_num(pcfd, class_def, "gi", "i_id");
}

int ssds_get_item_value(int pcfd, char *full_def, char *res, int len)
{
  return ssds_get_gen_name(pcfd, full_def, "gvf", "value", res, len);
}

int ssds_get_item_value_bin(int pcfd, char *full_def, char *reply, int len)
{
  return -1;
}
int ssds_set_item_value(int pcfd, char *full_def, char *value, char *res,
			int len)
{
  char stmp[132];
  sprintf(stmp, "%s=%s", full_def, value);
  return ssds_get_gen_name(pcfd, stmp, "sv", "result", res, len);
}

int ssds_set_item_value_bin(int pcfd, char *sdef, char *value,
			    int vlen, char *res, int len)
{
  char *sp;
  int ret;

  char rbuf[256];
  char sbuf[256];
  int slen;
  int argc;
  char **argv;
  char *myargv[32];
  char *rkey =  "result";
  int i;
  float fval;
  char *spv;

  argv = myargv;
  sprintf(sbuf,"msg 5678:action:svb.%s:len:%d:data:",sdef, vlen);
  if(cl_db) {
    printf("sending (%s) len %d vlen %d\n", sbuf, (int) strlen(sbuf), vlen);
  }
  slen = strlen(sbuf);
  sp = sbuf+slen;
  i = vlen;
  fval = 9.876;
  spv = (char*)&fval;
  while(i--){
    //    *sp++ = *value++;
    // printf( " %02x ", *spv);
    *sp++ = *spv++;
  }

  ret = ssds_send_len(pcfd, sbuf, slen+vlen, rbuf, 256);
  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, rkey, res, len);

  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }

  if(cl_db) cl_db--;

  return ret;
}


int ssds_add_group_item(int pcfd, char *list_name, char *idef, char *res, int rlen)
{
  int ret;
  int i;
  char rbuf[256];
  char sbuf[256];
  int argc;
  char **argv;
  char *myargv[32];
  char *rkey =  "result";
  argv = myargv;


  if(idef) {
    if(idef[0] == '-'){
      /* special to reset the group */
      sprintf(sbuf,"msg 5678:action:make_group:name:%s:item:-1,gv.%s:",
	      list_name, &idef[1]);
    } else {
      sprintf(sbuf,"msg 5678:action:make_group:name:%s:item:1,gv.%s:",
	      list_name, &idef[0]);
    }
  } else {
    sprintf(sbuf,"msg 5678:action:make_group:name:%s:",
	    list_name);
  }
  ret = ssds_send(pcfd, sbuf, rbuf);
  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, rkey, res, rlen);

  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }

  if(cl_db) cl_db--;

  return ret;
}

int ssds_create_group(int pcfd, char *list_name, char *res , int rlen)
{
  return ssds_add_group_item(pcfd, list_name,NULL, res, rlen);
}

int ssds_get_group_len(int pcfd, char *list_name, char *res , int rlen)
{
  int ret;
  int i;
  char rbuf[256];
  char sbuf[256];
  int argc;
  char **argv;
  char *myargv[32];
  char *rkey =  "len";
  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_group_len:name:%s:",
	  list_name);

  if(cl_db) {
    printf("sent (%s) \n", sbuf);
  }

  ret = ssds_send_len(pcfd, sbuf, strlen(sbuf),  rbuf, 256);

  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, rkey, res, rlen);

  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }

  if(cl_db) cl_db--;
  if(ret > 0){
    res[ret]=0;
    ret = atoi(res);
  }
  return ret;
}

int ssds_get_group_data(int pcfd, char *list_name, char *res, int rlen)
{
  int ret;
  int i;
  char rbuf[256];
  char sbuf[256];
  int argc;
  char **argv;
  char *myargv[32];
  char *rkey =  "items";
  argv = myargv;
  sprintf(sbuf,"msg 5678:action:get_group:name:%s:",
	  list_name);

  if(cl_db) {
    printf("sent (%s) \n", sbuf);
  }

  ret = ssds_send_len(pcfd, sbuf, strlen(sbuf),  rbuf, 256);

  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, rkey, res, rlen);

  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }

  if(cl_db) cl_db--;

  return ret;
}

int ssds_get_group_data_bin(int pcfd, char *list_name,
			    char *data, int dlen, char *res, int rlen)
{
  int ret;
  int i;
  char rbuf[256];
  char sbuf[256];
  int argc;
  char **argv;
  char *myargv[32];
  char *rkey =  "data";
  char lbuf[32];
  char xlen;

  argv = myargv;

  sprintf(sbuf,"msg 5678:action:get_group_bin:name:%s:",
	  list_name);

  if(cl_db) {
    printf("sent (%s) \n", sbuf);
  }

  ret = ssds_send_len(pcfd, sbuf, strlen(sbuf),  rbuf, 256);

  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }

  lbuf[0]=0;
  ret = get_arg_char(argc, argv, "len", lbuf, 32);
  if(cl_db) {
    printf("ret %d for key (%s) lbuf (%s)\n", ret, "len", lbuf);
  }
  xlen = 0;
  if(ret >0) {
    xlen =  atoi(lbuf);
    if(xlen > dlen) xlen = dlen;
    res[0]=0;
    ret = get_arg_bin(argc, argv, rkey, data, xlen);
  }
  if(cl_db) {
    printf("ret %d for key (%s) res (%p) xlen %d\n", ret, rkey, res, xlen);
  }

  res[0]=0;
  ret = get_arg_char(argc, argv, "len", res, rlen);

  if(cl_db) cl_db--;

  return ret;
}

int ssds_set_group_data_bin(int pcfd, char *list_name, char*data
			    , int dlen, char *res, int rlen)
{
  int ret;
  int i;
  char rbuf[256];
  char sbuf[256];
  int argc;
  char **argv;
  char *myargv[32];
  char lbuf[32];

  argv = myargv;

  ret = sprintf(sbuf,"msg 5678:action:set_group_bin:name:%s:len:%d:data:",
		list_name, dlen);
  if(cl_db) {
    printf("sent (%s) + data %d\n", sbuf, dlen);
  }

  for (i=0;i<dlen;i++) {
    sbuf[ret] = data[i];
    ret++;
  }
  ret = ssds_send_len(pcfd, sbuf, ret,  rbuf, 256);
  if(cl_db) {
    printf("received (%s) \n", rbuf);
  }
  argc = get_argc(rbuf, argv, 32);
  if(cl_db) {
    for (i=0; i<argc; i++) {
      printf(" argv[%d] (%s) \n", i, argv[i]);
    }
  }

  lbuf[0]=0;
  ret = get_arg_char(argc, argv, "len", lbuf, 32);

  if(cl_db) {
    printf("ret %d for key (%s) lbuf (%s)\n", ret, "len", lbuf);
  }
  res[0]=0;
  ret = get_arg_char(argc, argv, "len", res, 32);

  if(cl_db) cl_db--;

  return ret;
}

int ssds_set_group_data_asc(int pcfd,char *list_name, char *data)
{
 return -1;
}
