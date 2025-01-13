/*
MIT License

Copyright (c) 2016 Mike Estee

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c\n"
#define BYTE_TO_BINARY(byte)         \
	((byte) & 0x80 ? '1' : '0'),     \
		((byte) & 0x40 ? '1' : '0'), \
		((byte) & 0x20 ? '1' : '0'), \
		((byte) & 0x10 ? '1' : '0'), \
		((byte) & 0x08 ? '1' : '0'), \
		((byte) & 0x04 ? '1' : '0'), \
		((byte) & 0x02 ? '1' : '0'), \
		((byte) & 0x01 ? '1' : '0')

#include "TMC5160.h"

TMC5160_SPI::TMC5160_SPI(uint8_t chipSelectPin, SPIDevice spi, uint32_t fclk)
	: TMC5160(DEFAULT_F_CLK), _CS(chipSelectPin), _spi(spi)
{
	pinMode(_CS, OUTPUT);
	digitalWrite(_CS, HIGH);
}

void TMC5160_SPI::_beginTransaction()
{
	// _spi->beginTransaction(_spiSettings);
	digitalWrite(_CS, LOW);
}

void TMC5160_SPI::_endTransaction()
{
	digitalWrite(_CS, HIGH);
	//_spi->endTransaction();
}

uint32_t TMC5160_SPI::readRegister(uint8_t chip_id, uint8_t address)
{
	// request the read for the address
	uint8_t status = 0;

	uint8_t *buffer = (uint8_t *)heap_caps_aligned_alloc(4, 5 * CHAINED_CHIPS, MALLOC_CAP_DMA);
	_beginTransaction();
	for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	{
		if (i == chip_id)
		{
			buffer[i * 5] = (address);
			buffer[(i * 5) + 1] = (0x00);
			buffer[(i * 5) + 2] = (0x00);
			buffer[(i * 5) + 3] = (0x00);
			buffer[(i * 5) + 4] = (0x00);
		}
		else
		{
			buffer[i * 5] = (0x00);
			buffer[(i * 5) + 1] = (0x00);
			buffer[(i * 5) + 2] = (0x00);
			buffer[(i * 5) + 3] = (0x00);
			buffer[(i * 5) + 4] = (0x00);
		}
	}

	_spi.transfer(buffer, 5 * CHAINED_CHIPS);
	uint8_t spi_status = buffer[5 * chip_id];
	printf("SPI ST4TUS %d: " BYTE_TO_BINARY_PATTERN, chip_id, BYTE_TO_BINARY(spi_status));
	_endTransaction();

// Delay for the minimum CSN high time (2tclk + 10ns -> 176ns with the default 12MHz clock)
#ifdef TEENSYDUINO
	delayNanoseconds(2 * 1000000000 / _fclk + 10);
#else
	delayMicroseconds(1);
#endif
	memset(buffer, 0, 5 * CHAINED_CHIPS);
	// read it in the second cycle
	_beginTransaction();
	// for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	// {
	// 	if (i == chip_id)
	// 	{

	// 		uint8_t spi_status = _spi->transfer(address);
	// 		printf("SPI STATUS %d: " BYTE_TO_BINARY_PATTERN, chip_id, BYTE_TO_BINARY(spi_status));
	// 		value[i] |= (uint32_t)_spi->transfer(0x00) << 24;
	// 		value[i] |= (uint32_t)_spi->transfer(0x00) << 16;
	// 		value[i] |= (uint32_t)_spi->transfer(0x00) << 8;
	// 		value[i] |= (uint32_t)_spi->transfer(0x00);
	// 	}
	// 	else
	// 	{
	// 		_spi->transfer(0x00);
	// 		_spi->transfer(0x00);
	// 		_spi->transfer(0x00);
	// 		_spi->transfer(0x00);
	// 		_spi->transfer(0x00);
	// 	}
	// }
	_spi.read(buffer, 5 * CHAINED_CHIPS);
	 spi_status = buffer[5 * chip_id];
	printf("SPI STATUS %d: " BYTE_TO_BINARY_PATTERN, chip_id, BYTE_TO_BINARY(spi_status));
	value[chip_id] |= (uint32_t)buffer[5 * chip_id + 1] << 24;
	value[chip_id] |= (uint32_t)buffer[5 * chip_id + 2] << 16;
	value[chip_id] |= (uint32_t)buffer[5 * chip_id + 3] << 8;
	value[chip_id] |= (uint32_t)buffer[5 * chip_id + 4];

	_endTransaction();

	_lastRegisterReadSuccess = true; // In SPI mode there is no way to know if the TMC5130 is plugged...
	heap_caps_aligned_free(buffer);
	return value[chip_id];
}

uint8_t TMC5160_SPI::writeRegister(uint8_t chip_id, uint8_t address, uint32_t data)
{
	uint8_t status = 0;
	spi_transaction_t t;
	memset(&t, 0, sizeof(t));
	uint8_t *buffer = (uint8_t *)heap_caps_aligned_alloc(4, 5 * CHAINED_CHIPS, MALLOC_CAP_DMA);
	// address register
	_beginTransaction();
	for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	{
		if (i == chip_id)
		{
			buffer[i * 5] = (address | WRITE_ACCESS);
			
			// send new register value
			buffer[(i * 5) + 1] = ((data & 0xFF000000) >> 24);
			buffer[(i * 5) + 2] = ((data & 0xFF0000) >> 16);
			buffer[(i * 5) + 3] = ((data & 0xFF00) >> 8);
			buffer[(i * 5) + 4] = (data & 0xFF);
		}
		else
		{
			buffer[(i * 5)] = (0x00);
			buffer[(i * 5) + 1] = (0x00);
			buffer[(i * 5) + 2] = (0x00);
			buffer[(i * 5) + 3] = (0x00);
			buffer[(i * 5) + 4] = (0x00);
		}
	}
	_spi.transfer(buffer, 5 * CHAINED_CHIPS);
	printf("SPI STATUS 0: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(buffer[0]));
	printf("SPI STATUS 1: " BYTE_TO_BINARY_PATTERN, BYTE_TO_BINARY(buffer[5]));
	_endTransaction();
	heap_caps_aligned_free(buffer);
	return status;
}

uint8_t TMC5160_SPI::readStatus(uint8_t chip_id)
{
	// read general config
	_beginTransaction();
	// uint8_t status = _spi->transfer(TMC5160_Reg::GCONF);
	// // send dummy data
	// _spi->transfer(0x00);
	// _spi->transfer(0x00);
	// _spi->transfer(0x00);
	// _spi->transfer(0x00);
	_endTransaction();
	uint8_t status = 0;
	return status;
}
