/*
 * panel-xiaomi-m17.c
 * Upstream-conformant DRM panel driver for the Xiaomi M17 (Poco X5) panel.
 */

#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_connector.h>
#include <drm/drm_modes.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

/* Safe write macro that aborts the init sequence on failure */
#define mipi_dsi_dcs_write_seq_safe(dsi, cmd, seq...)               \
    do {                                                            \
        static const u8 d[] = { seq };                              \
        int _ret = mipi_dsi_dcs_write(dsi, cmd, d, ARRAY_SIZE(d));  \
        if (_ret < 0) {                                             \
            dev_err(&dsi->dev, "Failed to write DCS 0x%02x: %d\n",  \
                    cmd, _ret);                                     \
            return _ret;                                            \
        }                                                           \
    } while (0)

struct xiaomi_m17_panel {
    struct drm_panel panel;
    struct mipi_dsi_device *dsi;
    struct regulator_bulk_data supplies[2];
    struct gpio_desc *reset_gpio;
};

static int xiaomi_m17_configure_dsc(struct mipi_dsi_device *dsi)
{
    struct drm_dsc_config *dsc;

    dsc = devm_kzalloc(&dsi->dev, sizeof(*dsc), GFP_KERNEL);
    if (!dsc)
        return -ENOMEM;

    /* Values extracted from qcom,mdss-dsc-* properties */
    dsc->pic_width = 1080;
    dsc->pic_height = 2400;
    dsc->slice_width = 540;
    dsc->slice_height = 20;
    dsc->slice_count = 2;
    dsc->bits_per_component = 8;
    dsc->bits_per_pixel = 8 << 4; /* U4.4 fixed point */
    dsc->block_pred_enable = true;

    /* Standard DSC 1.1 parameters for 8bpc */
    dsc->dsc_version_major = 1;
    dsc->dsc_version_minor = 1;
    dsc->convert_rgb = 1;
    dsc->native_420 = 0;
    dsc->native_422 = 0;
    dsc->line_buf_depth = 9;
    dsc->rc_model_size = 8192;
    dsc->initial_offset = 6144;

    drm_dsc_compute_rc_parameters(dsc);
    
    /* 
     * Required: Informs the Qualcomm DPU hardware encoder to compress 
     * the stream. (The panel's TCON is configured separately via 0x9E).
     */
    dsi->dsc = dsc;

    return 0;
}

static int xiaomi_m17_panel_prepare(struct drm_panel *panel)
{
    struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
    struct mipi_dsi_device *dsi = ctx->dsi;
    int ret;

    ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret < 0) {
        dev_err(&dsi->dev, "Failed to enable regulators: %d\n", ret);
        return ret;
    }

    msleep(20);

    /* Hardware reset sequence */
    gpiod_set_value(ctx->reset_gpio, 1);
    usleep_range(10000, 11000);
    gpiod_set_value(ctx->reset_gpio, 0);
    msleep(20);
    gpiod_set_value(ctx->reset_gpio, 1);
    msleep(10);

    ret = mipi_dsi_dcs_exit_sleep_mode(dsi);
    if (ret < 0)
        return ret;
        
    msleep(20);

    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb2, 0x01, 0x31);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xdf, 0x09, 0x30, 0x95, 0x46, 0xe9);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0x9d, 0x01);

    /* Xiaomi/Novatek Quirk: DSC PPS must be sent as standard DCS long write */
    mipi_dsi_dcs_write_seq_safe(dsi, 0x9e, 0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09, 0x60,
        0x04, 0x38, 0x00, 0x28, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x00, 0x20, 0x03, 0xdd,
        0x00, 0x07, 0x00, 0x0c, 0x02, 0x77, 0x02, 0x8b, 0x18, 0x00, 0x10, 0xf0, 0x03, 0x0c, 0x20, 0x00,
        0x06, 0x0b, 0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
        0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40, 0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa, 0x19, 0xf8,
        0x1a, 0x38, 0x1a, 0x78, 0x1a, 0xb6, 0x2a, 0xf6, 0x2b, 0x34, 0x2b, 0x74, 0x3b, 0x74, 0x6b, 0xf4,
        0x00);

    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0x60, 0x21);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf7, 0x0b);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x15, 0xf6);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x28, 0xf6);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x3b, 0xf6);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x0a, 0xf4);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf4, 0x98);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x11, 0xf4);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf4, 0xee);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x18, 0xb2);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb2, 0x1c);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xfc, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x11, 0xfe);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xfe, 0x00);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xfc, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x0d, 0xb2);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb2, 0x20);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x0c, 0xb2);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb2, 0x30);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf7, 0x0b);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xfc, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xed, 0x01, 0xcd, 0x00);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xe1, 0x93);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xb0, 0x00, 0x06, 0xf4);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf4, 0x1f);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0xfc, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq_safe(dsi, 0x53, 0x20); 
    
    mipi_dsi_dcs_write_seq_safe(dsi, 0x51, 0x00, 0x00);
    msleep(100);

    ret = mipi_dsi_dcs_set_display_on(dsi);
    if (ret < 0)
        return ret;

    return 0;
}

