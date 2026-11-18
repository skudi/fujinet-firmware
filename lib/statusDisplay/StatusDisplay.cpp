#include "StatusDisplay.h"

#include "../../include/pinmap.h"

#if defined(ESP_PLATFORM) && defined(PIN_LCD_SCK)

#include <algorithm>
#include <cstdio>
#include <cstring>

#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "../../include/debug.h"
#include "ST7789Panel.h"
#include "bus.h"
#include "fnSystem.h"
#include "fnWiFi.h"
#include "fujiDevice.h"
#include "led.h"

#define STATUS_REFRESH_MS 250
#define STATUS_ACTIVITY_US 500000 // bus shown busy this long after the last command
#define STATUS_TASK_STACK 4096
#define STATUS_TASK_PRIORITY 1
#define STATUS_TASK_CORE 0

void StatusDisplay::start()
{
    ST7789Panel::Config cfg = {};
    cfg.host = SPI3_HOST;
    cfg.pin_mosi = PIN_LCD_MOSI;
    cfg.pin_sck = PIN_LCD_SCK;
    cfg.pin_cs = PIN_LCD_CS;
    cfg.pin_dc = PIN_LCD_DC;
    cfg.pin_rst = PIN_LCD_RST;
    cfg.pin_backlight = PIN_LCD_BL;
    cfg.width = LCD_WIDTH;
    cfg.height = LCD_HEIGHT;
    cfg.gap_x = LCD_GAP_X;
    cfg.gap_y = LCD_GAP_Y;
    cfg.swap_xy = LCD_SWAP_XY;
    cfg.mirror_x = LCD_MIRROR_X;
    cfg.mirror_y = LCD_MIRROR_Y;
    cfg.invert = LCD_INVERT;

    _panel = std::make_unique<ST7789Panel>(cfg);
    if (_panel->begin().is_error())
    {
        _panel.reset();
        return;
    }

    if (xTaskCreatePinnedToCore(&StatusDisplay::task_entry, "statusLCD", STATUS_TASK_STACK, this,
                                STATUS_TASK_PRIORITY, nullptr, STATUS_TASK_CORE) != pdPASS)
    {
        Debug_printv("could not create the status display task");
        _panel.reset();
    }
}

void StatusDisplay::task_entry(void *arg)
{
    static_cast<StatusDisplay *>(arg)->run();
}

void StatusDisplay::run()
{
    int rows = std::min(_panel->rows(), MAX_ROWS);
    int columns = std::min(_panel->columns(), MAX_COLUMNS);
    Line lines[MAX_ROWS];

    while (true)
    {
        compose(lines, rows, columns);
        for (int row = 0; row < rows; row++)
        {
            if (lines[row].fg == _shown[row].fg && strcmp(lines[row].text, _shown[row].text) == 0)
                continue;
            _panel->draw_row(row, lines[row].text, lines[row].fg, PanelColor::BLACK);
            _shown[row] = lines[row];
        }
        vTaskDelay(pdMS_TO_TICKS(STATUS_REFRESH_MS));
    }
}

// Name left, flag right-aligned in the last column
static void disk_line(char *out, int columns, int drive, const char *name, char flag)
{
    const char *base = strrchr(name, '/');
    base = base ? base + 1 : name;
    snprintf(out, columns + 1, "D%d %.*s", drive, std::max(columns - 5, 0), base);
    for (int pos = strlen(out); pos < columns; pos++)
        out[pos] = ' ';
    if (columns > 0)
        out[columns - 1] = flag;
    out[columns] = '\0';
}

void StatusDisplay::compose(Line *lines, int rows, int columns)
{
    for (int row = 0; row < rows; row++)
        lines[row] = Line();

    int row = 0;
    snprintf(lines[row].text, columns + 1, "FujiNet %s", fnSystem.get_fujinet_version(true));
    lines[row++].fg = PanelColor::CYAN;

    if (fnWiFi.connected())
    {
        snprintf(lines[row].text, columns + 1, "WiFi %s", fnWiFi.get_current_ssid().c_str());
        lines[row++].fg = PanelColor::GREEN;
        snprintf(lines[row].text, columns + 1, "IP   %s",
                 fnSystem.Net.get_ip4_address_str().c_str());
        lines[row++].fg = PanelColor::WHITE;
    }
    else
    {
        snprintf(lines[row].text, columns + 1, "WiFi not connected");
        lines[row++].fg = PanelColor::RED;
        row++;
    }

    // The last row is the bus line; the disks get what is left
    int disk_rows = rows - row - 1;
    int listed = 0, hidden = 0;
    fujiDevice *fuji = SYSTEM_BUS.theFuji;
    if (fuji && fuji->boot_config)
    {
        disk_line(lines[row + listed].text, columns, 1, "CONFIG", ' ');
        lines[row + listed++].fg = PanelColor::YELLOW;
    }
    for (int slot = 0; fuji && slot < MAX_DISK_DEVICES; slot++)
    {
        fujiDisk *disk = fuji->get_disk(slot);
        if (!fuji->get_disk_dev(slot)->isMounted() || disk->filename[0] == '\0')
            continue;
        if (listed == disk_rows)
        {
            hidden++;
            continue;
        }
        disk_line(lines[row + listed].text, columns, slot + 1, disk->filename,
                  (disk->access_mode & DISK_ACCESS_MODE_WRITE) ? 'W' : 'R');
        lines[row + listed++].fg = PanelColor::WHITE;
    }
    if (listed == 0)
    {
        snprintf(lines[row].text, columns + 1, "No disks mounted");
        lines[row].fg = PanelColor::GREY;
    }
    else if (hidden > 0)
    {
        snprintf(lines[row + disk_rows - 1].text, columns + 1, "+%d more", hidden + 1);
        lines[row + disk_rows - 1].fg = PanelColor::GREY;
    }

    int64_t since = esp_timer_get_time() - fnLedManager.last_on_us(eLed::LED_BUS);
    bool busy = fnLedManager.last_on_us(eLed::LED_BUS) != 0 && since < STATUS_ACTIVITY_US;
    snprintf(lines[rows - 1].text, columns + 1, busy ? "Bus busy" : "Bus idle");
    lines[rows - 1].fg = busy ? PanelColor::YELLOW : PanelColor::GREY;
}

#else

void StatusDisplay::start()
{
}

#endif // ESP_PLATFORM && PIN_LCD_SCK
