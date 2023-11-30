#define F_CPU 16000000UL		// 16MHz의 오실레이터를 사용
#include <avr/io.h>				//입출력 포트 사용을 위한 헤더 include
#include <avr/delay.h>			//딜레이 사용을 위한 헤더 inlcude
#include <avr/interrupt.h>		//인터럽트 사용을 위한 헤더 include
#include "lcd.h"				//LCD 사용을 위한 헤더 include -> " "외부의 파일을 include

// setting ///////////////////////////////////////////////////////////////////////////////
// time average
#define measurement_time 0.2								// 0.1 ~ 4 [s]
unsigned char time_average[10];
#define average_count 10									// 배렬 선언과 동일한 값

// sensor
#define TRIG 7												// PE7 (변경 불가)
#define TRIG_delay 80										// 기본 80 [ms], 연산 속도에 맞춰 변경 가능, 너무 줄일 시 측정 오류
#define interval 700										// 측정 간격 5 ~ 2700 [mm]

//measurement
#define standard 50											// %
#define night_period
#define daytime_period

///////////////////////////////////////////////////////////////////////////////

// time average
unsigned char count = 0;
unsigned char average[7] = {0};
unsigned char av = 0;
unsigned char measurement;
// sensor
unsigned char check = 0;

ISR(TIMER1_COMPA_vect) {
	unsigned char temp = time_average[count];
	time_average[count] = check;
	for (av = 0; av < 8; av++) {
		if (((temp ^ time_average[count]) >> av) & 0x01) {
			if ((temp >> av) & 0x01) {
				average[av] --;
			}
			else {
				average[av] ++;
			}
		}
	}
	
	count ++;
	if (count == average_count) {
		count = 0;
	}
	measurement = 1;
}
void measurement_time_Init() {						// 초음파 계측 주기 타이머
	TCCR1A = (0<<WGM11) | (0<<WGM10);
	TCCR1B = (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
	TIMSK |= (1 << OCIE1A);
	OCR1A = 15624 * measurement_time;
	sei();
}

int h = 12, m = 30, s = 30;
unsigned int front[7] = {0};
ISR(TIMER3_COMPA_vect) {
	s++;
	if (s > 59) {m++, s = 0;}
	if (m > 59) {h++, m = 0;}
	if (h > 23) {h = 0;}

	for (char a = 0 ; a < 7 ; a++) {
		if (average[a] <= (average_count * standard / 100)) {
			front[a]++;
		}
		else {
			front[a] = 0;
		}
	}
}
void time_Init() {									// 시간 타이머
	TCCR3A = (0<<WGM11) | (0<<WGM10);
	TCCR3B = (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
	ETIMSK |= (1 << OCIE3A);
	OCR3A = 15624;
	sei();
}

int tNum(unsigned int NUM) {
	unsigned char Buff[4] = "0";
	Buff[0] = '0'+((NUM %1000)/100);
	Buff[1] = '0'+((NUM %100)/10);
	Buff[2] = '0'+(NUM %10);
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


int main(void){
	DDRB=0xf0;
	DDRE=0x80;
	PORTE = 0x7f;
	UDR0 |= 0x00;
	
	MCU_Init();					//LCD 사용을 위한 MCU 설정
	LCDInit();					//LCD 사용을 위한 초기화 설정
	
	// 시간 설정
	static char string8[]="set Watch       ";
	unsigned char next = 4;
	while (next > 0) {
		// 화면
		LCDMove(0,0);
		if (next > 1) {
			for (char a = 12 ; a < 15 ; a++) {string8[a] = ' ';}
			if (next == 4) {string8[15] = 'h';}
			else if (next == 3) {string8[15] = 'm';}
			else {string8[15] = 's';}
		}
		else {
			string8[12] = 'n';
			string8[13] = 'e';
			string8[14] = 'x';
			string8[15] = 't';
		}
		LCDPuts(string8);
		Watch(1);
		
		// 버튼
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
	time_Init();
	measurement_time_Init();
	LCDInit();
	
	while(1){
		if (measurement == 1) {
			check = 0;
			TCCR0 = 7;
			
			PORTE |= (1<<TRIG);     //Trig=HIGH -> 거리 측정 명령 시작
			_delay_us(10);			//10us동안 유지
			PORTE &= ~(1<<TRIG);
			
			while (!(PINE & (1<<0))) {}
			TCNT0 = 0;
			
			while (TCNT0 < ((interval + 50) / 10.88)){
				for (unsigned char a = 0 ; a < 7; a++) {
					if (PINE & (1<<a)) {
						if (TCNT0 > ((interval) / 10.88)) {
							check |= (1 << a);
						}
					}
				}
			}
			TCCR0 = 0;
			measurement = 0;
		}
		
		
		
		
		
		
		
		//Watch(1);
		
		LCDMove(0,0);
		tNum(front[0]);
		LCDMove(0,4);
		tNum(front[1]);
		LCDMove(0,8);
		tNum(front[2]);
		LCDMove(0,12);
		tNum(front[3]);
		
		
		LCDMove(1,0);
		tNum(front[4]);
		LCDMove(1,4);
		tNum(front[5]);
		LCDMove(1,8);
		tNum(front[6]);
		LCDMove(1,12);
		tNum(check);
		
	}
}