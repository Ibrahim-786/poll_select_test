=====================================
Test for poll() and select() behavior
=====================================

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

:Date: 2019-08-01

Wake up and return:

- For waking up a task, the events triggered by kernel (wake
  up key) must contain one or more of the events requested
  by user.
- For returning events to the user (after the task is woken
  or before the task sleeps), the result events (which is
  generally the same as the triggered events) must contain
  one or more requested events. Only the matched events are
  returned.

Result events
-------------

The result event for an error condition is POLLERR.

Triggered events
----------------

The events triggered by kernel on an error condition are:

- Before commit 6e5d58fdc9bedd0255a8 ("skbuff: Fix not
  waking applications when errors are enqueued") in Linux
  4.16:

  - POLLIN and POLLPRI via sk_data_ready()
    (sock_def_readable()).

- After commit 6e5d58fdc9bedd0255a8:

  - POLLERR via sk_error_report() (sock_queue_err_skb()).

Requested events
----------------

The request events for poll(): [1]

- POLLERR | POLLHUP | <user_choice>

The request events for select(): [2][3]

- readfds:  POLLPRI | POLLERR | POLLIN  | POLLHUP
- writefds: POLLPRI | POLLERR | POLLOUT
- exeptfds: POLLPRI

See [4] for where select() checks whether the result events
were requested by user.

[1] https://git.kernel.org/pub/scm/linux/kernel/git/stable/
    linux.git/tree/fs/select.c?id=6e5d58fdc9bedd0255a8#n820

[2] https://git.kernel.org/pub/scm/linux/kernel/git/stable/
    linux.git/tree/fs/select.c?id=6e5d58fdc9bedd0255a8#n435

[3] https://git.kernel.org/pub/scm/linux/kernel/git/stable/
    linux.git/tree/fs/select.c?id=6e5d58fdc9bedd0255a8#n443

[4] https://git.kernel.org/pub/scm/linux/kernel/git/stable/
    linux.git/tree/fs/select.c?id=6e5d58fdc9bedd0255a8#n512

SO_SELECT_ERR_QUEUE
-------------------

The socket option SO_SELECT_ERR_QUEUE was introduced in
Linux 3.10 by the commit 7d4c04fc170087119727 ("net: add
option to enable error queue packets waking select"). It
simply adds POLLPRI to the result events (which already
contains POLLERR) before it is checked for return.
