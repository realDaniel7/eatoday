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
    INCLUDE_DIRS "."
    REQUIRES bsp esp_timer lvgl nvs_flash
)

# 中文子集字体由 tools/gen_eat_what_fonts.sh 生成到 assets/fonts/，
# 字符清单来自 main/eat_what_text.h、main/eat_what_text.c 与 main/eat_what_seed.c。
target_sources(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_16.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_20.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_24.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_32.c"
    "${CMAKE_CURRENT_LIST_DIR}/../assets/fonts/app_font_44.c"
)
