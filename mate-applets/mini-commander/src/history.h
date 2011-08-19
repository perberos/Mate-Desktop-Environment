#include "mini-commander_applet.h"

int exists_history_entry(int pos);
extern char *get_history_entry(int pos);
extern void set_history_entry(int pos, char * entry);
extern void append_history_entry(MCData *mcdata, const char * entry, gboolean load_history);
