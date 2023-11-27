#define F_CPU 16000000UL		// 16MHz의 오실레이터를 사용
#include <avr/io.h>				//입출력 포트 사용을 위한 헤더 include
#include <avr/delay.h>			//딜레이 사용을 위한 헤더 inlcude
#include <avr/interrupt.h>		//인터럽트 사용을 위한 헤더 include
#include "lcd.h"				//LCD 사용을 위한 헤더 include -> " "외부의 파일을 include

// setting ///////////////////////////////////////////////////////////////////////////////
unsigned int measurement_time = 15624 * ( 0.2 );				// 0 ~ 4 [s]
unsigned char time_average[( 29 )] = {0};
unsigned char average_count = ( 29 );							// 배렬 선언과 동일한 값
unsigned char threshold = ( 29 ) * ( 0.8 );						// 0 ~ 1 역치값


unsigned char count = 0;
unsigned char average[7] = {0};
ISR(TIMER1_COMPA_vect) {
	for (unsigned char av = 0; av < 8; av++) {
		if (0 != (time_average[count] & (0x01 << av))) {
			average[av] --;
		}
	}
	
	time_average[count] = PINB;
	for (unsigned char av = 0; av < 8; av++) {
		if (0 != (time_average[count] & (0x01 << av))) {
			average[av] ++;
		}
	}

	count ++;
	if (count == average_count + 1) {
	count = 0;
	}
}
void measurement_time_Init() {						// 초음파 계측 주기 : // 1초
	TCCR1A = (0<<WGM11) | (0<<WGM10);
	TCCR1B = (0<<WGM13) | (1<<WGM12) | (1<<CS12) | (0<<CS11) | (1<<CS10);
	TIMSK |= (1 << OCIE1A);
	OCR1A = measurement_time;
	sei();
}







int LCD_nNum(unsigned int NUM, unsigned int NUM2) {
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
int tNum(unsigned int NUM) {
	unsigned char Buff[6] = "0";
	Buff[0] = '0'+((NUM %100000)/10000);
	Buff[1] = '0'+((NUM %10000)/1000);
	Buff[2] = '0'+((NUM %1000)/100);
	Buff[3] = '0'+((NUM %100)/10);
	Buff[4] = '0'+(NUM %10);
	LCDPuts(Buff);
}


unsigned char pp = 0;
int s = 0;
int main(void){
	DDRB |= 0xf0;				//7~4 LED 3~0 스위치가 연결되어 있다. 상위 니블 7~4까지는 출력을로 설정 하위 니블 3~0 입력으로 설정
	DDRG |= (1<<PG3);			//PG3번에 buzzer 연결되어있음
	UDR0 |= 0x00;

	measurement_time_Init();
	MCU_Init();					//LCD 사용을 위한 MCU 설정
	LCDInit();					//LCD 사용을 위한 초기화 설정
		
	while(1){
			LCDMove(0,0);
			tNum(PINB);
			LCDMove(1,0);
			tNum(average[0]);
			LCDMove(1,7);
			tNum(average[7]);
	}
}




//unsigned char