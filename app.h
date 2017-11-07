/* Copyright (C) 2017 David O'Riva.  MIT License.
 *************************************************/
#ifndef APP_H_included
#define APP_H_included

struct AppInfo;
#define APP_NAME_LEN 			32

#include "channel.h"

typedef struct s_channel {
	struct AppInfo * 			app;
	unsigned int				multiplex 		: 1;
	unsigned int				closing 		: 1;
	unsigned int				active 			: 1;
	unsigned int				need_bytes 		: 1;
	unsigned int				need_header		: 1;
	unsigned int				bin_message		: 1;
	unsigned int				text_message	: 1;
	
	int32_t 					expect_length;
	struct Message * 			message;
	//struct Message * 			message;
	char * 						buffer;
	int 						buffer_max;
	int 						buffer_pos;
} t_channel;


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
