


#include <config.h>

#include "eggtypebuiltins.h"


/* enumerations from "egg-toolbars-model.h" */
#include "egg-toolbars-model.h"
static const GFlagsValue _egg_tb_model_flags_values[] = {
  { EGG_TB_MODEL_NOT_REMOVABLE, "EGG_TB_MODEL_NOT_REMOVABLE", "not-removable" },
  { EGG_TB_MODEL_NOT_EDITABLE, "EGG_TB_MODEL_NOT_EDITABLE", "not-editable" },
  { EGG_TB_MODEL_BOTH, "EGG_TB_MODEL_BOTH", "both" },
  { EGG_TB_MODEL_BOTH_HORIZ, "EGG_TB_MODEL_BOTH_HORIZ", "both-horiz" },
  { EGG_TB_MODEL_ICONS, "EGG_TB_MODEL_ICONS", "icons" },
  { EGG_TB_MODEL_TEXT, "EGG_TB_MODEL_TEXT", "text" },
  { EGG_TB_MODEL_STYLES_MASK, "EGG_TB_MODEL_STYLES_MASK", "styles-mask" },
  { EGG_TB_MODEL_ACCEPT_ITEMS_ONLY, "EGG_TB_MODEL_ACCEPT_ITEMS_ONLY", "accept-items-only" },
  { EGG_TB_MODEL_HIDDEN, "EGG_TB_MODEL_HIDDEN", "hidden" },
  { 0, NULL, NULL }
};

GType
egg_tb_model_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_flags_register_static ("EggTbModelFlags", _egg_tb_model_flags_values);

  return type;
}

static const GFlagsValue _egg_tb_model_name_flags_values[] = {
  { EGG_TB_MODEL_NAME_USED, "EGG_TB_MODEL_NAME_USED", "used" },
  { EGG_TB_MODEL_NAME_INFINITE, "EGG_TB_MODEL_NAME_INFINITE", "infinite" },
  { EGG_TB_MODEL_NAME_KNOWN, "EGG_TB_MODEL_NAME_KNOWN", "known" },
  { 0, NULL, NULL }
};

GType
egg_tb_model_name_flags_get_type (void)
{
  static GType type = 0;

  if (G_UNLIKELY (type == 0))
    type = g_flags_register_static ("EggTbModelNameFlags", _egg_tb_model_name_flags_values);

  return type;
}




