import warnings
warnings.warn("Module mate.vfs is deprecated; "
              "please import matevfs instead",
              DeprecationWarning)
del warnings

from matevfs import *
