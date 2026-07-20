#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>

#include <drm/drm_mipi_dsi.h>
#include <drm/drm_modes.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

struct xiaomi_m17 {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
};

static inline struct xiaomi_m17 *to_xiaomi_m17(struct drm_panel *panel)
{
	return container_of(panel, struct xiaomi_m17, panel);
}

static const struct drm_display_mode xiaomi_m17_mode = {
	.clock = 393348,
	.hdisplay = 1080,
	.hsync_start = 1080 + 120,
	.hsync_end = 1080 + 120 + 28,
	.htotal = 1080 + 120 + 28 + 120,
	.vdisplay = 2400,
	.vsync_start = 2400 + 20,
	.vsync_end = 2400 + 20 + 2,
	.vtotal = 2400 + 20 + 2 + 10,
	.type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED,
};

static int xiaomi_m17_prepare(struct drm_panel *panel)
{
	struct xiaomi_m17 *ctx_m17 = to_xiaomi_m17(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = ctx_m17->dsi };
	int ret;

	ret = regulator_bulk_enable(ARRAY_SIZE(ctx_m17->supplies), ctx_m17->supplies);
	if (ret)
		return ret;

	/* Hardware reset sequence */
	gpiod_set_value_cansleep(ctx_m17->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx_m17->reset_gpio, 0);
	usleep_range(10000, 11000);
	gpiod_set_value_cansleep(ctx_m17->reset_gpio, 1);
	msleep(10);

	/* Init Sequence */
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf1, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xd0, 0x08);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x25, 0xf2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf2, 0x50);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x2f, 0xf2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf2, 0x27);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf1, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x11, 0x00);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb2, 0x01, 0x31);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xdf, 0x09, 0x30, 0x95, 0x43, 0xa9, 0x43, 0xa9);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x9d, 0x01);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x9e, 0x11, 0x00, 0x00, 0x89, 0x30, 0x80, 0x09, 0x60, 0x04, 0x38, 0x00, 0x28, 0x02, 0x1c, 0x02, 0x1c, 0x02, 0x00, 0x02, 0x0e, 0x00, 0x20, 0x03, 0xdd, 0x00, 0x07, 0x00, 0x0c, 0x02, 0x77, 0x02, 0x8b, 0x18, 0x00, 0x10, 0xf0, 0x03, 0x0c, 0x20, 0x00, 0x06, 0x0b, 0x0b, 0x33, 0x0e, 0x1c, 0x2a, 0x38, 0x46, 0x54, 0x62, 0x69, 0x70, 0x77, 0x79, 0x7b, 0x7d, 0x7e, 0x01, 0x02, 0x01, 0x00, 0x09, 0x40, 0x09, 0xbe, 0x19, 0xfc, 0x19, 0xfa, 0x19, 0xf8, 0x1a, 0x38, 0x1a, 0x78, 0x1a, 0xb6, 0x2a, 0xf6, 0x2b, 0x34, 0x2b, 0x74, 0x3b, 0x74, 0x6b, 0xf4, 0x00);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x60, 0x01);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf7, 0x0b);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x40, 0xf2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf2, 0x03);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x15, 0xf6);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf6, 0xf0);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x28, 0xf6);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf6, 0xf0);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x3b, 0xf6);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf6, 0xf0);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x0a, 0xf4);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf4, 0x98);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x11, 0xf4);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf4, 0xee);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x18, 0xb2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb2, 0x1c);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x11, 0xfe);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xfe, 0x00);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x0d, 0xb2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb2, 0x05);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x0c, 0xb2);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb2, 0x30);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf7, 0x0b);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xfc, 0x5a, 0x5a);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xed, 0x01, 0xcd, 0x00);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xe1, 0x93);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xb0, 0x00, 0x06, 0xf4);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf4, 0x1f);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xf0, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0xfc, 0xa5, 0xa5);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x53, 0x28);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x51, 0x00, 0x00);
	mipi_dsi_msleep(&ctx, 1);
	mipi_dsi_dcs_write_seq_multi(&ctx, 0x29, 0x00);
	mipi_dsi_msleep(&ctx, 120);

	if (ctx.accum_err) {
		gpiod_set_value_cansleep(ctx_m17->reset_gpio, 0);
		regulator_bulk_disable(ARRAY_SIZE(ctx_m17->supplies), ctx_m17->supplies);
		return ctx.accum_err;
	}

	return 0;
}

