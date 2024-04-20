// =====================================
// ��� CPU : ATmega128-32M
// -------------------------------------
#define __STDIO_FDEVOPEN_COMPAT_12

// �⺻ ���
// === AVR includes ===
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "avr/io.h"
#include "avr/interrupt.h"
#include "avr/delay.h"

// =====================================
// USART ��� ���� ���� 
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
// MODE ���� ����
#define MENU_MODE 0
#define KEEP_MODE 1
#define FIND_MODE 2
#define MANAGER_MODE 3
#define ENTERPASS_MODE 4
// ����� �ΰ��� �ð� ���� (���� 60�� �� 1�޷�) 
#define SECONDS_PER_DOLLAR 60
// ���� ���� 
const char manager_password[6] = { 1, 2, 3, 4, 5, 6 }; // ������ ��й�ȣ 123456
const char menu[] = "\r\n������б�����б� ��ǰ�������Դϴ�. �ɼ��� �����ϼ���\r\n1. ��ǰ���� 2. ��ǰã�� 3. �����ڸ��\r\n"; 
const char error[] = "\r\n�Է��� �߸��Ǿ����ϴ�. �ùٸ� ���� �Է����ּ���\r\n"; 
const char keep[] = "\r\n��ǰ�� ������ �繰�� ��ȣ�� �Է����ּ���.(X:����� O:������� SPACE:�ڷΰ���)\r\n123456789\r\n";
const char find[] = "\r\n��ǰ�� ã�� �繰�� ��ȣ�� �Է����ּ���.(X:����� O:������� SPACE:�ڷΰ���)\r\n123456789\r\n";
const char manager[] = "\r\n������ ��й�ȣ�� �Է����ּ���.(BACKSPACE:�ٽ��Է� SPACE: �ڷΰ���)\r\n";
const char in_manager[] = "\r\n�ذ����ڸ��(SPACE: �ڷΰ���)��\r\n(LOCK�� �繰�� �Է½� ���� ����, ������� �繰�� ��й�ȣ ���)\r\n123456789\r\n"; 
const char unlock[] = "X�� �繰�� LOCK ���� �Ϸ�. ��� �ð�, ��� �ݾ�:\r\n";
const char enterpass[] = "X�� �繰���� ��й�ȣ�� �Է����ּ���. (BACKSPACE:�ٽ��Է� SPACE: �ڷΰ���)\r\n";
const char password_keep[] = "\r\n��ǰ�� �����Ͽ����ϴ�. \r\n";
const char password_wrong[] = "\r\n��й�ȣ�� Ʋ�Ƚ��ϴ�. Ʋ�� Ƚ��:";
const char password_same[] = "\r\n��ǰ�� ȸ���Ǿ����ϴ�. ��� �ð�, ��� �ݾ�:\r\n";
const char manager_password_wrong[] = "\r\n������ ��й�ȣ�� Ʋ�Ƚ��ϴ�.\r\n";
char usart0_rx_data;
char usart0_tx_menu = 0, usart0_rx_eflg = 0, usart0_tx_keep = 0, usart0_tx_find = 0, usart0_tx_manager = 0, usart0_tx_error = 0, usart0_tx_enterpass = 0; // usart�� ���� ��,��� flag
char menu_mode = 0, error_mode = 0, keep_mode = 0, find_mode = 0, manager_mode = 0, enterpass_mode = 0; // ��� flag (���� � ��忡 ��ġ�ϴ� ��) 
char selected_locker; // ���� ���õ� �繰�� ��ȣ
char locker_password[10][6]; // 1������ 9�������� �繰�� ��й�ȣ ( �繰�� ��ȣ �ε���, ��й�ȣ�� ������ �ε��� )
char find_locker_password[10][6]; // �繰���� ã�� �� �Է¹��� ��й�ȣ
char find_manager_password[6]; // �Է¹��� ������ ��й�ȣ
char locker_usage[10] = { -1, 0, 0, 0, 0, 0, 0, 0, 0, 0 }; // 1������ 9�������� �繰�� ��뿩�� (  0 : ������� . 1 : ����� , 2: ��� )
char password_error_cnt[10]; // 1������ 9�������� �繰�� ��й�ȣ Ʋ�� Ƚ��
char locker_idx = 1; // �繰�Կ� ���� �ε���
char password_idx = 0; // ��й�ȣ�� ���� �ε���
char manager_password_idx = 0; // ������ ��й�ȣ�� ���� �ε���
int keeping_seconds[10]; // 1������ 9������ �����ð�
int current_seconds = 0 , using_seconds = 0; // ������� ���� �ð��� ���ð�
int tens_minutes = 0 , ones_minutes = 0 ,tens_seconds = 0, ones_seconds = 0; // ���ð��� ��,�� ���� �ڸ���
int t = 0; // timer ������ ���� ����
int i = 0, j = 0, k = 0; // ������ ���� ����� ���� �ε���

