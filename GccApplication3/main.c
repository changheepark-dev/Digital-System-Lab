#define F_CPU 16000000UL		// 16MHz의 오실레이터를 사용
#include <avr/io.h>				//입출력 포트 사용을 위한 헤더 include
#include <avr/delay.h>			//딜레이 사용을 위한 헤더 inlcude
#include <avr/interrupt.h>		//인터럽트 사용을 위한 헤더 include
#include "lcd.h"				//LCD 사용을 위한 헤더 include -> " "외부의 파일을 include

// setting ///////////////////////////////////////////////////////////////////////////////
// time average
#define measurement_time ( 0.2 )							// 0.1 ~ 4 [s]
unsigned char time_average[( 10 )] = {0};
#define average_count ( 10 )								// 배렬 선언과 동일한 값

// sensor
#define TRIG 7												// PE7 (변경 불가)
#define TRIG_delay 80										// 기본 80, 연산 속도에 맞춰 변경 가능, 너무 줄일 시 측정 오류
#define interval 700										// 측정 간격 5 ~ 2700 [mm]


unsigned char count = 0;
unsigned char average[8] = {0};
unsigned char av = 0;
unsigned char measurement;

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

void measurement_time_Init() {						// 초음파 계측 주기 : // 1초
	TCCR1A = (0<<WGM11) | (0<<WGM10);
	TCCR1B = (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
	TIMSK |= (1 << OCIE1A);
	OCR1A = 15624 * measurement_time;
	sei();
}

int tNum(unsigned int NUM) {
	unsigned char Buff[4] = "0";
	Buff[0] = '0'+((NUM %1000)/100);
	Buff[1] = '0'+((NUM %100)/10);
	Buff[2] = '0'+(NUM %10);
	LCDPuts(Buff);
}



int main(void){
	DDRE=0x80;
	PORTE = 0x7f;

	measurement_time_Init();
	MCU_Init();					//LCD 사용을 위한 MCU 설정
	LCDInit();					//LCD 사용을 위한 초기화 설정
	
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
		
		LCDMove(0,0);
		tNum(average[0]);
		LCDMove(0,4);
		tNum(average[1]);
		LCDMove(0,8);
		tNum(average[2]);
		LCDMove(0,12);
		tNum(average[3]);
		
		LCDMove(1,0);
		tNum(average[4]);
		LCDMove(1,4);
		tNum(average[5]);
		LCDMove(1,8);
		tNum(average[6]);
		LCDMove(1,12);
		tNum(check);
	}
}