static int xiaomi_m17_unprepare(struct drm_panel *panel)
{
	struct xiaomi_m17 *ctx_m17 = to_xiaomi_m17(panel);
	struct mipi_dsi_multi_context ctx = { .dsi = ctx_m17->dsi };

	mipi_dsi_dcs_set_display_off_multi(&ctx);
	mipi_dsi_dcs_enter_sleep_mode_multi(&ctx);
	mipi_dsi_msleep(&ctx, 120);

	gpiod_set_value_cansleep(ctx_m17->reset_gpio, 0);
	regulator_bulk_disable(ARRAY_SIZE(ctx_m17->supplies), ctx_m17->supplies);

	return ctx.accum_err;
}

static int xiaomi_m17_get_modes(struct drm_panel *panel,
				struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &xiaomi_m17_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	connector->display_info.width_mm = 67;
	connector->display_info.height_mm = 149;
	drm_mode_probed_add(connector, mode);

	return 1;
}

static const struct drm_panel_funcs xiaomi_m17_panel_funcs = {
	.prepare = xiaomi_m17_prepare,
	.unprepare = xiaomi_m17_unprepare,
	.get_modes = xiaomi_m17_get_modes,
};

static int xiaomi_m17_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
	struct xiaomi_m17 *ctx;
	struct drm_dsc_config *dsc;
	int ret;

	ctx = devm_kzalloc(dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->supplies[0].supply = "vdd";
	ctx->supplies[1].supply = "vddio";
	ret = devm_regulator_bulk_get(dev, ARRAY_SIZE(ctx->supplies), ctx->supplies);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get regulators\n");

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_LOW);
	if (IS_ERR(ctx->reset_gpio))
		return dev_err_probe(dev, PTR_ERR(ctx->reset_gpio), "Failed to get reset gpio\n");

	/* DSC Configuration mapping */
	dsc = devm_kzalloc(dev, sizeof(*dsc), GFP_KERNEL);
	if (!dsc)
		return -ENOMEM;

	dsc->slice_height = 20;
	dsc->slice_width = 540;
	dsc->slice_count = 2;
	dsc->bits_per_component = 8;
	dsc->bits_per_pixel = 8 << 4;
	dsc->block_pred_enable = true;
	dsi->dsc = dsc;

	/* Interface format routing */
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_CLOCK_NON_CONTINUOUS;

	drm_panel_init(&ctx->panel, dev, &xiaomi_m17_panel_funcs, DRM_MODE_CONNECTOR_DSI);

	ret = drm_panel_of_backlight(&ctx->panel);
	if (ret)
		return dev_err_probe(dev, ret, "Failed to get backlight\n");

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0) {
		drm_panel_remove(&Normally I can help with things like this, but I don't seem to have access to that content. You can try again or ask me for something else.
drm_panel_remove(&ctx->panel);
		return dev_err_probe(dev, ret, "Failed to attach to DSI host\n");
	}

	return 0;
}

static void xiaomi_m17_remove(struct mipi_dsi_device *dsi)
{
	struct xiaomi_m17 *ctx = mipi_dsi_get_drvdata(dsi);
	int ret;

	ret = mipi_dsi_detach(dsi);
	if (ret < 0)
		dev_err(&dsi->dev, "Failed to detach from DSI host: %d\n", ret);

	drm_panel_remove(&ctx->panel);
}

static const struct of_device_id xiaomi_m17_of_match[] = {
	{ .compatible = "xiaomi,m17" },
	{ /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, xiaomi_m17_of_match);

static struct mipi_dsi_driver xiaomi_m17_driver = {
	.probe = xiaomi_m17_probe,
	.remove = xiaomi_m17_remove,
	.driver = {
		.name = "panel-xiaomi-m17",
		.of_match_table = xiaomi_m17_of_match,
	},
};
module_mipi_dsi_driver(xiaomi_m17_driver);

MODULE_AUTHOR("Thanvir Diouf");
MODULE_DESCRIPTION("DRM driver for Xiaomi M17 DSI panel");
MODULE_LICENSE("GPL");

