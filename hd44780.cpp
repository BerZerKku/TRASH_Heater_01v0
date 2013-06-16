/*
 * hd44780.cpp
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */
#include <avr/io.h>
#include "inc/hd44780.h"

#define LCD_SOST_INIT 0
#define LCD_SOST_PRINT 1
#define LCD_SOST_NEW_STRING 2

static char bufLcd[LCD_NUM_CHAR];
static volatile uint8_t pos = 0;
static volatile uint8_t sost = 0;

int lcdPutchar(char c, FILE *stream)
{
	if (pos < LCD_NUM_CHAR)
		bufLcd[pos++] = c;

	return 0;
}

int8_t setPos(uint8_t row, uint8_t col)
{
	int8_t p = (row - 1)*LCD_NUM_COL + col - 1;

	if (p >= LCD_NUM_CHAR)
	{
		p = -1;
	}
	else
		pos = p;

	return p;
}

uint8_t getPos()
{
	return pos;
}

static
void writeCom(char com)
{
	PORT_LCD &= ~(1 << PORT_LCD_RS);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD |= (1 << PORT_LCD_E);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD &= ~(0x0F << PORT_LCD_DB4);
	PORT_LCD |= ((com & 0x0F) << PORT_LCD_DB4);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD &= ~(1 << PORT_LCD_E);
	PORT_LCD |= (1 << PORT_LCD_RS);
}

static
void writeData(char data)
{
	PORT_LCD |= (1 << PORT_LCD_RS);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD |= (1 << PORT_LCD_E);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD &= ~(0x0F << PORT_LCD_DB4);
	PORT_LCD |= ((data & 0x0F) << PORT_LCD_DB4);
	asm volatile ("nop");
	asm volatile ("nop");
	PORT_LCD &= ~(1 << PORT_LCD_E);
	PORT_LCD |= (1 << PORT_LCD_RS);
}

void cycleLcd(void)
{
	static uint8_t delay = 0;
	static uint8_t p = 0;
	char tmp = 0;

	if (delay > 0)
		delay--;
	else
	{
		switch(sost)
		{
			case 10:
				tmp = p / LCD_NUM_COL;

				if (tmp >= LCD_NUM_ROW)
				{
					tmp = 0x80;
					p = 0;
				}
				else if (tmp == 1)
					tmp = 0xC0;
				else if (tmp == 2)
					tmp = 0x80 + LCD_NUM_COL;
				else if (tmp == 3)
					tmp = 0xC0 + LCD_NUM_COL;
				writeCom(tmp >> 4);
				writeCom(tmp);
				sost -= 1;
				break;
			case 9:
				tmp =  bufLcd[p++];
				writeData(tmp >> 4);
				writeData(tmp);
				if ((p % LCD_NUM_COL) == 0)
					sost += 1;
				break;
			case 6:
				// инкремент курсора
				writeCom(0x00);
				writeCom(0x06);
				sost += 1;
				break;
			case 8:
				// включение дисплея
				writeCom(0x00);
				writeCom(0x01);
				sost += 1;
				delay = 2;
				break;
			case 7:
				// включение дисплея
				writeCom(0x00);
				writeCom(0x0C);
				sost += 1;
				break;
			case 5:
				// установка 4-х битный интерфейса и 2-х линий
				writeCom(0x02);
				writeCom(0x08);
				sost += 1;
				break;
			case 4:
				// установка 4-х битный интерфейса
				writeCom(0x02);
				sost += 1;
				break;
			case 1:
			case 2:
			case 3:
				// трижды устанавливается 8-и битный интерфейс
				writeCom(0x03);
				sost += 1;
				delay = 10;
				p = 0;
				break;
			case 0:
				// ожидание 50мс
				delay = 50;
				sost = 1;
				break;

		}
	}
}

void initLcd(void)
{
	uint8_t tmp = 0;

	// старт инициализации ЖКИ
	sost = 0;

	for(uint_fast8_t i = 0; i < LCD_NUM_CHAR; i++)
		bufLcd[i] = ' ';

	setPos(0, 0);

	// настройка портов ЖКИ
	tmp = PORT_LCD_MASK;
	DDR_LCD |= tmp;
	PORT_LCD &= ~tmp;
}

