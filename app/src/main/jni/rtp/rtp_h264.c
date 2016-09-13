#include "rtp_h264.h"
#include "rtp_jni.h"

static int find_start_code1 (unsigned char *buffer); // find start code 0x000001
static int find_start_code2 (unsigned char *buffer); // find start code 0x00000001
static void *rtp_runnable(void *arg);
//#define RCV_FROM_NET

#ifdef RCV_FROM_NET
#include "rtp_queue.h"
queue_t queue;
#else
static FILE *fp = NULL;
#endif

static int start_code1 = 0;
static int start_code2 = 0;
static int rtp_run_flag = 0;
static rtp_hdr_t		*rtp_hdr;
static nalu_hdr_t			*nalu_hdr;
static fu_indicator_t			*fu_ind;
static fu_hdr_t			*fu_hdr;

void rtp_setup(char *s){
    char *path = NULL;
#ifndef RCV_FROM_NET
    if((path=(char *)malloc(sizeof(char) * 64))==NULL){
        loge("%s:malloc fail", __func__);
        return;
    }

    sprintf(path, "%s", s);
    loge("s=========%s, len =%d", s, strlen(s));
#endif RCV_FROM_NET
	pthread_t tid;
	if (pthread_create(&tid, NULL, rtp_runnable, (void *)path) != 0) {
		loge("Failed to start rtp thread");
	}
}

void rtp_destroy(){
	rtp_run_flag = 0;
}

//malloc memory for struct NALU_t
static NALU_t *alloc_nalu(int buffersize)
{
	NALU_t *n;

	if ((n = (NALU_t*)calloc (1, sizeof (NALU_t))) == NULL)	{
		loge("%s: calloc NALU_t fail", __func__);
		exit(0);
	}
	n->max_size = buffersize;

	if ((n->buf = (char*)calloc (buffersize, sizeof (char))) == NULL) {
		free (n);
		loge("AllocNALU: n->buf");
		exit(0);
	}

	return n;
}

static void free_nalu(NALU_t *n)
{
	if (n)
	{
		if (n->buf)
		{
			free(n->buf);
			n->buf=NULL;
		}
		free (n);
	}
}

static int get_annexb_nalu (NALU_t *nalu) {
    //logd("%s", __func__);
	int pos = 0;
	int startCodeFound, rewind;
	unsigned char *buffer;
#ifdef RCV_FROM_NET
	unsigned char *pbuff = buffer;
#endif
	if ((buffer = (unsigned char*)calloc (nalu->max_size , sizeof(char))) == NULL){
		loge("%s: Could not allocate buffer memory", __func__);
		return 0;
	}

	nalu->startcodeprefix_len = 3; // init start code prefix len as 3
#ifdef RCV_FROM_NET
	if(1==get_queue(&queue, pbuff, 3))
#else
	if (3 != fread(buffer, 1, 3, fp)) // read 3 byte from stream
#endif
	{
		free(buffer);
		loge("get 3 bytes fail");
		return 0;
	}
#ifdef RCV_FROM_NET
	start_code1 = find_start_code1 (pbuff); // check is 0x000001 or not
#else
	start_code1 = find_start_code1 (buffer);
#endif
	// is not 0x000001
	if(start_code1 != 1)
	{
		//read more one byte
#ifdef RCV_FROM_NET
		if(1==get_queue(&queue, pbuff + 3, 1))
#else
		if(1 != fread(buffer+3, 1, 1, fp))
#endif
		{
			free(buffer);
			loge("read more one byte");
			return 0;
		}

#ifdef RCV_FROM_NET
		start_code2 = find_start_code2 (pbuff); // check is 0x00000001 or not
#else
		start_code2 = find_start_code2 (buffer);
#endif
		// is not 0x00000001
		if (start_code2 != 1)
		{
			loge("It is not 0x00000001");
			free(buffer);
			return -1;
		}
		else
		{
			pos = 4;
			nalu->startcodeprefix_len = 4; // init start code prefix len as 4
		}
	}else{
		nalu->startcodeprefix_len = 3; // init start code prefix len as 3
		pos = 3;
	}
	// find next start code
	startCodeFound = 0;
	start_code1 = 0;
	start_code2 = 0;

	while (!startCodeFound)
	{
#ifndef RCV_FROM_NET
		if (feof (fp))// read arrive at file tail
		{
			nalu->len = (pos-1)-nalu->startcodeprefix_len;
			memcpy (nalu->buf, &buffer[nalu->startcodeprefix_len], nalu->len);
			nalu->forbidden_bit = nalu->buf[0] & 0x80; // 1 bit   
			nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
			nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
			free(buffer);
			return pos-1;
		}
		buffer[pos++] = fgetc (fp);// read more byte into buffer
		start_code2 = find_start_code2(&buffer[pos-4]); // check is 0x00000001 or not
#else
		get_queue(&queue, pbuff + pos, 1);
		pos ++;
		start_code2 = find_start_code2(&pbuff[pos-4]); // check is 0x00000001 or not
#endif
		if(start_code2 != 1)
			start_code1 = find_start_code1(&buffer[pos-3]); // check is 0x000001 or not
		startCodeFound = (start_code1 == 1 || start_code2 == 1);
	}

	// Here, we have found another start code (and read length of startcode bytes more than we should
	// have.  Hence, go back in the file
	rewind = (start_code2 == 1)? -4 : -3;
#ifndef RCV_FROM_NET
	if (0 != fseek (fp, rewind, SEEK_CUR))// move file pointer to prev NALU tail
	{
		free(buffer);
		printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
	}
#else
	queue.read_ptr += rewind;
#endif
	// Here the Start code, the complete NALU, and the next start code is in the buffer.
	// The size of buffer is pos, pos+rewind are the number of bytes excluding the next start code
	// (pos+rewind)-startcodeprefix_len is the size of the NALU excluding the start code

	nalu->len = (pos+rewind)-nalu->startcodeprefix_len;
	memcpy (nalu->buf, &buffer[nalu->startcodeprefix_len], nalu->len);//copy a NALU
	nalu->forbidden_bit = nalu->buf[0] & 0x80; // 1 bit
	nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
	nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
	free(buffer);

	return (pos+rewind);// return length between two start code (contain prev start code )
}

