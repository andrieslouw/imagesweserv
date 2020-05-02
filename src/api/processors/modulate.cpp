#include "processors/modulate.h"

namespace weserv {
namespace api {
namespace processors {

VImage Modulate::process(const VImage &image) const {
    auto brightness = query_->get_if<float>(
        /*"bri"*/"mod",
        [](float b) {
            // Brightness needs to be in range of 0 - 10000
            return b >= 0 && b <= 10000;
        },
        1.0F);
    auto saturation = query_->get_if<float>(
        "sat",
        [](float s) {
            // Saturation needs to be in range of 0 - 10000
            return s >= 0 && s <= 10000;
        },
        1.0F);
    auto hue = query_->get_if<int>(
        "hue",
        [](int h) {
            // Hue needs to be higher or equal to 0
            // Note: it's normalized to [0, 360] below
            return h >= 0;
        },
        0);

    // Should we process the image?
    if (brightness == 1.0 && saturation == 1.0 && hue == 0) {
        return image;
    }

    // Normalize hue to [0, 360]
    hue %= 360;

    // Get original colorspace
    VipsInterpretation type_before_modulate = image.interpretation();

#if !VIPS_VERSION_AT_LEAST(8, 9, 0)
    // Need to tag generic RGB interpretation as sRGB before libvips 8.9.0. See:
    // https://github.com/libvips/libvips/commit/e48f45187b350344ca90add63b4602f1d1f5a0a0
    if (type_before_modulate == VIPS_INTERPRETATION_RGB) {
        type_before_modulate = VIPS_INTERPRETATION_sRGB;
    }
#endif

    // Modulate brightness, saturation and hue
    if (image.has_alpha()) {
        // Separate alpha channel
        auto image_without_alpha = image.extract_band(
            0, VImage::option()->set("n", image.bands() - 1));
        auto alpha = image[image.bands() - 1];
        return image_without_alpha.colourspace(VIPS_INTERPRETATION_LCH)
            .linear({brightness, saturation, 1},
                    {0.0, 0.0, static_cast<double>(hue)})
            .colourspace(type_before_modulate)
            .bandjoin(alpha);
    } else {
        return image.colourspace(VIPS_INTERPRETATION_LCH)
            .linear({brightness, saturation, 1},
                    {0.0, 0.0, static_cast<double>(hue)})
            .colourspace(type_before_modulate);
    }
}

}  // namespace processors
}  // namespace api
}  // namespace weserv
