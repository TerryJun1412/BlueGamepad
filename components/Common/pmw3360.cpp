#include "pmw3360.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_rom_sys.h"

static const char *TAG = "PMW3360";
spi_device_handle_t pmw3360_spi;

static inline void cs_low()
{
    gpio_set_level(PMW3360_CS, 0);
    esp_rom_delay_us(1);
}

static inline void cs_high()
{
    esp_rom_delay_us(1);
    gpio_set_level(PMW3360_CS, 1);
}

void pmw3360_write(uint8_t reg, uint8_t data)
{
    uint8_t buf[2] = { (uint8_t)(reg | 0x80), data };

    spi_transaction_t t = {};
    t.length    = 16;
    t.tx_buffer = buf;

    cs_low();
    spi_device_transmit(pmw3360_spi, &t);
    cs_high();
    esp_rom_delay_us(180);
}

uint8_t pmw3360_read(uint8_t reg)
{
    uint8_t addr = reg & 0x7F;
    uint8_t data = 0x00;

    spi_transaction_t t_addr = {};
    t_addr.length    = 8;
    t_addr.tx_buffer = &addr;

    spi_transaction_t t_data = {};
    t_data.length    = 8;
    t_data.rx_buffer = &data;

    cs_low();
    spi_device_transmit(pmw3360_spi, &t_addr);
    esp_rom_delay_us(100); // tSRAD: required gap between address and data phases
    spi_device_transmit(pmw3360_spi, &t_data);
    cs_high();
    esp_rom_delay_us(19);

    return data;
}

void pmw3360_read_motion(pmw3360_motion_t *motion)
{
    uint8_t buf[6] = { 0 };
    uint8_t cmd    = REG_MOTION_BURST & 0x7F;

    spi_transaction_t t = {};

    cs_low();

    // send burst address
    t.length    = 8;
    t.tx_buffer = &cmd;
    t.rx_buffer = NULL;
    spi_device_transmit(pmw3360_spi, &t);

    esp_rom_delay_us(35);

    // read 6 bytes back
    t.length    = 48;
    t.tx_buffer = NULL;
    t.rx_buffer = buf;
    spi_device_transmit(pmw3360_spi, &t);

    cs_high();
    esp_rom_delay_us(1);

    motion->motion  = buf[0];
    motion->delta_x = (int16_t)((buf[3] << 8) | buf[2]);
    motion->delta_y = (int16_t)((buf[5] << 8) | buf[4]);
}

void pmw3360_set_cpi(uint16_t cpi)
{
    if (cpi < 100)   cpi = 100;
    if (cpi > 12000) cpi = 12000;
    pmw3360_write(REG_CONFIG1, (cpi / 100) - 1);
}

void pmw3360_init()
{
    // CS pin
    gpio_config_t cs_conf = {};
    cs_conf.mode         = GPIO_MODE_OUTPUT;
    cs_conf.pin_bit_mask = (1ULL << PMW3360_CS);
    cs_conf.pull_up_en   = GPIO_PULLUP_ENABLE;
    gpio_config(&cs_conf);
    cs_high();

    // SPI bus
    spi_bus_config_t bus = {};
    bus.mosi_io_num   = PMW3360_MOSI;
    bus.miso_io_num   = PMW3360_MISO;
    bus.sclk_io_num   = PMW3360_SCK;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    ESP_ERROR_CHECK(spi_bus_initialize(VSPI_HOST, &bus, 1));

    // SPI device
    spi_device_interface_config_t dev = {};
    dev.clock_speed_hz = 2000000;   // 2MHz
    dev.mode           = 3;         // SPI mode 3
    dev.spics_io_num   = -1;        // manual CS
    dev.queue_size     = 1;
    ESP_ERROR_CHECK(spi_bus_add_device(VSPI_HOST, &dev, &pmw3360_spi));

    // power up sequence
    cs_high();
    vTaskDelay(pdMS_TO_TICKS(50));

    pmw3360_write(REG_POWER_UP_RESET, 0x5A);
    vTaskDelay(pdMS_TO_TICKS(50));

    // clear motion registers
    pmw3360_read(REG_MOTION);
    pmw3360_read(REG_DELTA_X_L);
    pmw3360_read(REG_DELTA_X_H);
    pmw3360_read(REG_DELTA_Y_L);
    pmw3360_read(REG_DELTA_Y_H);

    // check product ID
    gpio_set_level(PMW3360_CS, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    uint8_t pid = pmw3360_read(REG_PRODUCT_ID);
    gpio_set_level(PMW3360_CS, 1);
    ESP_LOGI("PMW", "Product ID: 0x%02X", pid);

    pmw3360_set_cpi(800);
    ESP_LOGI(TAG, "PMW3360 ready");
}