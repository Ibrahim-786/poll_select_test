========================
Poll for send timestamps
========================

:Date: 2017-11-17


What is it?
===========

This is a test for ``poll()`` and ``select()`` created for
analyzing the behavior of ``SO_SELECT_ERR_QUEUE`` when
polling for transmit timestamps on a socket's error queue.

This program should be used with ``strace`` or another
tracing tool to visualize the behavior of the system calls.

  When the sender thread is enabled with ``--multi-thread``
  option, ``-f`` option should be passed to strace.

In normal behavior, the main thread send UDP packets to
localhost and poll in the same (unbound) socket. This has
no effect. Thus, options can be specified such as:

- Binding the socket (``--bind-socket``).
- Enabling transmit timestamps (``--tx-timestamp``).
- Requesting read-is-possible event (``--pollin``).

See all options with ``$ main --help``. The order of the
options does not matter.


Compiling
=========

``$ make``


Behavior
========

Tests done on Linux 4.13.12 (glibc 2.24).


With ``USE_POLLIN``
-------------------

The thread wakes up with ``POLLERR`` after every packet
sent, regardless of ``SEND_IN_SAME_THREAD``.


With ``USE_POLLPRI``
--------------------

The thread wakes up normally with ``POLLPRI|POLLERR``,
regardless of ``SEND_IN_SAME_THREAD``.


With ``POLLERR`` (default)
--------------------------

When ``SEND_IN_SAME_THREAD`` the thread wakes up normally
with ``POLLERR``. Otherwise ``loop()`` doesn't wake up when
a packet is sent from ``main()`` thread.

Note: ``POLLERR`` is always set by the kernel regardless
of the requested events.


Notes
=====


About ``SO_SELECT_ERR_QUEUE``
-----------------------------

| Linux kernel
| commit: ``7d4c04fc170087119727119074e72445f2bb192b``

::

	net: add option to enable error queue packets waking select
	
	Currently, when a socket receives something on the error
	queue it only wakes up the socket on select if it is in
	the "read" list, that is the socket has something to read.
	It is useful also to wake the socket if it is in the error
	list, which would enable software to wait on error queue
	packets without waking up for regular data on the socket.
	The main use case is for receiving timestamped transmit
	packets which return the timestamp to the socket via the
	error queue. This enables an application to select on the
	socket for the error queue only instead of for the regular
	traffic.

----------------------------------------

| linuxptp mailing list
| [Linuxptp-devel] [PATCH] sk: Modify poll to poll on ERRQUEUE

::

	On Fri, Oct 31, 2014 at 02:05:14PM -0500, Joe Schaack wrote:
	
	Previously the poll in `sk_receive` would "timeout" and when
	it did so would check the `ERRQUEUE` for data and set
	`POLLERR`.  This meant that if `sk_tx_timeout` was set to
	100 each poll would wait for 100ms rather than exiting
	immediately when `ERRQUEUE` data was available.
	
	Implement the `SO_SELECT_ERR_QUEUE` socket option that
	enables `ERRQUEUE` messages to be polled for under the
	`POLLPRI` flag, greatly increasing the number of packets per
	second that can be sent from linuxptp.

The rest of the message is a patch.


The ``POLLIN`` issue
--------------------

In normal behavior (when sending from a different thread)
polling with ``events = POLLIN`` makes the thread wake up with
``POLLERR``. However, when ``events = 0`` or ``events = POLLERR``
the thread simply doesn't wake up.

Note (24/09/2018):
This was actually a BUG solved by the commit 6e5d58fdc9bedd0255a8
("skbuff: Fix not waking applications when errors are enqueued").
