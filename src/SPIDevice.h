#pragma once

#include "driver/spi_master.h"

class SPIDevice
{
public:
    SPIDevice(int cs_pin, int clk_pin, int mosi_pin, int miso_pin, int max_transfer_sz = 0, int dma_channel = SPI_DMA_CH_AUTO);

    void begin();
    void transfer(uint8_t *data, uint8_t len);
    void read(uint8_t *data, uint8_t len);

private:
    spi_device_handle_t handle;
};