
/*
 * snmp_agent.c
 * this file is deprecated in favor of ssds_agent.c
 * small system data server snmp dynamic interface
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009

 * if you change this file name look in
 *
 *      agent/Makefile
 *      agent/mibgroup/Makefile
 *
 *
 * testing
 * in one term run
 *  agent/snmpd -M mibs -DALL -f -V > out.log 2>&1

 * in the other term run
 *  * snmpwalk -v1 -M mibs -c public   localhost .1.3.6.1.4.1.9334.100.104.1
    iso.3.6.1.4.1.9334.100.104.1.1.1.0 = INTEGER: 1
    iso.3.6.1.4.1.9334.100.104.1.1.2.0 = Opaque: Float: 1000.400024
    iso.3.6.1.4.1.9334.100.104.1.1.3.0 = Opaque: Float: 1100.400024
    iso.3.6.1.4.1.9334.100.104.1.1.4.0 = Opaque: Float: 1200.400024
    iso.3.6.1.4.1.9334.100.104.1.1.5.0 = Opaque: Float: 1300.400024
    iso.3.6.1.4.1.9334.100.104.1.1.6.0 = INTEGER: 56
    iso.3.6.1.4.1.9334.100.104.1.1.7.0 = INTEGER: 21

    this one gets the whole database
*   snmpwalk -v1 -M mibs -c public localhost .1.3.6.1.4.1.9334.100.104

*   snmpget -v1 -M /home/student-10/work/canam/net-snmp-5.2.5.1/mibs -c public
     -Cf  localhost .1.3.6.1.4.1.9334.100.104.1.1.1.7.0
    SNMPv2-SMI::enterprises.9334.100.104.1.1.7.0 = INTEGER: 21

*   snmpget -v1 -M /home/student-10/work/canam/net-snmp-5.2.5.1/mibs -c public
      -Cf  localhost .1.3.6.1.4.1.9334.100.104.1.1.5.0
    SNMPv2-SMI::enterprises.9334.100.104.1.1.1.5.0 = Opaque: Float: 1300.400024

*   snmpset -v1 -M /home/student-10/work/canam/net-snmp-5.2.5.1/mibs -c public
    localhost .1.3.6.1.4.1.9334.100.104.1.1.6.0 i 2
*/


#include <net-snmp/net-snmp-config.h>

#if HAVE_LIMITS_H
#include <limits.h>
#endif
#if HAVE_STDLIB_H
#include <stdlib.h>
#endif
#if HAVE_UNISTD_H
#include <unistd.h>
#endif
#if HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <ctype.h>
#include <signal.h>
#if HAVE_MACHINE_PARAM_H
#include <machine/param.h>
#endif
#if HAVE_SYS_VMMETER_H
#if !defined(bsdi2) && !defined(netbsd1)
#include <sys/vmmeter.h>
#endif
#endif
#if HAVE_SYS_CONF_H
#include <sys/conf.h>
#endif
#if HAVE_SYS_FS_H
#include <sys/fs.h>
#else
#if HAVE_UFS_FS_H
#include <ufs/fs.h>
#else
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_VNODE_H
#include <sys/vnode.h>
#endif
#ifdef HAVE_UFS_UFS_QUOTA_H
#include <ufs/ufs/quota.h>
#endif
#ifdef HAVE_UFS_UFS_INODE_H
#include <ufs/ufs/inode.h>
#endif
#if HAVE_UFS_FFS_FS_H
#include <ufs/ffs/fs.h>
#endif
#endif
#endif
#if HAVE_MTAB_H
#include <mtab.h>
#endif
#include <sys/stat.h>
#include <errno.h>
#if HAVE_FSTAB_H
#include <fstab.h>
#endif
#if HAVE_SYS_STATVFS_H
#include <sys/statvfs.h>
#endif
#if HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif
#if (!defined(HAVE_STATVFS)) && defined(HAVE_STATFS)
#if HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#if HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
#if HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#define statvfs statfs
#endif
#if HAVE_VM_SWAP_PAGER_H
#include <vm/swap_pager.h>
#endif
#if HAVE_SYS_FIXPOINT_H
#include <sys/fixpoint.h>
#endif
#if HAVE_MALLOC_H
#include <malloc.h>
#endif
#if HAVE_STRING_H
#include <string.h>
#else
#include <strings.h>
#endif
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <sys/utsname.h>

#include <net-snmp/net-snmp-includes.h>
#include <net-snmp/agent/net-snmp-agent-includes.h>
#include <net-snmp/agent/auto_nlist.h>

