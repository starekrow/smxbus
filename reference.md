
Message Structure
-----------------

The simplest message type is the Direct Message:

  	1 byte - binary packet marker/flags (0x00)
  	  bit 0 - please respond
  	  	  1 - answer
  	      2 - fragment
  	      3 \  00 - middle fragment  01 - first fragment
  	      4 /  10 - last fragment    11 - abort fragment
  	      5 - 
  	      6 - error
  	      7 - set to 1 (binary packet marker)
  	1 byte - padding

    word - src-id
    word - dest-id
  	word - via-app
  	dword - message-id
  	dword - message-length, max 2^31 - 1
    bytes - message body

text header:

	<src-id> <dest-id> -[<flag>,...] [message] 

	<flags> are:
	  r - please respond
	  a - answer
	  f=[fmla] - fragment first, middle, last, abort
	  e - error
	  v=<id> - via-app
	  m=<id> - message-id
	  l=<length> - message-length (includes EOL)
	  b - message is encoded (base64)


App IDs are limited to 15 bits, allowing up to ~32,000 modules to be attached to 
any given router. 
Remember, the router is for comparitively high-level data exchange, not
for attaching every single individual function of your million line enterprise
software stack as a separate process.

Text messages may not contain control characters other than a terminating LF
(which is required), one optional CR before the LF, and TAB. Attempting to
emit a packet with other characters between 0 and 31 will immediately close 
the pipe.

Clients may freely switch between text and binary mode for sent packets. The 
mode for replies is determined by the mode of the original message. If the
replying app sends a binary message in reply to a text message, the reply is 
converted to text with base64 encoding. If the replying app sends a text 
message in reply to a binary message, a binary header is built from information
in the text packet and the message body is sent unchanged, NOT including the 
terminating LF or CRLF. List messages are sent according to the mode of the 
message subscribing to the list. App-change messages are sent according to the
mode of the message that registered for them.

In this way, apps are not required to accept both text and binary messages,
and may choose the most natural format for the language or task they are 
undertaking. Note that to send packets with unescaped control characters 
(other than TAB, VT, or FF) you *must* send them as binary packets or as
base64-encoded text packets. This restriction allows the development of 
robust components with simple semantics; a shell script that uses text-mode 
packets exclusively need not include any logic to figure out whether a line 
is a complete packet or part of some other packet. Likewise, such a script
will never be presented with a binary packet simply because it will never
send one.

The message-id field is only required when a message is fragmented or sent
over a multiplexed channel. The router will usually ensure that the message-id
field is set correctly on behalf of apps that do not use multiplexing. However,
an app replying over a multiplexed channel *must* copy the message-id from the
received message into the message-id of the reply in order for the reply to 
be correctly routed.




Fragmented Messages
-------------------

It is possible for apps to send and receive messages in fragments. This allows
for very large messages to be sent in manageable pieces. The router helps the
receiving app by ensuring that such a message is well-formed.

*NOTE: The following description is for normal duplex senders or receivers. 
Apps that use multiplexed channels have other considerations, see below.*

Fragmented messages will always be delivered to the recipient in the original 
fragments and in the correct order. All header fields in a fragmented message 
must remain the same for all fragments, except for the fragment flags. 

The sending app may send other messages between fragments on the same channel,
and the receiving app might receive other messages on its channel while the 
fragmented message is arriving. It is possible to have multiple fragmented 
messages in progress by choosing a unique message-id for each compound 
message. Once the final fragment is received or sent, the usual blocking 
rules for replies apply.

The router will maintain some state information for fragmented messages but it
is the responsibility of the sender to ensure that all of the parts of the 
message are sent in good order.

When replying with a fragmented reply message, the first reply message should 
contain the first-fragment flags and, if necessary, a unique message-id value. 
Once the first fragment is sent, the channel becomes available for other 
messages. The remaining fragments should be sent, as they become available, 
in order with the same message-id value.

To abort the sending of a fragmented message, the sender should construct a 
message with the header fields set up the same as for a fragment, but with the
abort-fragment flag set. Once this message is sent, the sender
should not send any more fragments for the message. This message will
also be forwarded to the recipient. If the reply flag is set, the recipient
should send a reply as normal, which will be returned to the sender.

The recipient of a fragmented message can abort the message before it finishes
by sending a message with the header as follows:

  * Set the recipient to the sender of the fragmented message
  * Set the sender to the recipient's app-id
  * Set the abort-fragment flag
  * Set the message-id to the message-id of the fragmented message
  * Do not set the reply flag; this message cannot be replied to

