
#include "channel.h";


int32_t parse_int32( smxparser p )
{
	const char *			scan = p->scan;
	int 					neg  = 0;
	int32_t 				val  = 0;
	char 					c;

	if (scan == p->limit) {
		p->error = SMXE_NEED_MORE_DATA;
		return 0;
	}
	c = *scan++;
	if (c == '-') { 
		neg = 1;
		if (scan == p->limit) {
			p->error = SMXE_NEED_MORE_DATA;
			return 0;
		}
		c = *scan++;
	}
	if (c < '0' || c > '9') {
		p->error = SMXE_MISSING_VALUE;
		return 0;
	}
	for (;;) {
		val = val * 10 + (c - '0');
		if (scan == limit) {
			break;
		}
		c = *scan++;
		if (c < '0' || c > '9') {
			--scan;
			break;
		}
		if (val >= 214748364) {
			if (val != 214748364 || c > ( '7' + neg )) {
				p->error = SMXE_NUM_OVERFLOW
				return 0;
			}
		}
	}
	p->scan = scan;
	return neg ? -val : val;
}

/*
=====================
MessageParseHeader

Tries to parse a message header from the given data.
Return the status:
  0 - Insufficient data
  1 - Header read
 -1 - Invalid header
=====================
*/
int MessageParseHeader( const char *start, 
						const char *limit, 
						MessageBinHeader *hdr )
{

}

/*
=====================
ChanCheckForMessage

Check to see if a message or part of one is available from the channel's buffer.
Return the status:
  0 - buffer is empty
  1 - buffer contains partial header
  2 - buffer contains full header and partial data
  3 - buffer contains complete message
=====================
*/
int ChanCheckForMessage( smxchannel c )
{
	uint8_t *bs = c->buffer_start;
	uint8_t *be = c->buffer_end;

	if (bs == be) {
		return 0;
	}
	if (c->need_header) {
		if (bs == be) {
			return 0;
		}
		if (*bs & 0x80) {	// binary message
			if (be - bs < sizeof( smxmessage_hdr )) {
				return 0;
			}
			c->need_header = FALSE;
			c->bin_message = TRUE;
			c->expect_length = 
				(bs[12] | (bs[13] << 8) | (bs[14] << 16) | (bs[15] << 24));
			if (c->expect_length & 0x80000000) {
				ChanAbort( c );
				return 0;
			}
		} else {
			int step;
			c->src_id = 0;
			c->dst_id = 0;
			c->flag_str = NULL;
			if (*bs < '1' || *bs > '9') {
				ChanAbort( c );
				return 0;
			}
			c->src_id = parse_int32( bs, be, 10, &step );
			if (!step) {
				ChanAbort( c );
				return 0;				
			}
			if (bs == be) {
				return 0;
			}
			if (*bs != ' ') {
				ChanAbort( c );
				return 0;
			}
			if (*bs < '1' || *bs > '9') {
				ChanAbort( c );
				return 0;
			}
			c->src_id = parse_int32( bs, be, 10, &step );
			if (!step) {
				ChanAbort( c );
				return 0;				
			}
			while (bs < be) {
				c->src_id *= 10;
				c->
			}
			c->text_message = TRUE;
			while (bs != be) {
				c->expect_length = -1;
			}
		}
		if (*bs & 0x80) {	// binary message
			if (be - bs >= sizeof( smxmessage_hdr )) {
				
			}
		}

	}
	if (bs != be) {
	}

}

/*
=====================
parse_vint31

Parses a positive 31-bit integer from the input stream.
Returns 0 and sets p->error on error
=====================
*/
int32_t parse_vint31( smxparser p, int32_t initial )
{
	uint8_t *scan = p->scan;
	int32_t val = initial;
	uint8_t b;

	for (;;) {
		if (scan == p->limit) {
			p->error = SMXE_NEED_MORE_DATA;
			return 0;
		}
		if (val & 0xff000000) {
			p->error = SMXE_V31_OVERFLOW;
			return 0;
		}
		b = *scan++;
		val = (val << 7) | (b & 0x7f);
		if ((b & 0x80) == 0) {
			p->scan = scan;
			return val;
		}
		++scan;
	}
}