#include "mibdefs.h"
#include "struct.h"
#include "util_funcs.h"
#include "agent.h"


#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>


#define CANAM_MIB		1,3,6,1,4,1,9334
#define CANAM_ECM		100,104

#define SSDS_NODE_ADDR "127.0.0.1"
#define SSDS_PORT 4321
#define SSDS_DEF_RBUF_LEN 256
#define SSDS_DEF_SBUF_LEN 256
#define SSDS_MAX_DATA (1024 * 8)
#define SSDS_INT 1
#define SSDS_FLOAT 2
#define SSDS_STR 3

struct oid_store {
  struct oid_store *next;
  int id;
  int num_oids;
  int depth;
  //struct oid_store *oids;
  struct oid_store *first_oid;
  struct oid_store *last_oid;
  //application data
  int type;
  char name[32];
  char *new_data;
  char *old_data;
  int len;

};

char *def_node_addr = SSDS_NODE_ADDR;
int port_num = SSDS_PORT;

char ssds_data[SSDS_MAX_DATA];
int cli_debug = 0;
int ssds_num_data = 0;
int ssds_max_data = SSDS_MAX_DATA-1;
int ssds_send_debug = 0;
int ssds_fd = 0;

int cl_db;

char *gen_argvb[32];
int gen_argc;
char **gen_argv;
char gen_rbuf[SSDS_DEF_RBUF_LEN];
int gen_rlen = SSDS_DEF_RBUF_LEN;
char gen_sbuf[SSDS_DEF_SBUF_LEN];
int gen_slen = SSDS_DEF_SBUF_LEN;


struct variable7 *ssds_vars;
struct oid_store **ssds_oids;
int ssds_num_vars;

// this is for canam_ecm
oid ssds_variables_oid[] = { CANAM_MIB, CANAM_ECM};


struct oid_store i_oids; // one for each class containing
                         // one for each item containing
                         // one for each field containing
                         // new and backup data stores
struct oid_store c_oids; // one for each class containing
                         // one for each field containing
                         // data type and data size
                         // used to cache class field information

int ssds_get_oid_len(char *name, int num);
int ssds_get_oid_type(char *name, int num);
char *ssds_get_oid_new_data(char *name, int num);
char *ssds_get_oid_old_data(char *name, int num);


size_t ssds_snmp_get_name_len(int ssfd, oid *name, int len, int num);
int ssds_snmp_get_name_val(int ssfd, char *name, int num,
			      char *res, int rlen);
int ssds_snmp_set_name_val(int ssfd, char *name, int num,
			      char *res, int rlen);
int ssds_snmp_get_name_type(int ssfd, oid *name,int len, int num);

int ssds_get_num_classes(int pcfd);
int ssds_get_class_id(int pcfd, int num);
int ssds_get_num_class_fields(int pcfd, int num);
int ssds_get_num_class_items(int pcfd, int num);
int ssds_get_field_id(int pcfd, int cnum, int fnum);
int ssds_get_item_id(int pcfd, int cnum, int inum);
int ssds_get_field_len(int pcfd, int cnum, int fnum);
int ssds_get_field_tid(int pcfd, int cnum, int fnum);
int ssds_reset_rbuf(void);

char * ssds_malloc(int len)
{
  char *data;
  data = malloc(len);
  return data;
}

struct oid_store *new_oid(struct oid_store *old_oid)
{
  struct oid_store *oid;
  struct oid_store *last_oid;
  oid = (struct oid_store *)ssds_malloc(sizeof(struct oid_store));
  oid->depth=old_oid->depth+1;
  oid->num_oids=0;
  oid->first_oid=NULL;
  oid->last_oid=NULL;
  oid->new_data=NULL;
  oid->old_data=NULL;
  oid->id=0;
  oid->next=NULL;
  if(old_oid->num_oids==0) {
    old_oid->first_oid=oid;
  } else {
    last_oid=old_oid->last_oid;
    last_oid->next = oid;
  }
  old_oid->last_oid=oid;
  old_oid->num_oids++;
  return oid;
}

struct oid_store *init_oid(struct oid_store *oid, int num)
{
  int i;
  struct oid_store *this_oid;
  for(i=0;i<num;i++) {
    this_oid = new_oid(oid);
  }
  return oid;
}

struct oid_store *find_oid(struct oid_store *oid, int id)
{
  int i;

  struct oid_store *this_oid;
  struct oid_store *ret;