This will cause the router to stop sending fragments of the message to the
recipient. If the sender had already finished sending fragments and is waiting 
for a reply, the reply will contain the abort message from the sender. 
Otherwise, the sender should occasionally check for messages from the 
recipient and abort its own internal fragment-sending functions if an abort
message is pending.

The router may impose reasonable limits on app behavior with fragmented 
messages. For example, a fragmented message with no activity for an hour or
more may be unilaterally aborted by the router. If the router itself is feeling
resource pressure while reordering fragments for simplex or duplex recipients,
it may likewise simply abort the message. 

### Multiplexing Fragments

Multiplexing complicates the handling of fragmented messages, since it also 
allows fragments of the same message to be sent over multiple channels. In 
this scenario, it is possible (especially with poor thread management) for
some fragments - perhaps even the first fragment - to be substantially
delayed while others flow freely.

The router manages this complexity by imposing some additional requirements on
multiplexed senders and relaxing some guarantees for multiplexed receivers.

A multiplexed sender of a fragmented message is responsible for choosing a 
range of values for the message-id that will not be reused for the life of the 
message - that is, until the *recipient* has processed all of the fragments 
*and* issued a reply if one is desired. The first fragment must be given a 
non-zero message-id, and each following fragment must increase the message-id 
value by 1. This value will be used by the router and perhaps the recipient to 
ensure that no fragments are missed and all fragments are processed and in the 
correct order.

*Note: If the message-id "wraps" from 0xffffffff to 0 after incrementing, the
sender is *required* to additionally increment the value so that the value 0
does not appear in the sequence. Multiplexed recipients must account for this
as well when determining whether all fragments have arrived.*

When sending a fragmented message over multiplexed channels to an app that has
a normal duplex connection, the router takes responsibility for re-ordering the
fragments and managing the message-id field for the recipient.

A recipient using one or more multiplexed channels to receive messages or 
replies is responsible for ensuring that all fragments are received and put 
into the correct order; the router generally will not help much with this.


Special Recipients
------------------

There are some special recipient IDs:
    0 - broadcast to all listeners
    1 - the router itself
    2 - loopback (message to self)
    3 - black hole
    4-15 - reserved
    16-1024 - reserved for applications

    1024-32765 - available for applications, lists and streams

    32766 (0x7ffe) - reserved
    32767 (0x7fff) - reserved

Modules are allocated IDs starting at #16. Module IDs may be re-used, so it is
necessary to watch for ID invalidation messages. Any module that has requested 
the ID of another module may receive a module ID invalidation broadcast message,
and should clear its local ID cache if one is received.

Messages to the black hole are discarded. If you request a reply from the 
black hole, it will return an empty error.

Message Replies
---------------





Router Instructions
-------------------

These are the messages you can send to the router.

### register

  register [-xo] <name>

Register your app with the router. You supply a name and some flags. These 
include:

	`msgid` - 4 bytes - 0x01 - register
    `appname` - 32 bytes - application name, e.g. 'com.apache.httpd/2.4.17'
    `flags` - 4 bytes - bits:
      * 0 - exclusive. Only register if no other app of this name exists
        Once registered, disallow any other app of this same name
      * 1 - once. Only register if no other app of this name exists. Allow
        future registrations of the same app name, though.

It is possible to register for more than one name; registrations are 
cumulative.

Replies with the ID assigned to your app, or an error if registration failed.

You can also register for text-mode packets. These encode the same header 
information as binary packets. If you send a binary packet to a text-mode
application, the 'b' flag is set and the packet is encoded in base64.


### close

  close

Request that the router close the pipe. If this is the last pipe associated
with an application, all registrars are flushed.

### stream

Request a private stream connection to another application. This converts the
channel into a (more or less) direct link between the two applications. 
Depending on the OS, the router may need to do an extra copy of all data 
flowing through the link, but otherwise no processing at all is done on the
data. The only way to terminate the stream is to close the file descriptor.



There is a handshake that is performed to accomplish the conversion:

This creates a new
appID that represents the stream connection. Streams have the following
properties:

  * Communication may be one-way or bi-directional.
  * Replies are not allowed. Setting the reply bit on a message will result in 
    an error and the stream being closed.
  * Streams can be bound to a particular pipe.
  * They remain open if the pipe used to establish the connection is closed
  * Separate appID makes 




