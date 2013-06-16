/*
 * main.cpp
 *
 *  Created on: 25.05.2013
 *      Author: �������������
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <stdio.h>
#include "inc/hd44780.h"
#include "inc/keyboard.h"


// ������ ����������
#define PORT_CTRL 		PORTB
#define PIN_CTRL		PINB
#define DDR_CTRL 		DDRB
#define PORT_CTRL_1 	PB1
#define PORT_CTRL_2 	PB2

// �������� �����
#define PORT_TEST 		PORTD
#define PIN_TEST		PIND
#define DDR_TEST		DDRD
#define PORT_TEST_1		PD7

enum SOST
{
	SOST_STOP,
	SOST_PUSK,
	SOST_EDIT_T1_ON,
	SOST_EDIT_T2_ON,
	SOST_EDIT_T1_OFF
};

// ������������� ������������ ���������
static void init(void);

// ���������� ��������
static void ctrlOut(void);

// ���������� ������� ������
static void keyboard(KEYS key);

// ��������� ��������� �������� ��� ������ �����
static void ctrlNewCycle(void);

// ���� 500��
static volatile bool b500ms = false;
// ����� ��������� ������ ��������
static uint8_t cntOut1on = 0;
static uint8_t timeOut1on = 5;
// ����� ��������� ������ ��������
static uint8_t cntOut2on = 0;
static uint8_t timeOut2on = 3;
// ����� ���������� ������ ��������
static uint8_t cntOut1off = 0;
static uint8_t timeOut1off = 10;
// ������� ��������� ���� = false / ���� = true
static SOST sost = SOST_STOP;


static
void init(void)
{
	uint8_t tmp = 0;

	// ��������� ������ ����������
	tmp = (1 << PORT_CTRL_1) | (1 << PORT_CTRL_2);

	DDR_CTRL |= tmp;
	PORT_CTRL &= ~tmp;

	DDR_TEST |= (1 << PORT_TEST_1);
	PORT_TEST &= ~(1 << PORT_TEST_1);

	// ������������� ������� 2
	// (1��� / 8) / 125 = 1��
	TCCR2 = (0 << CS02) | (1 << CS01) | (0 << CS00);	// �������� �� 8
	TCCR2 |= (1 << WGM21) | (0 << WGM20); 	// ����� ���
	OCR2 = 125 - 1;
	TIMSK |= (1 << OCIE2);

	// ������������� ������� 1
	// (1��� / 8) / 12500 = 100vc
	TCCR1A = (0 << WGM11) | (0 << WGM10); 	//
	TCCR1B = (0 << WGM13) | (1 << WGM12);	// ����� ���
	OCR1A = 62500 - 1;
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10); // �������� �� 8
	TIMSK |= (1 << OCIE1A);
}


static
void ctrlNewCycle(void)
{
	PORT_CTRL &= ~((1 << PORT_CTRL_1) | (1 << PORT_CTRL_2));

	cntOut1on = timeOut1on;
	cntOut2on = timeOut2on;
	cntOut1off = timeOut1off - 1;
}

static
void ctrlOut(void)
{
	// �������� ������� ���. �������� 2
	if (cntOut2on > 0)
	{
		cntOut2on--;
	}
	else
		PORT_CTRL &= ~(1 << PORT_CTRL_2);

	// �������� ������� ��������� �������� 1
	if (cntOut1on > 0)
	{
		// ��������� �������� 1,2 � ������ �����
		if (cntOut1on == timeOut1on)
		{
			PORT_CTRL |= (1 << PORT_CTRL_1) | (1 << PORT_CTRL_2);
		}
		cntOut1on--;
	}
	else
	{
		// ���������� �������� 1
		PORT_CTRL &= ~(1 << PORT_CTRL_1);

		// �������� ������� ����. �������� 1
		if (cntOut1off > 0)
		{
			cntOut1off--;
		}
		else
		{
			// ��������� �������� ��� ������ �����
			ctrlNewCycle();
		}
	}
}

static
void keyboard(KEYS key)
{
	switch(key)
	{
		case KEY_PUSK:
			ctrlNewCycle();
			if (sost == SOST_STOP)
				sost = SOST_PUSK;
			else
				sost = SOST_STOP;
			break;
	}
}

__attribute((OS_main))
int main(void)
{
	char tmp = 0;

	init();
	initLcd();
	initKeyboard();

	ctrlNewCycle();

	fdevopen(&lcdPutchar,NULL);

	setPos(1, 1);
	printf("�����Ũ���������");
	setPos(2, 1);
	printf("����������������");

	sei();

	while(1)
	{
		if (b500ms)
		{
			// ��������� ������� ������
			KEYS key = getKey();
			if (key != KEY_NO)
			{
				keyboard(key);
			}

			// ���������� �������� �������������� ������ ��� �����
			// ����� ������ ����.
			if (sost == SOST_PUSK)
			{
				ctrlOut();
			}
			else
			{
				PORT_CTRL &= ~((1 << PORT_CTRL_1) | (1 << PORT_CTRL_2));
			}
			b500ms = false;

//			setPos(1, 1);
//			printf("%d", tmp % 10);
//			setPos(2, 1);
//			printf("%d", tmp % 10);
//			tmp++;

			refreshLcd();
		}
	};
}

ISR(TIMER2_COMP_vect)
{
	cycleLcd();
	scanKey();
}

ISR(TIMER1_COMPA_vect)
{
	b500ms = true;
}
