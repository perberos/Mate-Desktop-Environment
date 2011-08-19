#! /bin/sh

cd $MATECONF_SRCDIR || exit 1

glib-mkenums \
	--fhead '#include "mateconf-client.h"' \
	--fprod '\n/* enumerations from "@filename@" */' \
	--vhead 'GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {' \
	 --vprod '      { @VALUENAME@, "@VALUENAME@", "@valuenick@" },' \
	 --vtail '      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static ("@EnumName@", values);\n  }\n  return etype;\n}\n\n' \
	 $* > tmp-unfixed-mateconf-enum-types.c || exit 1

cat tmp-unfixed-mateconf-enum-types.c | sed -e 's/g_conf/mateconf/g' -e 's/TYPE_CONF/TYPE/g' > tmp-mateconf-enum-types.c || exit 1

rm -f tmp-unfixed-mateconf-enum-types.c || exit 1

if cmp -s tmp-mateconf-enum-types.c mateconf-enum-types.c; then
  echo "mateconf-enum-types.c is unchanged"
else
  echo "Replacing mateconf-enum-types.c"
  cp tmp-mateconf-enum-types.c mateconf-enum-types.c || exit 1
fi

rm -f tmp-mateconf-enum-types.c || exit 1

exit 0
