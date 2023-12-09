#include "debug_window.h"

DebugWindow::DebugWindow() {

}

DebugWindow::~DebugWindow() {
    
}

bool DebugWindow::IsOpen() {
    return open;
}

void DebugWindow::Draw() {
    Render();
}