/*
 * main.cpp
 *
 *  Created on: 25.05.2013
 *      Author: Администратор
 */
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <avr/eeprom.h>
#include <stdio.h>
#include "inc/hd44780.h"
#include "inc/keyboard.h"

const uint8_t* eeprom_time_1_on = (uint8_t*) 0x16;
const uint8_t* eeprom_time_1_off = (uint8_t*) 0x17;
const uint8_t* eeprom_time_2_on = (uint8_t*) 0x18;


// выходы управления
#define PORT_CTRL 		PORTB
#define PIN_CTRL		PINB
#define DDR_CTRL 		DDRB
#define PORT_CTRL_1 	PB1
#define PORT_CTRL_2 	PB2

// тестовый выход
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

// инициализация используемой периферии
static void init(void);

// управление выходами
static void ctrlOut(void);

// обработчик нажатия кнопки
//static void keyboard(KEYS key);

// установка начальных значений для нового цикла
static void ctrlNewCycle(void);

// флаг 500мс
static volatile bool b500ms = false;

// частота циклов, Гц
static const uint8_t freq_cycle = 2;
// значения таймеров устанаваливаются в секундах
// время включения первой нагрузки
static uint8_t cntOut1on = 0;
static uint8_t timeOut1on = 5;
static const uint8_t time_out_1_on_min = 1 * freq_cycle;
static const uint8_t time_out_1_on_max = 60 * freq_cycle;
// время включения второй нагрузки
static uint8_t cntOut2on = 0;
static uint8_t timeOut2on = 3;
static const uint8_t time_out_2_on_min = 1 * freq_cycle;
static const uint8_t time_out_2_on_max = 30 * freq_cycle;
// время выключения первой нагрузки
static uint8_t cntOut1off = 0;
static uint8_t timeOut1off = 10;
static const uint8_t time_out_1_off_min = 1 * freq_cycle;
static const uint8_t time_out_1_off_max = 60 * freq_cycle;

// время отображения помощи
static const uint8_t time_help = 5 * freq_cycle;

// указатель на изменяемую переменную
static uint8_t *editParam = &timeOut1on;

// текущее состояние стоп = false / пуск = true
static SOST sost = SOST_STOP;

static
void init(void)
{
	uint8_t tmp = 0;

	// настройка портов управления
	tmp = (1 << PORT_CTRL_1) | (1 << PORT_CTRL_2);

	DDR_CTRL |= tmp;
	PORT_CTRL &= ~tmp;

	DDR_TEST |= (1 << PORT_TEST_1);
	PORT_TEST &= ~(1 << PORT_TEST_1);

	// инициализация таймера 2
	// (1Мгц / 8) / 125 = 1мс
	TCCR2 = (0 << CS02) | (1 << CS01) | (0 << CS00);	// делитель на 8
	TCCR2 |= (1 << WGM21) | (0 << WGM20); 	// режим СТС
	OCR2 = 125 - 1;
	TIMSK |= (1 << OCIE2);

	// инициализация таймера 1
	// (1Мгц / 8) / 12500 = 100vc
	TCCR1A = (0 << WGM11) | (0 << WGM10); 	//
	TCCR1B = (0 << WGM13) | (1 << WGM12);	// режим СТС
	OCR1A = 62500 - 1;
	TCCR1B |= (0 << CS12) | (1 << CS11) | (0 << CS10); // делитель на 8
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
	// проверка времени вкл. нагружки 2
	if (cntOut2on > 0)
	{
		cntOut2on--;
	}
	else
		PORT_CTRL &= ~(1 << PORT_CTRL_2);

	// проверка времени включения нагрузки 1
	if (cntOut1on > 0)
	{
		// включение нагрузки 1,2 в начале цикла
		if (cntOut1on == timeOut1on)
		{
			PORT_CTRL |= (1 << PORT_CTRL_1) | (1 << PORT_CTRL_2);
		}
		cntOut1on--;
	}
	else
	{
		// отключение нагрузки 1
		PORT_CTRL &= ~(1 << PORT_CTRL_1);

		// проверка времени выкл. нагрузки 1
		if (cntOut1off > 0)
		{
			cntOut1off--;
		}
		else
		{
			// установка значений для нового цикла
			ctrlNewCycle();
		}
	}
}

//static
//void keyboard(KEYS key)
//{
//	switch(key)
//	{
//		case KEY_PUSK:
//			ctrlNewCycle();
//			if (sost == SOST_STOP)
//				sost = SOST_PUSK;
//			else
//				sost = SOST_STOP;
//			break;
//	}
//}

static
void editParameter(void)
{
	setPosLcd(1, 1);
	printf("+A");
	setPosLcd(2, 1);
	printf("-V");
	setPosLcd(1, 4);
	// вывод на экран времени 00,0
	printf("%02d,%d", *editParam/2, (*editParam%2)*5);
	setPosLcd(2, LCD_NUM_COL - 5);
	printf("< SOHR");
}

