#! /usr/bin/env python
import pygtk; pygtk.require("2.0")

import matecomponent, sys

TEST_MESSAGE="test-message"
CLOSURE_MESSAGE="closure-message"

def message_quit_cb(app_client, message, args):
    matecomponent.main_quit()

def message_cb(app_client, message, args):
    print "message_cb: %s(%s)" % (message, ",".join(map(str, args)))
    return 2*args[0]

def new_instance_cb(app, argc, argv):
    print "new-instance received, argv:", argv
    return argc

def closure_message_cb(app, arg_1, arg_2, user_data):
    print "closure_message_cb: ", app, arg_1, arg_2, user_data
    return arg_1 * arg_2



def main(argv):
    msg_arg = 3.141592654

    matecomponent.activate()

    app = matecomponent.Application("Libmatecomponent-Test-Uniqapp")

    app.register_message(CLOSURE_MESSAGE,
			 "This is a test message",
			 float, (int, float),
			 closure_message_cb, "user-data")

    client = app.register_unique(app.create_serverinfo(("LANG",)))
    if client is not None:
	print "I am an application client."
	app.unref(); del app
	
	msgdescs = client.msg_list()
	for msgdesc in msgdescs:
	    print "Application supports message:", msgdesc

	print "Sending message string '%s' with argument %f" % (
	    TEST_MESSAGE, msg_arg)
	retval = client.msg_send(TEST_MESSAGE, (msg_arg, "this is a string"))
	print "Return value:", retval

	print "Sending message string '%s' with arguments %i and %f" % (
	    CLOSURE_MESSAGE, 10, 3.141592654)
	retval = client.msg_send(CLOSURE_MESSAGE, (10, 3.141592654))
	print "Return value: ", retval

	print "Sending new-instance, with argv"
	i = client.new_instance(argv)
	print "new-instance returned ", i

	print "Asking the server to quit"
	client.msg_send("quit", ())
	return
    else:
	print "I am an application server"
	app.connect("message::test-message", message_cb)
	app.connect("new-instance", new_instance_cb)
	app.new_instance(argv)
	app.connect("message::quit", message_quit_cb)

	matecomponent.main()
	app.unref()

main(sys.argv)

