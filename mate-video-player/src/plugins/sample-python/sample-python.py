# From code by James Livingston

import totem
import gobject, gtk

class SamplePython(totem.Plugin):

	def __init__(self):
		totem.Plugin.__init__(self)
			
	def activate(self, totem):
		print "Activating sample Python plugin"
		totem.action_fullscreen_toggle()
	
	def deactivate(self, totem):
		print "Deactivating sample Python plugin"
