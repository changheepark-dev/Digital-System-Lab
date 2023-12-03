#define F_CPU 16000000UL		// 16MHz의 오실레이터를 사용
#include <avr/io.h>				//입출력 포트 사용을 위한 헤더 include
#define __DELAY_BACKWARD_COMPATIBLE__
#include <avr/delay.h>			//딜레이 사용을 위한 헤더 inlcude
#include <avr/interrupt.h>		//인터럽트 사용을 위한 헤더 include
#include "lcd.h"				//LCD 사용을 위한 헤더 include -> " "외부의 파일을 include

// setting ///////////////////////////////////////////////////////////////////////////////
// time average
#define measurement_time 0.2								// 0.1 ~ 4 [s]
unsigned char time_average[10];								// 시간 평균 갯수
#define average_count 10									// 배렬 선언과 동일한 값

// sensor
#define TRIG 3												// PE0 (변경 불가), echo(pe1 ~ 3, pd4 ~ 7)
#define TRIG_delay 80										// 기본 80 [ms], 연산 속도에 맞춰 변경 가능, 너무 줄일 시 측정 오류
#define interval 700										// 측정 간격 5 ~ 2700 [mm]

//measurement
#define standard 50											// 0 ~ 100[%], 측정 민감도 (낮을수록 늦게 상승, 빨리 감소)
#define night_period 20										// 0 ~ 65535 [s] 전면 센서 밤에 on 시간
#define daytime_period 30									// 0 ~ 65535 [s] 전면 센서 낮에 on 시간
#define person 10											// 0 ~ 65535 [s] 동행자 인지 판단 시간
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

unsigned char h = 12, m = 30, s = 30, l;
unsigned int front[7] = {0};
ISR(TIMER3_COMPA_vect) {
	// 시계
	s++;
	if (s > 59) {m++, s = 0;}
	if (m > 59) {h++, m = 0;}
	if (h > 23) {h = 0;}

	// 경과 시간 측정
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
	OCR3A = 5000;//15624;
	sei();
}


// 후면 감지
char ckd = 0;
ISR(INT0_vect) {
	if (ckd == 0) {ckd = 5;}
}
ISR(INT1_vect) {
	if (ckd == 0) {ckd = 6;}
}
ISR(INT2_vect) {
	if (ckd == 0) {ckd = 3;} // 사용 x
}
ISR(INT3_vect) {
	if (ckd == 0) {ckd = 7;}
}
ISR(INT4_vect) {
	if (ckd == 0) {ckd = 1;}
}
ISR(INT5_vect) {
	if (ckd == 0) {ckd = 2;}
}
ISR(INT6_vect) {
	if (ckd == 0) {ckd = 3;}
}
ISR(INT7_vect) {
	if (ckd == 0) {ckd = 4;}
}

void INTR_Init() {
	DDRD = 0x00;
	PORTD = 0xff;
	
	EICRA = 0xff;		//하강엣지 트리거
	EICRB = 0xff;
	EIMSK = 0xff;
	
	DDRE = 0x08;
	PORTE = 0xf7;
	sei();
}


