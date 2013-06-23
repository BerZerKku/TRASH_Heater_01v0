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
	KEY_SAVE,
	KEY_MENU,
	KEY_INC,
	KEY_DEC
};

// клавиатура
#define PORT_BUT 		PORTD
#define PIN_BUT			PIND
#define DDR_BUT 		DDRD
#define PIN_BUT_SAVE 	PIND0
#define PIN_BUT_MENU 	PIND1
#define PIN_BUT_INC 	PIND2
#define PIN_BUT_DEC 	PIND3
#define PIN_BUT_MASK	((1 << PIN_BUT_SAVE) | (1 << PIN_BUT_MENU) | \
						 (1 << PIN_BUT_INC)	 | (1 << PIN_BUT_DEC))

#define TIME_DELAY 5

// инициализация используемой периферии
extern void initKeyboard(void);

// считывание состояния клавиатуры
extern KEYS getKey(void);

// сканер клавиатуры, должен вызываться раз в (1-10)мс
extern void scanKey(void);

#endif /* KEYBOARD_H_ */
