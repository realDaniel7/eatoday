// main/eat_what_text.c —— 候选词表的数据部分（文案宏在头文件里）。
#include "eat_what_text.h"

const char *const EAT_CANDIDATE_WORDS[] = {
    "拉面", "盖饭", "饺子", "烤鸭", "火锅", "麻辣烫",
    "汉堡", "米线", "粥", "炒菜", "烤串", "包子",
};

const size_t EAT_CANDIDATE_COUNT =
    sizeof(EAT_CANDIDATE_WORDS) / sizeof(EAT_CANDIDATE_WORDS[0]);
