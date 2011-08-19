#ifndef H_MATE_SYSTEM_MONITOR_GKSU_H_1132171928
#define H_MATE_SYSTEM_MONITOR_GKSU_H_1132171928

#include <glib.h>

gboolean
procman_gksu_create_root_password_dialog(const char * command);

gboolean
procman_has_gksu(void) G_GNUC_CONST;

#endif /* H_MATE_SYSTEM_MONITOR_GKSU_H_1132171928 */
