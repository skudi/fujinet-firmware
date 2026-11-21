#ifndef STATUSDISPLAY_TEXTPANEL_H
#define STATUSDISPLAY_TEXTPANEL_H

#include <cstdint>

#include "global_types.h"

// RGB565
enum class PanelColor : uint16_t
{
    BLACK  = 0x0000,
    WHITE  = 0xFFFF,
    GREY   = 0x8410,
    GREEN  = 0x07E0,
    YELLOW = 0xFFE0,
    RED    = 0xF800,
    CYAN   = 0x07FF,
};

// A screen addressed as fixed rows of fixed-width text; one subclass per panel type
class TextPanel
{
public:
    virtual ~TextPanel() = default;

    virtual success_is_true begin() = 0;
    virtual int columns() const = 0;
    virtual int rows() const = 0;
    virtual void clear(PanelColor bg) = 0;
    // Draws text padded or cut to columns()
    virtual void draw_row(int row, const char *text, PanelColor fg, PanelColor bg) = 0;
};

#endif // STATUSDISPLAY_TEXTPANEL_H
