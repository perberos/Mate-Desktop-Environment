from gettext import gettext as _
import locale
from os.path import join
import gtk, gobject, mateconf
import invest
import currencies
import cPickle

class PrefsDialog:
	def __init__(self, applet):
		self.ui = gtk.Builder()
		self.ui.add_from_file(join(invest.BUILDER_DATA_DIR, "prefs-dialog.ui"))

		self.dialog = self.ui.get_object("preferences")
		self.treeview = self.ui.get_object("stocks")
		self.currency = self.ui.get_object("currency")
		self.currency_code = None
		self.currencies = currencies.Currencies.currencies

		self.ui.get_object("add").connect('clicked', self.on_add_stock)
		self.ui.get_object("add").connect('activate', self.on_add_stock)
		self.ui.get_object("remove").connect('clicked', self.on_remove_stock)
		self.ui.get_object("remove").connect('activate', self.on_remove_stock)
		self.ui.get_object("help").connect('clicked', self.on_help)
		self.treeview.connect('key-press-event', self.on_tree_keypress)
		self.currency.connect('key-press-event', self.on_entry_keypress)
		self.currency.connect('activate', self.on_activate_entry)
		self.currency.connect('focus-out-event', self.on_focus_out_entry)

		self.typs = (str, str, float, float, float, float)
		self.names = (_("Symbol"), _("Label"), _("Amount"), _("Price"), _("Commission"), _("Currency Rate"))
		store = gtk.ListStore(*self.typs)
		store.set_sort_column_id(0, gtk.SORT_ASCENDING)
		self.treeview.set_model(store)
		self.model = store

		completion = gtk.EntryCompletion()
		self.currency.set_completion(completion)
		liststore = gtk.ListStore(gobject.TYPE_STRING, gobject.TYPE_STRING)
		completion.set_model(liststore)
		completion.set_text_column(0)
		for code, label in self.currencies.items():
			liststore.append([self.format_currency(label, code), code])
		completion.set_match_func(self.match_func, 0)
		completion.connect("match-selected", self.on_completion_selection, 1)

		if invest.CONFIG.has_key("currency"):
			code = invest.CONFIG["currency"];
			if self.currencies.has_key(code):
				self.currency_code = code;
				currency = self.format_currency(self.currencies[self.currency_code], self.currency_code)
				self.currency.set_text(currency)

		for n in xrange (0, 5):
			self.create_cell (self.treeview, n, self.names[n], self.typs[n])
		if self.currency_code != None:
			self.add_exchange_column()

		stock_items = invest.STOCKS.items ()
		stock_items.sort ()
		for key, data in stock_items:
			label = data["label"]
			purchases = data["purchases"]
			for purchase in purchases:
				if purchase.has_key("exchange"):
					exchange =  purchase["exchange"]
				else:
					exchange = 0.0
				store.append([key, label, purchase["amount"], purchase["bought"], purchase["comission"], exchange])

		self.sync_ui()

	def on_cell_edited(self, cell, path, new_text, col, typ):
		try:
			if col == 0:    # stock symbols must be uppercase
				new_text = str.upper(new_text)
			if col < 2:
				self.model[path][col] = new_text
			else:
				value = locale.atof(new_text)
				self.model[path][col] = value
		except Exception, msg:
			invest.error('Exception while processing cell change: %s' % msg)
			pass

	def format(self, fmt, value):
		return locale.format(fmt, value, True)

	def get_cell_data(self, column, cell, model, iter, data):
		typ, col = data
		if typ == int:
			cell.set_property('text', "%d" % typ(model[iter][col]))
		elif typ == float:
			# provide float numbers with at least 2 fractional digits
			val = model[iter][col]
			digits = self.fraction_digits(val)
			fmt = "%%.%df" % max(digits, 2)
			cell.set_property('text', self.format(fmt, val))
		else:
			cell.set_property('text', typ(model[iter][col]))

	# determine the number of non zero digits in the fraction of the value
	def fraction_digits(self, value):
		text = "%g" % value	# do not use locale here, so that %g always is rendered to a number with . as decimal separator
		if text.find(".") < 0:
			return 0
		return len(text) - text.find(".") - 1

	def create_cell (self, view, column, name, typ):
		cell_description = gtk.CellRendererText ()
		if typ == float:
			cell_description.set_property("xalign", 1.0)
		cell_description.set_property("editable", True)
		cell_description.connect("edited", self.on_cell_edited, column, typ)
		column_description = gtk.TreeViewColumn (name, cell_description)
		if typ == str:
			column_description.set_attributes (cell_description, text=column)
			column_description.set_sort_column_id(column)
		if typ == float:
			column_description.set_cell_data_func(cell_description, self.get_cell_data, (float, column))
		view.append_column(column_description)

	def add_exchange_column(self):
		self.create_cell (self.treeview, 5, self.names[5], self.typs[5])

	def remove_exchange_column(self):
		column = self.treeview.get_column(5)
		self.treeview.remove_column(column)

	def show_run_hide(self, explanation = ""):
		expl = self.ui.get_object("explanation")
		expl.set_markup(explanation)
		self.dialog.show_all()
		if explanation == "":
			expl.hide()
		# returns 1 if help is clicked
		while self.dialog.run() == 1:
			pass
		self.dialog.destroy()

		invest.STOCKS = {}

		def save_symbol(model, path, iter):
			#if int(model[iter][1]) == 0 or float(model[iter][2]) < 0.0001:
			#	return

			if not model[iter][0] in invest.STOCKS:
				invest.STOCKS[model[iter][0]] = { 'label': model[iter][1], 'purchases': [] }
				
			invest.STOCKS[model[iter][0]]["purchases"].append({
				"amount": float(model[iter][2]),
				"bought": float(model[iter][3]),
				"comission": float(model[iter][4]),
				"exchange": float(model[iter][5])
			})
		self.model.foreach(save_symbol)
		try:
			cPickle.dump(invest.STOCKS, file(invest.STOCKS_FILE, 'w'))
			invest.debug('Stocks written to file')
		except Exception, msg:
			invest.error('Could not save stocks file: %s' % msg)

		invest.CONFIG = {}
		if self.currency_code != None and len(self.currency_code) == 3:
			invest.CONFIG['currency'] = self.currency_code
		try:
			cPickle.dump(invest.CONFIG, file(invest.CONFIG_FILE, 'w'))
			invest.debug('Configuration written to file')
		except Exception, msg:
			invest.debug('Could not save configuration file: %s' % msg)

	def sync_ui(self):
		pass

	def on_add_stock(self, w):
		iter = self.model.append(["GOOG", "Google Inc.", 0, 0, 0, 0])
		path = self.model.get_path(iter)
		self.treeview.set_cursor(path, self.treeview.get_column(0), True)

	def on_remove_stock(self, w):
		model, paths = self.treeview.get_selection().get_selected_rows()
		for path in paths:
			model.remove(model.get_iter(path))

	def on_help(self, w):
		invest.help.show_help_section("invest-applet-usage")

	def on_tree_keypress(self, w, event):
		if event.keyval == 65535:
			self.on_remove_stock(w)

		return False

	def format_currency(self, label, code):
		if code == None:
			return label
		return label + " (" + code + ")"

	def on_entry_keypress(self, w, event):
		# enter key was pressed
		if event.keyval == 65293:
			self.match_currency()
		return False

	# entry was activated (Enter)
	def on_activate_entry(self, entry):
		self.match_currency()

	# entry left focus
	def on_focus_out_entry(self, w, event):
		self.match_currency()
		return False

	# tries to find a currency for the text in the currency entry
	def match_currency(self):
		# get the text
		text = self.currency.get_text().upper()

		# if there is none, finish
		if len(text) == 0:
			self.currency_code = None
			self.pick_currency(None)
			return

		# if it is a currency code, take that one
		if len(text) == 3:
			# try to find the string as code
			if self.currencies.has_key(text):
				self.pick_currency(text)
				return
		else:
			# try to find the code for the string
			for code, label in self.currencies.items():
				# if the entry equals to the full label,
				# or the entry equals to the label+code concat
				if label.upper() == text or self.format_currency(label.upper(), code) == text:
					# then we take that code
					self.pick_currency(code)
					return

		# the entry is not valid, reuse the old one
		self.pick_currency(self.currency_code)

	# pick this currency, stores it and sets the entry text
	def pick_currency(self, code):
		if code == None:
			label = ""
			if len(self.treeview.get_columns()) == 6:
				self.remove_exchange_column()
		else:
			label = self.currencies[code]
			if len(self.treeview.get_columns()) == 5:
				self.add_exchange_column()
		self.currency.set_text(self.format_currency(label, code))
		self.currency_code = code

	# finds matches by testing candidate strings to have tokens starting with the entered text's tokens
	def match_func(self, completion, key, iter, column):
		keys = key.split()
		model = completion.get_model()
		text = model.get_value(iter, column).lower()
		tokens = text.split()

		# each key must have a match
		for key in keys:
			found_key = False
			# check any token of the completions start with the key
			for token in tokens:
				# remove the ( from the currency code
				if token.startswith("("):
					token = token[1:]
				if token.startswith(key):
					found_key = True
					break
			# this key does not have a match, this is not a completion
			if not found_key:
				return False
		# all keys matched, this is a completion
		return True

	# stores the selected currency code
	def on_completion_selection(self, completion, model, iter, column):
		self.pick_currency(model.get_value(iter, column))


def show_preferences(applet, explanation = ""):
	PrefsDialog(applet).show_run_hide(explanation)