int tNum2(unsigned int NUM) {
	unsigned char Buff[3] = "0";
	Buff[0] = '0'+((NUM %100)/10);
	Buff[1] = '0'+(NUM %10);
	LCDPuts(Buff);
}
int tNum3(unsigned int NUM) {
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

int bz(unsigned int a, unsigned int b) {
	PORTG |= (1<<PG3);
	_delay_ms(a);
	PORTG &= ~(1<<PG3);
	_delay_ms(b);
}

int main(void){
	DDRB = 0xf0;
	DDRG |= (1<<PG3);
	MCU_Init();
	LCDInit();
	
	// 시간 설정
	static char string1[]="set location";
	static char string8[]="set Watch       ";
	
	unsigned char next = 5;
	while (1) {
		// 화면
		if (next == 5) {
			LCDMove(0,0);
			LCDPuts(string1);
			LCDMove(1,0);
			tNum3(l);
		}
		else {
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
		}
		
		// 버튼
		if (PINB == 14) {
			if (next == 5 && l != 0) {l--;}
			if (next == 4 && h != 0) {h--;}
			if (next == 3 && m != 0) {m--;}
			if (next == 2 && s != 0) {s--;}
			_delay_ms(200);
		}
		if (PINB == 13) {
			if (next == 5 && l != 255) {l++;}
			if (next == 4 && h != 23) {h++;}
			if (next == 3 && m != 59) {m++;}
			if (next == 2 && s != 59) {s++;}
			_delay_ms(200);
		}
		if (PINB == 11 && next != 5) {
			LCDInit();
			next ++;
			_delay_ms(200);
		}
		if (PINB == 7) {
			next --;
			if(next == 0){
				break;
			}
			_delay_ms(200);
		}
	}
	time_Init();
	measurement_time_Init();
	INTR_Init();
	LCDInit();
	
	static char string2[]="location : 000  ";
	string2[11] = '0'+((l %1000)/100);
	string2[12] = '0'+((l %100)/10);
	string2[13] = '0'+(l %10);
	ckd = 0;
	
	// main
	unsigned char disp = 0;
	while(1){
		// 초음파 측정
		if (measurement == 1) {
			check = 0;
			TCCR0 = 7;
			
			PORTE |= (1<<TRIG);
			_delay_us(10);
			PORTE &= ~(1<<TRIG);
			
			while (!(PINE & (1<<0))) {}
			TCNT0 = 0;
			
			while (TCNT0 < ((interval + 50) / 10.88)){
				for (char a = 0 ; a < 3; a++) {
					if (PINE & (1<<a)) {
						if (TCNT0 > ((interval) / 10.88)) {
							check |= (1 << a);
						}
					}
				}
				for (char a = 4 ; a < 8; a++) {
					if (PIND & (1<<a)) {
						if (TCNT0 > ((interval) / 10.88)) {
							check |= (1 << a - 1);
						}
					}
				}
			}
			TCCR0 = 0;
			measurement = 0;
		}
		
		// 전면 센서 작동
		int largest = 1, secondLargest = 1;
		for (char a = 0; a < 7 ; a++) {
			if (front[a] >= largest) {
				secondLargest = largest;
				largest = front[a];
			}
			else if (front[a] > secondLargest && front[a] < largest) {
				secondLargest = front[a];
			}
		}
		if ((h >= 19) || (h < 7)) {
			if ((largest > night_period) && secondLargest < person) {
				PORTB = 0xf0;
			}
			else {PORTB = 0x00;}
		}
		else {
			if ((largest > daytime_period) && secondLargest < person) {
				PORTB = 0xf0;
			}
			else {PORTB = 0x00;}
		}
				
		// 후면 센서 작동
		while (ckd != 0 && disp == 14) {				
			string2[14] = '-';
			string2[15] = '0'+(ckd %10);
			LCDMove(0,0);
			LCDPuts(string2);
			
			for (char a = 0 ; a < 9 ; a++) {
				if ((PINB & 0x0f) != 15) {
					ckd = 0;
					string2[14] = ' ';
					string2[15] = ' ';
					LCDInit();
					break;
				}
				else if ((a > 5) || (a < 3)) {
					bz(50, 200);
				}
				else {
					bz(150, 200);
				}
			}
			_delay_ms(200);
		}
		
		
		
		// test display
		static char string3[]="sonic";
		static char string4[]="filter";
		static char string5[]="elapse";
		static char string6[]="motion";
		static char string7[]="largest : ";
		static char string9[]="second  : ";
		
		
		unsigned char x[] = {7, 10, 13, 0, 3, 6, 9};
		unsigned char y[] = {0, 0, 0, 1, 1, 1, 1};
		
		if ((PINB & 15) != 15) {
			disp = (PINB & 15);
			LCDInit();
		}
		LCDMove(0,0);
		
		// 초음파 테스트
		if(disp == 12) {
			LCDPuts(string3);
			for (char a = 0 ; a < 7 ; a++) {
				LCDMove(y[a], x[a]);
				tNum2(!((check >> a) & 0x01));
			}
			LCDMove(1,12);
			tNum3((~check) & 0x7f);
		}
		else if(disp == 10) {
			LCDPuts(string4);
			for (char a = 0 ; a < 7 ; a++) {
				LCDMove(y[a], x[a]);
				tNum2(average[a]);
			}
			LCDMove(1,12);
			tNum3((~check) & 0x7f);
		}
		else if(disp == 6) {
			LCDPuts(string5);
			for (char a = 0 ; a < 7 ; a++) {
				LCDMove(y[a], x[a]);
				tNum2(front[a]);
			}
			LCDMove(1,12);
			tNum3((~check) & 0x7f);
		}
		else if(disp == 5) {
			LCDMove(0,0);
			LCDPuts(string7);
			tNum3(largest);
			
			LCDMove(1,0);
			LCDPuts(string9);
			tNum3(secondLargest);
		}
		
		// 움직임 감지 테스트
		else if(disp == 9) {
			LCDPuts(string6);
			unsigned char aa = (PINE >> 4) | ((PIND & 0x0f) << 4);	
			for (char a = 0 ; a < 6 ; a++) {
				LCDMove(y[a], x[a]);
				tNum2(!((aa >> a) & 0x01));
			}
			LCDMove(1, 9);
			tNum2(!((aa >> 7) & 0x01));
			LCDMove(1,12);
			tNum3(aa);
		}
		
		else {
			disp = 14;
			LCDPuts(string2);
			Watch(1);
		}
	}
}
