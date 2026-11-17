/* SPDX-License-Identifier: GPL-2.0-only */
#ifndef EQUATION_INTL_H
#define EQUATION_INTL_H
#include "equation_build_config.h"
#if EQUATION_ENABLE_NLS
#include <libintl.h>
#define _(text) dgettext("equation", text)
#else
#define _(text) (text)
#endif
#define N_(text) (text)
void equation_intl_init(void);
#endif
