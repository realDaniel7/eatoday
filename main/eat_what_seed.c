// main/eat_what_seed.c —— 30 项种子池（首次启动写入 NVS 的出厂内容）。
#include "eat_what_seed.h"

// 顺序即轮盘顺序：京菜烤鸭 → 小吃快餐 → 火锅 → 川味 → 家常 → 亲民小馆 → 轻决策兜底。
const eat_seed_t EAT_SEED_POOL[] = {
    { "四季民福", EAT_CAT_ROAST_DUCK },
    { "便宜坊", EAT_CAT_ROAST_DUCK },
    { "大鸭梨", EAT_CAT_ROAST_DUCK },
    { "紫光园", EAT_CAT_ROAST_DUCK },

    { "庆丰包子铺", EAT_CAT_SNACK },
    { "方砖厂69号炸酱面", EAT_CAT_SNACK },
    { "嘉和一品", EAT_CAT_SNACK },
    { "西部马华", EAT_CAT_SNACK },
    { "南城香", EAT_CAT_SNACK },
    { "田老师红烧肉", EAT_CAT_SNACK },
    { "老家肉饼", EAT_CAT_SNACK },

    { "东来顺", EAT_CAT_HOTPOT },
    { "呷哺呷哺", EAT_CAT_HOTPOT },

    { "胡大饭馆", EAT_CAT_SICHUAN },
    { "龙人居水煮鱼", EAT_CAT_SICHUAN },
    { "江边城外", EAT_CAT_SICHUAN },
    { "眉州东坡", EAT_CAT_SICHUAN },

    { "小吊梨汤", EAT_CAT_HOMESTYLE },
    { "局气", EAT_CAT_HOMESTYLE },
    { "旺顺阁", EAT_CAT_HOMESTYLE },
    { "郭林家常菜", EAT_CAT_HOMESTYLE },

    { "刘妈妈肉汁饺子", EAT_CAT_LOCAL },
    { "明园餐厅", EAT_CAT_LOCAL },
    { "湘彬萱", EAT_CAT_LOCAL },

    { "麦当劳", EAT_CAT_FALLBACK },
    { "肯德基", EAT_CAT_FALLBACK },
    { "汉堡王", EAT_CAT_FALLBACK },
    { "吉野家", EAT_CAT_FALLBACK },
    { "和合谷", EAT_CAT_FALLBACK },
    { "永和大王", EAT_CAT_FALLBACK },
};

const size_t EAT_SEED_POOL_COUNT = sizeof(EAT_SEED_POOL) / sizeof(EAT_SEED_POOL[0]);

static const char *const CATEGORY_LABELS[EAT_CAT_COUNT] = {
    "京菜烤鸭", "小吃快餐", "火锅", "川味", "家常", "亲民小馆", "轻决策兜底",
};

const char *eat_category_label(eat_category_t category) {
    if ((int)category < 0 || category >= EAT_CAT_COUNT) return "";
    return CATEGORY_LABELS[category];
}
