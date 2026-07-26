#pragma once

namespace zarya {

// On macOS, Esc for modal QMessageBox sheets often never becomes a Qt key event
// (no focus / first responder). Install a local NSEvent monitor that dismisses
// the box the same way Qt would (escape / Cancel / single Ok button).
void installMacMessageBoxEscapeMonitor();

} // namespace zarya
