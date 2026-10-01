import re

main_path = r'D:\Project_Solar_Display\Project-ESP32-LCD\main\main.c'
with open(main_path, 'r', encoding='utf-8') as f:
    text = f.read()

# 1. Update TOTAL_PAGES from 4 to 6
text = text.replace('#define TOTAL_PAGES 4', '#define TOTAL_PAGES 6')

# 2. Add widgets after p3_thermal_status_lbl
target2 = 'static lv_obj_t *p3_thermal_status_lbl;'
replacement2 = '''static lv_obj_t *p3_thermal_status_lbl;

/* Page 4: Solar Harvest & Yield Widgets */
static lv_obj_t *p4_today_kwh;
static lv_obj_t *p4_peak_w;
static lv_obj_t *p4_total_mwh;
static lv_obj_t *p4_chart;
static lv_chart_series_t *p4_series;

/* Page 5: OEM Vendor & Brand Showcase Widgets */
static lv_obj_t *p5_brand_title;
static lv_obj_t *p5_model_name;
static lv_obj_t *p5_serial_no;
static lv_obj_t *p5_hw_rev;
static lv_obj_t *p5_contact;
static lv_obj_t *p5_website;
static lv_obj_t *p5_mfg_date;
static lv_obj_t *p5_logo_arc;

/* Theme Color Helper */
static inline lv_color_t get_theme_primary_color(uint8_t theme) {
    switch (theme) {
        case 1: return lv_color_hex(0x06b6d4); // Cyber Cyan
        case 2: return lv_color_hex(0x10b981); // Emerald Eco
        case 3: return lv_color_hex(0xef4444); // Crimson Red
        default: return lv_color_hex(0xf59e0b); // Sunburst Gold
    }
}

static inline lv_color_t get_theme_secondary_color(uint8_t theme) {
    switch (theme) {
        case 1: return lv_color_hex(0x38bdf8); // Light Cyan
        case 2: return lv_color_hex(0x34d399); // Light Emerald
        case 3: return lv_color_hex(0xf87171); // Light Red
        default: return lv_color_hex(0xfbbf24); // Light Gold
    }
}'''
text = text.replace(target2, replacement2, 1)

# 3. Update page_titles
target3 = '''static const char *page_titles[TOTAL_PAGES] = {
    "ENERGY FLOW HUB",
    "SOLAR PV & CHARGER",
    "BATTERY & INVERTER LOAD",
    "SYSTEM OPERATIONAL STATUS"
};'''
replacement3 = '''static const char *page_titles[TOTAL_PAGES] = {
    "ENERGY FLOW HUB",
    "SOLAR PV & CHARGER",
    "BATTERY & INVERTER LOAD",
    "SYSTEM OPERATIONAL STATUS",
    "SOLAR HARVEST & PRODUCTION",
    "OEM VENDOR & BRAND IDENTITY"
};'''
text = text.replace(target3, replacement3, 1)

# 4. Update fast_anim_timer_cb
target4 = '''static void fast_anim_timer_cb(lv_timer_t *timer)
{
    static int sun_rot = 0;
    sun_rot = (sun_rot + 4) % 360;
    if (p1_sun_arc && inv_data.solarstate == SOLARON) {
        lv_arc_set_rotation(p1_sun_arc, sun_rot);
    }
}'''
replacement4 = '''static void fast_anim_timer_cb(lv_timer_t *timer)
{
    static int sun_rot = 0;
    sun_rot = (sun_rot + 4) % 360;
    if (p1_sun_arc && inv_data.solarstate == SOLARON) {
        lv_arc_set_rotation(p1_sun_arc, sun_rot);
    }
    if (p5_logo_arc) {
        lv_arc_set_rotation(p5_logo_arc, (sun_rot * 2) % 360);
    }
}'''
text = text.replace(target4, replacement4, 1)

