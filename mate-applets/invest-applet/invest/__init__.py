import os, sys
from os.path import join, exists, isdir, isfile, dirname, abspath, expanduser
from types import ListType
import datetime

import gtk, gtk.gdk, mateconf, gobject
import cPickle

import networkmanager

# Autotools set the actual data_dir in defs.py
from defs import *

DEBUGGING = False

# central debugging and error method
def debug(msg):
	if DEBUGGING:
		print "%s: %s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"), msg)

def error(msg):
	print "%s: ERROR: %s" % (datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S.%f"), msg)


# Allow to use uninstalled invest ---------------------------------------------
UNINSTALLED_INVEST = False
def _check(path):
	return exists(path) and isdir(path) and isfile(path+"/Makefile.am")

name = join(dirname(__file__), '..')
if _check(name):
	UNINSTALLED_INVEST = True

# Sets SHARED_DATA_DIR to local copy, or the system location
# Shared data dir is most the time /usr/share/invest-applet
if UNINSTALLED_INVEST:
	SHARED_DATA_DIR = abspath(join(dirname(__file__), '..', 'data'))
	BUILDER_DATA_DIR = SHARED_DATA_DIR
	ART_DATA_DIR = join(SHARED_DATA_DIR, 'art')
else:
	SHARED_DATA_DIR = join(DATA_DIR, "mate-applets", "invest-applet")
	BUILDER_DATA_DIR = BUILDERDIR
	ART_DATA_DIR = SHARED_DATA_DIR

USER_INVEST_DIR = expanduser("~/.mate2/invest-applet")
if not exists(USER_INVEST_DIR):
	try:
		os.makedirs(USER_INVEST_DIR, 0744)
	except Exception , msg:
		error('Could not create user dir (%s): %s' % (USER_INVEST_DIR, msg))
# ------------------------------------------------------------------------------

# Set the cwd to the home directory so spawned processes behave correctly
# when presenting save/open dialogs
os.chdir(expanduser("~"))

#Gconf client
MATECONF_CLIENT = mateconf.client_get_default()

# MateConf directory for invest in window mode and shared settings
MATECONF_DIR = "/apps/invest"

# MateConf key for list of enabled handlers, when uninstalled, use a debug key to not conflict
# with development version
#MATECONF_ENABLED_HANDLERS = MATECONF_DIR + "/enabled_handlers"

# Preload mateconf directories
#MATECONF_CLIENT.add_dir(MATECONF_DIR, mateconf.CLIENT_PRELOAD_RECURSIVE)

# tests whether the given stocks are in the old labelless format
def labelless_stock_format(stocks):
	if len(stocks) == 0:
		return False

	# take the first element of the dict and check if its value is a list
	if type(stocks[stocks.keys()[0]]) is ListType:
		return True

	# there is no list, so it is already the new stock file format
	return False

# converts the given stocks from the labelless format into the one with labels
def update_to_labeled_stock_format(stocks):
	new = {}

	for k, l in stocks.items():
		d = {'label':"", 'purchases':l}
		new[k] = d

	return new

# tests whether the given stocks are in the format without exchange information
def exchangeless_stock_format(stocks):
	if len(stocks) == 0:
		return False

	# take the first element of the dict and check if its value is a list
	for symbol, data in stocks.items():
		purchases = stocks[symbol]["purchases"]
		if len(purchases) > 0:
			purchase = purchases[0]
			if not purchase.has_key("exchange"):
				return True

	return False

# converts the given stocks into format with exchange information
def update_to_exchange_stock_format(stocks):
	for symbol, data in stocks.items():
		purchases = data["purchases"]
		for purchase in purchases:
			purchase["exchange"] = 0

	return stocks

STOCKS_FILE = join(USER_INVEST_DIR, "stocks.pickle")

try:
	STOCKS = cPickle.load(file(STOCKS_FILE))

	# if the stocks file is in the stocks format without labels,
	# then we need to convert it into the new labeled format
	if labelless_stock_format(STOCKS):
		STOCKS = update_to_labeled_stock_format(STOCKS);

	# if the stocks file does not contain exchange rates, add them
	if exchangeless_stock_format(STOCKS):
		STOCKS = update_to_exchange_stock_format(STOCKS);
except Exception, msg:
	error("Could not load the stocks from %s: %s" % (STOCKS_FILE, msg) )
	STOCKS = {}

#STOCKS = {
#	"AAPL": {
#		"amount": 12,
#		"bought": 74.94,
#		"comission": 31,
#	},
#	"INTC": {
#		"amount": 30,
#		"bought": 25.85,
#		"comission": 31,
#	},
#	"GOOG": {
#		"amount": 1,
#		"bought": 441.4,
#		"comission": 31,
#	},
#}

CONFIG_FILE = join(USER_INVEST_DIR, "config.pickle")
try:
	CONFIG = cPickle.load(file(CONFIG_FILE))
except Exception, msg:
	CONFIG = {}       # default configuration


# set default proxy config
PROXY = None

# borrowed from Ross Burton
# http://burtonini.com/blog/computers/postr
# extended by exception handling and retry scheduling
def get_mate_proxy(client):
	sleep = 10	# sleep between attempts for 10 seconds
	attempts = 3	# try to get configuration from mateconf at most three times
	get_mate_proxy_retry(client, attempts, sleep)

def get_mate_proxy_retry(client, attempts, sleep):
	# decrease attempts counter
	attempts -= 1

	# sanity check if we still need to look for proxy configuration
	global PROXY
	if PROXY != None:
		return

	# try to get config from mateconfd
	try:
		if client.get_bool("/system/http_proxy/use_http_proxy"):
			host = client.get_string("/system/http_proxy/host")
			port = client.get_int("/system/http_proxy/port")
			if host is None or host == "" or port == 0:
				# mate proxy is not valid, stop here
				return

			if client.get_bool("/system/http_proxy/use_authentication"):
				user = client.get_string("/system/http_proxy/authentication_user")
				password = client.get_string("/system/http_proxy/authentication_password")
				if user and user != "":
					url = "http://%s:%s@%s:%d" % (user, password, host, port)
				else:
					url = "http://%s:%d" % (host, port)
			else:
				url = "http://%s:%d" % (host, port)

			# proxy config found, memorize
			PROXY = {'http': url}

	except Exception, msg:
		error("Failed to get proxy configuration from MateConfd:\n%s" % msg)
		# we did not succeed, schedule retry
		if attempts > 0:
			error("Retrying to contact MateConfd in %d seconds" % sleep)
			gobject.timeout_add(sleep * 1000, get_mate_proxy_retry, client, attempts, sleep)

# use mateconf to get proxy config
client = mateconf.client_get_default()
get_mate_proxy(client)


# connect to Network Manager to identify current network connectivity
nm = networkmanager.NetworkManager()
