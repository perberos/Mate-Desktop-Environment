import warnings
warnings.warn("Module mate.canvas is deprecated; "
              "please import matecanvas instead",
              DeprecationWarning, stacklevel=2)
del warnings

from matecanvas import *
