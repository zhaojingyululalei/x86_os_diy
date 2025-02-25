#ifndef __MAIN_H
#define __MAIN_H

#include "_stdlib.h"


#define ESC_CMD2(Pn, cmd)		    "\x1b["#Pn#cmd
#define	ESC_COLOR_ERROR			    ESC_CMD2(31, m)	// 红色错误
#define	ESC_COLOR_DEFAULT		    ESC_CMD2(39, m)	// 默认颜色
#define ESC_CLEAR_SCREEN		    ESC_CMD2(2, J)	// 擦除整屏幕
#define	ESC_MOVE_CURSOR(row, col)  "\x1b["#row";"#col"H"
#endif