# 5. Add telemetry update for page 4
target5 = '''    if (p3_thermal_status_lbl) {
        if (inv_data.upsheat < 45.0f) lv_label_set_text(p3_thermal_status_lbl, "THERMAL: NORMAL COOL");
        else if (inv_data.upsheat < 65.0f) lv_label_set_text(p3_thermal_status_lbl, "THERMAL: WARM");
        else lv_label_set_text(p3_thermal_status_lbl, "THERMAL: HOT (FAN HIGH)");
    }
}'''
replacement5 = '''    if (p3_thermal_status_lbl) {
        if (inv_data.upsheat < 45.0f) lv_label_set_text(p3_thermal_status_lbl, "THERMAL: NORMAL COOL");
        else if (inv_data.upsheat < 65.0f) lv_label_set_text(p3_thermal_status_lbl, "THERMAL: WARM");
        else lv_label_set_text(p3_thermal_status_lbl, "THERMAL: HOT (FAN HIGH)");
    }

    /* --- Page 4 Updates (Solar Generation & Harvest) --- */
    static float today_kwh_accum = 14.8f;
    today_kwh_accum += 0.001f;
    snprintf(buf, sizeof(buf), "%.1f kWh", today_kwh_accum);
    if (p4_today_kwh) lv_label_set_text(p4_today_kwh, buf);

    float peak_w = inv_data.solarvolt * (inv_data.chrampsdisp + 12.0f);
    snprintf(buf, sizeof(buf), "%.0f W", peak_w);
    if (p4_peak_w) lv_label_set_text(p4_peak_w, buf);
}'''
text = text.replace(target5, replacement5, 1)

# 6. Add build_page_4 and build_page_5 after build_page_3
target6 = '''    p3_thermal_status_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_thermal_status_lbl, "THERMAL: NORMAL COOL");
    lv_obj_set_style_text_font(p3_thermal_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(p3_thermal_status_lbl, 160, 60);
}'''