  if (oid == NULL) oid = &i_oids;
  ret =  NULL;
  this_oid = oid->first_oid;
  for (i=0;i< oid->num_oids;i++) {
    if(this_oid->id == id) {
      ret = this_oid;
      break;
    } else {
      this_oid = this_oid->next;
    }
  }
  return ret;
}

struct oid_store *find_oid_name(struct oid_store *xoid, char *name, int num)
{
  int i;
  struct oid_store *ret;
  ret = xoid;

  for (i=0; i<num; i++) {
    ret = find_oid(ret, name[i]);
    if(ret == NULL) {
      break;
    }
  }
  return ret;
}


int ssds_find_oid_magic(char *name, int num)
{
  struct oid_store *oid;
  int ret=-1;
  int ix;

  oid = find_oid_name(NULL, name, num);
  if(oid) {
    for (ix=0; ix<ssds_num_vars; ix++) {
      if(ssds_oids[ix] == oid) {
	ret = ix;
	break;
      }
    }
  } else {
  }
  printf("SDCSDB oid found %p magic %d\n", oid, ret);
  return ret;
}

int ssds_get_oid_len(char *name, int num)
{
  struct oid_store *oid;
  int ret=-1;
  oid = find_oid_name(NULL, name, num);
  if(oid) {
    ret = oid->len;
  }
  return ret;
}

int ssds_get_oid_type(char *name, int num)
{
  struct oid_store *oid;
  int ret=-1;
  oid = find_oid_name(NULL, name, num);
  if(oid) {
    ret = oid->type;
  }
  return ret;
}

char *ssds_get_oid_new_data(char *name, int num)
{
  struct oid_store *oid;
  char *ret=NULL;
  oid = find_oid_name(NULL, name, num);
  if(oid) {
    ret = oid->new_data;
  }
  return ret;
}


char *ssds_get_oid_old_data(char *name, int num)
{
  struct oid_store *oid;
  char *ret=NULL;

  oid = find_oid_name(NULL, name, num);
  if(oid) {
    ret = oid->old_data;
  }
  return ret;
}



