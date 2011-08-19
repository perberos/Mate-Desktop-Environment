from os.path import join
import mateapplet, gtk, gtk.gdk, mateconf, gobject
from gettext import gettext as _
import csv
import locale
from urllib import urlopen
import datetime
from threading import Thread

import invest, invest.about, invest.chart
import currencies

CHUNK_SIZE = 512*1024 # 512 kB
AUTOREFRESH_TIMEOUT = 15*60*1000 # 15 minutes

QUOTES_URL="http://finance.yahoo.com/d/quotes.csv?s=%(s)s&f=snc4l1d1t1c1ohgv&e=.csv"

# Sample (09/2/2010): "UCG.MI","UNICREDIT","EUR","UCG.MI",1.9410,"2/9/2010","6:10am",+0.0210,1.9080,1.9810,1.8920,166691232
QUOTES_CSV_FIELDS=["ticker", "label", "currency", ("trade", float), "date", "time", ("variation", float), ("open", float), ("high", float), ("low", float), ("volume", int)]

# based on http://www.johnstowers.co.nz/blog/index.php/2007/03/12/threading-and-pygtk/
class _IdleObject(gobject.GObject):
	"""
	Override gobject.GObject to always emit signals in the main thread
	by emmitting on an idle handler
	"""
	def __init__(self):
		gobject.GObject.__init__(self)

	def emit(self, *args):
		gobject.idle_add(gobject.GObject.emit,self,*args)

class QuotesRetriever(Thread, _IdleObject):
	"""
	Thread which uses gobject signals to return information
	to the GUI.
	"""
	__gsignals__ =  {
			"completed": (
				gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, []),
			# FIXME: We don't monitor progress, yet ...
			#"progress": (
			#	gobject.SIGNAL_RUN_LAST, gobject.TYPE_NONE, [
			#	gobject.TYPE_FLOAT])        #percent complete
			}

	def __init__(self, tickers):
		Thread.__init__(self)
		_IdleObject.__init__(self)
		self.tickers = tickers
		self.retrieved = False
		self.data = []
		self.currencies = []

	def run(self):
		quotes_url = QUOTES_URL % {"s": self.tickers}
		try:
			quotes_file = urlopen(quotes_url, proxies = invest.PROXY)
			self.data = quotes_file.readlines ()
			quotes_file.close ()
		except Exception, msg:
			invest.debug("Error while retrieving quotes data (url = %s): %s" % (quotes_url, msg))
		else:
			self.retrieved = True
		self.emit("completed")