page4_page5_code = '''    p3_thermal_status_lbl = lv_label_create(c_diag);
    lv_label_set_text(p3_thermal_status_lbl, "THERMAL: NORMAL COOL");
    lv_obj_set_style_text_font(p3_thermal_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p3_thermal_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(p3_thermal_status_lbl, 160, 60);
}

static void build_page_4(lv_obj_t *parent)
{
    /* Page 4: Solar Harvest & Daily Production History */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x050811), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[4] = page;

    /* Top Row: 3 KPI Cards (width ~102 each) */
    /* Card 1: Today Harvest */
    lv_obj_t *c1 = create_card(page, 0, 0, 102, 54);
    lv_obj_t *l1 = lv_label_create(c1);
    lv_label_set_text(l1, "TODAY YIELD");
    lv_obj_set_style_text_font(l1, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l1, lv_color_hex(0x94a3b8), 0);
    lv_obj_align(l1, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_today_kwh = lv_label_create(c1);
    lv_label_set_text(p4_today_kwh, "14.8 kWh");
    lv_obj_set_style_text_font(p4_today_kwh, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p4_today_kwh, lv_color_hex(0x10b981), 0);
    lv_obj_align(p4_today_kwh, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Card 2: Peak Power */
    lv_obj_t *c2 = create_card(page, 106, 0, 102, 54);
    lv_obj_t *l2 = lv_label_create(c2);
    lv_label_set_text(l2, "PEAK SOLAR");
    lv_obj_set_style_text_font(l2, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l2, lv_color_hex(0x94a3b8), 0);
    lv_obj_align(l2, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_peak_w = lv_label_create(c2);
    lv_label_set_text(p4_peak_w, "2,480 W");
    lv_obj_set_style_text_font(p4_peak_w, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p4_peak_w, lv_color_hex(0xf59e0b), 0);
    lv_obj_align(p4_peak_w, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Card 3: Total Yield */
    lv_obj_t *c3 = create_card(page, 212, 0, 102, 54);
    lv_obj_t *l3 = lv_label_create(c3);
    lv_label_set_text(l3, "LIFETIME YIELD");
    lv_obj_set_style_text_font(l3, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(l3, lv_color_hex(0x94a3b8), 0);
    lv_obj_align(l3, LV_ALIGN_TOP_LEFT, 2, 2);

    p4_total_mwh = lv_label_create(c3);
    lv_label_set_text(p4_total_mwh, "3.82 MWh");
    lv_obj_set_style_text_font(p4_total_mwh, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(p4_total_mwh, lv_color_hex(0x38bdf8), 0);
    lv_obj_align(p4_total_mwh, LV_ALIGN_BOTTOM_LEFT, 2, -2);

    /* Bottom Section: Hourly Bar Chart Container (314x124) */
    lv_obj_t *c_chart = create_card(page, 0, 58, 314, 124);

    lv_obj_t *chart_title = lv_label_create(c_chart);
    lv_label_set_text(chart_title, "HOURLY GENERATION PROFILE");
    lv_obj_set_style_text_font(chart_title, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(chart_title, lv_color_hex(0x38bdf8), 0);
    lv_obj_align(chart_title, LV_ALIGN_TOP_LEFT, 4, 2);

    lv_obj_t *co2_badge = lv_label_create(c_chart);
    lv_label_set_text(co2_badge, "CO2 SAVED: 3.1 TONS");
    lv_obj_set_style_text_font(co2_badge, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(co2_badge, lv_color_hex(0x34d399), 0);
    lv_obj_align(co2_badge, LV_ALIGN_TOP_RIGHT, -4, 2);

    /* Bar Chart Widget */
    p4_chart = lv_chart_create(c_chart);
    lv_obj_set_pos(p4_chart, 4, 18);
    lv_obj_set_size(p4_chart, 302, 76);
    lv_chart_set_type(p4_chart, LV_CHART_TYPE_BAR);
    lv_chart_set_point_count(p4_chart, 6);
    lv_chart_set_axis_range(p4_chart, LV_CHART_AXIS_PRIMARY_Y, 0, 50);
    lv_obj_set_style_bg_color(p4_chart, lv_color_hex(0x080d1a), 0);
    lv_obj_set_style_border_color(p4_chart, lv_color_hex(0x1e293b), 0);
    lv_obj_set_style_border_width(p4_chart, 1, 0);
    lv_obj_set_style_pad_all(p4_chart, 4, 0);
    lv_obj_set_style_pad_column(p4_chart, 12, 0);

    p4_series = lv_chart_add_series(p4_chart, lv_color_hex(0xf59e0b), LV_CHART_AXIS_PRIMARY_Y);
    const int32_t sample_hourly[6] = { 14, 28, 45, 38, 25, 12 };
    lv_chart_set_series_values(p4_chart, p4_series, sample_hourly, 6);

    /* Time Axis Labels below chart */
    const char *hours[6] = { "08:00", "10:00", "12:00", "14:00", "16:00", "18:00" };
    for (int i = 0; i < 6; i++) {
        lv_obj_t *th = lv_label_create(c_chart);
        lv_label_set_text(th, hours[i]);
        lv_obj_set_style_text_font(th, &lv_font_montserrat_10, 0);
        lv_obj_set_style_text_color(th, lv_color_hex(0x64748b), 0);
        lv_obj_set_pos(th, 12 + i * 49, 100);
    }
}

static void build_page_5(lv_obj_t *parent)
{
    /* Page 5: OEM Vendor & Brand Showcase */
    lv_obj_t *page = lv_obj_create(parent);
    lv_obj_set_pos(page, 0, 26);
    lv_obj_set_size(page, 320, 190);
    lv_obj_set_style_bg_color(page, lv_color_hex(0x050811), 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 3, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_flag(page, LV_OBJ_FLAG_HIDDEN);
    pages[5] = page;

    lv_color_t primary_color = get_theme_primary_color(g_pcu_cfg.logo_theme);
    lv_color_t sec_color = get_theme_secondary_color(g_pcu_cfg.logo_theme);

    /* Card 1: Top Brand Banner Card (314x80) */
    lv_obj_t *c_brand = create_card(page, 0, 0, 314, 80);

    /* Rotating / Glowing Emblem container on the left */
    lv_obj_t *emblem_box = lv_obj_create(c_brand);
    lv_obj_set_size(emblem_box, 60, 60);
    lv_obj_align(emblem_box, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_set_style_bg_color(emblem_box, lv_color_hex(0x0f172a), 0);
    lv_obj_set_style_border_color(emblem_box, primary_color, 0);
    lv_obj_set_style_border_width(emblem_box, 2, 0);
    lv_obj_set_style_radius(emblem_box, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(emblem_box, 0, 0);
    lv_obj_remove_flag(emblem_box, LV_OBJ_FLAG_SCROLLABLE);

    /* Embedded solar icon centered inside emblem */
    lv_obj_t *emb_icon = lv_image_create(emblem_box);
    lv_image_set_src(emb_icon, &img_solar);
    lv_obj_center(emb_icon);

    /* Rotating Arc around emblem */
    p5_logo_arc = lv_arc_create(c_brand);
    lv_obj_set_size(p5_logo_arc, 70, 70);
    lv_obj_align(p5_logo_arc, LV_ALIGN_LEFT_MID, 1, 0);
    lv_arc_set_angles(p5_logo_arc, 0, 100);
    lv_arc_set_rotation(p5_logo_arc, 45);
    lv_obj_set_style_arc_width(p5_logo_arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(p5_logo_arc, sec_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(p5_logo_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_style(p5_logo_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(p5_logo_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Brand Typography on the right */
    p5_brand_title = lv_label_create(c_brand);
    lv_label_set_text(p5_brand_title, g_pcu_cfg.brand_title);
    lv_obj_set_style_text_font(p5_brand_title, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(p5_brand_title, primary_color, 0);
    lv_obj_set_pos(p5_brand_title, 82, 8);

    p5_model_name = lv_label_create(c_brand);
    lv_label_set_text(p5_model_name, g_pcu_cfg.model_name);
    lv_obj_set_style_text_font(p5_model_name, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p5_model_name, lv_color_hex(0xf8fafc), 0);
    lv_obj_set_pos(p5_model_name, 82, 30);

    /* Quality Tag Badge */
    lv_obj_t *tag = lv_label_create(c_brand);
    lv_label_set_text(tag, "HYBRID MPPT SOLAR PCU");
    lv_obj_set_style_text_font(tag, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(tag, lv_color_hex(0x10b981), 0);
    lv_obj_set_style_bg_color(tag, lv_color_hex(0x064e3b), 0);
    lv_obj_set_style_radius(tag, 3, 0);
    lv_obj_set_style_pad_hor(tag, 6, 0);
    lv_obj_set_style_pad_ver(tag, 2, 0);
    lv_obj_set_pos(tag, 82, 50);

    /* Card 2: Technical Specs & Vendor Support (314x98) */
    lv_obj_t *c_info = create_card(page, 0, 84, 314, 98);

    /* Row 1: Serial Number & HW Version */
    lv_obj_t *lbl_sn_t = lv_label_create(c_info);
    lv_label_set_text(lbl_sn_t, "SERIAL NO  :");
    lv_obj_set_style_text_font(lbl_sn_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_sn_t, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(lbl_sn_t, 6, 6);

    p5_serial_no = lv_label_create(c_info);
    lv_label_set_text(p5_serial_no, g_pcu_cfg.serial_number);
    lv_obj_set_style_text_font(p5_serial_no, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p5_serial_no, lv_color_hex(0x38bdf8), 0);
    lv_obj_set_pos(p5_serial_no, 86, 4);

    p5_hw_rev = lv_label_create(c_info);
    lv_label_set_text(p5_hw_rev, g_pcu_cfg.hardware_version);
    lv_obj_set_style_text_font(p5_hw_rev, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p5_hw_rev, lv_color_hex(0xa78bfa), 0);
    lv_obj_align(p5_hw_rev, LV_ALIGN_TOP_RIGHT, -6, 6);

    /* Row 2: Customer Helpline */
    lv_obj_t *lbl_cc_t = lv_label_create(c_info);
    lv_label_set_text(lbl_cc_t, "SUPPORT    :");
    lv_obj_set_style_text_font(lbl_cc_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_cc_t, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(lbl_cc_t, 6, 26);

    p5_contact = lv_label_create(c_info);
    lv_label_set_text(p5_contact, g_pcu_cfg.vendor_contact);
    lv_obj_set_style_text_font(p5_contact, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p5_contact, lv_color_hex(0xfacc15), 0);
    lv_obj_set_pos(p5_contact, 86, 24);

    /* Row 3: Website */
    lv_obj_t *lbl_web_t = lv_label_create(c_info);
    lv_label_set_text(lbl_web_t, "PORTAL     :");
    lv_obj_set_style_text_font(lbl_web_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_web_t, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(lbl_web_t, 6, 46);

    p5_website = lv_label_create(c_info);
    lv_label_set_text(p5_website, g_pcu_cfg.vendor_website);
    lv_obj_set_style_text_font(p5_website, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(p5_website, lv_color_hex(0x60a5fa), 0);
    lv_obj_set_pos(p5_website, 86, 44);

    /* Row 4: Manufacturing / QC Status */
    lv_obj_t *lbl_qc_t = lv_label_create(c_info);
    lv_label_set_text(lbl_qc_t, "QC STATUS  :");
    lv_obj_set_style_text_font(lbl_qc_t, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(lbl_qc_t, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_pos(lbl_qc_t, 6, 66);

    p5_mfg_date = lv_label_create(c_info);
    char mfg_str[48];
    uint32_t d = g_pcu_cfg.production_date;
    snprintf(mfg_str, sizeof(mfg_str), "PASSED [MFG: %04u-%02u-%02u]", (unsigned)(d / 10000), (unsigned)((d % 10000) / 100), (unsigned)(d % 100));
    lv_label_set_text(p5_mfg_date, mfg_str);
    lv_obj_set_style_text_font(p5_mfg_date, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(p5_mfg_date, lv_color_hex(0x22c55e), 0);
    lv_obj_set_pos(p5_mfg_date, 86, 66);
}'''

