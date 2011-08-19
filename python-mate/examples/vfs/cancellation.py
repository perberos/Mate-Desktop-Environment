#! /usr/bin/env python
import time
import matevfs
import thread
import sys

context = matevfs.Context()

def do_something(context):
    print 'Running counter in thread %s' % thread.get_ident()
    c = 0
    while True:
        time.sleep(0.1)
        c += 1
        print c
        if context.check_cancellation():
            print 'Cancelled counter'
            break

def cancel_in_thread(context):
    print 'Calling cancel in thread %s' % thread.get_ident()
    context.cancel()

if __name__ == '__main__':
    thread.start_new_thread(do_something, (context,))
    thread.start_new_thread(cancel_in_thread, (context,))
    time.sleep(1)
    context.cancel()
    time.sleep(1)