### register-name

	`msgid` - 4 bytes - 0x02 - lookup
	`name` - 32 bytes - name within the app to look up

### lookup

Discover the ID of an application, by name

	`msgid` - 4 bytes - 0x02 - lookup
	`app` - 32 bytes - application name to look up
	`name` - 32 bytes - name within the app to look up

	`msgid` - 4 bytes - 0x03 - lookup-sub
	`app` - 2 bytes - application ID
	`name` - 32 bytes - name within the app to look up

Returns app ID or 0 if not found.

### filter

Request that messages to or from you be passed through a filter. You specify
the app ID of the filtering application and an optional payload to include
with the forwarded message.

	`msgid` - 4 bytes - 0x10 - filter
	`id` - 2 bytes - app ID to filter through
	`source` - 2 bytes - app ID to filter messages from (or to)
	`flags` - 2 bytes
	  bit 0 - include wrapper
	  bit 1 - filter outgoing
	`prefix_len` - 4 bytes
	  length of prefix data
	`prefix` - N bytes
	  prefix data

Note that this is only needed for automatic filtering. Manual filtering of
outgoing messages is easy, just by constructing your message within the 
appropriate wrapper.

### route

Route a message through another application. This lets you send a message
that will be passed through another app. Your message will be passed to the
app you specify in `route_through`; its reply will be passed to the app 
specified in the message payload's `to` field.

The message will arrive at the eventual recipient marked as though it came from
your app, with the flags specified in the message payload's `to` field.

This command allows 

	`msgid` - 4 bytes - 0x11 - route
    `route_through` - 2 bytes - app ID to filter the message through
    `pad` - 10 bytes - alignment
    `msg` - N bytes - message to filter

### new-list

Create a distribution list. Messages sent to the list will be re-copied to each
recipient. It is not possible to reply to distribution list messages. Note that
you *must* be able to receive out-of-band messages to get messages from a list.

	`msgid` - 4 bytes - 0x20 - new-list
	`name` - 32 bytes - name of list

Returns an appid representing the list. Sending messages to that app will 
redistribute them to the list. 

### end-list

Closes a distribution list. Messages sent to the list will be re-copied to each
recipient. It is not possible to reply to distribution list messages. Note that
you *must* be able to receive out-of-band messages to get messages from a list.

	`msgid` - 4 bytes - 0x20 - new-list
	`name` - 32 bytes - name of list

### find-list

	`msgid` - 4 bytes - 0x22 - find-list
	`app` - 2 bytes - app ID
	`name` - 32 bytes - name of list

Returns an appid representing the list. Sending messages to that app will 
redistribute them to the list. 

### join

Join a distribution list. Messages sent to the list will be re-copied to each
recipient. It is not possible to reply to distribution list messages. Note that
you *must* be able to receive out-of-band messages to get messages from a list.

	`msgid` - 4 bytes - 0x21 - new-list
	`id` - 2 bytes - id of app
	`name` - 2 bytes - name of list

Replies with 

	`handle` - 4 bytes - a handle you can use to manage your subscription

### depart

Departs a distribution list.

	`handle` - 4 bytes - the subscription handle you were given.

### check

Check for pending messages.

	`timeout` - 4 bytes - milliseconds to wait for new messages

Set timeout to 0 if you wish an immediate answer.

If the reply is from the router itself and the message length is 0, there were
no messages to pick up. Otherwise, the reply is a complete broadcast or unicast
message.

### listen

Designate this channel for receiving messages. When a new message arrives for 
the app, if this channel is available the message will be written to it.
If a message sent to a listening channel requires a 
reply, that reply should be sent on the listening channel; a message that needs
a reply *will* block all other messages to that channel until the reply is 
sent.

You may open and mark multiple pipes as listeners. This allows the router to
select a listening channel that is not blocking on message receipt or waiting 
for a reply. You can also
use this to separate your broadcast and query channels.

	`command` - 1 byte - 0x09 (listen)
	`filter` - 1 byte - 
		bit 0 - block broadcast
		bit 1 - block unicast
		bit 2 - block queries

This message does not require a reply.

### multiplex

Designate this channel for multiplexed messages. This affects both sent and 
received messages.

The channel will be placed in multiplex mode immediately. No reply will be 
generated; if you 
If you request a 
reply to this message

