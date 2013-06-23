/*
 * hd44780.h
 *
 *  Created on: 25.05.2013
 *      Author: �������������
 */

#ifndef HD44780_H_
#define HD44780_H_

#include <stdio.h>

// ���-�� �����, �������� � �������� ������
#define LCD_NUM_ROW 2
#define LCD_NUM_COL 16
#define LCD_NUM_CHAR (LCD_NUM_ROW * LCD_NUM_COL)

// ���, DB4-DB7 ������ ���� ���������������
#define PORT_LCD 		PORTC
#define PIN_LCD			PINC
#define DDR_LCD 		DDRC
#define PORT_LCD_RS 	PC0
#define PORT_LCD_E 		PC1
#define PORT_LCD_DB4	PC2
#define PORT_LCD_MASK	((1 << PORT_LCD_RS) | (1 << PORT_LCD_E) | \
						 (0x0F << PORT_LCD_DB4))

// ������� ������ ��� printf
extern int lcdPutchar(char c, FILE *stream);

// ��������� ��������� ��� ���
extern void initLcd(void);

// ��������� ������� ������� ������� �� ������
// ���������� ������������� ������ �������, ���� -1 � ������ ������ �� ��������
extern int8_t setPosLcd(uint8_t row, uint8_t col);

// ���������� ������� ������ �������
extern uint8_t getPosLcd();

// �������� ���� ������ ���, �������� ��� � 1��
extern void cycleLcd(void);

// ���������� ���������� �� ������
extern void refreshLcd(void);

// ������� ������
extern void clearLcd(void);

#endif /* HD44780_H_ */