// �Լ� ����
void set_mode(char mode_name);
char password_is_same();
 
//=============================================
//MCU �ʱ�ȭ function
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


// =========== ���� ���α׷� ===========
// [ �μ� ] : void
//  ���� � mode������ ���� ���/������ ��츦 ��������.
//  ��й�ȣ ã��, ���� , �����ڸ�� ��� ��й�ȣ �Է� ������ �ϹǷ�
//  ��й�ȣ �Է� ���� ��й�ȣ �Է� ���� ������ �ϰ� �ִ��� �����Ͽ���.
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
		if(usart0_tx_menu) // �޴� ���ڿ� ���
		{
			set_mode(MENU_MODE);
			UCSR0B = 0xF8; // UDRE interrupt enable ( ���� �۽� O ) ���ڿ� ���� �������� �� disable ( ���� �۽� X )  
			usart0_tx_menu = 0;
		}
		if(usart0_tx_keep) // ���� ���ڿ� ���
		{
			set_mode(KEEP_MODE);
			UCSR0B = 0xF8;
			usart0_tx_keep = 0;
		}
		if(usart0_tx_find) // ã�� ���ڿ� ���
		{
			set_mode(FIND_MODE);
			UCSR0B = 0xF8;
			usart0_tx_find = 0;
		}
		if(usart0_tx_manager) // ������ ��� ���ڿ� ���
		{
			set_mode(MANAGER_MODE);
			UCSR0B = 0xF8;
			usart0_tx_manager = 0;
		}
		if(usart0_tx_enterpass) // ��й�ȣ �Է� ���� ���ڿ� ���
		{
			enterpass_mode = 1;
			UCSR0B = 0xF8;
			usart0_tx_enterpass = 0;
		}
		if(usart0_rx_eflg) 
		{
			if(menu_mode) // �޴���� ���� ����
			{
				if(usart0_rx_data == '1')	usart0_tx_keep = 1;
				else if(usart0_rx_data == '2')	usart0_tx_find = 1;
				else if(usart0_rx_data == '3')	usart0_tx_manager = 1;	 
			}
			else if(!enterpass_mode) // ����,ã��, �����ڸ�� �繰�� ���� ����
			{
				locker_num = usart0_rx_data - 48; // �Է¹��� ���ڷμ��� ���ڸ� �����μ��� ���ڷ� �Ǻ� ( 0(48) ~ 9(57) )
				if(locker_num >= 1 && locker_num <= 9 )
				{
					if((locker_usage[locker_num] == 0 && keep_mode) || (locker_usage[locker_num] == 1 && find_mode) || (locker_usage[locker_num] == 2 && manager_mode))
					{
						UDR0 = '\n'; 
						selected_locker = locker_num; // ������ �繰�� ��ȣ ����
						if(keep_mode || find_mode)	usart0_tx_enterpass = 1; // ��й�ȣ �Է� ���� ���ڿ� ���
						else	usart0_tx_manager = 1;
					}
				}
				else if(usart0_rx_data == ' ') // SPACE �Է�
				{
					usart0_tx_menu = 1;
					if(manager_mode) manager_password_idx = 0;
				}
			}
			else if(enterpass_mode) // ����, ã�� , ������ ��� ��й�ȣ ����
			{
				one_digit = usart0_rx_data - 48;
				if(one_digit >= 0 && one_digit <= 9 )
				{
					if(keep_mode) // �������
					{
						locker_password[selected_locker][password_idx++] = one_digit; // ������ �繰���� ��й�ȣ ����
						UDR0 = usart0_rx_data;
						if(password_idx >= 6)
							usart0_tx_enterpass = 1;
					}
					else if(find_mode) // ã����
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
					else if(manager_mode) // ������ ���
					{
						find_manager_password[manager_password_idx++] = one_digit;
						UDR0 = '*';
						if(manager_password_idx >= 6)
							usart0_tx_enterpass = 1;
					}
				}
				else if(usart0_rx_data == '\b') // BACK SPACE �Է� 
				{
					UDR0 = '\r';
					if(keep_mode || find_mode) password_idx = 0;
					else manager_password_idx = 0;
				}
				else if(usart0_rx_data == ' ') // SPACE �Է� 
				{
					if(password_idx) UDR0 = '\n'; // �Է��� �н����尡 ���� ��

					if(keep_mode || find_mode) password_idx = 0;
					else manager_password_idx = 0;

					if(keep_mode) usart0_tx_keep = 1;
					else if(find_mode) usart0_tx_find = 1;
					else if(manager_mode) usart0_tx_menu = 1;
				}
			}
			usart0_rx_eflg = 0;
		}
		if(usart0_tx_error) // �۽� �̻� �߻� �� menu�� �̵�
		{
			usart0_tx_menu = 1;
			usart0_tx_error = 0;
		}
	}
	return 0;
}

