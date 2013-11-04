/*
 * keyboard.cpp
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */
#include <avr/io.h>
#include "inc/keyboard.h"

// текущее состояние клавиатуры
KEYS key = KEY_NO;

void initKeyboard(void)
{
	uint8_t tmp = 0;

	// настройка портов клавиатуры
	tmp = PIN_BUT_MASK;

	DDR_BUT &= 	~tmp;
	PORT_BUT &= ~tmp;
}

KEYS getKey(void)
{
	KEYS tmp = key;

	key = KEY_NO;
	return tmp;
}

void scanKey(void)
{
	static uint8_t last_keys = PIN_BUT_MASK;
	static uint16_t cnt_delay = 0;

	uint8_t tmp = PIN_BUT & PIN_BUT_MASK;

	// есть нажатая кнопка
	if (tmp != last_keys)
	{
		// считанное состояние клавиатуры отличается от предыдущего
		// запомним его и сбросим счетчик
		last_keys = tmp;
		cnt_delay = 1;
	}
	else
	{
		// проверка длительности нажатия
		if (cnt_delay < TIME_DELAY)
		{
			// защита от помех + антидребезг
			cnt_delay++;
		}
		else if (cnt_delay == TIME_DELAY)
		{
			// проверим нажатую кнопку
			tmp = ~tmp;
			if (tmp & (1 << PIN_BUT_SAVE))
				key = KEY_SAVE;
			else if (tmp & (1 << PIN_BUT_MENU))
				key = KEY_MENU;
			else if (tmp & (1 << PIN_BUT_INC))
				key = KEY_INC;
			else if (tmp & (1 << PIN_BUT_DEC))
				key = KEY_DEC;
			else
				key = KEY_NO;

			cnt_delay++;
		}
		else if (cnt_delay < TIME_LONG_DELAY)
		{
			// ожидание времени до определения длительного нажатия
			cnt_delay++;
		}
		else if (cnt_delay == TIME_LONG_DELAY)
		{
			// проверим длительное нажатие кнопки
			tmp = ~tmp;
			if (tmp & (1 << PIN_BUT_INC))
				key = KEY_INC_LONG;

			cnt_delay++;
		}
	}
}



