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

TMC5160_SPI::TMC5160_SPI(uint8_t chipSelectPin, uint32_t fclk, const SPISettings &spiSettings, SPIClass &spi)
	: TMC5160(fclk), _CS(chipSelectPin), _spiSettings(spiSettings), _spi(&spi)
{
	pinMode(_CS, OUTPUT);
	digitalWrite(_CS, HIGH);
}

void TMC5160_SPI::_beginTransaction()
{
	_spi->beginTransaction(_spiSettings);
	digitalWrite(_CS, LOW);
}

void TMC5160_SPI::_endTransaction()
{
	digitalWrite(_CS, HIGH);
	_spi->endTransaction();
}

uint32_t TMC5160_SPI::readRegister(uint8_t chip_id, uint8_t address)
{
	// request the read for the address
	_beginTransaction();
	for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	{
		if (i == chip_id)
		{
			_spi->transfer(address);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
		}
		else
		{
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
		}
	}
	_endTransaction();

// Delay for the minimum CSN high time (2tclk + 10ns -> 176ns with the default 12MHz clock)
#ifdef TEENSYDUINO
	delayNanoseconds(2 * 1000000000 / _fclk + 10);
#else
	delayMicroseconds(1);
#endif

	// read it in the second cycle
	_beginTransaction();
	for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	{
		if (i == chip_id)
		{
			uint8_t spi_status = _spi->transfer(address);
			printf("SPI STATUS %d: " BYTE_TO_BINARY_PATTERN, chip_id, BYTE_TO_BINARY(spi_status));
			value[i] |= (uint32_t)_spi->transfer(0x00) << 24;
			value[i] |= (uint32_t)_spi->transfer(0x00) << 16;
			value[i] |= (uint32_t)_spi->transfer(0x00) << 8;
			value[i] |= (uint32_t)_spi->transfer(0x00);
			
		}
		else
		{
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
		}
	}
	_endTransaction();

	_lastRegisterReadSuccess = true; // In SPI mode there is no way to know if the TMC5130 is plugged...

	return value[chip_id];
}

uint8_t TMC5160_SPI::writeRegister(uint8_t chip_id, uint8_t address, uint32_t data)
{
	uint8_t status = 0;
	// address register
	_beginTransaction();
	for (int i = CHAINED_CHIPS - 1; i >= 0; i--)
	{
		if (i == chip_id)
		{
			status = _spi->transfer(address | WRITE_ACCESS);
			printf("SPI STATUS %d: " BYTE_TO_BINARY_PATTERN, chip_id, BYTE_TO_BINARY(status));
			// send new register value
			_spi->transfer((data & 0xFF000000) >> 24);
			_spi->transfer((data & 0xFF0000) >> 16);
			_spi->transfer((data & 0xFF00) >> 8);
			_spi->transfer(data & 0xFF);
			
		}
		else
		{
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
			_spi->transfer(0x00);
		}
	}
	_endTransaction();

	return status;
}

uint8_t TMC5160_SPI::readStatus(uint8_t chip_id)
{
	// read general config
	_beginTransaction();
	uint8_t status = _spi->transfer(TMC5160_Reg::GCONF);
	// send dummy data
	_spi->transfer(0x00);
	_spi->transfer(0x00);
	_spi->transfer(0x00);
	_spi->transfer(0x00);
	_endTransaction();

	return status;
}
