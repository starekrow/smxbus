
Modular Servers
===============

Why, oh why, is it such a pain in the ass to integrate things? I was looking at
Discourse as a possibility for handling comments on here, and the very first 
thing I noticed is that it's written in Ruby. 

Well, s***.

My primary preferred stack is LAMP. Python is a regular co-inhabitant, and I'll 
add Node in if there's a compelling reason. But I really don't like a lot of 
software on my servers. Ruby adds another entire stack of crap that has to live 
on there if I want to use it. Another administrative PITA, another 
maybe-has-to-be-different install of MariaDB, another set of credentials to 
manage, another thing that needs security updates, etc etc blah blah blah.

What I *really* want is for each component to be largely self-managing. Even if
it's done through a horrible pile of shell scripts, I want to be able to say 
something like "Install PHP 7.1 with regular security updates" and just forget
about it. Someday when I want to switch to PHP 7.3 maybe I'll log in and do 
something intentional to the server again.

Same thing on the software component side. Good package management can go a 
long way toward this - Composer isn't actually bad, but I think that it's not
the right answer for the high-level sticking together of functions for a 
server. Docker is probably closer, but perhaps heavier-weight than necessary.
And Docker helps mostly with replicating a deployment, not managing one. 

These vaguely connected thoughts fit in well with a line of thinking that I've
been chasing for years, since building my first big PHP-based API: How, exactly,
can server functions be modularized without multiplying your administrative 
load by the number of components you want to use? 

I think I have some parts, at least, of the answer:

  * Distribute high-level functions (duh)
  * Concentrate low-level functions
  * Build and use an internal request router
  * Strongly isolate performance-critical tasks

Request Routing
---------------

The real key here is the request routing. Every major function you invoke 
should go through a router. This gives you an oppportunity to log, profile,
intercept and authenticate requests. Even more importantly, it frees you to
use the best tool for each job. Want to mix Python, PHP, Go and Node code to 
respond to a single HTTP request? You can do that with good routing. Need to
use two different versions of Python? No problem. Want to mix in some C++?
It just works.

Modern kernels are really, really good at passing messages from one running
process to another. And there are lots of (well, at least two) good libraries
available with multiple language bindings to wrap those messages up and manage
their flow. All that's left for you to do is write a little bit of shim code
to bind the pieces you want to use to some sensical calling convention.

