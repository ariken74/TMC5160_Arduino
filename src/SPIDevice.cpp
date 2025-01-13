#include "TMC5160.h"
#include "esp_log.h"

static const char *TAG = "MY_TAG";

void print_buffer(char *TAG, const void* data, size_t size) {
    const uint8_t* bytes = (const uint8_t*)data;
    char hex_str[size * 3 + 1]; // 3 characters per byte + null terminator
    hex_str[0] = '\0'; 

    for (size_t i = 0; i < size; ++i) {
        sprintf(&hex_str[i * 3], "%02X ", bytes[i]); 
    }

    ESP_LOGI(TAG, "%s", hex_str);
}



SPIDevice::SPIDevice(int cs_pin, int clk_pin, int mosi_pin, int miso_pin, int max_transfer_sz, int dma_channel)
{
    spi_device_handle_t spi;
    spi_bus_config_t bus_config = {
        .mosi_io_num = mosi_pin, // Replace with your actual MOSI pin
        .miso_io_num = miso_pin, // Replace with your actual MISO pin
        .sclk_io_num = clk_pin,  // Replace with your actual SCK pin
        .quadwp_io_num = -1,     // Not used
        .quadhd_io_num = -1,     // Not used
        .max_transfer_sz = 4096, // Adjust as needed
    };

    spi_device_interface_config_t dev_config = {
        .mode = 0,
        .clock_speed_hz = 4000000, // 10.5 * 1000 * 1000, // Clock out at 10 MHz					 // SPI mode 0
        .spics_io_num = cs_pin,     // CS pin
        // .flags = SPI_DEVICE_HALFDUPLEX,
        .queue_size = 8,

    };
    esp_err_t ret;
    ret = spi_bus_initialize(SPI2_HOST, &bus_config, dma_channel);
    ESP_ERROR_CHECK(ret);
    ret = spi_bus_add_device(SPI2_HOST, &dev_config, &spi);
    ESP_ERROR_CHECK(ret);
    ESP_LOGI("TAG", "SPI INIZI");
    SPIDevice::handle = spi;
}

void SPIDevice::begin()
{
}

void SPIDevice::transfer(uint8_t *data, uint8_t len)
{
    uint8_t rcv[len] = {0};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    print_buffer("TRANSFER     ", data, len);
    t.tx_buffer = data;
    t.length = len * 8;
    t.rx_buffer = rcv;
    t.rxlength = len * 8;
    
    // t.flags = SPI_DEVICE_HALFDUPLEX;
    spi_device_transmit(handle, &t);
    print_buffer("TRANSFER_READ", rcv, len);
    memcpy(data, rcv, len);
}

void SPIDevice::read(uint8_t *data, uint8_t len)
{
    uint8_t send[len] = {0};
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    // t.flags = SPI_DEVICE_HALFDUPLEX;
    
    t.tx_buffer = send;
    t.length = len * 8;
    t.rx_buffer = data;
    t.rxlength = len * 8;
    spi_device_transmit(handle, &t);
    print_buffer("READ         ", data, len);
}