/*
=====================
parse_vuint32

Parses a positive 32-bit integer from the input stream.
Returns 0 and sets p->error on error
=====================
*/
uint32_t parse_vuint31( smxparser p, uint32_t initial )
{
	uint8_t *scan = p->scan;
	uint32_t val = initial;
	uint8_t b;

	for (;;) {
		if (scan == p->limit) {
			p->error = SMXE_NEED_MORE_DATA;
			return 0;
		}
		if (val & 0xfe000000) {
			p->error = SMXE_V32_OVERFLOW;
			return 0;
		}
		b = *scan++;
		val = (val << 7) | (b & 0x7f);
		if ((b & 0x80) == 0) {
			p->scan = scan;
			return val;
		}
		++scan;
	}
}

/*
=====================
ChanEndMessage

Cleans up any vars needed to prep for the next message
=====================
*/
void ChanEndMessage( smxchannel c )
{
	c->message_id = 0;
	c->message_ready = FALSE;
	c->fragment = FALSE;
	c->fragment_last = FALSE;
	c->fragment_abort = FALSE;
	c->fragment_id = 0;
	c->is_error = FALSE;
	c->expect_length = -1;
	c->is_reply = FALSE;
	c->want_reply = FALSE;
}


/*
=====================
ChanTextOp

Returns 1 if a message is ready to send.
Otherwise 0.
=====================
*/
int ChanTextOp( smxchannel c )
{
	smxparser 		p = c->parser;
	uint8_t *		next;
	uint8_t 		b;
	int32_t 		v31;
	size_t 			used = 0;

	for (;;) {
		if (p->error) {
			break;
		}
		next = p->scan;
		if (p->scan == p->limit) {
			break;
		}
		b = *p->scan++;
		switch (b) {
		case ' ',',','\t':
			break;

		case '\r':
			// allowed if and only if the next character is a LF
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
		    } else if (*p->scan != '\n') {
		    	p->error = SMXE_BAD_EOL;
		    } else {
			    ++p->scan;
		    }
			break;

		case '\n':
			// no packet
			break;

		case ':':
			// got a packet
			c->expect_length = -1;
			c->getting_message = TRUE;
			c->handler = ChanTextMessage;
			return 0;

		case '~':
			c->src_id = parse_int32( p );
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
			}
			break;

		case '@':
			c->src_id = parse_int32( p );
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
			}
			break;

		case '^':
			c->bin_mode = TRUE;
			c->text_mode = FALSE;
			c->handler = ChanBinOp;
			return 0;

		case '#':
			if (p->scan != p->limit && *p->scan == 'r') {
				c->is_reply = TRUE;
				++p->scan;
			}
			c->message_id = parse_int32( p );
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
		    }
			break;

		case '!':
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
				break;
			}
			b = *p->scan++;
			if (b == 'e') {
				// TODO
			} else if (b >= '0' && b <= '9') {

			}
			break;

		case '?':
			c->want_reply = TRUE;
			if (c->is_reply) {
				p->error = SMXE_REPLY_LOOP;
			}
			break;

		case '.':
			c->is_reply = TRUE;
			if (c->want_reply) {
				p->error = SMXE_REPLY_LOOP;
			}
			break;

		case '%':
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
				break;
			}
			c->fragment = TRUE;
			b = *p->scan++;
			if (b == 'e') {
				c->fragment_last = TRUE;
				c->fragment_id = parse_int32( p, 0 );
			} else if (b == 'a') {
				c->fragment_abort = TRUE;
			} else if (b == 'c') {
				c->fragment_id = parse_int32( p, 0 );
			} else if (b == 'f') {
				c->fragment_last = TRUE;
				c->fragment_auto = TRUE;
			} else if (b == 'd') {
				c->fragment_auto = TRUE;
			} else if (b >= '0' && b <= '9') {
				c->fragment_id = parse_int32( p, 0 );				
			} else {
				p->error = SMXE_UNKNOWN_PACKET;
			}
			if (p->scan == p->limit) {
				p->error = SMXE_NEED_MORE_DATA;
			}
			break;

		default:
			p->error = SMXE_UNKNOWN_PACKET;
			break;
		}
	}
	p->scan = next;
	if (p->error) {
		c->error = p->error;
		p->error = 0;
	}
	return 0;
}


