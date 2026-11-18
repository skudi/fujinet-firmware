#ifdef ESP_PLATFORM

#include "ST7789Panel.h"

#include <algorithm>

#include <esp_heap_caps.h>
#include <esp_lcd_io_spi.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_st7789.h>

#include "../../include/debug.h"
#include "fnSystem.h"
#include "font8x16.h"

#define ST7789_PCLK_HZ (40 * 1000 * 1000)

// The panel shifts each RGB565 pixel in MSB first, so it is stored byte-swapped
static inline uint16_t wire_color(PanelColor color)
{
    uint16_t value = static_cast<uint16_t>(color);
    return (uint16_t)((value >> 8) | (value << 8));
}

void ST7789Panel::HeapCapsDeleter::operator()(uint16_t *ptr) const
{
    heap_caps_free(ptr);
}

ST7789Panel::~ST7789Panel()
{
    if (_panel)
        esp_lcd_panel_del(_panel);
    if (_io)
        esp_lcd_panel_io_del(_io);
    if (_bus_ready)
        spi_bus_free(_cfg.host);
    if (_done)
        vSemaphoreDelete(_done);
}

bool IRAM_ATTR ST7789Panel::on_trans_done(esp_lcd_panel_io_handle_t io,
                                          esp_lcd_panel_io_event_data_t *edata, void *ctx)
{
    BaseType_t woken = pdFALSE;
    xSemaphoreGiveFromISR(static_cast<ST7789Panel *>(ctx)->_done, &woken);
    return woken == pdTRUE;
}

success_is_true ST7789Panel::begin()
{
    size_t line_pixels = (size_t)_cfg.width * FONT8X16_HEIGHT;
    _line.reset((uint16_t *)heap_caps_malloc(line_pixels * sizeof(uint16_t), MALLOC_CAP_DMA));
    _done = xSemaphoreCreateBinary();
    if (!_line || !_done)
    {
        Debug_printv("no memory for the LCD line buffer");
        RETURN_ERROR_AS_FALSE();
    }

    spi_bus_config_t bus = {};
    bus.mosi_io_num = _cfg.pin_mosi;
    bus.miso_io_num = -1;
    bus.sclk_io_num = _cfg.pin_sck;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = (int)(line_pixels * sizeof(uint16_t));
    esp_err_t err = spi_bus_initialize(_cfg.host, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK)
    {
        Debug_printv("LCD SPI bus init failed: %s", esp_err_to_name(err));
        RETURN_ERROR_AS_FALSE();
    }
    _bus_ready = true;

    esp_lcd_panel_io_spi_config_t io_cfg = {};
    io_cfg.cs_gpio_num = _cfg.pin_cs;
    io_cfg.dc_gpio_num = _cfg.pin_dc;
    io_cfg.spi_mode = 0;
    io_cfg.pclk_hz = ST7789_PCLK_HZ;
    io_cfg.trans_queue_depth = 4;
    io_cfg.on_color_trans_done = &ST7789Panel::on_trans_done;
    io_cfg.user_ctx = this;
    io_cfg.lcd_cmd_bits = 8;
    io_cfg.lcd_param_bits = 8;
    err = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)_cfg.host, &io_cfg, &_io);
    if (err != ESP_OK)
    {
        Debug_printv("LCD panel IO init failed: %s", esp_err_to_name(err));
        RETURN_ERROR_AS_FALSE();
    }

    esp_lcd_panel_dev_config_t dev_cfg = {};
    dev_cfg.reset_gpio_num = _cfg.pin_rst;
    dev_cfg.rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB;
    dev_cfg.bits_per_pixel = 16;
    err = esp_lcd_new_panel_st7789(_io, &dev_cfg, &_panel);
    if (err != ESP_OK)
    {
        Debug_printv("ST7789 init failed: %s", esp_err_to_name(err));
        RETURN_ERROR_AS_FALSE();
    }

    esp_lcd_panel_reset(_panel);
    esp_lcd_panel_init(_panel);
    esp_lcd_panel_invert_color(_panel, _cfg.invert);
    esp_lcd_panel_swap_xy(_panel, _cfg.swap_xy);
    esp_lcd_panel_mirror(_panel, _cfg.mirror_x, _cfg.mirror_y);
    esp_lcd_panel_set_gap(_panel, _cfg.gap_x, _cfg.gap_y);

    _top = (_cfg.height - rows() * FONT8X16_HEIGHT) / 2;
    clear(PanelColor::BLACK);
    esp_lcd_panel_disp_on_off(_panel, true);

    fnSystem.set_pin_mode(_cfg.pin_backlight, gpio_mode_t::GPIO_MODE_OUTPUT);
    fnSystem.digital_write(_cfg.pin_backlight, DIGI_HIGH);

    RETURN_SUCCESS_AS_TRUE();
}

int ST7789Panel::columns() const
{
    return _cfg.width / FONT8X16_WIDTH;
}

int ST7789Panel::rows() const
{
    return _cfg.height / FONT8X16_HEIGHT;
}

// Sends the first `height` lines of _line to the panel at y and waits, since _line is reused
void ST7789Panel::blit(int y, int height)
{
    esp_lcd_panel_draw_bitmap(_panel, 0, y, _cfg.width, y + height, _line.get());
    xSemaphoreTake(_done, pdMS_TO_TICKS(100));
}

void ST7789Panel::clear(PanelColor bg)
{
    std::fill_n(_line.get(), (size_t)_cfg.width * FONT8X16_HEIGHT, wire_color(bg));
    for (int y = 0; y < _cfg.height; y += FONT8X16_HEIGHT)
        blit(y, std::min(FONT8X16_HEIGHT, _cfg.height - y));
}

void ST7789Panel::draw_row(int row, const char *text, PanelColor fg, PanelColor bg)
{
    if (row < 0 || row >= rows())
        return;

    uint16_t fg_px = wire_color(fg);
    uint16_t bg_px = wire_color(bg);
    bool ended = false;

    for (int col = 0; col < columns(); col++)
    {
        char chr = ' ';
        if (!ended && text[col] != '\0')
            chr = text[col];
        else
            ended = true;
        if (chr < FONT8X16_FIRST || chr > FONT8X16_LAST)
            chr = '?';

        const uint8_t *glyph = font8x16[chr - FONT8X16_FIRST];
        for (int line = 0; line < FONT8X16_HEIGHT; line++)
        {
            uint16_t *px = _line.get() + line * _cfg.width + col * FONT8X16_WIDTH;
            for (int bit = 0; bit < FONT8X16_WIDTH; bit++)
                px[bit] = (glyph[line] & (0x80 >> bit)) ? fg_px : bg_px;
        }
    }

    blit(_top + row * FONT8X16_HEIGHT, FONT8X16_HEIGHT);
}

#endif // ESP_PLATFORM