For example, say you want to write a little blogging platform. First thing you
do is break down what you're likely to need:

  * Web server (it's a web log, right?)
  * Some decent database, SQL or NoSQL is fine. Unless you want to store 
    everything in files. That's cool, too. Stupid, but cool.
  * Secret management (every single server in the world needs this)
  * Update management (every single server in the world needs this)
  * Blog post handling:
    * Create post
    * Render post
    * Manage post
  * Web-request-to-blog-page mapper
  * Comment handling (what's a blog without comments? what's the sound of one
    hand clapping?):
    * Post comment
    * Render comment
    * Manage comment
  * Email client (for notifications to admin)

Now, one approach you can take to this is to figure out what programming 
language can do all of this and write it. That would obviously be insane.
So, the first obvious break is between the web server and everything else.
The second obvious break is between the database and everything else. The third
obvious break is - wait, what? Everything else is just part of the blog, right?

That's the kind of thinking that has us, in 2017, fighting completely 
unnecessary site breaches. But there are some reasons for it that seem really
good on the surface. The first one looks like this (warning, ASCII art ahead):

	+-------------------------------+
	| OS                            |
	|                               |
	|   Kernel                      |
	|   SELinux                     |
	|   Users                       |
	|   SSH                         |
	|                               |
	|       +------------------+    |
	|       |    web server    |    |
	|       +------------------+    |
	|              ^                |
	|              |                |
	|              v                |
	|       +------------------+    |
	|       |   application    |    |
	|       +------------------+    |
	|              ^                |
	|              |                |
	|              v                |
	|       +------------------+    |
	|       |     database     |    |
	|       +------------------+    |
	+-------------------------------+

Look familiar? That's what a typical LAMP stack running a typical 
small-to-medium web app looks like. It's common, well-understood, you can build 
it out on shared hosting or your own bare metal and it works just about the 
same.  You can use Node or Python instead of PHP and/or Apache httpd, may add 
haproxy in front of the web server for fancy caching, but the diagram still 
looks pretty similar.

Some things to notice about this that are typically just accepted as part of 
the stack:

  * The web server doesn't have users like the OS does. You can make that 
    happen, but that causes other problems later on.
  * The database doesn't have users like the OS does. You can't make that 
    happen.
  * In fact, all that OS management stuff (security, users, remote management) 
    is happening outside the careful little boxes around the web server, 
    application and database.
  * The web server and database can't talk to each other. That means that 
    *any* aspects of the web request that might depend on information from the 
    database have to be resolved by the application.
  * The OS doesn't know much of anything about the database. It's in a box.
    If you wanted anything in the OS to make decisions using information from 
    the database, you'd have to write that yourself. Probably from scratch.
  * Most attempts to improve the security of the OS end up breaking  
    the application, the web server or both of them in different ways. This is
    because they don't know much about each other and they have no common 
    language to describe what is "expected" behavior and what is "dangerous"
    behavior.
  * Most attempts to improve the security of the web server and application
    end up making them brittle and unreliable, because they have to bind to 
    OS-specific (even version-specific) features that are sure to change when
    someone comes up with a better way to secure the OS.

Every full-stack solution I've seen to those problems is expensive and comes
with a widget-certified network consultant at $200/hr.

Well, I can't afford one of those and they haven't seemed to do a substantially
better job preventing breaches than anyone else. Instead, I'm going try to 
tackle the problem with the skills I have - mostly large-scale API design and
a deep understanding of how computers and networks actually work.

A Better Architecture
---------------------

So here's one way to try to remedy this situation without rewriting everything
from the OS on up.

	+-----------------------------------------------------+
	| OS                                                  |
	|                                                     |
	|   Kernel                                            |
	|   SELinux                                           |
	|   Users                                             |
	|   SSH                                               |
	|                                    +--------+       |
	|                                    | Router |       |
	|                                    |        |       |
	|    +-------------+                 |        |       |
	|    | OS services | <-------------> |        |       |
	|    +-------------+                 |        |       |
	|                                    |        |       |
	|    +----------------+              |        |       |
	|    | rights manager | <----------> |        |       |
	|    +----------------+              |        |       |
	|                                    |        |       |
	|    +------------+     +------+     |        |       |
	|    | web server | <-> | shim | <-> |        |       |
	|    +------------+     +------+     |        |       |
	|                                    |        |       |
	|    +------------+     +------+     |        |       |
	|    | database   | <-> | shim | <-> |        |       |
	|    +------------+     +------+     |        |       |
	|                                    |        |       |
	|    +-------------+                 |        |       |
	|    | application | <-------------> |        |       |
	|    +-------------+                 |        |       |
	|                                    |        |       |
	|    +------------+     +------+     |        |       |
	|    | service1   | <-> | shim | <-> |        |       |
	|    +------------+     +------+     |        |       |
	|                                    |        |       |
	|                                    |        |       |
	|                                    +--------+       |
	|                                                     |
	+-----------------------------------------------------+

Obviously, there's one big change in organization. None of the pieces are bound
directly to each other; instead, a big-ass box named "Router" is binding to
each piece. And there are some new pieces there. Also, the OS is essentially
unchanged, and the "web server", "database" and "application" boxes are still 
there (and by implication not much changed).

But the change is actually much deeper than that. Because suddenly, the web 
server can communicate with *anything* attached to the router. Likewise, the 
application can easily respond to activity in the OS or some other module, not 
just web requests. And that "service1" box there is a stand-in for any existing
service that you want to make accessible to the other components. Caches,
proxies, other servers (DNS, email, etc) or really any piece of software that
can run more-or-less independently on your hardware can be shimmed to the
router.

Better yet, if one of those services gets too big to live on the server with 
everything else, you can just move it off and put a network connection in its
place.

At least, that's the ideal. The devil really is in the details for this kind
of architecture, and making this work as easily and quickly as a regular LAMP 
stack is  not trivial. But with a little bit of work and some clever packaging, 
we should be able to get there.


Step 1 - The Router
-------------------

The router itself is, of course, critical to the success of this project. It 
has to be fast, reliable and easy to talk to. There are also some specific
capabilities that are required:

  * Addressing - messages can be addressed to a specific recipient. 
    If the recipient isn't available, some kind of failure should be apparent.
  * Replies - It would be *really* handy to be able to bind a reply to a 
    message, or conversely to require a reply to a message.
  * Broadcast - It should be possible to send a message to everyone
    currently attached to the router.

Already, this kind of looks like TCP/IP. And that would be a possible solution,
but it's a bit heavy-handed for what we need. Often, the OS will not make
local sockets as fast or lightweight as they could be for our purposes. 
Another possibility is an existing messaging library, one that has lots 
of existing language bindings, minimal requirements and works well; zeromq is a 
good example of this. Another approach would be to build something that is
accessible from just about every programming language on the planet with clean
semantics and scalable performance: The file system. More particularly, local
pipes.

Local pipes don't get a lot of love. They're easy to abuse, hard to multiplex
and they have weird permissions... but... BUT... just about any program that
can open a file can open a local named pipe. Reading and writing are as easy
or as complex as you need them to be. And the router only needs to manage
one pipe, accessible to the entire (local) world, because all the rights
management has to be internal anyway (since the OS will likely upend its own
rights management before your webapp falls out of use).

Better yet, it's actually not that hard to do smart things with pipes these
days, especially if you're willing to break your functions up across 
processes. And since most of what the router is doing is pushing data from
one place to another through the kernel, those process boundaries aren't that
important anyway.

So let's look at what a simple router architecture might be:

  * One process
    * very restricted OS privileges, once the pipe itself is listening
  * One pipe listening at, say, /var/superouter/bus
  * Basic functions:
    * Attach pipe
    * Detach pipe
    * Register endpoint
    * Receive message
    * Send message
    * Receive reply
    * Send reply
    * Filter message

Some technical details, easy enough to achieve and perhaps soothing for the 
folks that care about such things:

  * One thread/CPU-core, edge-triggered I/O
  * Zero-copy, where possible
    
One thing to notice right off the bat: The router itself has no privileges.
It can't authenticate your message. It doesn't understand the contents. It 
can't do much of anything except make sure that your well-formed message is 
passed to the correct recipient. If you did manage to hack into the router
process, you'd actually be running with fewer permissions than most regular 
users.