/*
=====================
ChanBinOp
=====================
*/
int ChanBinOp( smxchannel c )
{
	smxparser 			p 			= c->parser;
	uint8_t *			next 		= p->scan;
	uint8_t *			limit 		= p->limit;
	uint8_t 			b;
	int32_t 			v31;

	for (;;) {
		if (p->error) {
			break;
		}
		next = p->scan;
		if (p->scan == limit) {
			break;
		}
		b = *p->scan++;
		if ((b & 0xc0) == 0x80) {
			v31 = b & 0xf;
			if (b & 0x10) {
				v31 = parse_vint31( p, v31 );
				if (p->error) {
					break;
				}
			} 
			c->expect_length = v31;
			c->want_reply = (b & 0x20) ? TRUE : FALSE;
			if (c->want_reply && c->is_reply) {
				p->error = SMXE_REPLY_LOOP;
				break;
			}
			c->getting_message = TRUE;
			c->handler = ChanBinMessage;
			return 0;
		}
		switch (b) {
		case 0xC1:
			c->src_id = parse_vint31( p, 0 );
			break;

		case 0xC2:
			c->dst_id = parse_vint31( p, 0 );
			break;

		case 0xC7:
			c->message_id = parse_vuint32( p, 0 );
			c->is_reply = TRUE;
			break;

		case 0xCC:
			c->bin_mode = FALSE;
			c->text_mode = TRUE;
			c->handler = ChanTextOp;
			return 0;

		case 0xCD:
			c->message_id = parse_vuint32( p, 0 );
			break;

		case 0xE0: case 0xE1: case 0xE2: case 0xE3:
		case 0xE4: case 0xE5: case 0xE6: case 0xE7:
		case 0xE8: case 0xE9:
		case 0xED:
		case 0xEE:
			break;

		case 0xF0: case 0xF1: case 0xF2: case 0xF3:
		case 0xF4: case 0xF5: case 0xF6: case 0xF7:
		case 0xF8: case 0xF9:
			c->fragment = TRUE;
			c->fragment_id = b & 0xf;
			break;

		case 0xFA:
			c->fragment = TRUE;
			c->fragment_abort = TRUE;
			break;

		case 0xFC:
			c->fragment = TRUE;
			c->fragment_id = parse_vuint32( p, 0 );
			break;

		case 0xFD:
			c->fragment = TRUE;
			c->fragment_auto = TRUE;
			break;

		case 0xFE:
			c->fragment = TRUE;
			c->fragment_last = TRUE;
			c->fragment_id = parse_vuint32( p, 0 );
			break;

		case 0xFF:
			c->fragment = TRUE;
			c->fragment_last = TRUE;
			c->fragment_auto = TRUE;
			break;
		default:
			p->error = SMXE_UNKNOWN_PACKET;
			break;
		}
	}
	p->scan = next;
	if (p->error) {
		c->error = p->error;
		p->error = 0;
	}
	return 0;
}

/*
=====================
ChanError

Handles an error condition on the channel
=====================
*/
int ChanError( smxchannel c )
{
	switch (c->error) {
		case SMXE_NEED_MORE_DATA:
			if (!c->getting_message) {
				// instructional packet is fragmented across sub-buffer 
				// boundaries. Merge.
				c->expect_bytes = 1;
			} else {
				c->handler = ChanRead;
			}
		default:
			// TODO
			return 1;
	}
}

