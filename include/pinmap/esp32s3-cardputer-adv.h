#ifdef PINMAP_CARDPUTER_ADV

/* SD Card (built-in microSD slot) */
#define PIN_CARD_DETECT         GPIO_NUM_NC  // no card-detect line
#define PIN_CARD_DETECT_FIX     GPIO_NUM_NC

#define PIN_SD_HOST_CS          GPIO_NUM_12
#define PIN_SD_HOST_MISO        GPIO_NUM_39
#define PIN_SD_HOST_MOSI        GPIO_NUM_14
#define PIN_SD_HOST_SCK         GPIO_NUM_40  // also routed to the EXT header

/* UART */
#define PIN_UART0_RX            GPIO_NUM_44  // not wired to USB-C
#define PIN_UART0_TX            GPIO_NUM_43
#define PIN_UART1_RX            GPIO_NUM_1   // Grove port
#define PIN_UART1_TX            GPIO_NUM_2
#define PIN_UART2_RX            GPIO_NUM_15  // EXT header
#define PIN_UART2_TX            GPIO_NUM_13

/* USB-C is the S3's USB-Serial-JTAG, not UART0: debug through the IDF console */
#define FN_DEBUG_TO_STDOUT

/* Buttons */
#define PIN_BUTTON_C            GPIO_NUM_NC  // GPIO11 is the keyboard INT, not a button

/* LEDs */
#define PIN_LED_WIFI            GPIO_NUM_NC
#define PIN_LED_BUS             GPIO_NUM_NC
#define PIN_LED_BT              GPIO_NUM_NC

/* Atari SIO Pins (EXT header) */
#define PIN_INT                 GPIO_NUM_3
#define PIN_PROC                GPIO_NUM_4
#define PIN_CKO                 GPIO_NUM_NC
#define PIN_CKI                 GPIO_NUM_NC
#define PIN_MTR                 GPIO_NUM_6
#define PIN_CMD                 GPIO_NUM_5

/* Status LCD: ST7789V2, shown landscape */
#define PIN_LCD_MOSI            GPIO_NUM_35
#define PIN_LCD_SCK             GPIO_NUM_36
#define PIN_LCD_CS              GPIO_NUM_37
#define PIN_LCD_DC              GPIO_NUM_34  // RS on the schematic
#define PIN_LCD_RST             GPIO_NUM_33
#define PIN_LCD_BL              GPIO_NUM_38
#define LCD_WIDTH               240
#define LCD_HEIGHT              135
#define LCD_GAP_X               40
#define LCD_GAP_Y               53
#define LCD_SWAP_XY             true
#define LCD_MIRROR_X            true
#define LCD_MIRROR_Y            false
#define LCD_INVERT              true

/* Audio Output */
#define PIN_DAC1                GPIO_NUM_NC  // ES8311 codec over I2S, no DAC

#include "atari-common.h"
#include "common.h"

#endif /* PINMAP_CARDPUTER_ADV */