void *rtp_runnable(void *arg)
{
	loge("%s: running------------------------------", __func__);
	NALU_t*				n;
	char*				nalu_payload;
	char				sendbuf[1500];

	unsigned short		seq_num =0;
	int					bytes=0;
	int					socket1;
	
	struct sockaddr_in	server;
	int					len =sizeof(server);
	float				framerate=29;
	unsigned int		timestamp_increse=0;
	unsigned int		ts_current=0;

	pthread_detach(pthread_self());
#ifndef RCV_FROM_NET
    char *file = arg;
	logi("file path:%s", file);
	//open h264 file
    if (NULL == (fp=fopen(file, "rb")))
    {
        loge("open file error\n");
        goto error_exit;
    }
#endif
	timestamp_increse		= (unsigned int)(90000.0 / framerate);
	server.sin_family		= AF_INET;
	server.sin_port			= htons(DEST_PORT);   
	server.sin_addr.s_addr	= inet_addr(DEST_IP);
	socket1 = socket(AF_INET, SOCK_DGRAM, 0);
	connect(socket1, (struct sockaddr *)&server, len) ;
	n = alloc_nalu(8000000);//为结构体nalu_t及其成员buf分配空间。返回值为指向nalu_t存储空间的指针
	rtp_run_flag = 1;
#ifdef RCV_FROM_NET
    //RTP Payload = [NAL Header] + [NAL Payload]
	while(rtp_run_flag)
#else
	while(!feof(fp) && rtp_run_flag)
#endif
	{
		/* after call get_annexb_nalu(),file pointer will point to prev NALU tail 
		  * meanwhile the position is the next NALU start code 0x000001 header
		  */
		get_annexb_nalu(n);//每执行一次，文件的指针指向本次找到的NALU的末尾，下一个位置即为下个NALU的起始码0x000001

		// clear sendbuf
		memset(sendbuf, 0 ,1500);//清空sendbuf；此时会将上次的时间戳清空，因此需要ts_current来保存上次的时间戳值

		//rtp固定包头，为12字节,该句将sendbuf[0]的地址赋给rtp_hdr，以后对rtp_hdr的写入操作将直接写入sendbuf。
		rtp_hdr =(rtp_hdr_t*)&sendbuf[0];
		// set RTP HEADER
		rtp_hdr->version	= 2;  // RTP Version
		rtp_hdr->payload	= H264;  // payload type
		rtp_hdr->marker		= 0;   // defined by protocol
		rtp_hdr->ssrc		= htonl(12345678); //随机指定为12345678，并且在本RTP会话中全局唯一

		// NALU less than 1400 byte, send in one RTP packet 
		if(n->len > 0 && n->len<=1400)//	当一个NALU小于1400字节的时候，采用一个单RTP包发送
		{
			// set RTP Header m
			rtp_hdr->marker=1;
			// set RTP Header sequence number
			rtp_hdr->seq_no     = htons(seq_num ++);//序列号，每发送一个RTP包增1

			//set NALU HEADER
			nalu_hdr = (nalu_hdr_t*)&sendbuf[12];//将sendbuf[12]的地址赋给nalu_hdr，之后对nalu_hdr的写入就将写入sendbuf中；
			nalu_hdr->F = n->forbidden_bit;

			// Valid data on the 6,7 bit,must move 5bit to right
			nalu_hdr->NRI	= n->nal_reference_idc>>5;
			nalu_hdr->TYPE	= n->nal_unit_type;

			// get sendbuf[13] address assigned to nalu_payload
			nalu_payload=&sendbuf[13];
//			printf("ddd nalu_payload=%d, n->len-1=%d\n", nalu_payload, (n->len-1));
			// copy nalu to nalu_payload except start code
			memcpy(nalu_payload,n->buf+1, n->len-1);

			ts_current = ts_current + timestamp_increse;
			rtp_hdr->timestamp=htonl(ts_current);
			bytes=n->len + 12 ;
//			usleep(40 * 1000);
			// send rtp package
			send( socket1, sendbuf, bytes, 0 );
		}

		else if(n->len>1400)//打分片包
		{
			//得到该nalu需要用多少长度为1400字节的RTP包来发送
			int k=0, l=0;
			k=n->len/1400; // //分片数目
			l=n->len%1400;// 最后一个包的长度
			int t=0; // 分片包片号
			ts_current=ts_current+timestamp_increse; //use the same timestamp
			rtp_hdr->timestamp=htonl(ts_current);
			while(t<=k)
			{
				rtp_hdr->seq_no = htons(seq_num ++); // sequence ++, 包号
				if(!t)// first RTP pkg set set FU HEADER S:E:R = 1:0:0, 分片打包第一个分片
				{
					// set RTP Header m
					rtp_hdr->marker=0;
					//set FU INDICATOR
					fu_ind =(fu_indicator_t*)&sendbuf[12];
					fu_ind->F=n->forbidden_bit;
					fu_ind->NRI=n->nal_reference_idc>>5;
					fu_ind->TYPE=28;

					// set FU HEADER, get sendbuf[13] address assigned to fu_hdr
					fu_hdr =(fu_hdr_t*)&sendbuf[13];
					fu_hdr->E=0;
					fu_hdr->R=0;
					fu_hdr->S=1;
					fu_hdr->TYPE=n->nal_unit_type;

					//  get sendbuf[14] address assigned to nalu_payload
					nalu_payload=&sendbuf[14];
					memcpy(nalu_payload,n->buf+1,1400);

					// rtp_header+fu_ind+fu_hdr = 14bit
					bytes=1400+14;
//					usleep(40 * 1000);
					send( socket1, sendbuf, bytes, 0 );
					t++;//分片包的第几包
				}
				//发送的是最后一个分片，注意最后一个分片的长度可能超过1400字节（当l>1386时）
				else if(k==t)// last RTP pkg set set FU HEADER S:E:R = 0:1:0
				{
					// set RTP Header m
					// the last slice set m=1
					rtp_hdr->marker=1;
					// set FU INDICATOR
					fu_ind =(fu_indicator_t*)&sendbuf[12];
					fu_ind->F=n->forbidden_bit;
					fu_ind->NRI=n->nal_reference_idc>>5;
					fu_ind->TYPE=28;

					// set FU HEADER
					fu_hdr =(fu_hdr_t*)&sendbuf[13];
					fu_hdr->R=0;
					fu_hdr->S=0;
					fu_hdr->TYPE=n->nal_unit_type;
					fu_hdr->E=1;

					// set NALU payload
					nalu_payload=&sendbuf[14];
					//将nalu最后剩余的l-1(去掉了一个字节的NALU头)字节内容写入sendbuf[14]开始的字符串
					memcpy(nalu_payload,n->buf+t*1400+1,l-1);
					//获得sendbuf的长度,为剩余nalu的长度l-1加上rtp_header，fu_indicator_t,FU_HEADER三个包头共14字节
					bytes=l-1+14;
					usleep(40 * 1000);
					send( socket1, sendbuf, bytes, 0 );
					t++;
				}
				else if(t<k && 0 != t)//分片打包不是最后一个包也不是第一个包
				{
					rtp_hdr->marker=0;
					fu_ind =(fu_indicator_t*)&sendbuf[12];//将sendbuf[12]的地址赋给fu_ind，之后对fu_ind的写入就将写入sendbuf中；
					fu_ind->F=n->forbidden_bit;
					fu_ind->NRI=n->nal_reference_idc>>5;
					fu_ind->TYPE=28;

					fu_hdr =(fu_hdr_t*)&sendbuf[13];
					//fu_hdr->E=0;
					fu_hdr->R=0;
					fu_hdr->S=0;
					fu_hdr->E=0;
					fu_hdr->TYPE=n->nal_unit_type;

					nalu_payload=&sendbuf[14];//同理将sendbuf[14]的地址赋给nalu_payload
					memcpy(nalu_payload,n->buf+t*1400+1,1400);//去掉起始前缀的nalu剩余内容写入sendbuf[14]开始的字符串
					bytes=1400+14;//获得sendbuf的长度,为nalu的长度（除去原NALU头）加上rtp_header，fu_ind，fu_hdr的固定长度14字节
//					usleep(40 * 1000);
					// set rtp package
					send( socket1, sendbuf, bytes, 0 );
					t++;
				}
			}
		}
	}

	free_nalu(n);
	close(socket1);
#ifndef RCV_FROM_NET
	fclose(fp);
	free(file);
#endif
	logw("rtp thread exiting");
error_exit:
	pthread_exit(NULL);
	return NULL;
}

static int find_start_code1 (unsigned char *buffer)
{
	if(buffer[0]!=0 || buffer[1]!=0 || buffer[2] !=1) return 0; // check buf is 0x000001, true return 1
	else return 1;
}

static int find_start_code2 (unsigned char *buffer)
{
	if(buffer[0]!=0 || buffer[1]!=0 || buffer[2] !=0 || buffer[3] !=1) return 0; // check buf is 0x00000001, true return 1
	else return 1;
}
