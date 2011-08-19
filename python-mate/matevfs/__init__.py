# -*- mode: python; encoding: utf-8; -*-

from _matevfs import *
from _matevfs import _PyMateVFS_API


def mime_get_default_component(*args, **kwargs):
    import matevfsmatecomponent
    return matevfsmatecomponent.mime_get_default_component(*args, **kwargs)

def mime_get_short_list_components(*args, **kwargs):
    import matevfsmatecomponent
    return matevfsmatecomponent.mime_get_short_list_components(*args, **kwargs)

def mime_get_all_components(*args, **kwargs):
    import matevfsmatecomponent
    return matevfsmatecomponent.mime_get_all_components(*args, **kwargs)

