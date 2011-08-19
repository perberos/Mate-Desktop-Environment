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


class ExamplePlugin (musicapplet.player.MPRISPlugin):
    """
    The class that implements the plugin itself, for plugins that use D-Bus to
    communicate with their music player via the common MPRIS interface.

    There's really very little to do here, other than tell the base class
    where it can find the music player's MPRIS interface.  Seriously, the
    code below is all you really need, as far as Python goes.  Take a look
    at the Audacious or VLC plugin if you don't believe me.

    You should define a MateConf schema for any settings you use, and use the
    following naming convention for them in the schema:

            /schemas/apps/music-applet/prefs/Plugin-Name/key_name

    The MPRIS Plugin base class expects two MateConf keys: "launch" and "command".
    You can pretty much copy and paste the schemas for some other MPRIS-using
    plugin and change the names to match yours.

    If any of this confuses you, look at some of the plugins that ship with Music
    Applet to see how applets typically behave.
    """

    def __init__ (self, conf):
        musicapplet.player.MPRISPlugin.__init__ (self, conf,
                                                 INFO["name"], INFO["icon-name"],
                                                 "org.mpris.whatever-the-player-name-is")


def create_instance (conf):
    return ExamplePlugin (conf)
