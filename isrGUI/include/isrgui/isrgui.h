#ifndef _isrgui_h
#define _isrgui_h

#ifdef __cplusplus
extern "C" {
#endif

#include "isrgui/plot/plot_real.h"
#include "isrgui/plot/plot_scatter.h"
#include "isrgui/plot/plot_complex.h"
#include "isrgui/plot/plot_waterfall.h"
#include "isrgui/plot/text_edit.h"
#include "isrgui/plot/key_value.h"

int  sdrgui_init();
int  sdrgui_init_title(const char *window_title);
void sdrgui_exit();

#ifdef __cplusplus
}
#endif

#endif
