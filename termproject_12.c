// =====================================
// 사용 CPU : ATmega128-32M
// -------------------------------------
#define __STDIO_FDEVOPEN_COMPAT_12

// 기본 헤더
// === AVR includes ===
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "avr/io.h"
#include "avr/interrupt.h"
#include "avr/delay.h"

// =====================================
// USART 통신 관련 선언 
#define RXB8 1
#define TXB8 0
#define UPE 2
#define OVR 3
#define FE 4
#define UDRE 5
#define RXC 7

#define FRAMING_ERROR (1<<FE)
#define PARITY_ERROR (1<<UPE)
#define DATA_OVERRUN (1<<OVR)
#define DATA_REGISTER_EMPTY (1<<UDRE)
#define RX_COMPLETE (1<<RXC)

// ======================================
// MODE 관련 선언
#define MENU_MODE 0
#define KEEP_MODE 1
#define FIND_MODE 2
#define MANAGER_MODE 3
#define ENTERPASS_MODE 4
// 요금이 부과될 시간 간격 (현재 60초 당 1달러) 
#define SECONDS_PER_DOLLAR 60
// 변수 선언 
const char manager_password[6] = { 1, 2, 3, 4, 5, 6 }; // 관리자 비밀번호 123456
const char menu[] = "\r\n서울과학기술대학교 물품보관함입니다. 옵션을 선택하세요\r\n1. 물품보관 2. 물품찾기 3. 관리자모드\r\n"; 
const char error[] = "\r\n입력이 잘못되었습니다. 올바른 수를 입력해주세요\r\n"; 
const char keep[] = "\r\n물품을 보관할 사물함 번호를 입력해주세요.(X:사용중 O:비어있음 SPACE:뒤로가기)\r\n123456789\r\n";
const char find[] = "\r\n물품을 찾을 사물함 번호를 입력해주세요.(X:사용중 O:비어있음 SPACE:뒤로가기)\r\n123456789\r\n";
const char manager[] = "\r\n관리자 비밀번호를 입력해주세요.(BACKSPACE:다시입력 SPACE: 뒤로가기)\r\n";
const char in_manager[] = "\r\n※관리자모드(SPACE: 뒤로가기)※\r\n(LOCK된 사물함 입력시 해제 가능, 사용중인 사물함 비밀번호 목록)\r\n123456789\r\n"; 
const char unlock[] = "X번 사물함 LOCK 해제 완료. 사용 시간, 사용 금액:\r\n";
const char enterpass[] = "X번 사물함의 비밀번호를 입력해주세요. (BACKSPACE:다시입력 SPACE: 뒤로가기)\r\n";
const char password_keep[] = "\r\n물품을 보관하였습니다. \r\n";
const char password_wrong[] = "\r\n비밀번호가 틀렸습니다. 틀린 횟수:";
const char password_same[] = "\r\n물품이 회수되었습니다. 사용 시간, 사용 금액:\r\n";
const char manager_password_wrong[] = "\r\n관리자 비밀번호가 틀렸습니다.\r\n";
char usart0_rx_data;
char usart0_tx_menu = 0, usart0_rx_eflg = 0, usart0_tx_keep = 0, usart0_tx_find = 0, usart0_tx_manager = 0, usart0_tx_error = 0, usart0_tx_enterpass = 0; // usart를 통한 입,출력 flag
char menu_mode = 0, error_mode = 0, keep_mode = 0, find_mode = 0, manager_mode = 0, enterpass_mode = 0; // 모드 flag (현재 어떤 모드에 위치하는 지) 
char selected_locker; // 현재 선택된 사물함 번호
char locker_password[10][6]; // 1번부터 9번까지의 사물함 비밀번호 ( 사물함 번호 인덱스, 비밀번호를 저장할 인덱스 )
char find_locker_password[10][6]; // 사물함을 찾을 때 입력받을 비밀번호
char find_manager_password[6]; // 입력받을 관리자 비밀번호
char locker_usage[10] = { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 1번부터 9번까지의 사물함 사용여부 (  0 : 비어있음 . 1 : 사용중 , 2: 잠김 )
char password_error_cnt[10]; // 1번부터 9번까지의 사물함 비밀번호 틀린 횟수
char locker_idx = 1; // 사물함에 대한 인덱스
char password_idx = 0; // 비밀번호에 대한 인덱스
char manager_password_idx = 0; // 관리자 비밀번호에 대한 인덱스
int keeping_seconds[10]; // 1번부터 9번까지 보관시간
int current_seconds = 0 , using_seconds = 0; // 현재까지 지난 시간과 사용시간
int tens_minutes = 0 , ones_minutes = 0 ,tens_seconds = 0, ones_seconds = 0; // 사용시간의 분,초 단위 자릿수
int t = 0; // timer 구현을 위한 변수
int i = 0, j = 0, k = 0; // 원할한 문자 출력을 위한 인덱스

// 함수 선언
void set_mode(char mode_name);
char password_is_same();
 
//=============================================
//MCU 초기화 function
void cpu_init(void);
//---------------------------------------------

//=============================================
//USART setup function
void usart01_setup(void);
//---------------------------------------------

//=============================================
//external memory setup function
void Ext_memory_setup(void);
//---------------------------------------------
//Timer setup function
void timer_counter2_setup(void);


// =========== 메인 프로그램 ===========
// [ 인수 ] : void
//  현재 어떤 mode인지에 따라 출력/수신의 경우를 나누었다.
//  비밀번호 찾기, 보관 , 관리자모드 모두 비밀번호 입력 행위를 하므로
//  비밀번호 입력 모드로 비밀번호 입력 관련 행위를 하고 있는지 구분하였다.
// -------------------------------------
int main(void)
{
	cpu_init();
	usart01_setup(); // UCSR0B = 0xF8
	timer_counter2_setup();
	usart0_tx_menu = 1;
	int locker_num, one_digit;
	
	while(1)
	{ 
		if(usart0_tx_menu) // 메뉴 문자열 출력
		{
			set_mode(MENU_MODE);
			UCSR0B = 0xF8; // UDRE interrupt enable ( 연속 송신 O ) 문자열 끝에 도달했을 때 disable ( 연속 송신 X )  
			usart0_tx_menu = 0;
		}
		if(usart0_tx_keep) // 보관 문자열 출력
		{
			set_mode(KEEP_MODE);
			UCSR0B = 0xF8;
			usart0_tx_keep = 0;
		}
		if(usart0_tx_find) // 찾기 문자열 출력
		{
			set_mode(FIND_MODE);
			UCSR0B = 0xF8;
			usart0_tx_find = 0;
		}
		if(usart0_tx_manager) // 관리자 모드 문자열 출력
		{
			set_mode(MANAGER_MODE);
			UCSR0B = 0xF8;
			usart0_tx_manager = 0;
		}
		if(usart0_tx_enterpass) // 비밀번호 입력 관련 문자열 출력
		{
			enterpass_mode = 1;
			UCSR0B = 0xF8;
			usart0_tx_enterpass = 0;
		}
		if(usart0_rx_eflg) 
		{
			if(menu_mode) // 메뉴모드 관련 수신
			{
				if(usart0_rx_data == '1')	usart0_tx_keep = 1;
				else if(usart0_rx_data == '2')	usart0_tx_find = 1;
				else if(usart0_rx_data == '3')	usart0_tx_manager = 1;	 
			}
			else if(!enterpass_mode) // 보관,찾기, 관리자모드 사물함 선택 수신
			{
				locker_num = usart0_rx_data - 48; // 입력받은 문자로서의 숫자를 정수로서의 숫자로 판별 ( 0(48) ~ 9(57) )
				if(locker_num >= 1 && locker_num <= 9 )
				{
					if((locker_usage[locker_num] == 0 && keep_mode) || (locker_usage[locker_num] == 1 && find_mode) || (locker_usage[locker_num] == 2 && manager_mode))
					{
						UDR0 = '\n'; 
						selected_locker = locker_num; // 선택한 사물함 번호 저장
						if(keep_mode || find_mode)	usart0_tx_enterpass = 1; // 비밀번호 입력 관련 문자열 출력
						else	usart0_tx_manager = 1;
					}
				}
				else if(usart0_rx_data == ' ') // SPACE 입력
				{
					usart0_tx_menu = 1;
					if(manager_mode) manager_password_idx = 0;
				}
			}
			else if(enterpass_mode) // 보관, 찾기 , 관리자 모드 비밀번호 수신
			{
				one_digit = usart0_rx_data - 48;
				if(one_digit >= 0 && one_digit <= 9 )
				{
					if(keep_mode) // 보관모드
					{
						locker_password[selected_locker][password_idx++] = one_digit; // 선택한 사물함의 비밀번호 저장
						UDR0 = usart0_rx_data;
						if(password_idx >= 6)
							usart0_tx_enterpass = 1;
					}
					else if(find_mode) // 찾기모드
					{	
						find_locker_password[selected_locker][password_idx++] = one_digit;
						if(password_idx <= 3 )	UDR0 = usart0_rx_data;
						else
						{
							UDR0 = '*';
							if(password_idx >= 6)
								usart0_tx_enterpass = 1;		
						}
					}
					else if(manager_mode) // 관리자 모드
					{
						find_manager_password[manager_password_idx++] = one_digit;
						UDR0 = '*';
						if(manager_password_idx >= 6)
							usart0_tx_enterpass = 1;
					}
				}
				else if(usart0_rx_data == '\b') // BACK SPACE 입력 
				{
					UDR0 = '\r';
					if(keep_mode || find_mode) password_idx = 0;
					else manager_password_idx = 0;
				}
				else if(usart0_rx_data == ' ') // SPACE 입력 
				{
					if(password_idx) UDR0 = '\n'; // 입력한 패스워드가 있을 때

					if(keep_mode || find_mode) password_idx = 0;
					else manager_password_idx = 0;

					if(keep_mode) usart0_tx_keep = 1;
					else if(find_mode) usart0_tx_find = 1;
					else if(manager_mode) usart0_tx_menu = 1;
				}
			}
			usart0_rx_eflg = 0;
		}
		if(usart0_tx_error) // 송신 이상 발생 시 menu로 이동
		{
			usart0_tx_menu = 1;
			usart0_tx_error = 0;
		}
	}
	return 0;
}

// =========== CPU 초기화 =============
void cpu_init(void)
{
/*
	모든 interrupt 신호는 발생하지 않도록 조치한다.
	이유 : 원하지 않는 interrupt에 대한 처리 routine이
	없을 경우 MCU가 이상 동작을 할 수 있다. 
	(아래 코드는 그대로 사용하도록 함 ) 
*/

//	External (외부) interrut 0~7 발생하지 않도록 (disable) -> datasheet p. 90 ~ 92
	EICRA = 0x00;
	EICRB = 0x00;
	EIMSK = 0x00; 

// EEPROM의 interrupy 발생하지 않도록 (disable) -> datasheet p. 22
	EECR = 0x00;

// Timer(s) / Counter(s) Interrupt (s) 발생하지 않도록 (disable)
	TIMSK = 0x00;
	ETIMSK = 0x00;

// USART 0,1의 interrupt 발생하지 않도록 (disable) -> datasheet p. 190
	UCSR0B = 0x00;
	UCSR1B = 0x00;

// SPI interrupt 발생하지 않도록 (disable) -> datasheet p. 167
	SPCR = 0x00;

// TWI의 interrupt 발생하지 않도록 (disable) -> datasheet p. 206
	TWCR = 0x00;

// Analog-to-Digital converter의 interrupt 발생하지 않도록 (disable) -> datasheet p. 224
	ACSR = 0x00;

/* 
	Global interrupts enable : sei 함수가 실행이 되어야 
	각 block의 interrupt를 enable/disable할 수 있음 --> 무조건 사용함 
	sei 함수는 interrupt.h에 선언되어 있음
*/
	sei();

/*
	범용 digital I/O pin 기본 설정 : 기본적으로
	입력으로 설정하고, pull-up 저항을 사용하지 않도록 한다.
*/

// PortA 설정 
	PORTA = 0x00;	// 포트 출력 reg  
	DDRA = 0x00;	// 포트 방향 reg(0입력, 1출력)

// PortB 설정 
	PORTB = 0x00;	// 포트 출력 reg
	DDRB = 0x00;	// 포트 방향 reg(0입력, 1출력)

// PortC 설정 
	PORTC = 0x00;	// 포트 출력 reg
	DDRC = 0x00;	// 포트 방향 reg(0입력, 1출력)

// PortD 설정 
	PORTD = 0x00;	// 포트 출력 reg
	DDRD = 0x00;	// 포트 방향 reg(0입력, 1출력)

// PortE 설정 
	PORTE = 0x00;	// 포트 출력 reg
	DDRE = 0x00;	// 포트 방향 reg(0입력, 1출력)

// PortF 설정 
	PORTF = 0x00;	// 포트 출력 reg
	DDRF = 0x00;	// 포트 방향 reg(0입력, 1출력)
// PortF 설정 
	//PORTG = 0x00;	// 포트 출력 reg
	//DDRG = 0x00;	// 포트 방향 reg(0입력, 1출력)

	PORTG = 0xFF;	// 포트 출력 reg
	DDRG = 0XFF; 	// 포트 방향 reg(0입력, 1출력) 

}

void usart01_setup(void)
{

//	USART port 통신 초기화
//	USART0 : 19200Bps, 8Data, 1 Stop, No Parity

	UCSR0A = 0x00;
	UCSR0C = 0x06;
	UBRR0H = 0x00;
	UBRR0L = 0x33;

	UCSR1A = 0x00;
	UCSR1B = 0x18;
	UCSR1C = 0x06;
	UBRR1H = 0x00;
	UBRR1L = 0x33; 	

	UCSR0B = 0xF8; 
}


SIGNAL(SIG_UART0_DATA) // UDR Empty Interrupt : 연속된 송신시 사용 
{
	if(menu_mode) 
	{ // 메뉴 문자열 출력
		if(menu[i] == '\0') // 메뉴 문자열 끝에 도달
		{
			UCSR0B = 0xD8; // UDRE interrupt disable
			i = 0; 
		}
		else
			UDR0 = menu[i++];
	}
	else if(!enterpass_mode)  // 비밀번호 입력과 관련 없음
	{ 
	    if(keep_mode) 
	    {// 보관 문자열 출력 
	        if(keep[i] == '\0') // 보관 문자열 끝에 도달하면 사물함 사용현황 출력
	        {
	            if(locker_idx >= 1 && locker_idx <= 9)
	            {
		           	if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // 사물함이 사용중이면  
		         	else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// 사물함이 비어 있으면
					else  UDR0 = 'L';// 사물함이 잠겨 있으면
	            }
	            else if(locker_idx == 10)
	                UDR0 = '\r';
	            else if(locker_idx == 11)
	            {

	                UDR0 = '\n';
					UCSR0B = 0xD8;
	                i = 0; locker_idx = 0;
	            }
	            locker_idx++;
	        }
	        else
	            UDR0 = keep[i++];
	    }
	    else if(find_mode) 
	    {//  찾기 문자열 출력
	        if(find[i] == '\0') // 찾기 문자열 끝에 도달하면 사물함 사용현황 출력
	        {
	            if(locker_idx >= 1 && locker_idx <= 9)
	            {
		           	if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // 사물함이 사용중이면  
		         	else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// 사물함이 비어 있으면
					else  UDR0 = 'L';// 사물함이 잠겨 있으면
	            }
	            else if(locker_idx == 10)
	                UDR0 = '\r';
	            else if(locker_idx == 11)
	            {
	                UDR0 = '\n';
	                UCSR0B = 0xD8;
	                i = 0; locker_idx = 0;
	            }
	            locker_idx++;
	        }
	        else
	            UDR0 = find[i++];
	    }
	    else if(manager_mode) 
	    {// 관리자모드 관련 문자열 출력
			if(manager_password_idx >= 6 && locker_usage[selected_locker] == 2) // 잠긴 자물함을 해제 할 때 문자열 출력
			{
				if(unlock[i] == '\0')
			    {
					if(j == 0)
					{
						using_seconds = current_seconds - keeping_seconds[selected_locker];
						divide_seconds(using_seconds);
						UDR0 = tens_minutes + 48;
					}
					else if(j == 1)
						UDR0 = ones_minutes + 48;
					else if(j == 2)
						UDR0 = ':';
					else if(j == 3)
						UDR0 = tens_seconds + 48;
					else if(j == 4)
						UDR0 = ones_seconds + 48;
					else if(j == 5)	
						UDR0 = ' ';
					else if(j == 6)
						UDR0 = (using_seconds / SECONDS_PER_DOLLAR+ 1) + 48; // 30초당 1달러
					else if(j == 7)
						UDR0 = '$';
					else if(j == 8)
						UDR0 = '\r';
					else
					{	
					 	UDR0 = '\n';
						UCSR0B = 0xD8;
			       	 	locker_usage[selected_locker] = 0; password_error_cnt[selected_locker] = 0;
						usart0_tx_menu = 1;
						manager_password_idx = 0;
						i = 0; j = -1;
					}
					j++;
			    }
			    else
				{	
					if( i == 0 ) UDR0 = selected_locker + 48;
					else UDR0 = unlock[i];
					i++;
				}
			}
			else // 관리자모드 보안 문자열 출력
			{
		        if(manager[i] == '\0')
		        {
		            UCSR0B = 0xD8;
		            enterpass_mode = 1; // 바로 비밀번호 입력을 받을 수 있게 끔
					manager_password_idx = 0;
		            i = 0; 
		        }
		        else
		         	UDR0 = manager[i++];
			}
	    }
	}
	else if(enterpass_mode) // 비밀번호 입력과 관련있음
	{
		if(keep_mode || find_mode)
		{
			if(password_idx >= 6) // 비밀번호를 전부 입력했을 때의 문자열 출력
			{
    			if(keep_mode) // 물품 보관 성공
    			{
        			if(password_keep[i] == '\0')
        			{
						UCSR0B = 0xD8;
            			locker_usage[selected_locker] = 1;
						keeping_seconds[selected_locker] = current_seconds;
            			usart0_tx_menu = 1;
            			i = 0; password_idx = 0;
        			}
        			else
            			UDR0 = password_keep[i++];
    			}
	    		if(find_mode) 
				{
				    if(password_is_same()) // 물품 찾기 성공 (비밀번호 같음)
				    {
				        if(password_same[i] == '\0') 
				        {
							if(j == 0)
							{
								using_seconds = current_seconds - keeping_seconds[selected_locker];
								divide_seconds(using_seconds);
								UDR0 = tens_minutes + 48;
							}
							else if(j == 1)
								UDR0 = ones_minutes + 48;
							else if(j == 2)
								UDR0 = ':';
							else if(j == 3)
								UDR0 = tens_seconds + 48;
							else if(j == 4)
								UDR0 = ones_seconds + 48;
							else if(j == 5)
								UDR0 = ' ';
							else if(j == 6)
								UDR0 = (using_seconds / SECONDS_PER_DOLLAR + 1) + 48; // 30초당 1달러
							else if(j == 7)
								UDR0 = '$';
							else if(j == 8)
								UDR0 = '\r';

							else 
							{
								UDR0 = '\n';
								UCSR0B = 0xD8;
				          	  	locker_usage[selected_locker] = 0;
								password_error_cnt[selected_locker] = 0; // 틀린 횟수 초기화
				         	   	usart0_tx_menu = 1;
				           		i = 0; j = -1; password_idx = 0;
							}			
							j++;
				        } 	
				        else 
				            UDR0 = password_same[i++];
				    } 
				    else 
				    { // 물품 찾기 실패 (비밀번호 다름) 
				        if(password_wrong[i] == '\0') 
				        {
							if(j == 0 )	UDR0 = ++password_error_cnt[selected_locker] + 48;
							else if(j == 1)	UDR0 = '\r';
							else if(j == 2) UDR0 = '\n';
							else
							{
								UDR0 = '\a';
								UCSR0B = 0xD8;
								if(password_error_cnt[selected_locker] >= 3) 
								{
									locker_usage[selected_locker] = 2;
									usart0_tx_menu = 1;
								}
				            	i = 0; j = -1; password_idx = 0;
							}
							j++;
				        }
				        else 
				        {
				            UDR0 = password_wrong[i++];
				        }
				    }
				}
			}
			else // 비밀번호 입력 받기 전 문자열 출력
			{
				if(enterpass[i] == '\0')
				{
					UCSR0B = 0xD8;
					i = 0; 
				}
				else
				{	
					if( i == 0 ) UDR0 = selected_locker + 48;
					else UDR0 = enterpass[i];
					i++;
				}
			}
		}
		else // 관리자 모드
		{	
			if(manager_password_idx >= 6) // 비밀번호를 전부 입력했을 때의 문자열 출력
			{
				if(password_is_same())  // 관리자 비밀번호 같음
				{	
					if(in_manager[i] == '\0') // 관리자모드 관련문자열이 끝나면
					{
						if(j == 0) // 사물함 현황 출력
						{
							if(locker_idx >= 1 && locker_idx <= 9)
		            		{
		                		if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // 사물함이 사용중이면  
		                		else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// 사물함이 비어 있으면
								else  UDR0 = 'L';// 사물함이 잠겨 있으면
		            		}
		            		else if(locker_idx == 10)
		                		UDR0 = '\r';
			            	else if(locker_idx == 11)
			            	{
			                	UDR0 = '\n';
								locker_idx = 0; j++;
			            	}
							locker_idx++;
						}
						else // j가 0에서 1이 되면서 잠겨있는 사물함 비밀번호 출력 
						{
							if(locker_idx >= 10) // 사물함 비밀번호를 모두 출력한 상태면
							{
								if(locker_idx == 10 )UDR0 = '\r';
								else
								{
									UCSR0B = 0xD8;
									UDR0 = '\n';
									enterpass_mode = 0; // 잠긴 사물함 선택을 할 수 있도록
									i = 0; j = 0; password_idx = 0; locker_idx = 0;
								}
								locker_idx++;
							}	
							else if(locker_usage[locker_idx] == 1) // 보관된 사물함 비밀번호 출력
							{
								if(password_idx == 6) 
								{
									UDR0 = ' ';
									locker_idx++;
									password_idx = 0; j = 1;
								}
								else
								{
									if(j == 1) UDR0 = '#';
									else if(j == 2) UDR0 = locker_idx + 48;
									else if(j == 3) UDR0 = ':';
									else	
									{
										UDR0 = locker_password[locker_idx][password_idx] + 48;
										password_idx++;
									}
									j++;
								}
							}
							else // 사물함이 비어있거나 잠겨 있으면 출력 X
								locker_idx++;
						}
					}
					else // 관리자모드 내부 문자열 출력
						UDR0 = in_manager[i++];
				}
				else // 관리자 비밀번호 다름
				{
					if(manager_password_wrong[i] == '\0')
				    {
			            UDR0 = '\a';
			            UCSR0B = 0xD8;
					    i = 0; manager_password_idx = 0;
					}
					else
					   	UDR0 = manager_password_wrong[i++];
				}
			}
		}
	}
}
SIGNAL(SIG_UART0_TRANS) // UDR TxC Interrupt : 송신 완료 시 사용
{ // TxC Flag가 자동적으로 1에서 0이됨 (TxC 0, UDRE 1)
	char status = UCSR0A;
	if((status & 0x60) != 0x20) // TxC 0 , UDRE 1이 아니면
		usart0_tx_error = 1;
}

SIGNAL(SIG_UART0_RECV) // UDR RxC Interrupt : 수신 완료 시 사용
{
    char status, data;
    status = UCSR0A;
    data = UDR0;
    
    if((status & (FRAMING_ERROR | PARITY_ERROR | DATA_OVERRUN)) == 0) 
	{
        usart0_rx_data = data;
        usart0_rx_eflg = 1;
    }
}

SIGNAL(SIG_OUTPUT_COMPARE2)
{

	if( t < 2004 ) t++; // 1주기(1msec)에 2씩증가 t가 2000에 가까울 때 1초
	else 
	{
	 	t = 0;
		current_seconds++;
	}
		
}

SIGNAL(SIG_OVERFLOW2)
{

	if( t < 2004 ) t++; 
	else 
	{
	 	t = 0;
		current_seconds++;
	}
		
}


void set_mode(char mode_name)
{//mode_name외 나머지 mode flag 0 
  menu_mode = keep_mode = find_mode = manager_mode = enterpass_mode = 0;

	if(mode_name == MENU_MODE) 
    	menu_mode = 1;
  	else if(mode_name == KEEP_MODE)
    	keep_mode = 1;
	else if(mode_name == FIND_MODE)
   		find_mode = 1;
	else if(mode_name == MANAGER_MODE)
   		manager_mode = 1;
}

char password_is_same()
{// 선택한 사물함에 대한 비밀번호가 입력한 비밀번호와 같은지 확인
	if(manager_mode)
	{// 관리자 비밀번호
		for(int i = 0 ; i < 6; i++)
		{
			if(manager_password[i] != find_manager_password[i]) 
				return 0; // 비밀번호가 다름
		}
	}
	else
	{// 일반 비밀번호
		for(int i = 0 ; i < 6; i++)
		{
			if(locker_password[selected_locker][i] != find_locker_password[selected_locker][i]) 
				return 0; // 비밀번호가 다름
		}
	}
	return 1; // 비밀번호가 같음 
}

void divide_seconds(int seconds) 
{//초를 분과 초로 나누고 분과 초를 자릿수 별로 나눔
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
	
	tens_minutes = minutes / 10;
	ones_minutes = minutes % 10;
    tens_seconds = remaining_seconds / 10;
    ones_seconds = remaining_seconds % 10;
}


void timer_counter2_setup(void)
{
	TCCR2 = 0x6B; // Fast PWM mode, Clear on match , 256kHz -> 3.906nsec * 256 -> 1주기 1msec
	OCR2 = 0x80; // 128(10) 
	DDRB = 0x80;

	TIMSK = 0xC0; // OCF, TOV interrupt enable
	TCNT2 = 0;
}

void Ext_memory_setup(void)
{
	MCUCR = 0x80;
	XMCRA = 0x00;
	XMCRB = 0x00;
}