// =========== CPU �ʱ�ȭ =============
void cpu_init(void)
{
/*
	��� interrupt ��ȣ�� �߻����� �ʵ��� ��ġ�Ѵ�.
	���� : ������ �ʴ� interrupt�� ���� ó�� routine��
	���� ��� MCU�� �̻� ������ �� �� �ִ�. 
	(�Ʒ� �ڵ�� �״�� ����ϵ��� �� ) 
*/

//	External (�ܺ�) interrut 0~7 �߻����� �ʵ��� (disable) -> datasheet p. 90 ~ 92
	EICRA = 0x00;
	EICRB = 0x00;
	EIMSK = 0x00; 

// EEPROM�� interrupy �߻����� �ʵ��� (disable) -> datasheet p. 22
	EECR = 0x00;

// Timer(s) / Counter(s) Interrupt (s) �߻����� �ʵ��� (disable)
	TIMSK = 0x00;
	ETIMSK = 0x00;

// USART 0,1�� interrupt �߻����� �ʵ��� (disable) -> datasheet p. 190
	UCSR0B = 0x00;
	UCSR1B = 0x00;

// SPI interrupt �߻����� �ʵ��� (disable) -> datasheet p. 167
	SPCR = 0x00;

// TWI�� interrupt �߻����� �ʵ��� (disable) -> datasheet p. 206
	TWCR = 0x00;

// Analog-to-Digital converter�� interrupt �߻����� �ʵ��� (disable) -> datasheet p. 224
	ACSR = 0x00;

/* 
	Global interrupts enable : sei �Լ��� ������ �Ǿ�� 
	�� block�� interrupt�� enable/disable�� �� ���� --> ������ ����� 
	sei �Լ��� interrupt.h�� ����Ǿ� ����
*/
	sei();

/*
	���� digital I/O pin �⺻ ���� : �⺻������
	�Է����� �����ϰ�, pull-up ������ ������� �ʵ��� �Ѵ�.
*/

// PortA ���� 
	PORTA = 0x00;	// ��Ʈ ��� reg  
	DDRA = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

// PortB ���� 
	PORTB = 0x00;	// ��Ʈ ��� reg
	DDRB = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

// PortC ���� 
	PORTC = 0x00;	// ��Ʈ ��� reg
	DDRC = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

// PortD ���� 
	PORTD = 0x00;	// ��Ʈ ��� reg
	DDRD = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

// PortE ���� 
	PORTE = 0x00;	// ��Ʈ ��� reg
	DDRE = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

// PortF ���� 
	PORTF = 0x00;	// ��Ʈ ��� reg
	DDRF = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)