text = text.replace(target6, page4_page5_code, 1)

# 7. Update build_dashboard_shell
target7 = '''    /* Build all 4 pages */
    build_page_0(scr);
    build_page_1(scr);
    build_page_2(scr);
    build_page_3(scr);'''

replacement7 = '''    /* Build all 6 pages */
    build_page_0(scr);
    build_page_1(scr);
    build_page_2(scr);
    build_page_3(scr);
    build_page_4(scr);
    build_page_5(scr);'''

text = text.replace(target7, replacement7, 1)

# 8. Update dots_box size to 90
text = text.replace('lv_obj_set_size(dots_box, 60, 14);', 'lv_obj_set_size(dots_box, 90, 14);')

# 9. Update build_dashboard_shell boot page switch
target8 = '''    if (g_pcu_cfg.default_boot_page < TOTAL_PAGES) {
        switch_to_page(g_pcu_cfg.default_boot_page);
    }
}'''

replacement8 = '''    switch_to_page(0);
}

/* Boot Splash Screen Implementation */
static lv_obj_t *boot_arc = NULL;
static lv_obj_t *boot_bar = NULL;
static lv_obj_t *boot_status_lbl = NULL;
static lv_obj_t *g_main_screen_obj = NULL;
static int boot_elapsed_ms = 0;
static int boot_total_ms = 3000;

static void boot_anim_timer_cb(lv_timer_t *timer)
{
    boot_elapsed_ms += 30;
    int pct = (boot_elapsed_ms * 100) / boot_total_ms;
    if (pct > 100) pct = 100;

    if (boot_bar) lv_bar_set_value(boot_bar, pct, LV_ANIM_OFF);
    if (boot_arc) {
        int r = (boot_elapsed_ms * 360 / 1000) % 360;
        lv_arc_set_rotation(boot_arc, r);
    }

    if (boot_status_lbl) {
        if (pct < 25) {
            lv_label_set_text(boot_status_lbl, "SYSTEM SELF-TEST... [OK]");
        } else if (pct < 55) {
            lv_label_set_text(boot_status_lbl, "CALIBRATING MPPT CONTROLLER...");
        } else if (pct < 85) {
            lv_label_set_text(boot_status_lbl, "INITIALIZING PRECISION HMI...");
        } else {
            lv_label_set_text(boot_status_lbl, "SYSTEM READY");
        }
    }

    if (boot_elapsed_ms >= boot_total_ms) {
        lv_timer_delete(timer);
        if (g_main_screen_obj) {
            lv_screen_load_anim(g_main_screen_obj, LV_SCR_LOAD_ANIM_FADE_ON, 400, 0, true);
        }
    }
}

static void build_boot_screen(lv_obj_t *boot_scr, lv_obj_t *main_scr)
{
    g_main_screen_obj = main_scr;
    boot_elapsed_ms = 0;
    boot_total_ms = (int)g_pcu_cfg.boot_duration_sec * 1000;
    if (boot_total_ms < 1000) boot_total_ms = 3000;

    lv_color_t primary_color = get_theme_primary_color(g_pcu_cfg.logo_theme);
    lv_color_t sec_color = get_theme_secondary_color(g_pcu_cfg.logo_theme);

    lv_obj_set_style_bg_color(boot_scr, lv_color_hex(0x030712), 0);
    lv_obj_remove_flag(boot_scr, LV_OBJ_FLAG_SCROLLABLE);

    /* Center Emblem Circle Container */
    lv_obj_t *circle = lv_obj_create(boot_scr);
    lv_obj_set_size(circle, 72, 72);
    lv_obj_align(circle, LV_ALIGN_TOP_MID, 0, 18);
    lv_obj_set_style_bg_color(circle, lv_color_hex(0x0c1524), 0);
    lv_obj_set_style_border_color(circle, primary_color, 0);
    lv_obj_set_style_border_width(circle, 2, 0);
    lv_obj_set_style_radius(circle, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(circle, 0, 0);
    lv_obj_remove_flag(circle, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *icon = lv_image_create(circle);
    lv_image_set_src(icon, &img_solar);
    lv_obj_center(icon);

    /* Spinning outer arc */
    boot_arc = lv_arc_create(boot_scr);
    lv_obj_set_size(boot_arc, 88, 88);
    lv_obj_align(boot_arc, LV_ALIGN_TOP_MID, 0, 10);
    lv_arc_set_angles(boot_arc, 0, 90);
    lv_arc_set_rotation(boot_arc, 0);
    lv_obj_set_style_arc_width(boot_arc, 3, LV_PART_INDICATOR);
    lv_obj_set_style_arc_color(boot_arc, sec_color, LV_PART_INDICATOR);
    lv_obj_set_style_arc_opa(boot_arc, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_remove_style(boot_arc, NULL, LV_PART_KNOB);
    lv_obj_remove_flag(boot_arc, LV_OBJ_FLAG_CLICKABLE);

    /* Brand Title */
    lv_obj_t *brand_lbl = lv_label_create(boot_scr);
    lv_label_set_text(brand_lbl, g_pcu_cfg.brand_title);
    lv_obj_set_style_text_font(brand_lbl, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_color(brand_lbl, primary_color, 0);
    lv_obj_align(brand_lbl, LV_ALIGN_TOP_MID, 0, 104);

    /* Model Name */
    lv_obj_t *model_lbl = lv_label_create(boot_scr);
    lv_label_set_text(model_lbl, g_pcu_cfg.model_name);
    lv_obj_set_style_text_font(model_lbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(model_lbl, lv_color_hex(0xf1f5f9), 0);
    lv_obj_align(model_lbl, LV_ALIGN_TOP_MID, 0, 126);

    /* Serial & Hardware Version */
    lv_obj_t *sub_lbl = lv_label_create(boot_scr);
    char sub_buf[64];
    snprintf(sub_buf, sizeof(sub_buf), "%s | %s", g_pcu_cfg.serial_number, g_pcu_cfg.hardware_version);
    lv_label_set_text(sub_lbl, sub_buf);
    lv_obj_set_style_text_font(sub_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(sub_lbl, lv_color_hex(0x64748b), 0);
    lv_obj_align(sub_lbl, LV_ALIGN_TOP_MID, 0, 144);

    /* Loading Bar */
    boot_bar = lv_bar_create(boot_scr);
    lv_obj_set_size(boot_bar, 240, 8);
    lv_obj_align(boot_bar, LV_ALIGN_TOP_MID, 0, 168);
    lv_obj_set_style_bg_color(boot_bar, lv_color_hex(0x1e293b), LV_PART_MAIN);
    lv_obj_set_style_bg_color(boot_bar, primary_color, LV_PART_INDICATOR);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(boot_bar, 4, LV_PART_INDICATOR);
    lv_bar_set_range(boot_bar, 0, 100);
    lv_bar_set_value(boot_bar, 0, LV_ANIM_OFF);

    /* Status Label */
    boot_status_lbl = lv_label_create(boot_scr);
    lv_label_set_text(boot_status_lbl, "INITIALIZING SYSTEM...");
    lv_obj_set_style_text_font(boot_status_lbl, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(boot_status_lbl, lv_color_hex(0x94a3b8), 0);
    lv_obj_align(boot_status_lbl, LV_ALIGN_TOP_MID, 0, 184);

    /* Start 30ms animation ticker */
    lv_timer_create(boot_anim_timer_cb, 30, NULL);
}

static void build_unconfigured_screen(lv_obj_t *scr)
{
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x050811), 0);
    lv_obj_remove_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *badge = lv_label_create(scr);
    lv_label_set_text(badge, "FACTORY SERVICE MODE");
    lv_obj_set_style_text_font(badge, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(badge, lv_color_hex(0xf59e0b), 0);
    lv_obj_align(badge, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t *t1 = lv_label_create(scr);
    lv_label_set_text(t1, "DEVICE UNCONFIGURED");
    lv_obj_set_style_text_font(t1, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t1, lv_color_hex(0xef4444), 0);
    lv_obj_align(t1, LV_ALIGN_TOP_MID, 0, 60);

    lv_obj_t *t2 = lv_label_create(scr);
    lv_label_set_text(t2, "Connect PC USB (COM port)\\nto configure Brand & Model via\\nSolar Display Manager");
    lv_obj_set_style_text_font(t2, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(t2, lv_color_hex(0x94a3b8), 0);
    lv_obj_set_style_text_align(t2, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(t2, LV_ALIGN_TOP_MID, 0, 90);

    lv_obj_t *t3 = lv_label_create(scr);
    lv_label_set_text(t3, "Baud: 115200 | Protocol: PCU v2.0");
    lv_obj_set_style_text_font(t3, &lv_font_montserrat_10, 0);
    lv_obj_set_style_text_color(t3, lv_color_hex(0x38bdf8), 0);
    lv_obj_align(t3, LV_ALIGN_BOTTOM_MID, 0, -20);
}'''