Note that listening and multiplexing are separate and combinable modes; putting
your channel in multiplex mode does not automatically start delivery of 
non-query messages. You can still issue "check" messages on a multiplex 
channel, but they must be marked no-reply. For each "check" message sent,
if there are any pending broadcast messages for the app, one will be chosen and 
fed into this channel's input queue. The `timeout` parameter should be set to
0.

You cannot receive broadcast messages as a reply to a "check" messag on a 
multiplexed channel. Sending a "check" message with the reply bit set will
always result in an empty reply, and the pending broadcast queue will not
be checked.

Sending messages:

Prefix every message sent with a unique 32-bit ID that is non-zero. It need 
only be unique to your application. You may re-use any ID once you have 
received the reply for it. 

Receiving messages:

You may receive replies to your sent messages or new broadcast or direct
messages on the channel. Each message received will have a 32-bit prefix.
If the prefix is zero, the message is a new message to you.
If the prefix is non-zero, the message is a reply to a message that your
application previously sent with that same prefix.


### check-priv

Checks whether a given app has a particular privilege with regard to you.
This is fast.

You may cache the 

	`msgid` - 4 bytes - 0x30 (check-priv)
	`appid` - 2 bytes - app to check
	`priv` - 2 bytes - bits determine which rights to check
	  0 - direct message
	  1 - hear broadcasts
	  2 - join lists
	  8 - boolean policy
	  9 - int32 policy
	  10 - double policy
	  11 - string policy
	  12 - custom policy
	`policy` - 64 bytes - check a policy by name
	`value` - 4 bytes - integer value or string length
	`value2` - N bytes - remainder of value as needed



### app-flush

Sent when an application that you have had previous contact with changes.

	  bit 0 - app disconnected
	      1 - app privileges changed




Privilege Management
--------------------

The privilege manager is an app that can answer questions about what's allowed
and what isn't. This is how other apps on the router can tell the difference
between the PHP service you installed and the "PHP" service that 
$random_bad_dude started running after getting guest on your server. 

Rather than integrate directly with the OS to read user/group IDs or process
information from the opener of a pipe - this is somewhat brittle - the router 
is designed to operate  multiple pipes, with OS privileges configured to 
restrict access to just the intended users. The initial identity and privileges
come from the exact pipe that was opened. There are two pipes that are always
created: "system/root" and "system/public". "system/root" can only be accessed
by an administrative user, and "system/public" is accessible by any user on
the system.

You can create a new pipe for each intrinsic identity you want to track.
Handy pipe names might be "server/httpd" or "server/php". You may need to 
create a new OS user for some pipes to properly ensure that only the intended
application can connect to the pipe.

Once you have the intrinsic identities set up correctly and the router 
configured, the only thing needed to start things off is to start the router.
The router itself can start processes that need to operate according to its
configuration.

There are two layers of privilege management, "static" and "runtime". "Static"
privileges are configured within the router itself. Runtime privileges,
as the name implies, can be set up by each app, either when it connects or 
at any other time. There is no functional difference between static and runtime
privileges, except that static privileges are automatically loaded and
applied.

Privileges are assigned in a "mandatory access control" scheme. This means that
by default, apps are not allowed to communicate at all with other apps, only
with the router itself. You set up rules that allow apps to talk to each other,
and you can set up rules that apps can use to decide what to allow other apps
to do. Consider the following policy group:

	=== groups ===

	servercore:
			- /server/logger
			- /server/healthmon
			- /group/sysadmins
			- /user/starekrow

	admins:
			- /group/sysadmins
			- /user/starekrow
	
	=== rules ===

	/server/php:
		allow-contact: 
			- /server/httpd
			- servercore
		policy-update-config:
			allow: admins

	/server/memcached:
		allow-contact: 
			- /server/php
			- servercore

	/server/mariadb:
		allow-contact: 
			- /server/httpd
			- /server/php
			- servercore

	/service/sockets:
		allow-contact:
			- all

		policy-listen:
			- allow: /user/root
			  custom:
			  	port-min: 1
			  	port-max: 1023
			  	address: *
			- allow: /server/php
			  custom:
			  	: 556

		policy-open:
			- allow: /server/php
			  values:
			  	- 80
			  	- 443


Configuration
-------------

The router can be configured interactively; this is the preferred method for
configuration, since a history of changes is kept for reference or rollback.
Apps are encouraged to adopt this approach as well.

Message Filters
---------------