static int xiaomi_m17_panel_unprepare(struct drm_panel *panel)
{
    struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
    int ret;

    mipi_dsi_dcs_set_display_off(ctx->dsi);
    msleep(20);
    
    mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
    msleep(150);

    gpiod_set_value(ctx->reset_gpio, 1);
    
    ret = regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret < 0)
        dev_err(&ctx->dsi->dev, "Failed to disable regulators: %d\n", ret);

    return 0;
}

/* Exact timings from qcom,mdss-dsi-display-timings */
static const struct drm_display_mode xiaomi_m17_mode = {
    .clock = 393396,
    .hdisplay = 1080,
    .hsync_start = 1080 + 120,
    .hsync_end = 1080 + 120 + 28,
    .htotal = 1080 + 120 + 28 + 120,
    .vdisplay = 2400,
    .vsync_start = 2400 + 20,
    .vsync_end = 2400 + 20 + 2,
    .vtotal = 2400 + 20 + 2 + 10,
    .width_mm = 70,
    .height_mm = 155,
};

static int xiaomi_m17_panel_get_modes(struct drm_panel *panel,
                     struct drm_connector *connector)
{
    struct drm_display_mode *mode;

    mode = drm_mode_duplicate(connector->dev, &xiaomi_m17_mode);
    if (!mode)
        return -ENOMEM;

    drm_mode_set_name(mode);
    mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
    drm_mode_probed_add(connector, mode);

    connector->display_info.width_mm = mode->width_mm;
    connector->display_info.height_mm = mode->height_mm;

    return 1;
}

static const struct drm_panel_funcs xiaomi_m17_panel_funcs = {
    .prepare = xiaomi_m17_panel_prepare,
    .unprepare = xiaomi_m17_panel_unprepare,
    .get_modes = xiaomi_m17_panel_get_modes,
};

static int xiaomi_m17_bl_update_status(struct backlight_device *bl)
{
    struct xiaomi_m17_panel *ctx = bl_get_data(bl);
    u16 brightness = (u16)backlight_get_brightness(bl);

    if (brightness > 2047)
        brightness = 2047;

    return mipi_dsi_dcs_set_display_brightness(ctx->dsi, brightness);
}

static const struct backlight_ops xiaomi_m17_bl_ops = {
    .update_status = xiaomi_m17_bl_update_status,
};

static int xiaomi_m17_panel_probe(struct mipi_dsi_device *dsi)
{
    struct xiaomi_m17_panel *ctx;
    struct backlight_properties props = { 0 };
    int ret;

    ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
    if (!ctx)
        return -ENOMEM;

    ctx->supplies[0].supply = "vddio";
    ctx->supplies[1].supply = "vdd";
    ret = devm_regulator_bulk_get(&dsi->dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret < 0)
        return ret;

    ctx->reset_gpio = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
    if (IS_ERR(ctx->reset_gpio))
        return PTR_ERR(ctx->reset_gpio);

    ctx->dsi = dsi;
    mipi_dsi_set_drvdata(dsi, ctx);

    dsi->lanes = 4;
    dsi->format = MIPI_DSI_FMT_RGB888;
    
    dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
                      MIPI_DSI_MODE_LPM;

    ret = xiaomi_m17_configure_dsc(dsi);
    if (ret)
        return dev_err_probe(&dsi->dev, ret, "Failed to configure DSC\n");

    drm_panel_init(&ctx->panel, &dsi->dev, &xiaomi_m17_panel_funcs, DRM_MODE_CONNECTOR_DSI);
    
    ctx->panel.prepare_prev_first = true;

    /* Register DCS Backlight with correct 11-bit max value */
    props.type = BACKLIGHT_RAW;
    props.max_brightness = 2047;
    ctx->panel.backlight = devm_backlight_device_register(&dsi->dev, "xiaomi_m17_bl", 
                           &dsi->dev, ctx, &xiaomi_m17_bl_ops, &props);
    if (IS_ERR(ctx->panel.backlight))
        return dev_err_probe(&dsi->dev, PTR_ERR(ctx->panel.backlight),
                     "Failed to register backlight\n");

    drm_panel_add(&ctx->panel);
    
    ret = mipi_dsi_attach(dsi);
    if (ret < 0) {
        drm_panel_remove(&ctx->panel);
        return ret;
    }

    return 0;
}

static void xiaomi_m17_panel_remove(struct mipi_dsi_device *dsi)
{
    struct xiaomi_m17_panel *ctx = mipi_dsi_get_drvdata(dsi);

    mipi_dsi_detach(dsi);
    drm_panel_remove(&ctx->panel);
}

static const struct of_device_id xiaomi_m17_of_match[] = {
    { .compatible = "xiaomi,moonstone-m17" },
    { }
};
MODULE_DEVICE_TABLE(of, xiaomi_m17_of_match);

static struct mipi_dsi_driver xiaomi_m17_panel_driver = {
    .probe = xiaomi_m17_panel_probe,
    .remove = xiaomi_m17_panel_remove,
    .driver = {
        .name = "panel-xiaomi-m17",
        .of_match_table = xiaomi_m17_of_match,
    },
};
module_mipi_dsi_driver(xiaomi_m17_panel_driver);

MODULE_DESCRIPTION("Xiaomi M17 DRM Panel Driver");
MODULE_LICENSE("GPL v2");
