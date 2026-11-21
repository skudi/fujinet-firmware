#ifndef STATUSDISPLAY_ST7789PANEL_H
#define STATUSDISPLAY_ST7789PANEL_H

#ifdef ESP_PLATFORM

#include <memory>

#include <driver/spi_master.h>
#include <esp_lcd_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "TextPanel.h"

// An ST7789 on its own SPI bus, drawn one text row at a time so no frame buffer is needed
class ST7789Panel : public TextPanel
{
public:
    struct Config
    {
        spi_host_device_t host;
        int pin_mosi, pin_sck, pin_cs, pin_dc, pin_rst, pin_backlight;
        int width, height;   // visible area after swap_xy
        int gap_x, gap_y;    // offset of the visible area in controller RAM
        bool swap_xy, mirror_x, mirror_y, invert;
    };

    explicit ST7789Panel(const Config &config) : _cfg(config) {}
    ~ST7789Panel() override;

    success_is_true begin() override;
    int columns() const override;
    int rows() const override;
    void clear(PanelColor bg) override;
    void draw_row(int row, const char *text, PanelColor fg, PanelColor bg) override;

private:
    struct HeapCapsDeleter
    {
        void operator()(uint16_t *ptr) const;
    };

    static bool on_trans_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t *edata,
                              void *ctx);
    void blit(int y, int height);

    Config _cfg;
    esp_lcd_panel_io_handle_t _io = nullptr;
    esp_lcd_panel_handle_t _panel = nullptr;
    SemaphoreHandle_t _done = nullptr;
    bool _bus_ready = false;
    std::unique_ptr<uint16_t, HeapCapsDeleter> _line; // one text row of pixels, DMA-capable
    int _top = 0;                                      // centers the rows vertically
};

#endif // ESP_PLATFORM
#endif // STATUSDISPLAY_ST7789PANEL_H