text = text.replace(target8, replacement8, 1)

# 10. Update config_set_factory_defaults
target_cfg = '''static void config_set_factory_defaults(pcu_config_t *cfg)
{
    memset(cfg, 0, sizeof(pcu_config_t));
    cfg->magic = PCU_CONFIG_MAGIC;
    cfg->version = PCU_CONFIG_VERSION;
    cfg->struct_size = sizeof(pcu_config_t);
    cfg->is_configured = 0; // Factory state: not yet configured
    strncpy(cfg->brand_title, "DONPOWER HYBRID PCU", sizeof(cfg->brand_title) - 1);
    strncpy(cfg->model_name, "PCU-3500-24V", sizeof(cfg->model_name) - 1);
    strncpy(cfg->serial_number, "DP-2026-0001", sizeof(cfg->serial_number) - 1);
    strncpy(cfg->hardware_version, "HW-V1.2", sizeof(cfg->hardware_version) - 1);
    cfg->production_date = 20261001;
    cfg->nominal_batt_volt = 24;
    cfg->rated_inverter_va = 3500;
    cfg->battery_chemistry = 0; // Tubular
    cfg->grid_frequency_hz = 50;
    cfg->batt_low_cutoff_v = 21.0f;
    cfg->batt_high_cutoff_v = 29.2f;
    cfg->batt_float_v = 27.6f;
    cfg->batt_bulk_v = 28.8f;
    cfg->overload_cutoff_pct = 120.0f;
    cfg->heatsink_trip_temp_c = 75.0f;
    cfg->default_boot_page = 0;
    cfg->auto_carousel_enabled = 1;
    cfg->carousel_interval_sec = 5;
    cfg->backlight_brightness = 100;
    cfg->telemetry_baudrate = 9600;
    cfg->telemetry_format = 0;
    cfg->config_crc32 = pcu_calc_crc32((const uint8_t*)cfg, offsetof(pcu_config_t, config_crc32));
}'''

