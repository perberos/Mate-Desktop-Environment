#include <config.h>

#include <glib/gi18n.h>
#include <glibmm/optionentry.h>


#include "argv.h"

namespace procman
{
  OptionGroup::OptionGroup()
    : Glib::OptionGroup("", ""),
      show_system_tab(false)
  {
    Glib::OptionEntry sys_tab;
    sys_tab.set_long_name("show-system-tab");
    sys_tab.set_short_name('s');
    sys_tab.set_description(_("Show the System tab"));
    this->add_entry(sys_tab, this->show_system_tab);
  }
}

