#ifndef _TTA_TCP_PRIVATE_H_
#define _TTA_TCP_PRIVATE_H_

#define _TTA_TCP_PREHEADER_LENGTH		4
#define _TTA_TCP_HEADER_LENGTH			8
#define _TTA_TCP_CHECKSUM_LENGTH		0

/* In both structures, the transaction ID must be placed on first position
   to have a quick access not dependant of the TCP backend */
typedef struct _tta_tcp {
	/* Extract from TTA Messaging on TCP/IP Implementation Guide V1.0b
	   (page 23/46):
	   The transaction identifier is used to associate the future response
	   with the request. This identifier is unique on each TCP connection. */
	uint16_t tid;
	/* TCP port */
	int port;
	/* IP address */
	char ip[16];
} tta_tcp_t;

#endif /* _TTA_TCP_PRIVATE_H_ */