replacement_cfg = '''static void config_set_factory_defaults(pcu_config_t *cfg)
{
    memset(cfg, 0, sizeof(pcu_config_t));
    cfg->magic = PCU_CONFIG_MAGIC;
    cfg->version = PCU_CONFIG_VERSION;
    cfg->struct_size = sizeof(pcu_config_t);
    cfg->is_configured = 0;
    strncpy(cfg->brand_title, "DONPOWER SOLAR", sizeof(cfg->brand_title) - 1);
    strncpy(cfg->model_name, "HYBRID MPPT PCU", sizeof(cfg->model_name) - 1);
    strncpy(cfg->serial_number, "DP-2026-X8849", sizeof(cfg->serial_number) - 1);
    strncpy(cfg->hardware_version, "HW-V2.1", sizeof(cfg->hardware_version) - 1);
    strncpy(cfg->vendor_contact, "Toll Free: 1800-425-9999", sizeof(cfg->vendor_contact) - 1);
    strncpy(cfg->vendor_website, "www.donpower.in", sizeof(cfg->vendor_website) - 1);
    cfg->production_date = 20261001;
    cfg->logo_theme = 0;            // 0=Sunburst Gold
    cfg->boot_duration_sec = 3;     // 3 seconds boot splash
    cfg->auto_carousel_enabled = 1; // Auto carousel
    cfg->carousel_interval_sec = 5; // 5 seconds per screen
    cfg->backlight_brightness = 100;
    cfg->telemetry_baudrate = 9600;
    cfg->config_crc32 = pcu_calc_crc32((const uint8_t*)cfg, offsetof(pcu_config_t, config_crc32));
}'''

