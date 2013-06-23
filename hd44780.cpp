/*
 * hd44780.cpp
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */
#include <avr/io.h>
#include <avr/pgmspace.h>
#include "inc/hd44780.h"

// состояния работы
#define LCD_SOST_NO 0
#define LCD_SOST_INIT 1
#define LCD_SOST_PRINT 2
#define LCD_SOST_NEW_STRING 3

// буфер данных выводимых на ЖКИ
static char bufLcd[LCD_NUM_CHAR];
// позиция текущего символа в буфере
static uint8_t pos = 0;
// состояние работы
static uint8_t sost = LCD_SOST_NO;

// массив преобразования кода русских букв в коды ЖКИ
static char decode[] PROGMEM = {0x41,0xA0,0x42,0xA1,0xE0,0x45,0xA3,0xA4,
								0xA5,0xA6,0x4B,0xA7,0x4D,0x48,0x4F,0xA8,
								0x50,0x43,0x54,0xA9,0xAA,0x58,0xE1,0xAB,
								0xAC,0xE2,0xAD,0xAE,0xAD,0xAF,0xB0,0xB1,
								0x61,0xB2,0xB3,0xB4,0xE3,0x65,0xB6,0xB7,
							 	0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0x6F,0xBE,
								0x70,0x63,0xBF,0x79,0xE4,0x78,0xE5,0xC0,
								0xC1,0xE6,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7};

int lcdPutchar(char c, FILE *stream)
{
	if (pos < LCD_NUM_CHAR)
	{
		if ((c >= 'А') && (c <= 'я'))
			c = pgm_read_byte(decode + (c - 'А'));

		// символ окончания строки игнорируется
		if (c != 0x00)
			bufLcd[pos++] = c;
	}

	return 0;
}

int8_t setPosLcd(uint8_t row, uint8_t col)
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

uint8_t getPosLcd()
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
	// время до выполнения следующей команды
	static uint8_t delay = 0;
	// индекс отображаемого символа
	static uint8_t curIndex = 0;
	// шаг инициализации
	static uint8_t step = 0;

	char tmp = 0;

	if (sost == LCD_SOST_PRINT)
	{
		// вывод на экран текущего символа
		tmp = bufLcd[curIndex++];
		writeData(tmp >> 4);
		writeData(tmp);
		// проверка на необходимость перехода на следующую строку
		if (((curIndex % LCD_NUM_COL) == 0) || (curIndex >= LCD_NUM_CHAR))
			sost = LCD_SOST_NEW_STRING;
	}
	else if (sost == LCD_SOST_NEW_STRING)
	{
		// установка в ЖКИ необходимой строки
		// при переходе на первую, уход в режим ожидания
		tmp = curIndex / LCD_NUM_COL;

		sost = LCD_SOST_PRINT;

		if (tmp >= LCD_NUM_ROW)
		{
			tmp = 0x80;
			curIndex = 0;
			sost = LCD_SOST_NO;
		}
		else if (tmp == 1)
			tmp = 0xC0;
		else if (tmp == 2)
			tmp = 0x80 + LCD_NUM_COL;
		else if (tmp == 3)
			tmp = 0xC0 + LCD_NUM_COL;

		writeCom(tmp >> 4);
		writeCom(tmp);

	}
	else if (sost == LCD_SOST_INIT)
	{
		// инициализация ЖКИ
		if (delay > 0)
			delay--;
		else
		{

			switch (step)
			{
				case 0:
					// ожидание 50мс
					delay = 50;
					step++;
					break;
				case 1:
				case 2:
				case 3:
					// трижды устанавливается 8-и битный интерфейс
					writeCom(0x03);
					step++;
					delay = 10;
					break;
				case 4:
					// установка 4-х битный интерфейса
					writeCom(0x02);
					step++;
					break;
				case 5:
					// установка 4-х битный интерфейса и 2-х строк
					writeCom(0x02);
					writeCom(0x08);
					step++;
					break;
				case 6:
					// инкремент курсора
					writeCom(0x00);
					writeCom(0x06);
					step++;
					break;
				case 7:
					// включение дисплея
					writeCom(0x00);
					writeCom(0x0C);
					step++;
					break;
				case 8:
					// очистка дисплея
					writeCom(0x00);
					writeCom(0x01);
					step++;
					delay = 2;
					break;
				case 9:
					// переход в режим вывода данных
					step = 0;
					curIndex = 0;
					sost = LCD_SOST_PRINT;
					break;
				default:
					step = 0;
					break;
			}
		}
	}
}

void initLcd(void)
{
	uint8_t tmp = 0;

	clearLcd();

	setPosLcd(0, 0);

	// настройка портов ЖКИ
	tmp = PORT_LCD_MASK;
	DDR_LCD |= tmp;
	PORT_LCD &= ~tmp;

	// старт инициализации ЖКИ
	sost = LCD_SOST_INIT;
}

void refreshLcd(void)
{
	if (sost == LCD_SOST_NO)
		sost = LCD_SOST_PRINT;
}

void clearLcd(void)
{
	for (uint_fast8_t i = 0; i < LCD_NUM_CHAR; i++)
			bufLcd[i] = ' ';
}