class QuoteUpdater(gtk.ListStore):
	updated = False
	last_updated = None
	quotes_valid = False
	timeout_id = None
	SYMBOL, LABEL, CURRENCY, TICKER_ONLY, BALANCE, BALANCE_PCT, VALUE, VARIATION_PCT, PB = range(9)
	def __init__ (self, change_icon_callback, set_tooltip_callback):
		gtk.ListStore.__init__ (self, gobject.TYPE_STRING, gobject.TYPE_STRING, gobject.TYPE_STRING, bool, float, float, float, float, gtk.gdk.Pixbuf)
		self.set_update_interval(AUTOREFRESH_TIMEOUT)
		self.change_icon_callback = change_icon_callback
		self.set_tooltip_callback = set_tooltip_callback
		self.set_sort_column_id(1, gtk.SORT_ASCENDING)
		self.refresh()

		# tell the network manager to notify me when network status changes
		invest.nm.set_statechange_callback(self.nm_state_changed)

	def set_update_interval(self, interval):
		if self.timeout_id != None:
			invest.debug("Canceling refresh timer")
			gobject.source_remove(self.timeout_id)
			self.timeout_id = None
		if interval > 0:
			invest.debug("Setting refresh timer to %s:%02d.%03d" % ( interval / 60000, interval % 60000 / 1000, interval % 1000) )
			self.timeout_id = gobject.timeout_add(interval, self.refresh)

	def nm_state_changed(self):
		# when nm is online but we do not have an update timer, create it and refresh
		if invest.nm.online():
			if self.timeout_id == None:
				self.set_update_interval(AUTOREFRESH_TIMEOUT)
				self.refresh()

	def refresh(self):
		invest.debug("Refreshing")

		# when nm tells me I am offline, stop the update interval
		if invest.nm.offline():
			invest.debug("We are offline, stopping update timer")
			self.set_update_interval(0)
			return False

		if len(invest.STOCKS) == 0:
			return True

		tickers = '+'.join(invest.STOCKS.keys())
		quotes_retriever = QuotesRetriever(tickers)
		quotes_retriever.connect("completed", self.on_retriever_completed)
		quotes_retriever.start()

		return True


	# locale-aware formatting of the percent float (decimal point, thousand grouping point) with 2 decimal digits
	def format_percent(self, value):
		return locale.format("%+.2f", value, True) + "%"

	# locale-aware formatting of the float value (decimal point, thousand grouping point) with sign and 2 decimal digits
	def format_difference(self, value):
		return locale.format("%+.2f", value, True, True)

	def on_retriever_completed(self, retriever):
		if retriever.retrieved == False:
			tooltip = [_('Invest could not connect to Yahoo! Finance')]
			if self.last_updated != None:
				# Translators: %s is an hour (%H:%M)
				tooltip.append(_('Updated at %s') % self.last_updated.strftime("%H:%M"))
			self.set_tooltip_callback('\n'.join(tooltip))
		else:
			self.populate(self.parse_yahoo_csv(csv.reader(retriever.data)))
			self.updated = True
			self.last_updated = datetime.datetime.now()
			self.update_tooltip()

	def on_currency_retriever_completed(self, retriever):
		if retriever.retrieved == False:
			invest.error("Failed to retrieve currency rates!")
		else:
			self.convert_currencies(self.parse_yahoo_csv(csv.reader(retriever.data)))
		self.update_tooltip()

	def update_tooltip(self):
		tooltip = []
		if self.simple_quotes_count > 0:
			# Translators: This is share-market jargon. It is the average percentage change of all stock prices. The %s gets replaced with the string value of the change (localized), including the percent sign.
			tooltip.append(_('Average change: %s') % self.format_percent(self.avg_simple_quotes_change))
		for currency, stats in self.statistics.items():
			# get the statsitics
			balance = stats["balance"]
			paid = stats["paid"]
			change = self.format_percent(balance / paid * 100)
			balance = self.format_difference(balance)

			# Translators: This is share-market jargon. It refers to the total difference between the current price and purchase price for all the shares put together for a particular currency. i.e. How much money would be earned if they were sold right now. The first string is the change value, the second the currency, and the third value is the percentage of the change, formatted using user's locale.
			tooltip.append(_('Positions balance: %s %s (%s)') % (balance, currency, change))
		tooltip.append(_('Updated at %s') % self.last_updated.strftime("%H:%M"))
		self.set_tooltip_callback('\n'.join(tooltip))


	def parse_yahoo_csv(self, csvreader):
		result = {}
		for fields in csvreader:
			if len(fields) == 0:
				continue

			result[fields[0]] = {}
			for i, field in enumerate(QUOTES_CSV_FIELDS):
				if type(field) == tuple:
					try:
						result[fields[0]][field[0]] = field[1](fields[i])
					except:
						result[fields[0]][field[0]] = 0
				else:
					result[fields[0]][field] = fields[i]
			# calculated fields
			try:
				result[fields[0]]['variation_pct'] = result[fields[0]]['variation'] / float(result[fields[0]]['trade'] - result[fields[0]]['variation']) * 100
			except ZeroDivisionError:
				result[fields[0]]['variation_pct'] = 0
		return result

	# Computes the balance of the given purchases using a certain current value
	# and optionally a current exchange rate.
	def balance(self, purchases, value, currentrate=None):
		current = 0
		paid = 0

		for purchase in purchases:
			if purchase["amount"] != 0:
				buyrate = purchase["exchange"]
				# if the buy rate is invalid, use 1.0
				if buyrate <= 0:
					buyrate = 1.0

				# if no current rate is given, ignore buy rate
				if currentrate == None:
					buyrate = 1.0
					rate = 1.0
				else:
					# otherwise, take use buy rate and current rate to compute the balance
					rate = currentrate

				# current value is the current rate * amount * value
				current += rate * purchase["amount"] * value
				# paid is buy rate * ( amount * price + commission )
				paid += buyrate * (purchase["amount"] * purchase["bought"] + purchase["comission"])

		balance = current - paid
		if paid != 0:
			change = 100*balance/paid
		else:
			change = 100 # Not technically correct, but it will look more intuitive than the real result of infinity.

		return (balance, change)

	def populate(self, quotes):
		if (len(quotes) == 0):
			return

		self.clear()
		self.currencies = []

		try:
			quote_items = quotes.items ()
			quote_items.sort ()

			simple_quotes_change = 0
			self.simple_quotes_count = 0
			self.statistics = {}

			for ticker, val in quote_items:
				pb = None

				# get the label of this stock for later reuse
				label = invest.STOCKS[ticker]["label"]
				if len(label) == 0:
					if len(val["label"]) != 0:
						label = val["label"]
					else:
						label = ticker

				# make sure the currency field is upper case
				val["currency"] = val["currency"].upper();

				# the currency of currency conversion rates like EURUSD=X is wrong in csv
				# this can be fixed easily by reusing the latter currency in the symbol
				if len(ticker) == 8 and ticker.endswith("=X"):
					val["currency"] = ticker[3:6]

				# indices should not have a currency, though yahoo says so
				if ticker.startswith("^"):
					val["currency"] = ""

				# sometimes, funny currencies are returned (special characters), only consider known currencies
				if len(val["currency"]) > 0 and val["currency"] not in currencies.Currencies.currencies:
					invest.debug("Currency '%s' is not known, dropping" % val["currency"])
					val["currency"] = ""

				# if this is a currency not yet seen and different from the target currency, memorize it
				if val["currency"] not in self.currencies and len(val["currency"]) > 0:
					self.currencies.append(val["currency"])

				# Check whether the symbol is a simple quote, or a portfolio value
				is_simple_quote = True
				for purchase in invest.STOCKS[ticker]["purchases"]:
					if purchase["amount"] != 0:
						is_simple_quote = False
						break

				if is_simple_quote:
					self.simple_quotes_count += 1
					row = self.insert(0, [ticker, label, val["currency"], True, 0, 0, val["trade"], val["variation_pct"], pb])
					simple_quotes_change += val['variation_pct']
				else:
					(balance, change) = self.balance(invest.STOCKS[ticker]["purchases"], val["trade"])
					row = self.insert(0, [ticker, label, val["currency"], False, balance, change, val["trade"], val["variation_pct"], pb])
					self.add_balance_change(balance, change, val["currency"])

				if len(ticker.split('.')) == 2:
					url = 'http://ichart.europe.yahoo.com/h?s=%s' % ticker
				else:
					url = 'http://ichart.yahoo.com/h?s=%s' % ticker

				image_retriever = invest.chart.ImageRetriever(url)
				image_retriever.connect("completed", self.set_pb_callback, row)
				image_retriever.start()

			if self.simple_quotes_count > 0:
				self.avg_simple_quotes_change = simple_quotes_change/float(self.simple_quotes_count)
			else:
				self.avg_simple_quotes_change = 0

			if self.avg_simple_quotes_change != 0:
				simple_quotes_change_sign = self.avg_simple_quotes_change / abs(self.avg_simple_quotes_change)
			else:
				simple_quotes_change_sign = 0

			# change icon
			if self.simple_quotes_count > 0:
				self.change_icon_callback(simple_quotes_change_sign)
			else:
				positions_balance_sign = self.positions_balance/abs(self.positions_balance)
				self.change_icon_callback(positions_balance_sign)
				
			# mark quotes to finally be valid
			self.quotes_valid = True

		except Exception, msg:
			invest.debug("Failed to populate quotes: %s" % msg)
			invest.debug(quotes)
			self.quotes_valid = False

		# start retrieving currency conversion rates
		if invest.CONFIG.has_key("currency"):
			target_currency = invest.CONFIG["currency"]
			symbols = []

			invest.debug("These currencies occur: %s" % self.currencies)
			for currency in self.currencies:
				if currency == target_currency:
					continue

				invest.debug("%s will be converted to %s" % ( currency, target_currency ))
				symbol = currency + target_currency + "=X"
				symbols.append(symbol)

			if len(symbols) > 0:
				tickers = '+'.join(symbols)
				quotes_retriever = QuotesRetriever(tickers)
				quotes_retriever.connect("completed", self.on_currency_retriever_completed)
				quotes_retriever.start()

	def convert_currencies(self, quotes):
		# if there is no target currency, this method should never have been called
		if not invest.CONFIG.has_key("currency"):
			return

		# reset the overall balance
		self.statistics = {}

		# collect the rates for the currencies
		rates = {}
		for symbol, data in quotes.items():
			currency = symbol[0:3]
			rate = data["trade"]
			rates[currency] = rate

		# convert all non target currencies
		target_currency = invest.CONFIG["currency"]
		iter = self.get_iter_first()
		while iter != None:
			currency = self.get_value(iter, self.CURRENCY)
			symbol = self.get_value(iter, self.SYMBOL)
			# ignore stocks that are currency conversions
			# and only convert stocks that are not in the target currency
			# and if we have a conversion rate
			if not ( len(symbol) == 8 and symbol[6:8] == "=X" ) and \
			   currency != target_currency and rates.has_key(currency):
				# first convert the balance, it needs the original value
				if not self.get_value(iter, self.TICKER_ONLY):
					ticker = self.get_value(iter, self.SYMBOL)
					value = self.get_value(iter, self.VALUE)
					(balance, change) = self.balance(invest.STOCKS[ticker]["purchases"], value, rates[currency])
					self.set(iter, self.BALANCE, balance)
					self.set(iter, self.BALANCE_PCT, change)
					self.add_balance_change(balance, change, target_currency)

				# now, convert the value
				value = self.get_value(iter, self.VALUE)
				value *= rates[currency]
				self.set(iter, self.VALUE, value)
				self.set(iter, self.CURRENCY, target_currency)

			else:
				# consider non-converted stocks here
				balance = self.get_value(iter, self.BALANCE)
				change  = self.get_value(iter, self.BALANCE_PCT)
				self.add_balance_change(balance, change, currency)

			iter = self.iter_next(iter)

	def add_balance_change(self, balance, change, currency):
		if balance == 0 and change == 0:
			return

		if self.statistics.has_key(currency):
			self.statistics[currency]["balance"] += balance
			self.statistics[currency]["paid"] += balance/change*100
		else:
			self.statistics[currency] = { "balance" : balance, "paid" : balance/change*100 }

	def set_pb_callback(self, retriever, row):
		self.set_value(row, self.PB, retriever.image.get_pixbuf())

	# check if we have only simple quotes
	def simple_quotes_only(self):
		res = True
		for entry, data in invest.STOCKS.iteritems():
			purchases = data["purchases"]
			for purchase in purchases:
				if purchase["amount"] != 0:
					res = False
					break
		return res

if gtk.pygtk_version < (2,8,0):
	gobject.type_register(QuoteUpdater)