static
void getParamFromEeprom()
{
	uint8_t tmp = 0;

	tmp = eeprom_read_byte(eeprom_time_1_on);
	if ((tmp >= time_out_1_on_min) && (tmp <= time_out_1_on_max))
		timeOut1on = tmp;
	else
		timeOut1on = time_out_1_on_min;

	tmp = eeprom_read_byte(eeprom_time_1_off);
	if ((tmp >= time_out_1_off_min) && (tmp <= time_out_1_off_max))
		timeOut1off = tmp;
	else
		timeOut1off = time_out_1_off_max;

	tmp = eeprom_read_byte(eeprom_time_2_on);
	if ((tmp >= time_out_2_on_min) && (tmp <= time_out_2_on_max))
		timeOut2on = tmp;
	else
		timeOut2on = time_out_2_on_min;

}

__attribute((OS_main))
int main(void)
{
	char tmp = 0;
	uint8_t lvl_menu = 0;
	uint8_t help = 0;

	init();
	initLcd();
	initKeyboard();

	getParamFromEeprom();

	ctrlNewCycle();

	fdevopen(&lcdPutchar,NULL);
	sei();

	while(1)
	{
		if (b500ms)
		{
//			KEYS key = getKey();
//			if (key != KEY_NO)
//			{
//				keyboard(key);
//				PORT_TEST ^= (1 << PORT_TEST_1);
//			}

			// управление выходами осуществляется только при пуске
			// иначе выходы выкл.
			if (sost == SOST_PUSK)
			{
				ctrlOut();
			}
			else
			{
				PORT_CTRL &= ~((1 << PORT_CTRL_1) | (1 << PORT_CTRL_2));
			}
			b500ms = false;

//			setPosLcd(1, 1);
//			printf("%d", tmp % 10);
//			setPosLcd(2, 1);
//			printf("%d", tmp % 10);
//			tmp++;

			clearLcd();

			KEYS key = getKey();
			if (help > 0)
			{
				help--;
				setPosLcd(1, 6);
				printf("%02d,%dR", (timeOut1on/2), (timeOut1on%2)*5);
				setPosLcd(2, 6);
				printf("%02d,%dP", (timeOut1off/2), (timeOut1off%2)*5);
				if (key == KEY_SAVE)
				{
					help = 0;
				}
			}
			else
			{
				switch(lvl_menu)
				{
				case 0:
					setPosLcd(1, 5);
					printf("%02d,%d", timeOut1on/2, (timeOut1on%2)*5);
					setPosLcd(2, 1);
					printf("+ TURBO");
					setPosLcd(2, LCD_NUM_COL - 5);
					printf("MENU >");

					if (key == KEY_MENU)
					{
						lvl_menu = 1;
					}
					else if (key == KEY_SAVE)
					{
						help = time_help;
					}
					break;
				case 1:
					setPosLcd(1, 1);
					printf("TAIMER RABOTA");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						lvl_menu = 2;
						editParam = &timeOut1on;
					}
					break;
				case 2:
					editParameter();

					if (key == KEY_SAVE)
					{
						lvl_menu = 3;
					}
					else if (key == KEY_INC)
					{
						(*editParam)++;
					}
					else if (key == KEY_DEC)
					{
						(*editParam)--;
					}

					if (*editParam > time_out_1_on_max)
						*editParam = 2;
					else if (*editParam < time_out_1_on_min)
						*editParam = 2;
					break;
				case 3:
					setPosLcd(1, 1);
					printf("TAIMER PAUSA");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						lvl_menu = 4;
						editParam = &timeOut1off;
					}
					break;
				case 4:
					editParameter();

					if (key == KEY_SAVE)
					{
						lvl_menu = 5;
					}
					else if (key == KEY_INC)
					{
						(*editParam)++;
					}
					else if (key == KEY_DEC)
					{
						(*editParam)--;
					}
					break;

					if (*editParam > time_out_1_off_max)
						*editParam = 2;
					else if (*editParam < time_out_1_off_min)
						*editParam = 2;
				case 5:
					setPosLcd(1, 1);
					printf("TAIMER N2");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						lvl_menu = 6;
						editParam = &timeOut2on;
						tmp = 0;
					}
					break;
				case 6:
					editParameter();

					if (key == KEY_SAVE)
					{
						tmp = 1;
					}
					else if (key == KEY_INC)
					{

						if (tmp == 1)
						{
							lvl_menu = 7;
							if (sost == SOST_STOP)
								sost = SOST_PUSK;
						}
						else
							(*editParam)++;
					}
					else if (key == KEY_DEC)
					{
						(*editParam)--;
					}
					break;

					if (*editParam > time_out_2_on_max)
						*editParam = 2;
					else if (*editParam < time_out_2_on_min)
						*editParam = 2;
				case 7:
					if (cntOut1on > 0)
					{
						setPosLcd(1, 7);
						tmp = timeOut1on - cntOut1on;
						printf("%02d,%d", (tmp/2), (tmp%2)*5);
						setPosLcd(2, 3);
						printf("RABOTA SCHET");
					}
					else
					{
						setPosLcd(1, 7);
						tmp = timeOut1off - cntOut1off;
						printf("%02d,%d", (tmp/2), (tmp%2)*5);
						setPosLcd(2, 3);
						printf("PAUZA SCHET");
					}

					if (key == KEY_INC)
					{
						if (sost == SOST_PUSK)
							sost = SOST_STOP;
						lvl_menu = 0;
					}
					else if (key == KEY_SAVE)
					{
						help = time_help;
					}
					break;
				}
			}
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
