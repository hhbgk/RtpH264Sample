#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "rtp_jni.h"

#ifndef IPINPUT_H_
#define IPINPUT_H_


typedef struct _queue {
	pthread_mutex_t locker;
	pthread_cond_t cond;
	uint8_t* buf;
	int bufsize;
	int write_ptr;
	int read_ptr;

} queue_t;

queue_t queue;

typedef struct _test{
	pthread_mutex_t locker;
	pthread_cond_t cond;
	uint8_t* buf;
	int a;
	int b;
	int c;
}test_t;
test_t test;

typedef struct _udp_param {
	char** argv;
	queue_t *queue;
	int size;
} UdpParam;

//void* udp_ts_recv(void* param);
//void init_queue(queue_t *que, int size);
void free_queue(queue_t* que);
void put_queue(queue_t* que, uint8_t* buf, int size);
int get_queue(queue_t* que, uint8_t* buf, int size);

#endif /* IPINPUT_H_ */