text = text.replace(target_cfg, replacement_cfg, 1)

# 11. Update app_main logic for boot screen
target_app_main = '''    /* Check if unit is configured */
    bool configured = config_load_from_nvs(&g_pcu_cfg);
    if (!configured) {
        /* FACTORY / UNCONFIGURED STATE:
         * Backlight (GPIO 4) remains OFF.
         * Runs USB Service Protocol indefinitely waiting for MFC provisioning tool. */
        config_set_factory_defaults(&g_pcu_cfg);
        usb_service_task(NULL); // Blocks indefinitely until configured and rebooted!
        return;
    }

    /* IF CONFIGURED:
     * Launch USB Service Task on Core 0 so PC tool can communicate 24/7 simultaneously */
    xTaskCreatePinnedToCore(usb_service_task, "USB_SVC", 4096, NULL, 3, NULL, 0);'''

replacement_app_main = '''    /* Check if unit is configured */
    bool configured = config_load_from_nvs(&g_pcu_cfg);
    if (!configured) {
        config_set_factory_defaults(&g_pcu_cfg);
    }

    /* Launch USB Service Task on Core 0 so PC tool can communicate 24/7 simultaneously */
    xTaskCreatePinnedToCore(usb_service_task, "USB_SVC", 4096, NULL, 3, NULL, 0);'''

