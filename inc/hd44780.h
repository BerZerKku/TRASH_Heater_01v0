/*
 * hd44780.h
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */

#ifndef HD44780_H_
#define HD44780_H_

#include <stdio.h>

// кол-во строк, столбцов и символов экрана
#define LCD_NUM_ROW 2
#define LCD_NUM_COL 16
#define LCD_NUM_CHAR (LCD_NUM_ROW * LCD_NUM_COL)

// ЖКИ, DB4-DB7 должны идти последовательно
#define PORT_LCD 		PORTC
#define PIN_LCD			PINC
#define DDR_LCD 		DDRC
#define PORT_LCD_RS 	PC0
#define PORT_LCD_E 		PC1
#define PORT_LCD_DB4	PC2
#define PORT_LCD_MASK	((1 << PORT_LCD_RS) | (1 << PORT_LCD_E) | \
						 (0x0F << PORT_LCD_DB4))

// функция вывода для printf
extern int lcdPutchar(char c, FILE *stream);

// настройка переферии для ЖКИ
extern void initLcd(void);

// установка текущей позиции курсора на экране
// возвращает установленный индекс массива, либо -1 в случае выхода за диапазон
extern int8_t setPosLcd(uint8_t row, uint8_t col);

// возвращает текущий индекс массива
extern uint8_t getPosLcd();

// основной цикл работы ЖКИ, вызывать раз в 1мс
extern void cycleLcd(void);

// обновление информации на экране
extern void refreshLcd(void);

// очистка экрана
extern void clearLcd(void);

#endif /* HD44780_H_ */
