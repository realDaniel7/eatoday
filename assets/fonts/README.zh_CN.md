<p align="right">
  <strong>简体中文</strong> · <a href="README.md">English</a>
</p>

# 应用字体

`app_font_<size>.c` 是“今天吃什么”（`feature/eat-what` 分支）用的 LVGL 位图字体源文件。
它们随仓库提交，因为生成过程依赖 Node.js 与一份有授权的 CJK 轮廓字体，而固件构建不应依赖这两者。

| 文件 | 字号 | 用途 |
| --- | --- | --- |
| `app_font_16.c` | 16 px / 4 bpp | 标题、电量、统计行、按键提示 |
| `app_font_20.c` | 20 px / 4 bpp | 添加页提问、候选词行 |
| `app_font_24.c` | 24 px / 4 bpp | 中央卡片里的长条目 |
| `app_font_32.c` | 32 px / 4 bpp | 中等长度条目、已输入文字 |
| `app_font_44.c` | 44 px / 4 bpp | 短条目（界面视觉焦点） |

五个文件合计约 2.3 MB 只读 Flash；除 LVGL 字形缓存外不占内部 RAM。

## 来源与许可

- 源字体：Noto Sans SC Regular（Subset OTF），文件 `NotoSansSC-Regular.otf`，
  来自 [noto-cjk](https://github.com/notofonts/noto-cjk) 仓库。
- 许可证：SIL Open Font License 1.1，见 [`OFL.txt`](OFL.txt)。生成的子集是该字体的衍生作品，
  沿用同一许可证。
- 8 MB 的 `.otf` 源文件不入库，只提交生成的子集与许可证。

## 字符清单

`tools/eat_what_glyphs.py` 从 `main/eat_what_text.h`、`main/eat_what_text.c`
与 `main/eat_what_seed.c` 的字符串字面量里收集所有非 ASCII 字符。这三个文件正是
文字会显示到 240 × 320 屏幕上的地方；日志与注释被排除，避免把字体撑大。
ASCII `0x20`–`0x7E` 始终包含，用于数字、`%` 与 `--`。

## 重新生成

```bash
npm install lv_font_conv@1.5.3            # Node.js 转换器，不是固件依赖
EAT_FONT_SOURCE=/path/to/NotoSansSC-Regular.otf ./tools/gen_eat_what_fonts.sh
```

脚本会用 `--bpp 4 --format lvgl --no-compress` 生成 16、20、24、32、44 五档
`assets/fonts/app_font_<size>.c`。

## 接入方式

`main/CMakeLists.txt` 用 `target_sources` 把这五个源文件加入 `main` 组件，
`main/app_fonts.h` 用 `LV_FONT_DECLARE` 声明它们；`eat_font_for_name()` 按字符数选档。

## 校验

`tests/test_eat_what_font_coverage.py` 解析每个生成文件的 cmap 结构，缺字即失败，
并带有负向用例（U+9F98 必须不存在），保证这条检查不会恒真。
屏幕上的实际渲染仍需在真机上确认，见交付说明。
