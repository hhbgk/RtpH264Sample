#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#pragma pack (1)

#define PACKET_BUFFER_END	(unsigned int)0x00000000
#define MAX_RTP_PKT_LENGTH	1400
#define DEST_IP				"192.168.8.174"
#define DEST_PORT			5004
#define H264				96

#define TRUE				1
#define FALSE				0

void rtp_setup(char *s);
void rtp_destroy();

typedef struct {
	int startcodeprefix_len;	// 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
	unsigned len;			// Length of the NAL unit(包含nal 头的nal 长度，从第一个00000001到下一个000000001的长度)
	unsigned max_size;		// Nal Unit Buffer size
	int forbidden_bit;		// should be always FALSE
	int nal_reference_idc;	// NALU_PRIORITY_xxxx
	int nal_unit_type;		// NALU_TYPE_xxxx
	char *buf;			// contains the first byte followed by the EBSP
	unsigned short lost_packets;  // true, if packet loss is detected
} NALU_t;

typedef struct rtp_hdr {
	/*  0                   1                   2                   3
	0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|V=2|P|X|  CC   |M|     PT      |       sequence number         |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                           timestamp                           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           synchronization source (SSRC) identifier            |
	+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
	|            contributing source (CSRC) identifiers             |
	|                             ....                              |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    */
	/* byte 0 */
	unsigned char csrc_len:4;/* expect 0 */
	unsigned char extension:1;/* expect 1, see RTP_OP below */
	unsigned char padding:1;/* expect 0 */
	unsigned char version:2;/* expect 2 */
	/* byte 1 */
	unsigned char payload:7;/* payload type */
	unsigned char marker:1;/* expect 1 */
	/* bytes 2,3 */
	unsigned short seq_no;
	/* bytes 4-7 */
	unsigned long timestamp;
	/* bytes 8-11 */
	unsigned long ssrc;/* stream number is used here. */
} rtp_hdr_t;

/*
VCL	视频编码层	进行视频编解码
NAL	网络提取层	采用适当的格式对视频数据进行封装打包
VCL数据：
即被压缩编码后的视频数据序列。在VCL数据要封装到NAL单元中之后，才可以用来传输或存储。
*/

//NALU Header结构
typedef struct NALU_HEADER{
	/*
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|F|NRI|  Type   |
	+---------------+
	*/
	unsigned char TYPE:5;//NALU类型
	unsigned char NRI:2;//重要性指示位
	unsigned char F:1;//禁止位
} nalu_hdr_t;

//FU Indicator 结构
typedef struct FU_INDICATOR{
	/*
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|F|NRI|  Type   |
	+---------------+
	*/
	unsigned char TYPE:5;//28表示FU-A，29表示FU-B
	unsigned char NRI:2;//NALU的重要性，取00-11
	unsigned char F:1;//当网络识别此单元存在比特错误时，可将其设为 1，以便接收方丢掉该单元
} fu_indicator_t;

//FU Header 结构
//由于NALU被拆分多个分片（按顺序发送），必须要有一个标示表明FUs的第一个分片和最后一个分片
typedef struct FU_HEADER{
	/*
	+---------------+
	|0|1|2|3|4|5|6|7|
	+-+-+-+-+-+-+-+-+
	|S|E|R|  Type   |
	+---------------+
	*/
	unsigned char TYPE:5;//与NALU的Header中的Type类型一致。（注意不是FU Indicator中的类型）
	unsigned char R:1;//保留位必须设置为0，接收者必须忽略该位
	unsigned char E:1;//1表示NALU结束分片，0表示非结束NALU分片
	unsigned char S:1;//1表示NALU开始分片，0表示非开始NALU分片
} fu_hdr_t;