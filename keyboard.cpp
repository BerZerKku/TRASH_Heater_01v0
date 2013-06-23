/*
 * keyboard.cpp
 *
 *  Created on: 25.05.2013
 *      Author: �������������
 */
#include <avr/io.h>
#include "inc/keyboard.h"

// ������� ��������� ����������
KEYS key = KEY_NO;

void initKeyboard(void)
{
	uint8_t tmp = 0;

	// ��������� ������ ����������
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
	static uint8_t cnt_delay = 0;

	uint8_t tmp = PIN_BUT & PIN_BUT_MASK;

	// ���� ������� ������
	if (tmp != last_keys)
	{
		// ��������� ��������� ���������� ���������� �� �����������
		// �������� ��� � ������� �������
		last_keys = tmp;
		cnt_delay = 1;
	}
	else
	{
		// �������� ������������ �������
		if (cnt_delay < TIME_DELAY)
		{
			cnt_delay++;
		}
		else if (cnt_delay == TIME_DELAY)
		{
			tmp = ~tmp;
			// �������� ������� ������
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
	}
}



