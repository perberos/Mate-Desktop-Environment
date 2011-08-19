#include <config.h>
#include <matecorba/util/matecorba-util.h>

gulong
MateCORBA_wchar_strlen (CORBA_wchar *wstr)
{
	gulong i;

	for (i = 0; wstr[i]; i++)
		;

	return i;
}
