/*
 * keyboard.h
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

enum KEYS
{
	KEY_NO = 0,
	KEY_PUSK,
	KEY_RES,
	KEY_SET,
	KEY_INC,
	KEY_DEC,
	KEY_SEL,
	KEY_TIME
};

// клавиатура
#define PORT_BUT 		PORTD
#define PIN_BUT			PIND
#define DDR_BUT 		DDRD
#define PIN_BUT_PUSK 	PIND0
#define PIN_BUT_RES 	PIND1
#define PIN_BUT_SET 	PIND2
#define PIN_BUT_INC 	PIND3
#define PIN_BUT_DEC 	PIND4
#define PIN_BUT_SEL 	PIND5
#define PIN_BUT_TIME 	PIND6
#define PIN_BUT_MASK	((1 << PIN_BUT_PUSK) | (1 << PIN_BUT_RES) | \
						 (1 << PIN_BUT_SET)	 | (1 << PIN_BUT_INC) | \
						 (1 << PIN_BUT_DEC)	 | (1 << PIN_BUT_SEL) | \
						 (1 << PIN_BUT_TIME))

#define TIME_DELAY 5

// инициализация используемой периферии
extern void initKeyboard(void);

// считывание состояния клавиатуры
extern KEYS getKey(void);

// сканер клавиатуры, должен вызываться раз в (1-10)мс
extern void scanKey(void);

#endif /* KEYBOARD_H_ */
