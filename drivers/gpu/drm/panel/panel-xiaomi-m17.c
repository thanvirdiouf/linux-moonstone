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

#ifndef mipi_dsi_dcs_write_seq
#define mipi_dsi_dcs_write_seq(dsi, cmd, seq...)            \
	do {                                                    \
		static const u8 d[] = { seq };                      \
		mipi_dsi_dcs_write(dsi, cmd, d, ARRAY_SIZE(d));     \
	} while (0)
#endif

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

	dsc->pic_width = 1080;
	dsc->pic_height = 2400;
	dsc->slice_width = 540;
	dsc->slice_height = 20;
	dsc->slice_count = 2;
	dsc->bits_per_component = 8;
	dsc->bits_per_pixel = 8 << 4; /* U4.4 fixed point */
	dsc->block_pred_enable = true;

	drm_dsc_compute_rc_parameters(dsc);
	dsi->dsc = dsc;

	return 0;
}

static int xiaomi_m17_panel_prepare(struct drm_panel *panel)
{
    struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);
    struct mipi_dsi_device *dsi = ctx->dsi;
    int ret;

    ret = regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
    if (ret < 0)
        return ret;

    msleep(20);

    /* Hardware Reset */
    gpiod_set_value(ctx->reset_gpio, 1);
    usleep_range(10000, 11000);
    gpiod_set_value(ctx->reset_gpio, 0);
    msleep(20);
    gpiod_set_value(ctx->reset_gpio, 1);
    msleep(10);

    /* --- TRANSLATED DOWNSTREAM on_cmds --- */
    
    mipi_dsi_dcs_exit_sleep_mode(dsi);
    msleep(20); /* 0x14 delay from downstream array */

    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x01, 0x31);
    mipi_dsi_dcs_write_seq(dsi, 0xdf, 0x09, 0x30, 0x95, 0x46, 0xe9);
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq(dsi, 0x9d, 0x01);

    /* 
     * PANEL QUIRK: Send hardcoded DSC PPS via DCS long write to 0x9E 
     * The standard mipi_dsi_picture_parameter_set() helper fails here.
     */
    mipi_dsi_dcs_write_seq(dsi, 0x9e, 0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09, 0x60,
        0x04, 0x38, 0x00, 0x28, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x00, 0x20, 0x03, 0xdd,
        0x00, 0x07, 0x00, 0x0c, 0x02, 0x77, 0x02, 0x8b, 0x18, 0x00, 0x10, 0xf0, 0x03, 0x0c, 0x20, 0x00,
        0x06, 0x0b, 0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b,
        0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40, 0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa, 0x19, 0xf8,
        0x1a, 0x38, 0x1a, 0x78, 0x1a, 0xb6, 0x2a, 0xf6, 0x2b, 0x34, 0x2b, 0x74, 0x3b, 0x74, 0x6b, 0xf4,
        0x00);

    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq(dsi, 0x60, 0x21);
    mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0b);
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x15, 0xf6);
    mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x28, 0xf6);
    mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x3b, 0xf6);
    mipi_dsi_dcs_write_seq(dsi, 0xf6, 0xf0);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0a, 0xf4);
    mipi_dsi_dcs_write_seq(dsi, 0xf4, 0x98);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x11, 0xf4);
    mipi_dsi_dcs_write_seq(dsi, 0xf4, 0xee);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x18, 0xb2);
    mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x1c);
    mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
    
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x11, 0xfe);
    mipi_dsi_dcs_write_seq(dsi, 0xfe, 0x00);
    mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);
    
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0d, 0xb2);
    mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x20);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x0c, 0xb2);
    mipi_dsi_dcs_write_seq(dsi, 0xb2, 0x30);
    mipi_dsi_dcs_write_seq(dsi, 0xf7, 0x0b);
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
    
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq(dsi, 0xfc, 0x5a, 0x5a);
    mipi_dsi_dcs_write_seq(dsi, 0xed, 0x01, 0xcd, 0x00);
    mipi_dsi_dcs_write_seq(dsi, 0xe1, 0x93);
    mipi_dsi_dcs_write_seq(dsi, 0xb0, 0x00, 0x06, 0xf4);
    mipi_dsi_dcs_write_seq(dsi, 0xf4, 0x1f);
    mipi_dsi_dcs_write_seq(dsi, 0xf0, 0xa5, 0xa5);
    mipi_dsi_dcs_write_seq(dsi, 0xfc, 0xa5, 0xa5);

    mipi_dsi_dcs_write_seq(dsi, 0x53, 0x20); 
    
    /* Brightness delay */
    mipi_dsi_dcs_write_seq(dsi, 0x51, 0x00, 0x00);
    msleep(100); /* 0x64 delay from downstream array */

    mipi_dsi_dcs_set_display_on(dsi);

    return 0;
}

static int xiaomi_m17_panel_unprepare(struct drm_panel *panel)
{
    struct xiaomi_m17_panel *ctx = container_of(panel, struct xiaomi_m17_panel, panel);

    mipi_dsi_dcs_set_display_off(ctx->dsi);
    msleep(20);
    
    mipi_dsi_dcs_enter_sleep_mode(ctx->dsi);
    msleep(150);

    /* Assert reset to physically power down the TCON */
    gpiod_set_value(ctx->reset_gpio, 1);
    regulator_bulk_disable(ARRAY_SIZE(ctx->supplies), ctx->supplies);

    return 0;
}


static const struct drm_display_mode xiaomi_m17_mode = {
    .clock = 393396, /* Uncompressed pixel clock: 1348 * 2432 * 120Hz / 1000 */
    .hdisplay = 1080,
    .hsync_start = 1080 + 120,        /* hdisplay + h-front-porch */
    .hsync_end = 1080 + 120 + 28,     /* hsync_start + h-pulse-width */
    .htotal = 1080 + 120 + 28 + 120,  /* hsync_end + h-back-porch = 1348 */
    .vdisplay = 2400,
    .vsync_start = 2400 + 20,         /* vdisplay + v-front-porch */
    .vsync_end = 2400 + 20 + 2,       /* vsync_start + v-pulse-width */
    .vtotal = 2400 + 20 + 2 + 10,     /* vsync_end + v-back-porch = 2432 */
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

static int xiaomi_m17_panel_probe(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17_panel *ctx;
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
		return ret;

	drm_panel_init(&ctx->panel, &dsi->dev, &xiaomi_m17_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	
	/* Required for QCOM hosts to ensure link is powered before transmitting commands */
	ctx->panel.prepare_prev_first = true;

	struct backlight_properties props = {
		.type = BACKLIGHT_RAW,
		.max_brightness = 2047,
	    };

	    /* Register standard DCS backlight control */
	ctx->panel.backlight = devm_backlight_device_register(&dsi->dev, 
		"xiaomi_m17_bl", &dsi->dev, ctx, 
		&mipi_dsi_dcs_backlight_ops, &props);
	    
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
