
INCLUDES = $(PYTHON_INCLUDES) $(PYMATECORBA_CFLAGS) -DMATECORBA2_STUBS_API

pyexec_PYTHON = CORBA.py PortableServer.py

pyexec_LTLIBRARIES = MateCORBA.la

common_ldflags = -module -avoid-version $(PYTHON_LDFLAGS)

MateCORBA_la_LDFLAGS = $(common_ldflags) -export-symbols-regex initMateCORBA
MateCORBA_la_LIBADD = $(PYMATECORBA_LIBS) $(PYTHON_LIBS)
MateCORBA_la_SOURCES = \
  MateCORBAmodule.c \
  pycorba-typecode.c \
  pycorba-object.c \
  pycorba-policy.c \
  pycorba-method.c \
  pycorba-marshal.c \
  pycorba-orb.c \
  pycorba-any.c \
  pycorba-exceptions.c \
  pycorba-struct.c \
  pycorba-enum.c \
  pycorba-fixed.c \
  stub-gen.c \
  pymatecorba-servant.c \
  pymatecorba-poa.c \
  pymatecorba-utils.c \
  pymatecorba-private.h

pymatecorbaincludedir = $(includedir)/pymatecorba-2

pymatecorbainclude_HEADERS = pymatecorba.h
