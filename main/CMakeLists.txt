idf_component_register(
    SRCS "main.c"
         "app_fonts.c"
         "eat_ui.c"
         "eat_what_pool.c"
         "eat_what_router.c"
         "eat_what_seed.c"
         "eat_what_store.c"
         "eat_what_text.c"
         "eat_what_wheel.c"
         "eat_sets.c"
         "eat_wifi.c"
         "eat_web.c"
         "eat_web_page.c"
    INCLUDE_DIRS "."
    REQUIRES bsp esp_http_server esp_timer esp_wifi json lvgl nvs_flash
)

# 中文子集字体由 tools/gen_eat_what_fonts.sh 生成到 assets/fonts/，
# 字符清单来自 main/eat_what_text.h、main/eat_what_text.c 与 main/eat_what_seed.c。
# 自定义条目用的运行时 TTF（GB2312 一级 3755 字）：以只读数据嵌入固件，
# 由 LVGL 的 Tiny TTF 在运行时栅格化，因此换新店名不需要重新生成字体。
target_add_binary_data(${COMPONENT_LIB} "../assets/fonts/noto-sc-gb2312.ttf" TEXT)

target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_16.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_20.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_24.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_32.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_44.c"
)
