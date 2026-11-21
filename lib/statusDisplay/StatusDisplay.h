#ifndef STATUSDISPLAY_STATUSDISPLAY_H
#define STATUSDISPLAY_STATUSDISPLAY_H

#include <memory>

#include "TextPanel.h"

// Shows WiFi, mounted disks and bus activity on a board's built-in screen, if the pinmap has one
class StatusDisplay
{
public:
    void start();

private:
    static constexpr int MAX_ROWS = 16;
    static constexpr int MAX_COLUMNS = 40;

    struct Line
    {
        char text[MAX_COLUMNS + 1] = "";
        PanelColor fg = PanelColor::WHITE;
    };

    static void task_entry(void *arg);
    void run();
    void compose(Line *lines, int rows, int columns);

    std::unique_ptr<TextPanel> _panel;
    Line _shown[MAX_ROWS];
};

#endif // STATUSDISPLAY_STATUSDISPLAY_H
