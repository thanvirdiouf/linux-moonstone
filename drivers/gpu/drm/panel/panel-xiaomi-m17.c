#include <linux/delay.h>
#include <linux/gpio/consumer.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/display/drm_dsc.h>
#include <drm/display/drm_dsc_helper.h>

struct moonstone_panel {
	struct drm_panel panel;
	struct mipi_dsi_device *dsi;
	struct regulator_bulk_data supplies[2];
	struct gpio_desc *reset_gpio;
};

static int moonstone_panel_configure_dsc(struct mipi_dsi_device *dsi)
{
	struct drm_dsc_config *dsc = dsi->dsc;

	/* 
	 * Hardware DSC topology matching the downstream qcom,mdss-dsc-* specification.
	 * The split-screen anomaly occurs when the DPU splits the video stream without 
	 * the panel receiving matching compression parameters.
	 */
	dsc->pic_width = 1080;
	dsc->pic_height = 2400;
	dsc->slice_width = 540;      /* qcom,mdss-dsc-slice-width = <0x21c> */
	dsc->slice_height = 20;      /* qcom,mdss-dsc-slice-height = <0x14> */
	dsc->slice_count = 2;        /* Derived from pic_width / slice_width */
	dsc->bits_per_component = 8; /* qcom,mdss-dsc-bit-per-component */
	dsc->bits_per_pixel = 8 << 4;/* qcom,mdss-dsc-bit-per-pixel (U4.4 fixed point) */
	dsc->block_pred_enable = true;

	drm_dsc_compute_rc_parameters(dsc);
	drm_dsc_pps_payload_pack(&dsi->dsc_pps_payload, dsc);

	return 0;
}

static int moonstone_panel_prepare(struct drm_panel *panel)
{
	struct moonstone_panel *ctx = container_of(panel, struct moonstone_panel, panel);

	regulator_bulk_enable(ARRAY_SIZE(ctx->supplies), ctx->supplies);
	msleep(20);

	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000, 11000);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(20);
	gpiod_set_value(ctx->reset_gpio, 1);
	msleep(10);

	mipi_dsi_dcs_exit_sleep_mode(ctx->dsi);
	msleep(120);

	/* 
	 * Transcribed from qcom,mdss-dsi-on-command array.
	 * Bypassing payload wrappers for direct array writes maintains precise
	 * downstream packet alignment.
	 */
	mipi_dsi_generic_write(ctx->dsi, (u8[]){ 0xf0, 0x5a, 0x5a }, 3);
	mipi_dsi_generic_write(ctx->dsi, (u8[]){ 0xb2, 0x01, 0x31 }, 3);
	mipi_dsi_generic_write(ctx->dsi, (u8[]){ 0xdf, 0x09, 0x30, 0x95, 0x46, 0xe9 }, 6);
	mipi_dsi_generic_write(ctx->dsi, (u8[]){ 0xf0, 0xa5, 0xa5 }, 3);
	
	/* Transmit the generated Picture Parameter Set */
	mipi_dsi_picture_parameter_set(ctx->dsi, &ctx->dsi->dsc_pps_payload);

	mipi_dsi_dcs_set_display_on(ctx->dsi);
	msleep(20);

	return 0;
}

static const struct drm_display_mode moonstone_mode = {
	.clock = 414050, 
	.hdisplay = 1080,
	.hsync_start = 1080 + 120,    /* h-front-porch = 0x78 */
	.hsync_end = 1080 + 120 + 28, /* h-pulse-width = 0x1c */
	.htotal = 1080 + 120 + 120 + 28, /* h-back-porch = 0x78 */
	.vdisplay = 2400,
	.vsync_start = 2400 + 20,     /* v-front-porch = 0x14 */
	.vsync_end = 2400 + 20 + 2,   /* v-pulse-width = 0x02 */
	.vtotal = 2400 + 20 + 10 + 2, /* v-back-porch = 0x0a */
	.width_mm = 67,
	.height_mm = 149,
};

static int moonstone_panel_get_modes(struct drm_panel *panel,
				     struct drm_connector *connector)
{
	struct drm_display_mode *mode;

	mode = drm_mode_duplicate(connector->dev, &moonstone_mode);
	if (!mode)
		return -ENOMEM;

	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	connector->display_info.width_mm = mode->width_mm;
	connector->display_info.height_mm = mode->height_mm;

	return 1;
}

static const struct drm_panel_funcs moonstone_panel_funcs = {
	.prepare = moonstone_panel_prepare,
	.get_modes = moonstone_panel_get_modes,
};

static int moonstone_panel_probe(struct mipi_dsi_device *dsi)
{
	struct moonstone_panel *ctx;

	ctx = devm_kzalloc(&dsi->dev, sizeof(*ctx), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	ctx->supplies[0].supply = "vddio";
	ctx->supplies[1].supply = "vdd";
	devm_regulator_bulk_get(&dsi->dev, 2, ctx->supplies);

	ctx->reset_gpio = devm_gpiod_get(&dsi->dev, "reset", GPIOD_OUT_LOW);
	ctx->dsi = dsi;
	mipi_dsi_set_drvdata(dsi, ctx);

	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags = MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_NO_EOT_PACKET;

	dsi->dsc = devm_kzalloc(&dsi->dev, sizeof(*dsi->dsc), GFP_KERNEL);
	moonstone_panel_configure_dsc(dsi);

	drm_panel_init(&ctx->panel, &dsi->dev, &moonstone_panel_funcs, DRM_MODE_CONNECTOR_DSI);
	drm_panel_add(&ctx->panel);
	mipi_dsi_attach(dsi);

	return 0;
}

static const struct of_device_id moonstone_of_match[] = {
	{ .compatible = "xiaomi,moonstone-m17" },
	{ }
};
MODULE_DEVICE_TABLE(of, moonstone_of_match);

static struct mipi_dsi_driver moonstone_panel_driver = {
	.probe = moonstone_panel_probe,
	.driver = {
		.name = "panel-xiaomi-moonstone",
		.of_match_table = moonstone_of_match,
	},
};
module_mipi_dsi_driver(moonstone_panel_driver);
