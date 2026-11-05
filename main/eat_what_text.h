// main/eat_what_text.h —— 屏幕上真正显示的每一段文字都集中在这里。
//
// 集中放有两个原因：
//   1) tools/eat_what_glyphs.py 只扫描本文件与 eat_what_seed.c 的字符串字面量，
//      生成“够用就好”的中文字体子集；日志文案不会误进字体。
//   2) 改文案时不必在界面代码里翻找，也方便和 docs 里的交互说明对齐。
#pragma once

#include <stddef.h>

#define EAT_TEXT_TITLE "北京 · 今天吃什么"
#define EAT_TEXT_STATS_FMT "种子池 %u · 我的 %u"
#define EAT_TEXT_SCOPE_ALL "范围 全部"
#define EAT_TEXT_SCOPE_SEED_ONLY "范围 仅种子池"
#define EAT_TEXT_SPINNING "旋转中…"
#define EAT_TEXT_ACCEPTED "就它了"
#define EAT_TEXT_HINT_MAIN "长按↑ 旋转   ↓ 下一个\n长按↓ 范围   长按确定 添加"
#define EAT_TEXT_BATTERY_UNKNOWN "--"
#define EAT_TEXT_BATTERY_FMT "%d%%"

#define EAT_TEXT_ADD_PROMPT "今天吃了什么？"
#define EAT_TEXT_ADD_EMPTY "选一个词"
#define EAT_TEXT_ADD_HINT_CHOOSE "↑↓ 选词    短按确定 选中"
#define EAT_TEXT_ADD_HINT_SAVE "长按确定 保存    短按确定 追加"
#define EAT_TEXT_ADD_HINT_SUFFIXED "长按确定 保存    已追加短名称"
#define EAT_TEXT_TOAST_ADDED "已加入轮盘 ★"
#define EAT_TEXT_TOAST_REPEAT "这个已经有了"

// 名称连接符（U+00B7），用于“品类·短名称”。
#define EAT_TEXT_JOINER "·"

// 添加页的高频关键词。第一版没有键盘输入，只在这张表里选。
extern const char *const EAT_CANDIDATE_WORDS[];
extern const size_t EAT_CANDIDATE_COUNT;
