

#define F_CPU 16000000UL		// 16MHz의 오실레이터를 사용
#include <avr/io.h>				//입출력 포트 사용을 위한 헤더 include
#include <avr/delay.h>			//딜레이 사용을 위한 헤더 inlcude
#include <avr/interrupt.h>		//인터럽트 사용을 위한 헤더 include
#include "lcd.h"				//LCD 사용을 위한 헤더 include -> " "외부의 파일을 include

/* 타이머 */
int Timer = 10000, Stopwatch = 0, timer = 0;
ISR(TIMER0_OVF_vect) {
	timer++;
	if (timer == 1000) {
		Timer --;
		Stopwatch ++;
		timer = 0;
	}
	if (Timer < 0) {Timer = 9999;}
	if (Stopwatch > 9999) {Stopwatch = 0;}
	TCNT0=131;
}
void TIMER0_Init() {
	TCCR0=5;
	TIMSK=1;
	sei();	//전역인터럽트 활성화
}

int h = 12, m = 30, s = 30;
ISR(TIMER1_COMPA_vect) {
	s++;
	if (s > 59) {m++, s = 0;}
	if (m > 59) {h++, m = 0;}
	if (h > 23) {h = 0;}
}
void TIMER1_Init()
{
	TCCR1A = (0<<WGM11) | (0<<WGM10);
	TCCR1B = (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
	TIMSK |= (1 << OCIE1A);
	OCR1A=15624;
	sei();								//전역인터럽트 활성화
}
/* 타이머 끝 */

/*AD 변환*/
int readAnalog(unsigned char ch, char x, char y)		// AD 채널 값을 받아서 AD를 값을 읽어오는 함수.
{
	unsigned char Buff[8] = " 0";
	
	ADCSRA = 1<<ADEN | 0b111<<ADPS0;	// 128 분주율로 AD를 사용 인에이블 설정
	ADMUX = 0b01<<REFS0 | ch<<MUX0;		// AREF를 기준전압으로 사용, AREF와 GND사이에 콘덴서 접속
	ADCSRA |= 1<<ADSC;					// A/D 변환 시작
	
	Buff[0] = '0'+((ADC %10000)/1000);
	Buff[1] = '0'+((ADC %1000)/100);
	Buff[2] = '0'+((ADC %100)/10);
	Buff[3] = '0'+(ADC %10);
	
	for (unsigned char i=0; i<=2; i++)
	{
		if (Buff[i] != '0')
		{
			break;
		}
		Buff[i]=' ';
	}
	Buff[4]= '\0';
	
	LCDMove(y, x);
	LCDPuts(Buff);
}

/*숫자 변환*/
int LCD_nNum(unsigned int NUM, unsigned int NUM2)
{
	unsigned char Buff[13] = "0";
	Buff[0] = '0'+((NUM %10000)/1000);
	Buff[1] = '0'+((NUM %1000)/100);
	Buff[2] = '0'+((NUM %100)/10);
	Buff[3] = '0'+(NUM %10);
	Buff[4] = 's';
	
	Buff[5] = ' ';
	Buff[6] = '0'+(((NUM2) %1000)/100);
	Buff[7] = '0'+(((NUM2) %100)/10);
	Buff[8] = '0'+((NUM2) %10);
	Buff[9] = 'm';
	Buff[10] = 's';
	
	LCDPuts(Buff);
}
int Watch(unsigned int y) {
	LCDMove(y,0);
	unsigned char wc[13] = "0";
	wc[0] = '0'+((h %100)/10);
	wc[1] = '0'+(h %10);
	wc[2] = 'h';
	wc[3] = ' ';
	
	wc[4] = '0'+((m %100)/10);
	wc[5] = '0'+(m %10);
	wc[6] = 'm';
	wc[7] = ' ';
	
	wc[8] = '0'+((s %100)/10);
	wc[9] = '0'+(s %10);
	wc[10] = 's';
	LCDPuts(wc);
}
/*숫자 변환 끝*/


char display = 16;
int main(void)
{
	DDRB |= 0xf0;				//7~4 LED 3~0 스위치가 연결되어 있다. 상위 니블 7~4까지는 출력을로 설정 하위 니블 3~0 입력으로 설정
	DDRG |= (1<<PG3);			//PG3번에 buzzer 연결되어있음
	UDR0 |= 0x00;

	MCU_Init();					//LCD 사용을 위한 MCU 설정
	LCDInit();					//LCD 사용을 위한 초기화 설정
	
	static char string1[]="- 2 team : DEA -";
	static char string2[]="Start with 1 - 4";
	static char string3[]="Resistance";
	static char string4[]="Watch";
	static char string5[]="Stopwatch";
	static char string6[]="Timer";
	static char string7[]="reselection";
	static char string8[]="set Watch       ";

	//초기 화면
	while (PINB == 15) {
		LCDMove(0,0);			//표시할 문자의 위치를 선택->HOME 위치
		LCDPuts(string1);
		LCDMove(1,0);			//LCD의 두번쨰 줄
		LCDPuts(string2);
	}
	_delay_ms(200);
	LCDInit();
	
	// 시간 설정
	unsigned char next = 4;
	while (next > 0) {
		LCDMove(0,0);
		string8[15] = '0' + next -1;
		LCDPuts(string8);
		Watch(1);
		
		if (PINB == 14) {
			if (next == 4 && h != 0) {h--;}
			if (next == 3 && m != 0) {m--;}
			if (next == 2 && s != 0) {s--;}
			_delay_ms(200);
		}
		if (PINB == 13) {
			if (next == 4 && h != 23) {h++;}
			if (next == 3 && m != 59) {m++;}
			if (next == 2 && s != 59) {s++;}
			_delay_ms(200);
		}
		
		if (PINB == 11 && next != 4) {
			next ++;
			_delay_ms(200);
		}
		if (PINB == 7) {
			next --;
			_delay_ms(200);
		}
	}
	LCDInit();
	display = 14;
	PORTB = ~(display << 4);
	
	TIMER0_Init();
	TIMER1_Init();
	
	//main
	int resS = 0;
	int resT = 0;
	while (1) {
		
		// 화면 선택
		if ((PINB & 15) != 15) {
			display = (PINB & 15);
			PORTB = ~(display << 4);
			LCDInit();
		}
		LCDMove(0,0);
		
		// 1
		if(display == 14) {
			LCDPuts(string3);
			readAnalog(6, 0, 1);
		}
		
		// 2
		else if(display == 13) {
			LCDPuts(string4);
			Watch(1);
		}
		
		// 3
		else if(display == 11) {
			LCDPuts(string5);
			LCDMove(1,0);
			LCD_nNum(Stopwatch, timer);
			
			resS = Stopwatch;
			while ((PINB & 15) == 11) {
				if (((Stopwatch - resS) > 2) && resS != 0) {
					Stopwatch = 0, timer = 0;
					break;
				}
				LCDMove(1,0);
				LCD_nNum(Stopwatch, timer);
			}
		}
		
		// 4
		else if(display == 7) {
			LCDPuts(string6);
			LCDMove(1,0);
			LCD_nNum(Timer, 999 - timer);
			
			resT = Timer;
			while ((PINB & 15) == 7) {
				if (((resT - Timer) > 2) && resT != 9999) {
					Timer = 9999, timer = 0;
					break;
				}
				LCDMove(1,0);
				LCD_nNum(Timer, 999 - timer);
			}
		}
		
		// 해당 없음
		else {
			LCDMove(0,0);
			LCDPuts(string7);
		}
	}
}
