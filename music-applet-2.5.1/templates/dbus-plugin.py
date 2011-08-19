# [Your copyright and license notice goes here]

import musicapplet.player


# The INFO hash provides the basic metadata for the plugin.  It's the
# first thing the plugin loader will look for, and if it can't find it,
# the loader will ignore the plugin entirely.

INFO = {

        # The name of your plugin.  This should normally be the same as
        # whatever music player the plugin works with.
        "name": "Example",

        # The version of your plugin.  This is *not* the version of the
        # music player it works with, nor the version of Music Applet it
        # is coded for.  This is your own version number to define however
        # you see fit, but note that the plugin loader will assume it's a
        # string of numbers separated by dots.  The plugin loader uses
        # this version number to choose the latest version of the plugin
        # in case it finds more than one.
        "version": "1.2.3",

        # The name of the themable icon that represents your plugin.
        # This should normally be the same as the icon used by the music
        # player your plugin interfaces with.  If you don't have an icon,
        # set this to None.
        "icon-name": "example-app",

        # The name of whoever wrote this plugin.  That's you!  Including
        # your e-mail address is a good idea.
        "author": "John Doe <jdoe@example.com>",

        # The copyright line for your plugin.
        "copyright": "(C) 20XX John Doe",

        # The website for the plugin.  This is where users would go if they
        # want to find the latest version of your plugin, for instance.
        "website": "http://www.example.com/~jdoe/plugins/",

}


class ExamplePlugin (musicapplet.player.DBusPlugin)
    """
    The class that implements the plugin itself, for plugins that use D-Bus to
    communicate with their music player.

    All the interesting bits of the plugin will be methods that implement the
    base class.  Each of these will be described separately below.

    Data and methods you're creating yourself should start with two
    underscores (e.g. __my_method) so Python knows to threat them as private.
    This will prevent problems in the future if the base class decides to use
    that name for itself later on.

    Your plugin will communicate with Music Applet by updating the following
    properties as appropriate:

        connected -- True if the plugin is connected to the music player, and
                     False otherwise.

        playing -- True if the music player is currently playing something, and
                   False otherwise.

        title -- The title of the current song, or None if there isn't one.

        artist -- The artist who created the current song, or None if there
                  isn't one.

        album -- The album the current song is from, or None if there isn't
                 one.

        duration -- The duration in seconds of the current song, or -1 if
                    there isn't one.

        elapsed -- The elapsed time in seconds of the current song, or -1 if
                   there isn't one.

        rating -- The rating of the current song, or -1.0 if there is no
                  current song or ratings are not supported.

        min_rating -- The minimum allowed rating, or -1.0 if ratings are
                      not supported.

        max_rating -- The maximum allowed rating, or -1.0 if ratings are
                      not supported.

        art -- a gtk.gdk.Pixbuf of the artwork associated with the current
               song (probably an album cover), or None if there isn't any.

    The _conf property is an instance of musicapplet.conf.Conf you can use
    to handle your plugin's configuration in MateConf.  You should define a
    MateConf schema for any settings you use, and use the following naming
    convention for them in the schema:

            /schemas/apps/music-applet/prefs/Plugin-Name/key_name

    Note that the default implementation of the "launch" method assumes your
    plugin defines a "command" setting in MateConf that it will try to run as a
    shell command to launch the player.

    If all this confuses you, look at some of the plugins that ship with Music
    Applet to see how applets typically behave.
    """

    def __init__ (self, conf):
        """
        Initializes a new instance of the plugin.

        Do any very basic initialization here, but don't start actually trying
        to do any communicating with the music player yet.  The base class will
        take care of that for you, and all you need to do is tell it what name
        to look for on the bus in the parent constructor.
        """
        musicapplet.player.DBusPlugin.__init__ (self, conf,
                                                INFO["name"], INFO["icon-name"],
                                                "dbus.name.for.the.music.player")

    def _connect (self):
        """
        Called when the music player is available on the bus.

        This method should set up the proxies and interfaces you'll use to
        communicate with the player, as well as fetch its current state.
        """
        pass

    def _disconnect (self):
        """
        Called when the music player has disconnected from the bus.

        This method should clean up after anything that happened in _connect
        or after the connection was established.  Basically, the plugin's
        state should return to what it was immediately after "__init__" was
        called.
        """
        pass

    def toggle_playback (self):
        """
        Toggles playback of the current song.
        """
        pass

    def previous (self):
        """
        Changes to the previous song.
        """
        pass

    def next (self):
        """
        Changes to the next song.
        """
        pass

    # These next methods are optional.  If your plugin doesn't support what
    # they do, comment them out entirely.

    def rate_song (self, rating):
        """
        Set the rating of the current song, using the five-star scale.

        Comment this method out if the player doesn't support rating songs.
        """
        pass

    # These methods have default implementations that are probably good
    # enough for most cases, but you may want to override them if you need
    # to.

    #def create_config_dialog (self):
    #    """
    #    Returns a gtk.Dialog that lets the user configure the plugin.
    #
    #    Call the "_construct_config_dialog" to get a glade.XML object
    #    for the default D-Bus plugin configuration dialog, which you
    #    can then customize.
    #
    #    Unless you're defining additional configuration parameters, the
    #    base class's implementation will be good enough.
    #    """
    #    pass

    #def launch (self):
    #    """
    #    Attempts to launch the music player this plugin works with.
    #
    #    The base class's implementation of this function should normally be
    #    good enough.  It looks up your plugin's "command" MateConf key and
    #    runs its value as a shell command.
    #    """
    #    pass


def create_instance (conf):
    """
    Creates an instance of the plugin object.

    All this does is call your plugin's constructor.  This method only exists
    because the plugin manager needs to know what function to call to create
    the instance, and each plugin will have a different name for its class.

    You should only change this to reflect whatever you named the plugin's
    class.  If you think you should make any other changes to this, you're
    probably doing something wrong.
    """
    return ExamplePlugin (conf)
