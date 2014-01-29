/*
 * small system data server
 *
 * Copyright 2009 System Design & Consulting Services, LLC
 *
 *                     All Rights Reserved
 * Licensed under GPLv2, see file LICENSE in this tarball for details.
 *
 * May 12, 2009
 *  ssds_server.c
 * opens clients
 * send out help
 * services data
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>

#define SSDS_LOCAL
#include "ssds.h"


char *node_addr = "0.0.0.0";
int port_num     = 4321;

int debug;
char *fname;
char *version="0.91";


/* current number of clients */
int num_clients;

/* config file name */
extern char *ssds_file_name;


struct ssds_client ssds_client[NUM_CLIENTS];

struct ssds_client *get_ssds_client(int ix)
{
  struct ssds_client *cli = NULL;

  if (ix < (NUM_CLIENTS-1)) {
    cli = &ssds_client[ix];
  }
  return cli;
}

#include "ssds_find_group.c.inc"

int init_sys(void)
{
  int i;
  for (i = 0; i < NUM_CLIENTS; i++) {
    ssds_client[i].fd = -1;
    ssds_client[i].ix = i;
    ssds_client[i].num_groups = 0;
    ssds_client[i].max_groups = NUM_GROUPS;

  }
  /* grab the first client for the server */
  ssds_client[0].fd = 0;
  ssds_set_find_group(do_find_group);

  return 0;
}

struct ssds_client *new_client(int fd)
{
  int i;

  struct ssds_client *client;
  client = NULL;

  for (i = 0; i < NUM_CLIENTS; i++) {
    if(ssds_client[i].fd == -1) {
      ssds_client[i].fd = fd;
      ssds_client[i].ix = 1;
      client = &ssds_client[i];
      break;
    }
  }
  return client;
}

struct ssds_client *find_cli_fd(int fd)
{
  int i;

  struct ssds_client *client;

  client = NULL;
  for (i = 0; i < NUM_CLIENTS; i++) {
    if(ssds_client[i].fd == fd) {
      client = &ssds_client[i];
      break;
    }
  }
  return client;
}

int free_client(struct ssds_client *client)
{
  if (client->fd > 0){
    close(client->fd);
    client->fd = -1;
  }
  return (client->idx);
}

int poll_add_clients(struct pollfd *fds, int num, int max)
{
  int i;
  struct ssds_client *client;

  for (i = 0; i < NUM_CLIENTS; i++) {
    client = &ssds_client[i];
    if( client->fd > 0) {
      fds[num].fd = client->fd;
      fds[num].events = POLLIN | POLLHUP;
      if (num < max) num++;
    }
  }
  return num;
}

int do_serv(int serv_sock)
{
  int ret;
  int cli_sock, cli_len;
  struct sockaddr_in cli_addr;
  struct ssds_client *new_cli;

  ret = 0;
  cli_len = sizeof(cli_addr);
  cli_sock = accept(serv_sock,
		    (struct sockaddr *) &cli_addr,
		    (socklen_t *)&cli_len);

  new_cli = new_client(cli_sock);
  strncpy(new_cli->ipaddr,inet_ntoa(cli_addr.sin_addr), 32);
  new_cli->port = ntohs(cli_addr.sin_port);
  printf(" ssds_server.c: Server accepted connection from %s:%d cli id %d\n"
	 ,inet_ntoa(cli_addr.sin_addr),
	 ntohs(cli_addr.sin_port), new_cli->ix);

  if (!new_cli) {
    fprintf(stderr, " Unable to get client, exiting\n");
    ret =  -1;
  }
  new_cli->cli_addr = cli_addr;
  return new_cli->idx;
}

int ssds_cli_action(struct ssds_client *cli)
{
  int ret;
  char *sp;
  char snum[8];
  if(0)
    printf("running ssds_action rx (%s)\n",cli->rxbuf);
  ret = ssds_action(cli->fd, cli->rxbuf, cli->txbuf, cli);

  /* if we are positive then write the length */
  if (ret > 0) {
    sprintf(snum,"%04d", ret);
    sp = snum;
    cli->txbuf[4] = sp[0];
    cli->txbuf[5] = sp[1];
    cli->txbuf[6] = sp[2];
    cli->txbuf[7] = sp[3];
  }
  if(ret<0) {
    ret = -ret;
  }
  if(0)
    printf("done ssds_action ret = %d tx (%s)\n", ret, cli->txbuf);

  return ret;
}

int do_cli_debug=0;

