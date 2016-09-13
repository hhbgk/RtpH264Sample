#include "rtp_queue.h"

void init_queue(queue_t *que, int size) {
	pthread_mutex_init(&que->locker, NULL);
	pthread_cond_init(&que->cond, NULL);
	que->buf = (uint8_t*)malloc(sizeof(uint8_t)*size);
	que->read_ptr = que->write_ptr = 0;
	que->bufsize = size;
	logi("%s:que=%p, bufsize=%d\n", __func__, que, size);
}

void free_queue(queue_t* que) {
	logi("%s", __func__);
	pthread_mutex_destroy(&que->locker);
	pthread_cond_destroy(&que->cond);
	free(que->buf);
}

void put_queue(queue_t*que, uint8_t* buf, int size) {
//	av_log(NULL, AV_LOG_INFO, "%s", __func__);
	uint8_t* dst = que->buf + que->write_ptr;

	pthread_mutex_lock(&que->locker);
	if ((que->write_ptr + size) > que->bufsize) {

		memcpy(dst, buf, (que->bufsize - que->write_ptr));

		memcpy(que->buf, buf+(que->bufsize - que->write_ptr), size-(que->bufsize - que->write_ptr));

	} else {
		memcpy(dst, buf, size*sizeof(uint8_t));
	}
	que->write_ptr = (que->write_ptr + size) % que->bufsize;

	pthread_cond_signal(&que->cond);
	pthread_mutex_unlock(&que->locker);
//	logw("%s:que=%p, write_ptr=%d, read_ptr=%d, size=%d", __func__, que, que->write_ptr, que->read_ptr, size);
}

int get_queue(queue_t*que, uint8_t* buf, int size) {
	uint8_t* src = que->buf + que->read_ptr;
	int wrap = 0;

	pthread_mutex_lock(&que->locker);

	int pos = que->write_ptr;

	if (pos < que->read_ptr) {
		pos += que->bufsize;
		wrap = 1;
	}

//	loge("%s:que=%p, write_ptr=%d, read_ptr=%d, size=%d", __func__, que, pos, que->read_ptr, size);
	if ( (que->read_ptr + size) > pos) {
		pthread_mutex_unlock(&que->locker);
//		return 1;
		struct timespec timeout;
		timeout.tv_sec=time(0)+1;
		timeout.tv_nsec=0;
		pthread_cond_timedwait(&que->cond, &que->locker, &timeout);
		if ( (que->read_ptr + size) > pos ) {
			pthread_mutex_unlock(&que->locker);
			return 1;
		}
	}

	if (wrap) {
		memcpy(buf, src, (que->bufsize - que->read_ptr));
		memcpy(buf+(que->bufsize - que->read_ptr), src+(que->bufsize - que->read_ptr), size-(que->bufsize - que->read_ptr));
	} else {
		memcpy(buf, src, sizeof(uint8_t)*size);
	}
	que->read_ptr = (que->read_ptr + size) % que->bufsize;
	pthread_mutex_unlock(&que->locker);
	return 0;
}

