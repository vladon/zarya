// Stub when Qt Svg is unavailable (static kits often omit it).
// Style generation uses PNG icons; --render-svg is unused for lib_ui spike.
#include "codegen/style/render_svg.h"

namespace codegen::style {

int RenderSvg(const Options & /*options*/) {
	return 1;
}

} // namespace codegen::style
