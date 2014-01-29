#
# makefile for ssds
# Phil Wilshire
#
CFLAGS += -g -Wall

all: ssds_test ssds_server ssds_client ssds_cgi ssds_user_test


ssds_test: ssds.o ssds_test.o
	$(CC) $(LDFLAGS) -o $@ ssds.o ssds_test.o

ssds_user_test: ssds_user.o ssds_user_test.o
	$(CC) $(LDFLAGS) -o $@ ssds_user.o ssds_user_test.o

ssds_server: ssds.o ssds_server.o
	$(CC) $(LDFLAGS) -o $@ ssds.o ssds_server.o

ssds_client: ssds_client_test.o
	$(CC) $(LDFLAGS) -o $@ $<

ssds_cgi: ssds_client.o ssds.o ssds_cgi.o
	$(CC) $(LDFLAGS) -o $@ ssds_client.o ssds.o ssds_cgi.o

ssds_client.o: ssds_client.c
	$(CC)  $(CFLAGS) -c $<

ssds_client_test.o: ssds_client.c
	$(CC) $(CFLAGS) -DCLIENT_TEST -c $< -o $@

ssds_cgi.o:ssds_cgi.c ssds.h
	$(CC) $(CFLAGS) -c $<

ssds.o:ssds.c ssds.h
	$(CC) $(CFLAGS) -c $<

ssds_user.o:ssds_user.c ssds_user.h
	$(CC) $(CFLAGS) -c $<

ssds_user_test.o:ssds_user_test.c ssds_user.h
	$(CC) $(CFLAGS) -c $<

ssds_test.o:ssds_test.c ssds.h
	$(CC) $(CFLAGS) -c $<

ssds_server.o:ssds_server.c ssds.h
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f *.o *.gdb ssds_test ssds_server ssds_client ssds_cgi

romfs:
	$(ROMFSINST) /bin/ssds_server
	$(ROMFSINST) ssds.conf.asys /etc/ssds.conf
	$(ROMFSINST) -d ssds_cgi /home/html/cgi-bin/ssds_cgi.cgi
	$(ROMFSINST) /bin/ssds_client_test
	$(ROMFSINST) /bin/ssds_user_test
	$(ROMFSINST) /bin/ssds_test



