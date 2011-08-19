#! /bin/sh

cd $MATECONF_SRCDIR || exit 1

glib-mkenums \
	--fhead "#ifndef __MATECONF_ENUM_TYPES_H__\n#define __MATECONF_ENUM_TYPES_H__\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n\n" \
	--fprod "/* enumerations from \"@filename@\" */\n\n" \
	--vhead "GType @enum_name@_get_type (void);\n#define MATECONF_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n\n" \
	--ftail "G_END_DECLS\n\n#endif /* __MATECONF_ENUM_TYPES_H__ */" \
	$* > tmp-unfixed-mateconf-enum-types.h || exit 1

cat tmp-unfixed-mateconf-enum-types.h | sed -e 's/g_conf/mateconf/g' -e 's/TYPE_CONF/TYPE/g' > tmp-mateconf-enum-types.h || exit 1

rm -f tmp-unfixed-mateconf-enum-types.h || exit 1

if cmp -s tmp-mateconf-enum-types.h mateconf-enum-types.h; then
  echo "mateconf-enum-types.h is unchanged"
else
  echo "Replacing mateconf-enum-types.h"
  cp tmp-mateconf-enum-types.h mateconf-enum-types.h || exit 1
fi

rm -f tmp-mateconf-enum-types.h || exit 1

exit 0
