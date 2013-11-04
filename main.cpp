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

uint8_t* const eeprom_time_1_on = (uint8_t*) 0x16;
uint8_t* const eeprom_time_1_off = (uint8_t*) 0x17;
uint8_t* const eeprom_time_2_on = (uint8_t*) 0x18;


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

enum dSOST
{
	SOST_STOP,
	SOST_PUSK
};

enum dLVL_MENU
{
	LVL_START,
	LVL_TIME_1_ON,
	LVL_TIME_1_ON_EDIT = LVL_TIME_1_ON + 1,
	LVL_TIME_1_OFF,
	LVL_TIME_1_OFF_EDIT = LVL_TIME_1_OFF + 1,
	LVL_TIME_2_ON,
	LVL_TIME_2_ON_EDIT = LVL_TIME_2_ON + 1,
	LVL_WORK
};

// инициализация используемой периферии
static void init();

// установка начальных значений для нового цикла
static void ctrlNewCycle();

// управление выходами
static void ctrlOut();

// выводит на экран меню редактирования параметра
static void editParameterMenu(uint8_t value);

// увеличение значения параметра, с проверкой на максимальное значение
static uint8_t incValue(uint8_t value, uint8_t max);

// уменьшение значения параметра, с проверкой на минимальное значение
static uint8_t decValue(uint8_t value, uint8_t min);

// загрузка значений параметров из ЕЕПРОМ
// по-умолчанию будут выставлены минимальное время работы и максимальная пауза
static void loadParamFromEeprom();

// запись значений параметров в ЕЕПРОМ
static void saveParamToEeprom();

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

// текущее состояние стоп = false / пуск = true
static dSOST sost = SOST_STOP;

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
	// (1Мгц / 8) / 12500 = 100мс
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

static
void editParameterMenu(uint8_t value = 0)
{
	setPosLcd(1, 1);
	printf("+A");
	setPosLcd(2, 1);
	printf("-V");
	setPosLcd(1, 4);
	// вывод на экран времени 00,0
	printf("%02d,%d", value/freq_cycle, (value%freq_cycle)*5);
	setPosLcd(2, LCD_NUM_COL - 5);
	printf("< SOHR");
}

static
uint8_t incValue(uint8_t value, uint8_t max)
{
	return (value < max) ? value + 1 : max;
}

static
uint8_t decValue(uint8_t value, uint8_t min)
{
	return (value > min) ? value - 1 : min;
}

static
void loadParamFromEeprom()
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

static
void saveParamToEeprom()
{
	eeprom_update_byte(eeprom_time_1_on, timeOut1on);
	eeprom_update_byte(eeprom_time_1_off, timeOut1off);
	eeprom_update_byte(eeprom_time_2_on, timeOut2on);
}

__attribute((OS_main))
int main(void)
{
	char tmp = 0;
	dLVL_MENU eLvlMenu = LVL_START;
	uint8_t help = 0;

	// редактирование значения параметра
	uint8_t *enterParam = &timeOut1on;
	uint8_t enterValue = 0;
	uint8_t enterValueMin = 0;
	uint8_t enterValueMax = 0;

	init();
	initLcd();
	initKeyboard();

	loadParamFromEeprom();

	ctrlNewCycle();

	fdevopen(&lcdPutchar,NULL);
	sei();

	while(1)
	{
		if (b500ms)
		{
			b500ms = false;

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

			clearLcd();
			KEYS key = getKey();
			if (help > 0)
			{
				// вывод на экран текущих настроек
				help--;
				setPosLcd(1, 6);
				printf("%02d,%dR", 	(timeOut1on/freq_cycle),
									(timeOut1on%freq_cycle)*5);
				setPosLcd(2, 6);
				printf("%02d,%dP",  (timeOut1off/freq_cycle),
									(timeOut1off%freq_cycle)*5);
				if (key == KEY_SAVE)
				{
					help = 0;
				}
			}
			else
			{
				// вывод на экран текущего уровня меню
				switch(eLvlMenu)
				{
				case LVL_START:
					setPosLcd(1, 5);
					printf("%02d,%d", 	timeOut1on/freq_cycle,
										(timeOut1on%freq_cycle)*5);
					setPosLcd(2, 1);
					printf("+ TURBO");
					setPosLcd(2, LCD_NUM_COL - 5);
					printf("MENU >");

					if (key == KEY_MENU)
					{
						eLvlMenu = LVL_TIME_1_ON;
					}
					else if (key == KEY_SAVE)
					{
						help = time_help;
					}
					else if (key == KEY_INC_LONG)
					{
						if (sost == SOST_STOP)
						{
							sost = SOST_PUSK;
							eLvlMenu = LVL_WORK;
						}
						else
						{
							sost = SOST_STOP;
							ctrlNewCycle();
						}
					}
					break;
				case LVL_TIME_1_ON:
					setPosLcd(1, 1);
					printf("TAIMER RABOTA");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						eLvlMenu = LVL_TIME_1_ON_EDIT;
						enterParam = &timeOut1on;
						enterValue = *enterParam;
						enterValueMin = time_out_1_on_min;
						enterValueMax = time_out_1_on_max;
					}
					break;
				case LVL_TIME_1_ON_EDIT:
				case LVL_TIME_1_OFF_EDIT:
				case LVL_TIME_2_ON_EDIT:
					editParameterMenu(enterValue);

					if (key == KEY_SAVE)
					{
						*enterParam = enterValue;
						if (eLvlMenu == LVL_TIME_2_ON_EDIT)
							eLvlMenu = LVL_START;
						else
							eLvlMenu = static_cast<dLVL_MENU> (eLvlMenu + 1);
					}
					else if (key == KEY_INC)
					{
						enterValue = incValue(enterValue, enterValueMax);
					}
					else if (key == KEY_DEC)
					{
						enterValue = decValue(enterValue, enterValueMin);
					}
					break;
				case LVL_TIME_1_OFF:
					setPosLcd(1, 1);
					printf("TAIMER PAUSA");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						eLvlMenu = LVL_TIME_1_OFF_EDIT;
						enterParam = &timeOut1off;
						enterValue = *enterParam;
						enterValueMin = time_out_1_off_min;
						enterValueMax = time_out_1_off_max;
					}
					break;
				case LVL_TIME_2_ON:
					setPosLcd(1, 1);
					printf("TAIMER N2");
					setPosLcd(2, LCD_NUM_COL - 10);
					printf("USTANOVKA >");

					if (key == KEY_MENU)
					{
						eLvlMenu = LVL_TIME_2_ON_EDIT;
						enterParam = &timeOut2on;
						enterValue = *enterParam;
						enterValueMin = time_out_2_on_min;
						enterValueMax = time_out_2_on_max;
					}
					break;
				case LVL_WORK:
					if (cntOut1on > 0)
					{
						setPosLcd(1, 7);
						tmp = timeOut1on - cntOut1on;
						printf("%02d,%d", (tmp/freq_cycle), (tmp%freq_cycle)*5);
						setPosLcd(2, 3);
						printf("RABOTA SCHET");
					}
					else
					{
						setPosLcd(1, 7);
						tmp = timeOut1off - cntOut1off;
						printf("%02d,%d", (tmp/freq_cycle), (tmp%freq_cycle)*5);
						setPosLcd(2, 3);
						printf("PAUZA SCHET");
					}

					if (key == KEY_INC)
					{
						if (sost == SOST_PUSK)
						{
							sost = SOST_STOP;
							ctrlNewCycle();
						}
						eLvlMenu = LVL_START;
					}
					else if (key == KEY_SAVE)
					{
						help = time_help;
					}
					break;
				}
			}
			saveParamToEeprom();
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
