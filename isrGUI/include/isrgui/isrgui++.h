#ifndef _isrguixx_h
#define _isrguixx_h

#include "isrgui/plot/Realplot.h"
#include "isrgui/plot/Scatterplot.h"
#include "isrgui/plot/Complexplot.h"
#include "isrgui/plot/Waterfallplot.h"
#include "isrgui/plot/TextEdit.h"
#include "isrgui/plot/KeyValue.h"

int  sdrgui_init();
int  sdrgui_init_title(const char *window_title);
void sdrgui_exit();

#endif
