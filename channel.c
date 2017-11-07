
#include "channel.h";


int32_t parse_int32( const void *start, const void *limit, int base, int *used )
{
	const char *scan = start;
	int neg = 0;
	int32_t val = 0;
	char c;
	do {
		c = (scan == limit) ? 0 : *scan++;
		if (c == '-') { 
			neg = 1;
			if (scan == limit) {
				break;
			}
			c = *scan++;
		}
		if (c < '0' || c > '9') {
			scan = start;
			break;
		}
		for (;;) {
			val = val * 10 + (c - '0');
			if (scan == limit) {
				break;
			}
			c = *scan++;
			if (c >= '0' && c <= '9') {
				if (val >= 214748364) {
					if (val != 214748364 || c > ( '7' + neg )) {
						scan = start;
						val = 0;
						break;
					}
				}
			} else {
				--scan;
				break;
			}
		}
	} while (0);
	if (used) {
		*used = (int)(scan - start);
	}
	return neg ? -val : val;
}

/*
=====================
MessageParseHeader

Tries to parse a message header from the given data.
Return the status:
  0 - buffer is empty
  1 - buffer contains partial header
  2 - buffer contains full header and partial data
  3 - buffer contains complete message
=====================
*/
int ChanCheckForMessage( smxchannel c )
{


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
ChanRead

Begins or continues the acquisition of a message from a channel.
If a complete message is available, finish packaging it and start distribution.

While running, this function owns all of the input-side channel fields.
=====================
*/
int ChanRead( smxchannel c )
{
	ssize_t got;
	uint8_t *bs = c->buffer_start;
	uint8_t *be = c->buffer_end;

	if (c->need_bytes) {
		if (c->buffer_max == c_buffer_end) {
			if (!ChanExpandBuffer( c )) {
				ChanAbort( c );
				return 0;
			}
		}
		got = read( c->handle, c->buffer + c->buffer_pos, 
			c->buffer_max - c->buffer_pos );
		if (got < 0) {
			if (errno == EWOULDBLOCK) {
				// TODO: schedule a notification
				return 0;
			}
			c->read_error = errno;
			ChanAbort( c );
			return 0;
		}
		if (!got) {
			// closed?
			c->closing = TRUE;
			ChanAbort( c );
			return 0;
		}
		c->buffer_end += got;
		be = c->buffer_end;
	}
	if (c->need_header) {

	}
	if (bs != be) {
		if (*bs & 0x80) {	// binary message
			if (be - bs >= sizeof( smxmessage_hdr )) {
				
			}
		}
	}


}