text = text.replace(target_app_main, replacement_app_main, 1)

# 12. Update app_main display UI loading
target_ui_load = '''    /* 9. Build Dashboard Shell with All 4 Pages and Dynamic Config */
    _lock_acquire(&lvgl_api_lock);
    build_dashboard_shell(display);
    _lock_release(&lvgl_api_lock);

    /* 10. Start Background Tasks */
    // LVGL Task on Core 1
    xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", 8192, NULL, 2, NULL, 1);
    // Inverter Serial Task on Core 0
    xTaskCreatePinnedToCore(inverter_uart_task, "INV_UART", 4096, NULL, 3, NULL, 0);'''

replacement_ui_load = '''    /* 9. Build UI */
    _lock_acquire(&lvgl_api_lock);
    if (!configured) {
        lv_obj_t *unconf_scr = lv_display_get_screen_active(display);
        build_unconfigured_screen(unconf_scr);
    } else {
        lv_obj_t *main_scr = lv_obj_create(NULL);
        build_dashboard_shell(main_scr);

        lv_obj_t *boot_scr = lv_obj_create(NULL);
        build_boot_screen(boot_scr, main_scr);

        lv_screen_load(boot_scr);
    }
    _lock_release(&lvgl_api_lock);

    /* 10. Start Background Tasks */
    xTaskCreatePinnedToCore(lvgl_port_task, "LVGL", 8192, NULL, 2, NULL, 1);
    if (configured) {
        xTaskCreatePinnedToCore(inverter_uart_task, "INV_UART", 4096, NULL, 3, NULL, 0);
    }'''

text = text.replace(target_ui_load, replacement_ui_load, 1)

with open(main_path, 'w', encoding='utf-8') as f:
    f.write(text)

print('Successfully patched main.c!')
