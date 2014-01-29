/*
 * ssds_cgi.c

 * small system data server
 * basic cgi interface system
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 *
 *  @@classes_start@@ @@classes_end@@
 *  @@class_items_start@@ @@class_items_end@@
 *  @@class_fields_start@@ @@class_fields_end@@
 *  @@items_start@@ @@items_end@@
 *  @@class_named@@
 *  @@field_named@@
 *  @@item_named@@
 *  @@class_name@@
 *  @@class_id@@
 *  @@class_num_fields@@
 *  @@class_num_items@@
 *  @@item_name@@
 *  @@item_id@@
 *  @@field_name@@
 *  @@field_id@@
 *  @@field_type@@
 *  @@item_field_name@@
 *  @@item_field_id@@
 *  @@item_field_type@@
 *  @@item_field_value@@
 *
 *
 * query string processing
 * set values
 * pick view / item / class etc
 * ?view=default
 * ?view=classes
             - will show all classes
 * view=class_fields&class=class_name
             - will show class fields for a class named <class_name>
 * view=class_items&class=class_name
             - will show class item for a class named <class_name>
 * view=item&class=class_name&item=item_name
             - will show item called <item_name> for a class named <class_name>


 *  we maintain a context stack based on start and end char
 *  lets see how it goes

 */

#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define DEF_FILE_NAME "cgi_file.txt"
#define DEF_FILE_DIR "/home/html/ssds"
#define Q_SIZE 1024
#define RAW_SIZE (1024 * 16)
#define DONE_SIZE (1024 * 16 *2)
#define NUM_TOKS 1024
#define NUM_CTX 1024
#define MAX_CTXIDX 1024
/* stops token overrun */
#define MAX_TOKENS 500


struct token {
  char tok[64];
  int tlen;
  char *tstart;
  int is_class;
  int is_field;
  int is_ifield;
  int is_item;
  int is_start;
  int is_end;

  struct ctx *this_ctxp;
  struct ctx *last_ctxp;

};

struct ctx {
  char * sp;
  int tlen;
  int toknum;

  //struct token *xtokp;
  int cur_class;
  int cur_field;
  int cur_ifield;
  int cur_item;

  int num_class;
  int num_field;
  int num_ifield;
  int num_item;

  int is_class;
  int is_field;
  int is_ifield;
  int is_item;

};


struct queryp {
  char *name;
  char *value;
};

int num_q;

struct queryp qp[1024];
int q_num;

char q_str[Q_SIZE];

struct token toks[NUM_TOKS];
struct ctx ctx[NUM_CTX];

char rawfile[RAW_SIZE];
char donefile[DONE_SIZE];

int ctxstack[MAX_CTXIDX];
int ctxidx=0;

int cur_ctx=0;
int num_ctx=0;
int num_tok=0;

int debug;
int debug_tok;
int debug_query;
/* ssds interface */

char *get_system_name(void);
char *get_system_vers(void);

int ssds_cgi_new_field(int fd,
		       char *classsp, char *namesp, char *typesp,
		       char *valsp, char *idsp, char *lensp);

int ssds_cgi_new_item(int fd, char *classsp, char *namesp,char *idsp);
int ssds_cgi_new_class(int fd, char *classsp, char *idsp);

int ssds_item_keyx(int fd, char *tok, char *sp, char *key);
int ssds_item_key(int fd, char *tok, char *sp, char *key);
int ssds_num_classes(int fd);
int ssds_num_fields(int fd, int class_num);
int ssds_num_items(int fd, int class_num);
int ssds_class_name(int fd, int class_num, char * sp);
int ssds_class_id(int fd, int class_num, char *sp);

int ssds_field_name(int fd, int class_num, int field_num, char *sp);
int ssds_field_id(int fd, int class_num, int field_num, char *sp);
int ssds_field_type(int fd, int class_num, int field_num, char *sp);
int ssds_item_name(int fd, int class_num, int item_num, char *sp);
int ssds_item_id(int fd, int class_num, int item_num, char *sp);
int ssds_field_value(int fd, int class_num, int field_num, int item_nnum,
		     char *sp);

int ssds_get_name(int fd, char *sp, char *name);
int ssds_get_vers(int fd, char *sp, char *name);

int ssds_item_value(int fd, int class_num, int field_num, int item_num
		    , char *sp);

int setup_ssds_client(int argc, char *argv[]);

int ssds_do_action(int fd, char *sp_action, char *rsp);

