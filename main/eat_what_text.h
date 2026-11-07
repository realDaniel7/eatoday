// main/eat_what_text.h —— 屏幕上真正显示的每一段文字都集中在这里。
//
// 集中放有两个原因：
//   1) tools/eat_what_glyphs.py 只扫描本文件与 eat_what_seed.c 的字符串字面量，
//      生成“够用就好”的中文字体子集；日志文案不会误进字体。
//   2) 改文案时不必在界面代码里翻找，也方便和 docs 里的交互说明对齐。
#pragma once

#include <stddef.h>

#define EAT_TEXT_TITLE "今天吃什么"
#define EAT_TEXT_WELCOME_TITLE "今天吃什么"
#define EAT_TEXT_WELCOME_HINT "按住 ↑ 开始转"
#define EAT_TEXT_WELCOME_HINT_NO_SET "按住 ↑ 开始转   长按确定 加自己的店"
#define EAT_TEXT_STATS_DEFAULT_FMT "默认池 %u 项"
#define EAT_TEXT_STATS_SET_FMT "%s %u 项"
#define EAT_TEXT_RANGE_DEFAULT "范围 默认池"
#define EAT_TEXT_RANGE_DEFAULT_NAME "默认"
#define EAT_TEXT_RANGE_SET_FMT "范围 %s"
#define EAT_TEXT_SPINNING "旋转中…"
#define EAT_TEXT_ACCEPTED "就它了"
#define EAT_TEXT_HINT_MAIN "长按↑ 旋转   ↓ 下一个\n长按↓ 切列表   长按确定 设置"
#define EAT_TEXT_BATTERY_UNKNOWN "--"
#define EAT_TEXT_BATTERY_FMT "%d%%"

#define EAT_TEXT_TOAST_ADDED "已加入轮盘 ★"
#define EAT_TEXT_TOAST_UPDATED "列表已更新 ★"

#define EAT_TEXT_WEB_TITLE "手机设置后台"
#define EAT_TEXT_WEB_WIFI_FMT "WiFi %s"
#define EAT_TEXT_WEB_PASS_FMT "密码 %s"
#define EAT_TEXT_WEB_URL_FMT "打开 %s"
#define EAT_TEXT_WEB_TIP "长按下键 退出"
#define EAT_TEXT_WEB_FAIL "热点启动失败"
#define EAT_TEXT_WEB_HELP "手机连上这个热点后打开上面的地址"
#define EAT_TEXT_WEB_STATS_FMT "已保存 %u 项 · 当前 %s"

// 3 套列表的默认名字。放在这里是为了让字体生成脚本把它们算进字符清单，
// 否则“我”“的”这两个字会缺字形，屏幕上显示成方框。
extern const char *const EAT_DEFAULT_SET_NAMES[];
#define EAT_DEFAULT_SET_NAME_COUNT 3

// 名称连接符（U+00B7），用于“品类·短名称”。
#define EAT_TEXT_JOINER "·"