// PortF ���� 
	//PORTG = 0x00;	// ��Ʈ ��� reg
	//DDRG = 0x00;	// ��Ʈ ���� reg(0�Է�, 1���)

	PORTG = 0xFF;	// ��Ʈ ��� reg
	DDRG = 0XFF; 	// ��Ʈ ���� reg(0�Է�, 1���) 

}

void usart01_setup(void)
{

//	USART port ��� �ʱ�ȭ
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


SIGNAL(SIG_UART0_DATA) // UDR Empty Interrupt : ���ӵ� �۽Ž� ��� 
{
	if(menu_mode) 
	{ // �޴� ���ڿ� ���
		if(menu[i] == '\0') // �޴� ���ڿ� ���� ����
		{
			UCSR0B = 0xD8; // UDRE interrupt disable
			i = 0; 
		}
		else
			UDR0 = menu[i++];
	}
	else if(!enterpass_mode)  // ��й�ȣ �Է°� ���� ����
	{ 
	    if(keep_mode) 
	    {// ���� ���ڿ� ��� 
	        if(keep[i] == '\0') // ���� ���ڿ� ���� �����ϸ� �繰�� �����Ȳ ���
	        {
	            if(locker_idx >= 1 && locker_idx <= 9)
	            {
		           	if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // �繰���� ������̸�  
		         	else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// �繰���� ��� ������
					else  UDR0 = 'L';// �繰���� ��� ������
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
	    {//  ã�� ���ڿ� ���
	        if(find[i] == '\0') // ã�� ���ڿ� ���� �����ϸ� �繰�� �����Ȳ ���
	        {
	            if(locker_idx >= 1 && locker_idx <= 9)
	            {
		           	if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // �繰���� ������̸�  
		         	else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// �繰���� ��� ������
					else  UDR0 = 'L';// �繰���� ��� ������
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
	    {// �����ڸ�� ���� ���ڿ� ���
			if(manager_password_idx >= 6 && locker_usage[selected_locker] == 2) // ��� �ڹ����� ���� �� �� ���ڿ� ���
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
						UDR0 = (using_seconds / SECONDS_PER_DOLLAR+ 1) + 48; // 30�ʴ� 1�޷�
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
			else // �����ڸ�� ���� ���ڿ� ���
			{
		        if(manager[i] == '\0')
		        {
		            UCSR0B = 0xD8;
		            enterpass_mode = 1; // �ٷ� ��й�ȣ �Է��� ���� �� �ְ� ��
					manager_password_idx = 0;
		            i = 0; 
		        }
		        else
		         	UDR0 = manager[i++];
			}
	    }
	}
	else if(enterpass_mode) // ��й�ȣ �Է°� ��������
	{
		if(keep_mode || find_mode)
		{
			if(password_idx >= 6) // ��й�ȣ�� ���� �Է����� ���� ���ڿ� ���
			{
    			if(keep_mode) // ��ǰ ���� ����
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
				    if(password_is_same()) // ��ǰ ã�� ���� (��й�ȣ ����)
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
								UDR0 = (using_seconds / SECONDS_PER_DOLLAR + 1) + 48; // 30�ʴ� 1�޷�
							else if(j == 7)
								UDR0 = '$';
							else if(j == 8)
								UDR0 = '\r';

							else 
							{
								UDR0 = '\n';
								UCSR0B = 0xD8;
				          	  	locker_usage[selected_locker] = 0;
								password_error_cnt[selected_locker] = 0; // Ʋ�� Ƚ�� �ʱ�ȭ
				         	   	usart0_tx_menu = 1;
				           		i = 0; j = -1; password_idx = 0;
							}			
							j++;
				        } 	
				        else 
				            UDR0 = password_same[i++];
				    } 
				    else 
				    { // ��ǰ ã�� ���� (��й�ȣ �ٸ�) 
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
			else // ��й�ȣ �Է� �ޱ� �� ���ڿ� ���
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
		else // ������ ���
		{	
			if(manager_password_idx >= 6) // ��й�ȣ�� ���� �Է����� ���� ���ڿ� ���
			{
				if(password_is_same())  // ������ ��й�ȣ ����
				{	
					if(in_manager[i] == '\0') // �����ڸ�� ���ù��ڿ��� ������
					{
						if(j == 0) // �繰�� ��Ȳ ���
						{
							if(locker_idx >= 1 && locker_idx <= 9)
		            		{
		                		if(locker_usage[locker_idx] == 1) UDR0 = 'X'; // �繰���� ������̸�  
		                		else if(locker_usage[locker_idx] == 0) UDR0 = 'O';// �繰���� ��� ������
								else  UDR0 = 'L';// �繰���� ��� ������
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
						else // j�� 0���� 1�� �Ǹ鼭 ����ִ� �繰�� ��й�ȣ ��� 
						{
							if(locker_idx >= 10) // �繰�� ��й�ȣ�� ��� ����� ���¸�
							{
								if(locker_idx == 10 )UDR0 = '\r';
								else
								{
									UCSR0B = 0xD8;
									UDR0 = '\n';
									enterpass_mode = 0; // ��� �繰�� ������ �� �� �ֵ���
									i = 0; j = 0; password_idx = 0; locker_idx = 0;
								}
								locker_idx++;
							}	
							else if(locker_usage[locker_idx] == 1) // ������ �繰�� ��й�ȣ ���
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
							else // �繰���� ����ְų� ��� ������ ��� X
								locker_idx++;
						}
					}
					else // �����ڸ�� ���� ���ڿ� ���
						UDR0 = in_manager[i++];
				}
				else // ������ ��й�ȣ �ٸ�
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
SIGNAL(SIG_UART0_TRANS) // UDR TxC Interrupt : �۽� �Ϸ� �� ���
{ // TxC Flag�� �ڵ������� 1���� 0�̵� (TxC 0, UDRE 1)
	char status = UCSR0A;
	if((status & 0x60) != 0x20) // TxC 0 , UDRE 1�� �ƴϸ�
		usart0_tx_error = 1;
}

SIGNAL(SIG_UART0_RECV) // UDR RxC Interrupt : ���� �Ϸ� �� ���
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

	if( t < 2004 ) t++; // 1�ֱ�(1msec)�� 2������ t�� 2000�� ����� �� 1��
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
{//mode_name�� ������ mode flag 0 
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
{// ������ �繰�Կ� ���� ��й�ȣ�� �Է��� ��й�ȣ�� ������ Ȯ��
	if(manager_mode)
	{// ������ ��й�ȣ
		for(int i = 0 ; i < 6; i++)
		{
			if(manager_password[i] != find_manager_password[i]) 
				return 0; // ��й�ȣ�� �ٸ�
		}
	}
	else
	{// �Ϲ� ��й�ȣ
		for(int i = 0 ; i < 6; i++)
		{
			if(locker_password[selected_locker][i] != find_locker_password[selected_locker][i]) 
				return 0; // ��й�ȣ�� �ٸ�
		}
	}
	return 1; // ��й�ȣ�� ���� 
}

void divide_seconds(int seconds) 
{//�ʸ� �а� �ʷ� ������ �а� �ʸ� �ڸ��� ���� ����
    int minutes = seconds / 60;
    int remaining_seconds = seconds % 60;
	
	tens_minutes = minutes / 10;
	ones_minutes = minutes % 10;
    tens_seconds = remaining_seconds / 10;
    ones_seconds = remaining_seconds % 10;
}


void timer_counter2_setup(void)
{
	TCCR2 = 0x6B; // Fast PWM mode, Clear on match , 256kHz -> 3.906nsec * 256 -> 1�ֱ� 1msec
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