int ssds_set_up_oids(int ssfd)
{
  int num_defs;
  int i,j,k;
  int num_fields;
  int num_items;
  int num_classes;
  int item_id;

  struct oid_store *this_ioid;
  struct oid_store *this_coid;
  struct oid_store *tmp_oid;
  struct oid_store *tmp_oid1;
  struct oid_store *tmp_oid2;

  ssds_reset_rbuf();
  num_classes = ssds_get_num_classes(ssfd);
  printf("SSDS - num_classes %d\n", num_classes);

  num_defs = 0;
  /*
   * first set up the base stores for items and fields
   * the c_oid store is used to keep class information
   * the i_oid store is the system structure base
   */
  i_oids.depth=0;
  c_oids.depth=0;
  init_oid(&i_oids, num_classes); /* item oid */
  init_oid(&c_oids, num_classes); /* class oid */

  this_ioid = i_oids.first_oid;
  this_coid = c_oids.first_oid;

  for (i=0; i<num_classes; i++) {
    ssds_reset_rbuf();
    this_ioid->id = ssds_get_class_id(ssfd, i);
    this_coid->id = ssds_get_class_id(ssfd, i);

    // Set up master fields for each class
    num_fields = ssds_get_num_class_fields(ssfd, i);
    printf("SSDS - class %d num_fields %d\n", i, num_fields);

    init_oid(this_coid, num_fields); // field oid
    tmp_oid = this_coid->first_oid;
    for (j = 0; j<num_fields; j++) {
      ssds_reset_rbuf();
      cl_db = 1;
      tmp_oid->id   = ssds_get_field_id(ssfd,i,j);
      tmp_oid->len  = ssds_get_field_len(ssfd,i,j);
      tmp_oid->type = ssds_get_field_tid(ssfd,i,j);
      tmp_oid = tmp_oid->next;
    }
    ssds_reset_rbuf();
    num_items = ssds_get_num_class_items(ssfd,i);
    printf("SSDS - class %d num_items %d\n", i, num_items);
    init_oid(this_ioid, num_items); // item oid

    num_defs += (num_items * num_fields);

    tmp_oid = this_ioid->first_oid;
    for (j=0; j<num_items; j++) {
      ssds_reset_rbuf();
      item_id = ssds_get_item_id(ssfd, i,j);
      tmp_oid->id=item_id;
      init_oid(tmp_oid, num_fields); // item oid
      // Copy field data for each item
      tmp_oid1           = tmp_oid->first_oid;
      tmp_oid2           = this_coid->first_oid;

      for (k=0; k<num_fields; k++) {
        //item             = field
	tmp_oid1->id       = tmp_oid2->id;
	tmp_oid1->len      = tmp_oid2->len;
	tmp_oid1->type     = tmp_oid2->type;
	tmp_oid1->old_data = ssds_malloc(tmp_oid1->len);
	tmp_oid1->new_data = ssds_malloc(tmp_oid1->len);

        tmp_oid1->name[0]= this_coid->id;
        tmp_oid1->name[1]= item_id;
        tmp_oid1->name[2]= tmp_oid2->id;

        tmp_oid1 = tmp_oid1->next;
        tmp_oid2 = tmp_oid2->next;
      } // end num fields
      tmp_oid = tmp_oid->next;
    } // end num items

    this_ioid =  this_ioid->next;
    this_coid =  this_coid->next;
  } // end num classes
  return num_defs;
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


char *ssds_alloc_data(int size)
{
  char *ret;
  ret = NULL;
  if ((size + ssds_num_data) < ssds_max_data) {
    ret = &ssds_data[ssds_num_data];
    ssds_num_data += size;
  }
  return ret;
}

int ssds_reset_rbuf(void)
{
  gen_rbuf[0] = 0;
  return 0;
}

int ssds_send_len(int fd, char *sbuf, int slen, char *rbuf, int rlen)
{
  int len;
  int mlen;
  int res;
  int ret;
  char snum[8];

  sprintf(snum, "%04d", slen);
  memcpy((void*)&sbuf[4], (void *)&snum[0], 4);
  if(ssds_send_debug)
    printf("sending len %04d \n", slen);
  res = write(fd, sbuf, slen);
  if (res == slen) {
    res = read(fd, rbuf, 8);
    if (res == 8) {
      memcpy((void *)&snum[0], (void *)&rbuf[4], 4);
      snum[4]=0;
      mlen=atoi(snum);
      if(ssds_send_debug){
	printf(" Client len (%d) \n", len);
	printf(" Client rbuf %c %c %c %c %x %x %x %x\n"
	       , rbuf[0], rbuf[1], rbuf[2], rbuf[3]
	       , rbuf[4], rbuf[5], rbuf[6], rbuf[7]);
      }
      len=mlen;
      if(len>rlen)len=rlen;
      res = read(fd, &rbuf[8], len-8);
      res += 8;
      rbuf[res]=0;
      if(ssds_send_debug){
	printf(" Client rbuf len %d (%s) \n", len, rbuf);
      }
      /* we have to throw the rest away */
      while(mlen > rlen) {
	ret = read(fd, snum, 8);
	if(ret >0) {
	  mlen -= ret;
	}
      }
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

int ssds_snmp_open(void /*struct ssds_connect *pccon*/)
{
   int cli_fd, cli_len;
   struct sockaddr_in cli_addr;
   char *node_addr = def_node_addr;
   int res;

   cli_fd = socket(AF_INET,SOCK_STREAM,0);
   cli_addr.sin_family = AF_INET;
   if(cli_debug)
     printf("<!--SSDS Client attempting to connect to %s:%d-->\n",
	    node_addr, port_num);
   res = inet_aton(node_addr, &cli_addr.sin_addr);

   if (res == 0) {
      perror("inet_aton error");
      exit(1);
   }
   if(cli_debug)
     printf("<!-- SSDS Client socket set up -->\n");
   cli_addr.sin_port = htons(port_num);
   cli_len = sizeof(cli_addr);
   res=connect(cli_fd,
                 (struct sockaddr *)&cli_addr,cli_len);

   if (res == -1) {
      perror(" Client cannot connect\n");
      exit(1);
   }

   if(cli_debug)
     printf("<!--SSDS Client connected -->\n");

   sprintf(gen_sbuf, "msg 5678:action:hello:");
   res = ssds_send(cli_fd, gen_sbuf, gen_rbuf);
   ssds_fd = cli_fd;
   return cli_fd;
}

int ssds_get_gen_buf(int ssfd, char *sdef, char *skey)
{
  int num;
  int res;
  int ret;
  int argc;
  //char sbuf[SSDS_DEF_SBUF_LEN];
  int i;

  gen_argv = gen_argvb;
  if(sdef) {
    sprintf(gen_sbuf,"msg 5678:action:%s.%s:", skey, sdef);
  } else {
    sprintf(gen_sbuf,"msg 5678:action:%s:", skey);
  }
  if(cl_db) {
    printf("sending (%s) \n", gen_sbuf);
  }
  res = ssds_send(ssfd, gen_sbuf, gen_rbuf);
  if(cl_db) {
    printf("got (%s) \n", gen_rbuf);
  }
  gen_argc = get_argc(gen_rbuf, gen_argv, 32);
  if(cl_db) {
    for (i = 0; i<gen_argc; i++) {
      printf(" argv[%d] (%s) \n", i, gen_argv[i]);
    }
  }
  return gen_argc;
}

int ssds_get_gen_num(int ssfd, char *sdef, char *skey, char *rkey)
{
  int num;
  int ret;

  if(strlen(gen_rbuf)== 0) {
    ssds_get_gen_buf(ssfd, sdef, skey);
  }
  ret = get_arg_int(gen_argc, gen_argv, rkey, &num);
  if(cl_db) {
    printf("ret %d key (%s) num %d\n", ret, rkey, num);
  }
  if(ret >= 0) {
    ret = num;
  }
  if(cl_db)cl_db--;
  return ret;
}

int ssds_get_gen_name(int ssfd, char *sdef, char *skey, char *rkey
		      , char *res, int rlen)
{
  int ret;

  if(strlen(gen_rbuf)== 0) {
    ssds_get_gen_buf(ssfd, sdef, skey);
  }
  res[0]=0;
  ret = get_arg_char(gen_argc, gen_argv, rkey, res, rlen);
  if(cl_db) {
    printf("ret %d for key (%s) res (%s)\n", ret, rkey, res);
  }
  if(cl_db) cl_db--;
  return ret;
}

int ssds_get_num_classes(int ssfd)
{
  return ssds_get_gen_num(ssfd, NULL, "num_classes", "reply");
}

int ssds_get_class_id(int ssfd, int num)
{
  char tbuf[32];
  sprintf(tbuf,"0%d",num);
  return ssds_get_gen_num(ssfd, tbuf, "gc", "id");
}

int ssds_get_item_id(int ssfd, int cnum, int inum)
{
  char tbuf[32];
  sprintf(tbuf,"0%d.0%d.0",cnum,inum);
  return ssds_get_gen_num(ssfd, tbuf, "gvf", "i_id");
}

size_t ssds_snmp_get_name_len(int ssfd, oid *name, int len, int num)
{
  size_t ret;
  char tbuf[32];
  sprintf(tbuf,"%d.%d.%d",name[len],name[len+1],name[len+2]);
  ret = ssds_get_gen_num(ssfd, tbuf, "gvf", "f_len");
  return ret;
}

int ssds_snmp_get_name_val(int ssfd, char *name,int num, char *res
			   , int rlen)
{
  char tbuf[32];
  int ret;

  sprintf(tbuf,"%d.%d.%d",name[0], name[1], name[2]);
  ret = ssds_get_gen_name(ssfd, tbuf, "gv", "value", res, rlen);
  return ret;
}

int ssds_snmp_set_name_val(int ssfd, char *name, int num, char *res
			   , int rlen)
{
  char tbuf[128];
  int ret;

  sprintf(tbuf,"%d.%d.%d=%s",name[0], name[1], name[2], res);
  ret = ssds_get_gen_name(ssfd, tbuf, "sv", "value", res, rlen);
  return ret;
}

int ssds_snmp_get_name_type(int ssfd, oid *name,int len, int num)
{
  char tbuf[32];
  int ret;

  sprintf(tbuf,"%d.%d.%d",name[len],name[len+1],name[len+2]);
  ret = ssds_get_gen_num(ssfd, tbuf, "gvf", "f_tid");
  return ret;
}

int ssds_get_field_id(int ssfd, int cnum, int fnum)
{
  char tbuf[32];
  sprintf(tbuf,"0%d.0%d", cnum, fnum);
  return ssds_get_gen_num(ssfd, tbuf, "gf", "f_id");
}

int ssds_get_field_tid(int ssfd, int cnum, int fnum)
{
  int ret;

  char tbuf[32];
  sprintf(tbuf,"0%d.0%d", cnum, fnum);
  ret = ssds_get_gen_num(ssfd, tbuf, "gf", "f_tid");
  return ret;
}

int ssds_get_field_len(int ssfd, int cnum, int fnum)
{
  char tbuf[32];
  sprintf(tbuf,"0%d.0%d",cnum,fnum);
  return ssds_get_gen_num(ssfd, tbuf, "gf", "f_len");
}


int ssds_get_num_class_fields(int ssfd, int num)
{
  char tbuf[32];
  sprintf(tbuf,"0%d",num);
  return ssds_get_gen_num(ssfd, tbuf, "gc", "num_fields");
}

int ssds_get_num_class_items(int ssfd, int num)
{
  char tbuf[32];
  sprintf(tbuf,"0%d",num);
  return ssds_get_gen_num(ssfd, tbuf, "gc", "num_items");
}

FindVarMethod var_extensible_ssds;

// look in
// /agent/mibgroup/mib_module_inits.h
// include/net-snmp/agent/var_struct.h
// use variable7 but only populate 3
//
//
/*
 * TODO turn this into a multi depth scanner
 */
int init_ssds_vars(int num_vars)
{
  int i,j,k;
  int var_num;
  int num_classes;
  int num_items;
  int num_fields;
  struct oid_store *this_oid;
  struct oid_store *this_coid;
  struct oid_store *this_ioid;
  struct oid_store *this_foid;
  struct variable7 *this_var;

  ssds_vars = (struct variable7 *)
    ssds_malloc(sizeof(struct variable7) * num_vars);

  ssds_oids = (struct oid_store **)
    ssds_malloc(sizeof(struct oid_store*) * num_vars);

  var_num = 0;
  this_oid = &i_oids;

  this_coid = this_oid->first_oid;
  num_classes = this_oid->num_oids;
  for (i = 0; i < num_classes ; i++) {
    num_items = this_coid->num_oids;
    this_ioid = this_coid->first_oid;
    for (j = 0; j < num_items ; j++) {
      num_fields = this_ioid->num_oids;

      this_foid = this_ioid->first_oid;
      for (k = 0; k < num_fields ; k++) {
	this_var = &ssds_vars[var_num];
        this_var->magic = var_num;
        if(this_foid->type == SSDS_FLOAT) {
	  this_var->type = ASN_OPAQUE_FLOAT;
	} else {
	  this_var->type = ASN_INTEGER;
	}
        this_var->acl = RWRITE;
        this_var->findVar = var_extensible_ssds;
        this_var->namelen = 3;
        this_var->name[0] = this_coid->id;
        this_var->name[1] = this_ioid->id;
        this_var->name[2] = this_foid->id;
        ssds_oids[var_num] = this_foid;
	var_num++;
	this_foid = this_foid->next;

      }
      this_ioid = this_ioid->next;

    }
    this_coid = this_coid->next;

  }
  return var_num;
}
/*
 * TODO turned this into a multi depth scanner
 */
#define MAX_DEPTH 6
/* test code not for use yet */
int do_depth(int depth, int *idx, int *num_items,
	     int var_num, struct oid_store **this_oid)
{
  int id;
  if(depth>=MAX_DEPTH) {
    printf(" max depth reached %d \n", depth);
    return var_num;
  }
  if(0)
    printf(" doing depth %d idx[depth] %d this_oid id %d num %d\n",
	   depth, idx[depth], this_oid[depth]->id,
	   this_oid[depth]->num_oids);
  this_oid[depth+1] = this_oid[depth]->first_oid;
  if (this_oid[depth+1] == NULL) {
    if(0) {
      /* process items, we have a leaf node */
      printf("Leaf found at depth %d var %d idx[", depth, var_num);
      for(id=0; id<depth+1; id++) {
	//printf(".%d",this_oid[idx[id]]->id);
	printf(".%d",idx[id]);
      }
      printf("]\n");
      printf("   Leaf id [");
      for(id=0; id<depth; id++) {
	printf(".%d",this_oid[id+1]->id);
	//printf(".%d",idx[id]);
      }
      printf("]\n");
    }
    return var_num + 1;
  } else {
    idx[depth+1]=0;
  }
  /* not a leaf go deeper */
  num_items[depth] = this_oid[depth]->num_oids;
  for (idx[depth]=0; idx[depth]<num_items[depth]; idx[depth]++) {
    var_num = do_depth(depth+1, idx, num_items, var_num, this_oid);
    this_oid[depth+1] = this_oid[depth+1]->next;
  }
  return var_num;
}


int init_ssds_var_depth(int num_vars)
{
  int i,j,k;
  int depth;
  int num_items[MAX_DEPTH];
  int idx[MAX_DEPTH];
  struct oid_store *this_oid[MAX_DEPTH];
  depth=0;
  int var_num;
  this_oid[depth] = &i_oids;
  var_num = 0;
  idx[0] = 0;
  var_num = do_depth(depth, idx, num_items, var_num, this_oid);

  return var_num;
}

// This is in agent/mib_modules.c
void
init_ssds(void)
{
    /*
     * Define the OID pointer to the top of the mib tree that we're
     * registering underneath
     */

    /*
     * register ourselves with the agent to handle our mib tree
     */
    int num_vars;
    int num_oids;
    char * descr;
    int ssds_fd;
    int num_classes;
    int num_fields;
    int num_items;
    int i,j,k;
    int oid1;
    int id;
    int ret;


    printf("SDCS init_ssds xxx called ...\n");
    ssds_fd = ssds_snmp_open();
    //printf("SDCS init_ssds ssds_fd %d...\n", ssds_fd);
    if(ssds_fd>0) {
      ssds_reset_rbuf();

      num_classes = ssds_get_num_classes(ssds_fd);
      //printf("SDCS init_ssds num classes %d...\n", num_classes);
      for (i=0;i<num_classes;i++) {
	ssds_reset_rbuf();
	cl_db = 0;
	oid1 = ssds_get_class_id(ssds_fd, i);
	//printf("SDCS init_ssds class %d id %d...\n", i, oid1);
	num_fields = ssds_get_num_class_fields(ssds_fd, i);
	num_items  = ssds_get_num_class_items(ssds_fd, i);
	if(0)
	  printf(
	       "SDCS init_ssds class %d oid %d num_fields %d num_items %d\n",
	       i, oid1, num_fields, num_items);
      }
    }

    //cl_db = 1;
    ssds_reset_rbuf();
    if(0) {
      id = ssds_get_item_id(ssds_fd, 1, 0);
      printf("SDCS get_item_id %d \n", id);
      cl_db = 1;
      ssds_reset_rbuf();
      id = ssds_get_field_id(ssds_fd, 1, 0);
      printf("SDCS get_field_id %d \n", id);
      id = ssds_get_field_tid(ssds_fd, 1, 0);
      printf("SDCS get_field_tid %d \n", id);
      id = ssds_get_field_len(ssds_fd, 1, 0);
      printf("SDCS get_field_len %d \n", id);
    }
    num_vars = ssds_set_up_oids(ssds_fd);
    ssds_num_vars = num_vars;
    //printf("SDCS num_vars %d \n", num_vars);
    ret = init_ssds_vars(num_vars);
    //printf("SDCS init_ssds_vars %d \n", ret);

    num_oids = sizeof(ssds_variables_oid)/sizeof(oid);
    descr = "ssds";
    if(register_mib( descr
		    , (struct variable *)ssds_vars
		    , sizeof(struct variable7)
		    , num_vars
		    , ssds_variables_oid
		    , num_oids) != MIB_REGISTERED_OK)
      DEBUGMSGTL(("register_mib", "%s registration failed\n", descr));
    printf("SDCS init_ssds yyy done ...\n");
}


int
write_ssdsStatus(int action,
	       u_char *var_val,
	       u_char var_val_type,
	       size_t var_val_len,
	       u_char *statP, oid *name, size_t name_len)
{
  int int_value;
  float fl_value;
  static int old_alStatus;
  static int new_alStatus;
  char oname [4];
  struct oid_store * oid;
  char new_val_tmp[32];
  char old_val_tmp[32];
  int ret;

  oname[0] = name[name_len-4];
  oname[1] = name[name_len-3];
  oname[2] = name[name_len-2];


  if(0)
    printf("SDCSDB write type %d len %d oid %d.%d.%d \n",
	   var_val_type, var_val_len, oname[0],oname[1],oname[2]);
  //if (var_val_type != ASN_INTEGER || var_val == NULL) {
  if (var_val == NULL) {
    snmp_log(LOG_ERR,
	     "var_val not valid \n");
    return SNMP_ERR_WRONGTYPE;
  }
  if (var_val_type == ASN_INTEGER) {
    int_value = *((long *) var_val);
  }
  if (var_val_type == ASN_OPAQUE_FLOAT) {
    fl_value = *((float *) var_val);
    int_value = 4321;
  }

  switch(action) {

  case RESERVE1:
    // this will create local slots to hold the old and new values
    // not needed since we have all this already in the oid store
    // ssds_snmp_reserve1(oname,3);
    break;

  case RESERVE2:
    if(0)printf(" SDCS RESERVE2 called \n");
    break;

  case FREE:
    if(0)printf(" SDCSDB FREE called \n");
    break;

  case ACTION:
    // copy the new value into new_data
    // copy the current value into old_data
    // this will save the old slot and save the new value
    //ssds_snmp_find_oid(oname, 3, (char *)var_val);
    //ssds_snmp_action(oname, 3, (char *)var_val);
    oid = find_oid_name(NULL, oname, 3);
    if(oid) {

      if (oid->type == SSDS_FLOAT) {
	memcpy(oid->new_data, (void *)&fl_value, oid->len);
      } else
      if (oid->type == SSDS_INT) {
	memcpy(oid->new_data, (void *)&int_value, oid->len);
      } else {
	memcpy(oid->new_data, (void *)&int_value, oid->len);
      }

      ssds_reset_rbuf();
      ret = ssds_snmp_get_name_val(ssds_fd, oname, 3, old_val_tmp, 32);

      if (oid->type == SSDS_FLOAT) {
	sscanf(old_val_tmp,"%f", (float*)oid->old_data);
      } else
      if (oid->type == SSDS_INT) {
	sscanf(old_val_tmp,"%d", (int*)oid->old_data);
      } else {
	sscanf("-456","%d", (int*)oid->old_data);
      }

    } else {
      snmp_log(LOG_ERR, "oid not found ERROR\n");
      return SNMP_ERR_WRONGTYPE;
    }
    break;

  case UNDO:
   // this will restore the value from  the old slot
   // ssds_snmp_undo(oname,3);
    if(0) printf(" SDCSDB UNDO called \n");
    oid = find_oid_name(NULL, oname, 3);

    if(oid) {
      if (oid->type == SSDS_FLOAT) {
	sprintf(old_val_tmp,"%f", *(float*)oid->old_data);
      } else if (oid->type == SSDS_INT) {
	sprintf(old_val_tmp,"%d", *(int*)oid->old_data);
      } else {
	//printf("SDCSDB BAD TYPE %d \n", oid->type);
	sprintf(old_val_tmp,"%d", -5678);
      }
      ssds_reset_rbuf();
      ret = ssds_snmp_set_name_val(ssds_fd, oname, 3, old_val_tmp, 32);
    }
    break;

  case COMMIT:
    // this will commit  the value from  the new slot
    oid = find_oid_name(NULL, oname, 3);
    if(oid) {
      if (oid->type == SSDS_FLOAT) {
	sprintf(new_val_tmp,"%f", *(float*)oid->new_data);
      } else if (oid->type == SSDS_INT) {
	sprintf(new_val_tmp,"%d", *(int*)oid->new_data);
      } else {
	//printf("SDCSDB BAD TYPE %d \n", oid->type);
	sprintf(new_val_tmp,"%d", -5678);
      }
      ssds_reset_rbuf();
      ret = ssds_snmp_set_name_val(ssds_fd, oname, 3, new_val_tmp, 32);
    }
    break;
  }
  return SNMP_ERR_NOERROR;
}

unsigned char  *
var_extensible_ssds(struct variable *vp,
		    oid *name,
		    size_t *length,
		    int exact,
		    size_t *var_len,
		    WriteMethod **write_method)
{

  static long     long_ret;
  static char     errmsg[300];
  static float    float_ret;
  int len;
  struct oid_store *s_oid;
  u_char *f_val;
  int f_type;
  u_char s_val[32];
  int ret;

  DEBUGMSGTL(("agent", "var_extensible_ssds: "));
  DEBUGMSGOID(("ssds", name, *length));
  DEBUGMSG(("ssds", " exact=%d\n", exact));

  snmp_log(LOG_INFO,
	   "SSDS **** agent.c:var_extensible_ssds length %d name[0] %d\n",
	   *length, name[0]);
  len = *length;

  snmp_log(LOG_INFO, "SSDS **** agent.c: name len %d \n", len);

  if (header_generic(vp, name, length, exact, var_len, write_method)) {

    //printf("SDCSDB header_generic return\n");
    return (NULL);
  }

  *write_method = write_ssdsStatus;
  //*var_len = (size_t)ssds_snmp_get_name_len(ssds_fd, name, len-3, 3);
  //f_type   = ssds_snmp_get_name_type(ssds_fd, name, len-3, 3);

  s_oid = ssds_oids[vp->magic];
  ssds_reset_rbuf();

  ret      =  ssds_snmp_get_name_val(ssds_fd, s_oid->name, 3, s_val, 32);
  *var_len = (size_t)s_oid->len;

  f_type   = (int)s_oid->type;
  f_val    = (u_char *)s_oid->new_data;

  if(f_val && (f_type > 0)) {
    if(f_type == 2) {
      sscanf(s_val,"%f",(float*)f_val);
    } else if(f_type == 1) {
      sscanf(s_val,"%d",(int*)f_val);
    } else {
      *(int*)f_val=-123;
    }
    return f_val;
  }
  return NULL;
}