/*
=====================
ChanLoadMessage
=====================
*/
int ChanLoadMessage( smxchannel c )
{
	
}

/*
=====================
ChanBinMessage
=====================
*/
int ChanBinMessage( smxchannel c )
{
	if (c->have_data < c->expect_data) {
		if (ChanRead( c )) {
			//BuffTrim
			return 1;
		}
	}
	if (c->have_data < c->expect_data) {
		// gotta wait for more
		return 1;
	}
	// ready to go!
	c->handler = ChanEmitMessage;
	return 0;
}

/*
=====================
ChanFindEOL
=====================
*/
int ChanFindEOL( smxchannel c )
{
	const uint8_t *scan = c->parser->scan;
	const uint8_t *limit = c->parser->limit;

	while (scan != limit) {
		uint8_t b = *scan++;
		if (b < ' ') {
			continue;
		} 
		if (b == '\r') {
			if (scan == limit) {
				c->parser->scan = scan - 1;
				return 0;
			} else if (*scan == '\n') {
				c->parser->scan = scan + 1;
				return 1;
				return (int)(scan - (const uint8_t *)start + 1);
			}
		} else if (b == '\n') {
			return (int)(scan - (const uint8_t *)start);
		}
		c->error = SMXE_BAD_MESSAGE;
	}
	return 0;
}

/*
=====================
ChanTextMessage
=====================
*/
int ChanTextMessage( smxchannel c )
{
	if (c->parser->scan != c->parser->limit) {
		if (ChanFindEOL( c )) {
			if (c->error) {
				return 0;
			}
			
		}
	}

	uint8_t *scan = b->cursor;
	while (scan != b->end) {
		if (*scan++ < ' ') {
			if (*scan == '\t') {
				continue;
			}
			c->error = SMXE_BAD_MESSAGE;
			return 0;
		}
	}
	b->cursor = scan;


	}
	if (c->have_data < c->expect_data) {
		if (ChanRead( c )) {
			//BuffTrim
			return 1;
		}
	}
	if (c->have_data < c->expect_data) {
		// gotta wait for more
		return 1;
	}
	// ready to go!
	c->handler = ChanEmitMessage;
	return 0;
}


/*
=====================
ChanRead

Read data from the channel. Does not switch handlers until at least 
expect_bytes bytes are available.

If expect_bytes is -1, reads until a line feed is encountered and throws errors
on control characters except (TAB, VT, FF). CR is only allowed 
=====================
*/
int ChanRead( smxchannel c )
{
	void *dest;
	size_t len;
	ssize_t got;
	if (!BuffRemaining( c->buffer )) {
		int step = max( BuffTotalSize( c->buffer ), 1024*1024 );
		if (!BuffAdd( c->buffer, step )) {
			c->error = SMXE_EXHAUSTED_RAM;
			return 0;
		}
	}
	BuffNextSegment( c->buffer, &dest, &len );
	got = read( c->os_handle, dest, len );
	if (got > 0) {
		c-.have_data += got;
		BuffMarkUsed( c->buffer, got );
		c->handler = ChanGotData;
		return 0;
	}

	// got enough data
	c->handler = ChanEmitMessage;
}

/*
=====================
ChanGotData

New data arrived; figure out what to do
=====================
*/
int ChanEmitMessage( smxchannel c )
{
	
}


/*
=====================
ChanEmitMessage

Send the message that is currently waiting on this channel.
=====================
*/
int ChanEmitMessage( smxchannel c )
{
	
}


/*
=====================
ChanRun

Begins or continues the acquisition of a message from a channel.
If a complete message is available, finish packaging it and start distribution.

While running, this function owns all of the input-side channel fields.
=====================
*/
int ChanRun( smxchannel c )
{
	for (;;) {
		if (c->error) {
			if (ChanError( c )) {
				break;
			}
		} else if (c->handler( c )) {
			break;
		}
	}
}