int get_item_num(char *str);
int get_class_num(char *str);
int get_field_num(char *str);

//char *get_field_id(int class_num, int field_num);
//char *get_field_type(int class_num, int field_num);
//char *get_item_field_name(int class_num, int field_num, int item_num);
//char *get_item_field_id(int class_num, int field_num, int item_num);
//char *get_item_field_value(int class_num, int field_num, int item_num);
//char *get_item_field_type(int class_num, int field_num, int item_num);
//char *get_item_name(int class_num, int item_num);
//char *get_item_id(int class_num, int item_num);


/*
 * context stack
 */

/* this may need work */
struct ssds_client *get_ssds_client(int ix)
{
  struct ssds_client *cli = NULL;
  return cli;
}

void ctx_push(int ctxid)
{
  if (ctxidx < 0) {
    ctxidx = 0;
  } else if (ctxidx < MAX_CTXIDX) {
    ctxstack[ctxidx]=ctxid;
    ctxidx++;
  }
}

int ctx_pop(void)
{
  int ret;
  ret  = -1;
  if (ctxidx > 0) {
    ctxidx--;
    ret=ctxstack[ctxidx];
  }
  return ret;
}

/*
 * print header
 */
int print_html_head(void)
{
  printf("Content-type: text/html\n\n");

  printf("<html><head><title>CGI File</title></head><body>\n");
  printf("<H2>CGI File</H2>\n\n");
  return 0;
}

int print_html_tail(void)
{
  printf("\n<EM>CGI_FILE</EM>\n");
  printf("</body></html>\n");
  return 0;
}

/*
 * get post
 * get post data
 */

char post_buf[1024];

int post_bytes;
int cl_bytes;
char cl_buf[132];
char rm_buf[132];

int get_post(int argc, char *argv[])
{
  char *qs;
  int bytes;

  bytes = 0;
  post_buf[bytes] = 0;
  post_bytes = 0;

  cl_bytes=0;
  qs = getenv("CONTENT_LENGTH");
  if(qs) {
    strncpy(cl_buf, qs, 132);
    cl_bytes = atoi(qs);
  } else {
    strcpy(cl_buf,"CL Not Defined");
  }
  qs = getenv("REQUEST_METHOD");
  if(qs) {
    strncpy(rm_buf, qs, 132);
  } else {
    strcpy(rm_buf,"RM Not Defined");
  }

  qs = getenv("QUERY_STRING");

  if(strcmp(rm_buf,"POST")==0) {
    bytes += read(0, &post_buf[bytes], cl_bytes);
    if(bytes>0) {
      //bytes += fread(&post_buf[bytes], 132, 1, fd);
      post_buf[bytes] = 0;
      post_bytes = bytes;
    }
  }
  return bytes;
}
/*
 * process the query string
 */
int do_query(void)
{
  char *qs;
  char *sp;
  int qnum;
  struct queryp *qsp;
  qnum = 0;
  qs = getenv("QUERY_STRING");

  if (qs) {
    strncpy(q_str, qs, Q_SIZE);
    sp = q_str;
    if(debug_query) {
      printf("QUERY_STRING (%s)\n",sp);
    }
  } else {
    post_buf[cl_bytes] =0;
    sp = post_buf;
  }
  while(*sp) {
    qsp = &qp[qnum++];
    qsp->name=sp;
    while (*sp && (*sp != '=') ) sp++;
    if(*sp == '=') {
      *sp = 0;
      sp++;
      qsp->value=sp;
      while (*sp && (*sp != '&') ) sp++;
      if(*sp == '&') {
	qsp =&qp[qnum];
	*sp = 0;
	sp++;
      }
    }
  }
  q_num = qnum;
  return qnum;
}

int show_query(int qnum)
{
  int i;
  struct queryp *qsp;
  printf("<ul>\n");

  for (i = 0; i < qnum; i++) {
      qsp =&qp[i];
      printf("<li>[%d] name (%s) value (%s) \n",i, qsp->name,qsp->value);
  }
  printf("</ul>\n");

  return 0;
}
/*
 * given a name find a query item
 */
char *find_query(char *name, int qnum)
{
  int i;
  struct queryp *qsp;
  char * ret;

  ret =  NULL;
  for (i = 0; i < qnum; i++) {
      qsp =&qp[i];
      if(strcmp(qsp->name, name)==0){
	ret = qsp->value;
	break;
      }
  }
  return ret;
}