int do_cli(struct ssds_client *cli)
{
  int cnt;
  int ret;
  int len=0;
  char snum[8];

  ret  = read(cli->fd, cli->rxbuf, 8);
  if (ret <= 0) {
    goto is_closed;
  }
  cnt = ret;

  cli->is_auto = 0;
  if(cnt == 8) {
    if(
       (cli->rxbuf[0] == 'm') &&
       (cli->rxbuf[1] == 's') &&
       (cli->rxbuf[2] == 'g') &&
       (cli->rxbuf[3] == ' ')) {
      cli->is_auto = 1;
      //printf(" Go auto\n");
    }
  }
  if(cli->is_auto) {
    memcpy((void *)&snum[0], (void *)&cli->rxbuf[4], 4);
    snum[4]=0;
    len = atoi(snum);


    ret = read(cli->fd, &cli->rxbuf[8], len-8);
    if (ret <= 0) {
      goto is_closed;
    }
    if(do_cli_debug)
      printf("do_cli: read cnt %d ret %d len %d (%s) \n",cnt,ret,len,
	     cli->rxbuf);
    cnt = ret;

    //cli->rxbuf[4]='a';
    //cli->rxbuf[5]='b';
    //cli->rxbuf[6]='c';
    //cli->rxbuf[7]='d';

  }

  if(!cli->is_auto) {
    ret = read(cli->fd, &cli->rxbuf[cnt], 512-cnt);
    if (ret <= 0) {
      goto is_closed;
    }
    cnt += ret;

    cli->rxbuf[cnt]=0;
    ret = ssds_cli_action(cli);
    if(ret>0) {
      cli->txbuf_len = ret;
      cli->txbuf[cli->txbuf_len]=0;
      ret = write(cli->fd, cli->txbuf, cli->txbuf_len);
      if (ret <= 0) {
	goto is_closed;
      }
      if(do_cli_debug)
	printf(" Server sends len %d to client\n", cli->txbuf_len);
    }
  }

  if(cli->is_auto) {

    cli->rxbuf_len = len + 8;
    cli->rxbuf[cli->rxbuf_len]=0;

    if(0)
      printf(" Server reads len %d from client data (%s)\n"
	     , len + 8, cli->rxbuf);
    ret = ssds_cli_action(cli);
    if(0)
      printf(" Server ssds ret len %d\n",ret);

   if(ret>0) {
      cli->txbuf_len = ret;
      cli->txbuf[cli->txbuf_len]=0;
      ret = write(cli->fd, cli->txbuf, cli->txbuf_len);
      if (ret <= 0) {
	goto is_closed;
      }
      if(0)
	printf(" Server sends len %d to client\n", cli->txbuf_len);
    }
  }
  return 0;

 is_closed:
  printf(" sdds_server.c:Remote Client closed connection id %d on %s:%d\n",
	 cli->ix, cli->ipaddr, cli->port);
  close(cli->fd);
  cli->fd = -1;
  cli->is_auto = 0;
  return 0;
}

int main(int argc , char * argv[])
{
  int serv_sock,serv_len;
  struct sockaddr_in serv_addr;
  struct pollfd fds[NUM_CLIENTS+2];
  struct ssds_client *cli;
  int done;

  int result;
  int num;
  int max;
  int pret;
  int timeout;
  int to_count;
  int i;
  int true;
  char *ipaddr;
  char *iport;


  ipaddr = getenv("SSDS_SRV_IPADDR");
  if(ipaddr) node_addr = ipaddr;
  iport = getenv("SSDS_PORT");
  if(iport) port_num = atoi(iport);

  to_count = 0;
  i = 1;
  true = 1;

  while (i < argc) {
    if(strcmp(argv[i],"-p") == 0) {
      if ((i+1) < argc) {
	port_num = atoi(argv[i+1]);
	i++;
	printf(" setting port number to %d\n", port_num);
      }
    }

    if(strcmp(argv[i],"-f") == 0) {
      if ((i+1) < argc) {
	ssds_file_name = argv[i+1];
	i++;
	printf(" setting config file to %s\n", ssds_file_name);
      }
    }
    if(strcmp(argv[i],"-d") == 0) {
      debug++;
      printf(" setting debug to %d\n", debug);
    }

    if(strcmp(argv[i],"-h") == 0) {
      printf(" use %s ", argv[0]);
      printf("version %s conf_file %s port %d\n"
	     , version, ssds_file_name, port_num);
      printf("         -d increase debug\n");
      printf("         -h print help\n");
      printf("         -f <file_name>set conf file name\n");
      printf("         -p <port_num> set port num\n");
      return 0;
    }

    i++;
  }

  serv_sock = socket(AF_INET,SOCK_STREAM,0);

  if((setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, (void *)&true,
		 sizeof(true))) == -1) {
    perror("inet_aton error");
    exit(0);
  }

  serv_addr.sin_family = AF_INET;

  result = inet_aton(node_addr, &serv_addr.sin_addr);

  if (result == 0) {
    perror("inet_aton error");
    exit(0);
  }

  serv_addr.sin_port = htons(port_num);
  serv_len = sizeof(serv_addr);
  bind(serv_sock, (struct sockaddr *)&serv_addr, serv_len);

  listen(serv_sock, 1);
  printf(" Server %s awaiting client on %s:%d\n", argv[0]
	 , node_addr, port_num);

  init_sys();
  done = 0;
  while(!done) {
    num = 0;
    fds[num].fd = serv_sock;
    fds[num].events = POLLIN | POLLHUP;
    num++;
    max = NUM_CLIENTS+2;
    num = poll_add_clients(fds, num, max);

    timeout = 2000;
    pret = poll(fds, num, timeout);

    if (pret < 0) {
      perror("poll error");
      done = 1;
    }

    if (pret == 0) {
      if(debug)fprintf(stdout, "poll timeout %d\n", to_count++);
    }
    if(fds[0].revents & fds[0].events) {
      do_serv(fds[0].fd);
      pret--;
    }

    if (pret > 0) {
      for (i = 1; i < num; i++) {
	if(fds[i].revents & fds[i].events) {
	  cli = find_cli_fd(fds[i].fd);
	  do_cli(cli);
	  pret--;
	}
      }
    }
  }
  return 0;
}
