import invest
from dbus.mainloop.glib import DBusGMainLoop
import dbus

# possible states, see http://projects.mate.org/NetworkManager/developers/spec-08.html#type-NM_STATE
STATE_UNKNOWN		= dbus.UInt32(0)
STATE_ASLEEP		= dbus.UInt32(1)
STATE_CONNECTING	= dbus.UInt32(2)
STATE_CONNECTED		= dbus.UInt32(3)
STATE_DISCONNEDTED	= dbus.UInt32(4)

class NetworkManager:
	def __init__(self):
		self.state = STATE_UNKNOWN
		self.statechange_callback = None

		try:
			# get an event loop
			loop = DBusGMainLoop()

			# get the NetworkManager object from D-Bus
			invest.debug("Connecting to Network Manager via D-Bus")
			bus = dbus.SystemBus(mainloop=loop)
			nmobj = bus.get_object('org.freedesktop.NetworkManager', '/org/freedesktop/NetworkManager')
			nm = dbus.Interface(nmobj, 'org.freedesktop.NetworkManager')

			# connect the signal handler to the bus
			bus.add_signal_receiver(self.handler, None,
					'org.freedesktop.NetworkManager',
					'org.freedesktop.NetworkManager',
					'/org/freedesktop/NetworkManager')

			# get the current status of the network manager
			self.state = nm.state()
			invest.debug("Current Network Manager status is %d" % self.state)
		except Exception, msg:
			invest.error("Could not connect to the Network Manager: %s" % msg )

	def online(self):
		return self.state == STATE_UNKNOWN or self.state == STATE_CONNECTED

	def offline(self):
		return not self.online()

	# the signal handler for signals from the network manager
	def handler(self,signal=None):
		if isinstance(signal, dict):
			state = signal.get('State')
			if state != None:
				invest.debug("Network Manager change state %d => %d" % (self.state, state) );
				self.state = state

				# notify about state change
				if self.statechange_callback != None:
					self.statechange_callback()

	def set_statechange_callback(self,handler):
		self.statechange_callback = handler