/*
 * process an action
 * display the result
 *
 */
int do_action(char *donesp, char *actsp, char *ressp)
{
  int ret;
  char *mysp;
  mysp = donesp;

  donesp += sprintf(donesp,"<H1>Action Sent</H1>\n");
  donesp += sprintf(donesp,"<table border=1><tr><td><pre>\n");
  donesp += sprintf(donesp, "%s", actsp);
  donesp += sprintf(donesp,"</pre></td></tr></table>\n");

  donesp += sprintf(donesp,"<H1>Action Result</H1>\n");
  donesp += sprintf(donesp,"<pre>\n");
  donesp += sprintf(donesp, "%s", ressp);
  donesp += sprintf(donesp,"</pre></td></tr></table>\n");
  ret = donesp - mysp;
  return ret;
}

/*
 * process a file
 * replace tokens with values
 * handle class , field and item loops
 *
 */
int do_file(char *donesp, char *sp, int bytes, int dbfd)
{
  int ret;
  int toknum;
  int ctxnum;
  struct token *tokp;
  struct ctx *ctxp;
  struct ctx *last_ctxp;
  int done;
  char *cursp;
  char *xsp;
  char *qsp;
  char qname[64];

  ret = 0;
  done = 0;
  toknum = 0;
  ctxnum = 0;
  cursp = sp;

  /* clear stack */
  ctx_push(-1);

  tokp = &toks[toknum];
  ctxp = &ctx[ctxnum];
  if(debug) {
    printf(" \n\nRunning file \n");
    //return 0;
  }
  while(!done) {
    // emit  chars from cursp to tokp->tstart
    while(sp < (tokp->tstart)) {
      *donesp++ = *sp++;
    }
    if(0 &&debug){
      static int dtoks=30;
      printf(" Token found (%s) \n", tokp->tok);
      printf(" Token found is_start = %d \n", tokp->is_start);
      printf(" Token found is_end = %d \n", tokp->is_end);
      if(dtoks-- == 0)return 0;
    }
    if(tokp->is_start){

      sp +=tokp->tlen;
      ctxp = tokp->this_ctxp;
      last_ctxp = tokp->last_ctxp;
      if(debug)
	printf(" Token found start last_ctxp = %p \n", last_ctxp);
      if(last_ctxp) {
	ctxp->cur_class  = last_ctxp->cur_class;
	ctxp->cur_field  = last_ctxp->cur_field;
	ctxp->cur_ifield = last_ctxp->cur_ifield;
	ctxp->cur_item   = last_ctxp->cur_item;
      }
      if(ctxp->is_class) {
	ctxp->cur_class = 0;
	ctxp->num_class = ssds_num_classes(dbfd);
	ctxp->sp = sp;
	ctxp->toknum = toknum;

	if(debug)
	  printf(" start_class num %d sp %p toknum %d\n",
		 ctxp->num_class, ctxp->sp, toknum);
      }
      if(ctxp->is_field) {
	ctxp->cur_field = 0;
	ctxp->num_field = ssds_num_fields(dbfd, ctxp->cur_class);
	ctxp->sp = sp;
	ctxp->toknum = toknum;
	if(debug)
	  printf(" start_field num %d sp %p toknum %d\n",
		 ctxp->num_field, ctxp->sp, toknum);

      }
      if(ctxp->is_ifield) {
	ctxp->cur_ifield = 0;
	ctxp->num_ifield = ssds_num_fields(dbfd, ctxp->cur_class);
	ctxp->sp = sp;
	ctxp->toknum = toknum;
	if(debug)
	  printf(" start_ifield num %d sp %p toknum %d\n",
		 ctxp->num_ifield, ctxp->sp, toknum);
      }
      if(ctxp->is_item) {
	ctxp->cur_item = 0;
	ctxp->num_item = ssds_num_items(dbfd, ctxp->cur_class);
	ctxp->sp = sp;
	ctxp->toknum = toknum;
	if(debug)
	  printf(" start_item num %d sp %p toknum %d\n",
		 ctxp->num_item, ctxp->sp, toknum);
      }
    }
    else if(tokp->is_end) {
      sp += tokp->tlen;
      ctxp = tokp->this_ctxp;
      if(debug)
	printf(" end ctxp %p\n", ctxp);

      if(ctxp->is_class) {
	if(debug)
	  printf(" end_class sp %p\n", ctxp->sp);
	ctxp->cur_class++;
        if(ctxp->cur_class < ctxp->num_class) {
	  sp = ctxp->sp;
          toknum=ctxp->toknum;
	}
      }
      if(ctxp->is_field) {
	if(debug)
	  printf(" end_field sp %p\n", ctxp->sp);
	ctxp->cur_field++;
        if(ctxp->cur_field < ctxp->num_field) {
	  sp = ctxp->sp;
          toknum=ctxp->toknum;
	}
      }
      if(ctxp->is_ifield) {
	if(debug)
	  printf(" end_ifield sp %p\n", ctxp->sp);
	ctxp->cur_ifield++;
        if(ctxp->cur_ifield < ctxp->num_ifield) {
	  sp = ctxp->sp;
          toknum=ctxp->toknum;
	}
      }

      if(ctxp->is_item) {
	if(debug)
	  printf(" end_item sp %p\n", ctxp->sp);
	ctxp->cur_item++;
        if(ctxp->cur_item < ctxp->num_item) {
	  sp = ctxp->sp;
          toknum=ctxp->toknum;
	}
      }
    } else {
      if(strncmp(tokp->tok,"@@query_",8) == 0) {
	sp +=tokp->tlen;
	qsp = qname;
	xsp = &tokp->tok[8];
        while (*xsp != '@') {
	  *qsp++ = *xsp++;
	}
	*qsp = 0;
        qsp = find_query(qname, q_num);
        if(qsp) {
	  donesp+=sprintf(donesp,"%s",qsp);
	} else {
	  donesp+=sprintf(donesp,"<!--query (%s) not found -->",qname);
	}
	ctxp = tokp->this_ctxp;
	if(debug)
	  printf(" name ctxp %p\n", ctxp);

      } else

      if(strcmp(tokp->tok,"@@post_data@@") == 0) {
	int ix;
	sp +=tokp->tlen;
        donesp+=sprintf(donesp,"<br>post bytes %d\n", post_bytes);
        donesp+=sprintf(donesp,"<br>cont len (%s)\n", cl_buf);
        donesp+=sprintf(donesp,"<br>req meth (%s)\n", rm_buf);
        if(1) {
	  for (ix=0;ix<10;ix++) {
            if(post_buf[ix] == 0) {
	      donesp+=sprintf(donesp,
			      "[%d] %x dec(%d)\n", ix, post_buf[ix]
			      , post_buf[ix]);
	    } else {
	      donesp+=sprintf(donesp,
			      "[%d] %x %c (%d)\n", ix, post_buf[ix]
			      , post_buf[ix], post_buf[ix]);

	    }
	  }
	}
      } else
      if(strncmp(tokp->tok,"@@blank",7) == 0) {
	sp +=tokp->tlen;
      } else
      if(strcmp(tokp->tok,"@@sys_name@@") == 0) {
	sp +=tokp->tlen;
	donesp+=ssds_get_name(dbfd, donesp, NULL);
      } else
      if(strcmp(tokp->tok,"@@sys_version@@") == 0) {
	sp +=tokp->tlen;
	donesp+=ssds_get_vers(dbfd, donesp, NULL);
      } else
      if(strncmp(tokp->tok,"@@gv.",5) == 0) {
        int res;
        res = 0;
	sp += tokp->tlen;
	res = ssds_item_keyx(dbfd, &tokp->tok[2], donesp, "value");
        if(res > 0) donesp += res;
      } else
      if(strncmp(tokp->tok,"@@setgn.",8) == 0) {
        int res;
        char stmp[32];
        res = 0;

	sp += tokp->tlen;
	ctxp = tokp->this_ctxp;
        //printf("\n ctxp 1 = %p ctxnum %d\n",ctxp, ctxnum);

	if(!ctxp) ctxp = &ctx[ctxnum];
        //printf("\n ctxp 2 = %p\n",ctxp);
	res = ssds_item_keyx(dbfd, &tokp->tok[5], stmp, "num");
	if(res > 0) {
	  stmp[res] = 0;
	  ctxp->cur_class = get_class_num(stmp);
	  ctxp->cur_field = get_field_num(stmp);
	  ctxp->cur_item = get_item_num(stmp);
	}
        // donesp += res;
      } else
      if(strncmp(tokp->tok,"@@gn.",5) == 0) {
        int res;
        res = 0;
	sp += tokp->tlen;
	res = ssds_item_keyx(dbfd, &tokp->tok[2], donesp, "name");
        if(res > 0) donesp += res;
      } else
      if(strncmp(tokp->tok,"@@ign.",6) == 0) {
        int res;
        res = 0;
	sp += tokp->tlen;
	res = ssds_item_keyx(dbfd, &tokp->tok[3], donesp, "id");
        if(res > 0) donesp += res;
      } else
      if(strncmp(tokp->tok,"@@sgn.",6) == 0) {
        int res;
        res = 0;
	sp += tokp->tlen;
	res = ssds_item_keyx(dbfd, &tokp->tok[3], donesp, "num");
        if(res > 0) donesp += res;
      } else
      if(strcmp(tokp->tok,"@@field_num@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+=sprintf(donesp,"%d",ctxp->cur_field);
	} else {
	  donesp+=sprintf(donesp,"<FNUM>");
	}
      } else
      if(strcmp(tokp->tok,"@@item_num@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+=sprintf(donesp,"%d", ctxp->cur_item);
	} else {
	  donesp+=sprintf(donesp,"<INUM>");
	}
      } else
      if(strcmp(tokp->tok,"@@class_num@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+=sprintf(donesp,"%d", ctxp->cur_class);
	} else {
	  donesp+=sprintf(donesp,"<CNUM>");
	}
      } else
      if(strcmp(tokp->tok,"@@class_num_fields@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
          //printf(" geting num fields for class %d\n",ctxp->cur_class);
	  ctxp->num_field = ssds_num_fields(dbfd, ctxp->cur_class);
	  donesp+=sprintf(donesp,"%d", ctxp->num_field);
          //printf(" got num fields as %d\n", ctxp->num_field);
          //return 0;
	}
      } else
      if(strcmp(tokp->tok,"@@num_classes@@") == 0) {
	int num;
	sp +=tokp->tlen;
	num = ssds_num_classes(dbfd);
	donesp+=sprintf(donesp,"%d", num);
      } else
      if(strcmp(tokp->tok,"@@item_class_name@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_class_name(dbfd, ctxp->cur_class, donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@class_name@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
          int numc;
	  if(debug) {
	    printf(" getting class name for class num %d\n",ctxp->cur_class);
	    //return 0;
	  }
	  numc = ssds_class_name(dbfd, ctxp->cur_class, donesp);
          //return 0;
	  donesp+= numc;

	  if(debug) {
	    printf(" got class name for class num %d numc %d\n"
		   ,ctxp->cur_class, numc);
	    //return 0;
	  }
	}
      } else
      if(strcmp(tokp->tok,"@@class_id@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_class_id(dbfd, ctxp->cur_class, donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@field_name@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_name(dbfd, ctxp->cur_class, ctxp->cur_field
				   , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@field_id@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_id(dbfd, ctxp->cur_class, ctxp->cur_field
				 , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@field_type@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_type(dbfd, ctxp->cur_class, ctxp->cur_field
				   , donesp);
}
      } else
      if(strcmp(tokp->tok,"@@item_name@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_item_name(dbfd, ctxp->cur_class, ctxp->cur_item
				   , donesp);
	  //return 0;
	}
      } else
      if(strcmp(tokp->tok,"@@item_id@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_item_id(dbfd, ctxp->cur_class, ctxp->cur_item
				   , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@item_field_name@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_name(dbfd, ctxp->cur_class, ctxp->cur_ifield
				   , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@item_field_num@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= sprintf(donesp,"%d", ctxp->cur_ifield);
	}
      } else
      if(strcmp(tokp->tok,"@@item_field_id@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_id(dbfd, ctxp->cur_class, ctxp->cur_ifield
				   , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@item_field_type@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  donesp+= ssds_field_type(dbfd, ctxp->cur_class, ctxp->cur_ifield
				   , donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@item_field_value@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
	//printf("SDCS return \n");
	//return 0;
        if(ctxp) {
	  donesp+= ssds_item_value(dbfd, ctxp->cur_class, ctxp->cur_ifield
				   , ctxp->cur_item,  donesp);
	}
      } else
      if(strcmp(tokp->tok,"@@class_num_items@@") == 0) {
	sp +=tokp->tlen;
	ctxp = tokp->this_ctxp;
        if(ctxp) {
	  ctxp->num_item = ssds_num_items(dbfd, ctxp->cur_class);
	  donesp+=sprintf(donesp,"%d", ctxp->num_item);
	}
      } else
      if(strcmp(tokp->tok,"@@done@@") == 0) {
	done = 1;
      }
    }

    toknum++;
    tokp = &toks[toknum];
    if(toknum > MAX_TOKENS) done = 1;
  }
  *donesp = 0;
  return ret;
}

int find_tokens(char *sp, int bytes)
{
  char *stok;
  int toknum;
  int done;
  char *tstart;
  char *tend;
  char tok[1024];
  int tlen;
  struct token *tokp;
  struct ctx *ctxp;
  struct ctx *lctxp;

  toknum = 0;
  stok = sp;
  done = 0;
  cur_ctx = 0;
  num_ctx = 0;
  /*
   * set up initial context
   * this should never be used
   */
  ctxp = &ctx[num_ctx];

  //ctxp->tstart=0;
  ctxp->tlen=0;
  //ctxp->tokp=NULL;
  ctxp->is_class=ctxp->is_field=ctxp->is_ifield=ctxp->is_item=0;
  if(debug_tok)
    printf("tokens from file\n<ul>\n");
  while (!done) {
    stok=strstr(stok,"@@");
    if(stok && *stok) {
      if(debug)
        printf("found token start at %ld (%c) ", (long)(stok-sp), *stok);
      tstart = stok;
      stok += 2;
      stok=strstr(stok,"@@");
      if(stok && *stok) {
	stok += 2;
	tend = stok;
	tlen = tend-tstart;
	if(debug)
	  printf("end at %ld (%c) ", (long)(stok-sp), *stok);
        strncpy(tok, tstart, tlen);
	tok[tlen] = 0;
	if(debug_tok)
	  printf("<li>--token <%s>\n", tok);

        tokp = &toks[toknum];
	tokp->this_ctxp = ctxp;
	strncpy(tokp->tok, tstart, tlen);
	tokp->tok[tlen] = 0;
	tokp->tlen = tlen;
	tokp->tstart = tstart;
	//	tokp->tend = tend;
	//tokp->ctx = cur_ctx;
	tokp->is_start = 0;
	tokp->is_end = 0;
        if(strstr(tok,"_start@@")) {
          /*
	   * we push the context
	   * we need a new context
	   * we push the context
	   * we inc the number of contexts
	   */
	  tokp->is_start = 1;
	  lctxp = tokp->last_ctxp = &ctx[cur_ctx];
          ctx_push(cur_ctx);
          num_ctx++;
	  cur_ctx = num_ctx;
	  ctxp = &ctx[num_ctx];
	  tokp->this_ctxp = &ctx[num_ctx];
	  //tokp->ctx = num_ctx;
          //ctxp->tstart=tstart;
          ctxp->tlen=tlen;
          //ctxp->tokp=tokp;
          ctxp->is_class=ctxp->is_field=ctxp->is_ifield=ctxp->is_item=0;
	  if(strstr(tok,"class_start")) {
	    ctxp->is_class=1;
	    if(debug)
	      printf(" Class start ctx %d\n",cur_ctx);
	  }
	  if(strstr(tok,"class_items_start")) {
	    ctxp->is_item=1;
	    if(debug)
	      printf(" Item start ctx %d\n",cur_ctx);
	  }
	  if(strstr(tok,"class_fields_start")) {
	    ctxp->is_field=1;
	    if(debug)
	      printf(" Field start ctx %d\n",cur_ctx);
	  }
	  if(strstr(tok,"item_fields_start")) {
	    ctxp->is_ifield=1;
	    if(debug)
	      printf(" Item Field start ctx %d\n",cur_ctx);
	  }
	}
        if(strstr(tok,"_end")) {
	  //tokp->ctx = cur_ctx;
	  tokp->is_end = 1;
	  tokp->this_ctxp = ctxp;
	  cur_ctx = ctx_pop();
	  ctxp = &ctx[cur_ctx];
	}
	toknum++;
      } else {
	done = 1;
      }
    } else {
      done = 1;
    }
  }
  num_tok = toknum;
  if(debug_tok)
    printf("</ul>\n");
  return toknum;
}


char *fname;
int read_file(char *sp, char * fname, int len)
{
  FILE *fd;
  int bytes;

  bytes = 0;
  fd =  fopen(fname, "r");
  if (fd) {
    while(fgets(sp, len, fd)) {
      len -= strlen(sp);
      bytes += strlen(sp);
      sp += strlen(sp);
    }
    fclose(fd);
  } else {
    bytes = -1;
  }
  return bytes;
}


int ssds_set_item_value(int fd, int class_num, int field_num, int item_num
			, char *valsp, int len, char * res);

/* query strings should be set up  */
int do_set_fields(int dbfd, int ctxidx, int qnum)
{
  struct ctx *ctxp;
  int i;
  int ret;
  int len;
  char *valsp;
  char qstr[132];
  char qres[132];

  ret = 0;
  ctxp = &ctx[ctxidx];
  ctxp->num_field = ssds_num_fields(dbfd, ctxp->cur_class);
  for (i = 0; i< ctxp->num_field; i++) {
    sprintf(qstr,"set_field_%d",i);
    valsp = find_query(qstr, qnum);
    if(valsp){
      len = strlen(valsp);
      ssds_set_item_value(dbfd,ctxp->cur_class,i,ctxp->cur_item,
			  valsp, len, qres);
      printf("<br>Set value [%d] (%s) res (%s)\n",i,valsp,qres);
      ret++;
    } else {
      printf("<br>Set value [%d] Value not found\n",i);
    }
  }
  return ret;
}
/* query strings should be set up  */
int do_add_field(int dbfd, int ctxidx, int qnum)
{
  //int i;
  int ret;
  //int len;
  char *namesp;
  char *typesp;
  char *valsp;
  char *idsp;
  char *lensp;
  char *classsp;
  //char qstr[132];
  //char qres[132];
  //struct ctx *ctxp;

  ret = 0;
  //printf(" Add Field Not Yet working \n");
  //ctxp = &ctx[ctxidx];
  classsp = find_query("class_name", qnum);
  namesp = find_query("new_field_name", qnum);
  typesp = find_query("new_field_type", qnum);
  valsp = find_query("new_field_value", qnum);
  idsp = find_query("new_field_id", qnum);
  lensp = find_query("new_field_len", qnum);

  if(classsp && namesp) {
    ret = ssds_cgi_new_field(dbfd,
			    classsp, namesp, typesp, valsp, idsp, lensp);
    printf("<br>Created new field (%s) in class (%s)  \n",
	   namesp, classsp);

  } else {
    printf("<br>New field Name not defined\n");
  }
  return ret;
}

int do_add_item(int dbfd, int ctxidx, int qnum)
{
  int ret;
  //int i;
  //int len;
  char *namesp;
  char *idsp;
  //char *typsp;
  //char *valsp;
  //char *lensp;
  char *classsp;
  //char qstr[132];
  //char qres[132];
  //struct ctx *ctxp;

  ret = 0;
  //ctxp = &ctx[ctxidx];
  classsp = find_query("class_name", qnum);
  namesp = find_query("new_item_name", qnum);
  idsp = find_query("new_item_id", qnum);
  //typsp = find_query("new_field_type", qnum);
  //valsp = find_query("new_field_value", qnum);
  //lensp = find_query("new_field_len", qnum);

  if(classsp && namesp) {
    ret = ssds_cgi_new_item(dbfd,
			    classsp, namesp, idsp);
    printf("<br>Created new item (%s) in class (%s)  \n",
	   namesp, classsp);
  } else {
    printf("<br>New Item Name not defined\n");
  }
  return ret;
}

int do_add_class(int dbfd, int ctxidx, int qnum)
{
  //int i;
  int ret;
  //int len;
  char *classsp;
  char *idsp;
  //char *namsp;
  //char *typsp;
  //char *valsp;
  //char *lensp;

  //char qstr[132];
  //char qres[132];

  ret = 0;
  classsp = find_query("new_class_name", qnum);
  idsp = find_query("new_class_id", qnum);
  //namesp = find_query("new_field_name", qnum);
  //typsp = find_query("new_field_type", qnum);
  //valsp = find_query("new_field_value", qnum);

  //lensp = find_query("new_field_len", qnum);

  if(classsp) {
    ret = ssds_cgi_new_class(dbfd,
			    classsp, idsp);
    printf("<br>Created new class (%s)  \n",
	   classsp);
  } else {
    printf("<br>New Class Name not defined\n");
  }
  return ret;
}

extern int client_debug;
extern FILE *stdin;
extern char *query_port_num;
extern char *query_node_addr;

int html_flag = 1;
int main(int argc, char *argv[])
{
  int ret;
  char *sp;
  char *spdone;
  int len;
  int bytes;
  int qnum;
  int pnum;
  int dbfd;
  char * qsp;
  char fbuf[128];
  struct ctx *ctxp;
  int i;

  if(argc>1) {
    for(i=1;i<argc;i++) {
      if(strcmp(argv[i],"-html") == 0) {
	html_flag=0;
      } else
      if(strcmp(argv[i],"-f") == 0) {
	if(i<argc-1) {
	  fname=argv[i+1]; i++;
	}
      }
    }
  }
  /*
   * process opts
   */
  pnum = get_post(argc, argv);
  qnum = do_query();
  qsp =find_query("ssds_host", qnum);
  if(qsp) query_node_addr=qsp;
  qsp =find_query("ssds_port", qnum);
  if(qsp) query_port_num=qsp;

  dbfd = setup_ssds_client(0, NULL);

  qsp =find_query("debug", qnum);
  if(qsp) debug=1;
  qsp =find_query("debug_tok", qnum );
  if(qsp) debug_tok=1;
  qsp =find_query("debug_query", qnum );
  if(qsp) debug_query=1;

  ctxp = &ctx[num_ctx];
  qsp =find_query("set_class_num", qnum );
  if(qsp)ctxp->cur_class = atoi(qsp);
  qsp =find_query("set_item_num", qnum );
  if(qsp)ctxp->cur_item = atoi(qsp);
  qsp =find_query("set_field_num", qnum );
  //if(qsp) debug=1;
  if(qsp){
    ctxp->cur_field = atoi(qsp);
    ctxp->cur_ifield = atoi(qsp);
  }

  qsp = find_query("set_fields", qnum);
  if(qsp){
    do_set_fields(dbfd, num_ctx, qnum);
  }
  qsp = find_query("add_field", qnum);
  if(qsp){
    do_add_field(dbfd, num_ctx, qnum);
  }
  qsp = find_query("add_item", qnum);
  if(qsp){
    do_add_item(dbfd, num_ctx, qnum);
  }

  qsp = find_query("add_class", qnum);
  if(qsp){
    do_add_class(dbfd, num_ctx, qnum);
  }

  fname =find_query("file", qnum);
  //return 0;
  if(fname == NULL) fname = DEF_FILE_NAME;

  qsp = find_query("action", qnum);

  if(html_flag)print_html_head();

  if(qsp) {
    //client_debug = 1;
    sp = rawfile;
    *sp = 0;
    ssds_do_action(dbfd, qsp, sp);
    if(0)printf("\nssds_do_action done \n qsp %p(%s) \n sp %p(%s)\n\n"
		,qsp, qsp, sp, sp);

    spdone = donefile;
    *spdone = 0;

    do_action(spdone, qsp, sp);
    if(0)
      printf("\nlocal do_action done\n");
    /* main html output for action */
    printf("%s", donefile);
    qsp = find_query("quit", qnum);
    if(qsp[0]=='y') return 0;
  }

  sp = rawfile;
  len = RAW_SIZE;

  sprintf(fbuf, "%s/%s", DEF_FILE_DIR, fname);

  bytes = read_file(sp, fname, len);

  if (bytes < 0) {
    bytes = read_file(sp, fbuf, len);
  }

  if (bytes < 0) {
    goto file_error;
  }

  if(debug)
    printf("<!-- bytes read  = %d\n-->", bytes);
  if (bytes > 0) {
    int tokz;
    sp = rawfile;
    if(0 && debug)
      printf("<!-- Rawfile = <%s> \n num bytes in rawfile = %d\n-->"
	     ,sp, bytes);
    tokz = find_tokens(sp, bytes);
    if(debug)
      printf("<!-- num tokens %d \n-->",tokz);
    //return 0;

    spdone = donefile;
    do_file(spdone, sp, bytes, dbfd);
    if(debug)
      printf("<!--done file = \n>>>>*******\n%s\n>>>>*******\n-->",donefile);
  } else {
    if(debug)
      printf("<!--unable to find file <%s> \n-->", fbuf);
  }

  if(debug)
    printf("<!--\nnum_ctx %d cur_ctx %d num_tok %d\n-->",
	   num_ctx, cur_ctx, num_tok);
  if(bytes > 0) {
    if(debug_query)
      show_query(qnum);
    /* main html output */
    printf("%s",donefile);
    goto file_ok;
  }
 file_error:
  printf("<h1>unable to find files </h1><ul><li><b>%s</b> <li><b>%s</b> </ul>", fname, fbuf);
 file_ok:
  if(html_flag)print_html_tail();
  ret = 0;
  return ret;
}
