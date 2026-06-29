#include "pmw3360.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "pmw3360_srom.h"
#include <cstring>

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
    esp_rom_delay_us(35); // tSRAD: required gap between address and data phases
    spi_device_transmit(pmw3360_spi, &t_data);
    cs_high();
    esp_rom_delay_us(19);

    return data;
}

void pmw3360_read_motion(pmw3360_motion_t *motion)
{
    uint8_t buf[12] = { 0 };
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
    t.length    = 96;
    t.tx_buffer = NULL;
    t.rx_buffer = buf;
    spi_device_transmit(pmw3360_spi, &t);

    cs_high();
    esp_rom_delay_us(1);

    // ESP_LOGI(TAG, "RAW: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
    //     buf[0], buf[1], buf[2], buf[3],
    //     buf[4], buf[5], buf[6], buf[7],
    //     buf[8], buf[9], buf[10], buf[11]);

    motion->motion  = buf[0];
    // X — low byte first then high byte
    int16_t xl = buf[2];
    int16_t xh = buf[3];
    motion->delta_x = (int16_t)((xh << 8) | xl);

    // Y — low byte first then high byte  
    int16_t yl = buf[4];
    int16_t yh = buf[5];
    motion->delta_y = (int16_t)((yh << 8) | yl);

    // ESP_LOGI(TAG, "motion=0x%02X dx=%d dy=%d xl=%02X xh=%02X yl=%02X yh=%02X",
    //     motion->motion,
    //     motion->delta_x,
    //     motion->delta_y,
    //     (uint8_t)xl, (uint8_t)xh,
    //     (uint8_t)yl, (uint8_t)yh);
    // static int log_counter = 0;
    // if (++log_counter >= 20)
    // {
    //     log_counter = 0;
    //     ESP_LOGI(TAG, "RAW: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
    //         buf[0], buf[1], buf[2],  buf[3],
    //         buf[4], buf[5], buf[6],  buf[7],
    //         buf[8], buf[9], buf[10], buf[11]);
    // }
}

void pmw3360_set_cpi(uint16_t cpi)
{
    if (cpi < 100)   cpi = 100;
    if (cpi > 12000) cpi = 12000;
    pmw3360_write(REG_CONFIG1, (cpi / 100) - 1);
}

// ── NEW: SROM Upload Function ──────────────────────────────
void pmw3360_upload_firmware()
{
    ESP_LOGI(TAG, "Uploading firmware...");
    
    spi_transaction_t t = {};
    uint8_t tx;
    
    // 1. Disable Rest mode
    pmw3360_write(REG_CONFIG2, 0x00);
    
    // 2. Initialize SROM
    pmw3360_write(REG_SROM_ENABLE, 0x1D);
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 3. Start SROM download
    pmw3360_write(REG_SROM_ENABLE, 0x18);
    
    // 4. Send firmware in burst
    cs_low();
    esp_rom_delay_us(10);
    
    // Send SROM_LOAD_BURST command
    tx = REG_SROM_LOAD_BURST | 0x80;
    t.length = 8;
    t.tx_buffer = &tx;
    t.rx_buffer = NULL;
    spi_device_transmit(pmw3360_spi, &t);
    esp_rom_delay_us(15);
    
    // Send firmware data
    for (int i = 0; i < firmware_length; i++) {
        tx = firmware_data[i];
        t.length = 8;
        t.tx_buffer = &tx;
        spi_device_transmit(pmw3360_spi, &t);
        esp_rom_delay_us(15);
    }
    
    cs_high();
    vTaskDelay(pdMS_TO_TICKS(10));
    
    // 5. Verify SROM upload
    uint8_t srom_id = pmw3360_read(REG_SROM_ID);
    ESP_LOGI(TAG, "SROM ID: 0x%02X (expected 0x04)", srom_id);
    
    // 6. Disable Rest mode again
    pmw3360_write(REG_CONFIG2, 0x00);
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
    uint8_t pid = pmw3360_read(REG_PRODUCT_ID);
    ESP_LOGI("PMW", "Product ID: 0x%02X (expected 0x42)", pid);

    if (pid != 0x42) {
        ESP_LOGE("PMW", "Sensor not responding! Check wiring.");
        return;
    }

    // ── UPLOAD FIRMWARE ──────────────────────────────────
    pmw3360_upload_firmware();

    // ── Set CPI ──────────────────────────────────────────
    pmw3360_set_cpi(3200);
    
    ESP_LOGI(TAG, "PMW3360 ready");
}