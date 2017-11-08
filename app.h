/* Copyright (C) 2017 David O'Riva.  MIT License.
 *************************************************/
#ifndef APP_H_included
#define APP_H_included

struct AppInfo;
#define APP_NAME_LEN 			32

#include "channel.h"

struct SMXChannel;
typedef struct SMXChannel *smxchannel;
typedef int (*SMXChannelHandler)( smxchannel c );

typedef struct SMXChannel {
	struct AppInfo * 			app;
	unsigned int				multiplex 		: 1;
	unsigned int				closing 		: 1;
	unsigned int				active 			: 1;
	unsigned int				need_bytes 		: 1;
	unsigned int				bin_mode		: 1;
	unsigned int				text_mode		: 1;
	unsigned int				fragment 		: 1;
	unsigned int				fragment_last	: 1;
	unsigned int				fragment_abort	: 1;
	unsigned int				is_error 		: 1;
	unsigned int				message_ready	: 1;
	unsigned int				is_reply		: 1;
	unsigned int				want_reply		: 1;

	uint32_t 					message_id;
	uint32_t 					fragment_id;
	uint32_t 					error;
	int32_t 					src_id;
	int32_t 					dst_id;

	int32_t 					expect_length;
	struct Message * 			message;
	//struct Message * 			message;
	char * 						buffer;
	int 						buffer_max;
	int 						buffer_pos;

	SMXChannelHandler			handler;

	// packet parsing
	smxparser 					parser;
} SMXChannel;

/* 
Miscellaneous parser support
*/
typedef struct SMXParser
{
	int 				type;
	const uint8_t *		start;
	const uint8_t *		base;
	const uint8_t *		scan;
	const uint8_t *		end;
	const uint8_t *		limit;

	void *				reference;
	int32_t 			error;
	int32_t 			used;
	int32_t 			state;
} SMXParser;

typedef struct SMXParser *		smxparser;

typedef struct SMXBuffer
{
	uint32_t 			refcount;
	
	struct SMXBuffer *	prev;
	struct SMXBuffer *	next;
	struct SMXBuffer *	head;
	struct SMXBuffer *	tail;

	const void *		allocation;
	const uint8_t *		begin;
	const uint8_t *		cursor;
	const uint8_t *		end;
	const uint8_t *		limit;
} SMXBuffer;

typedef struct SMXBuffer *		smxbuffer;



typedef struct s_message {
	void * 							raw;
	int32_t 						length;
	struct AppInfo * 				app;
	struct s_message_fragments * 	fragments;
	unsigned int 					please_respond 	: 1;
	unsigned int 					answer 			: 1;
	unsigned int 					error 			: 1;
	unsigned int 					text 			: 1;
	struct Channel * 				source;
} Message;

typedef struct MessageBinHeader {
	uint8_t 				flags;
	uint8_t 				pad_;
	uint16_t 				src;
	uint16_t 				dest;
	uint32_t 				msgid;
	uint32_t 				length;
} MessageBinHeader;



typedef struct AppInfo {
	int32_t 				refcount;
	int 					id;
	struct Channel *		channels;
	char 					name[ APP_NAME_LEN ];
	struct Message * 		pending;

} AppInfo;


#endif /* APP_H_included */
