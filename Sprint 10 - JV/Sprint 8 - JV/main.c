#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "nokia5110.h"

#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 conforme leitura do bit

typedef enum
{
	Tela_1, Tela_2, Tela_3, Tela_4, Tela_5, size_enumSelet
} enum_selet;

uint8_t verde = 1;
uint8_t amarelo = 1;
uint8_t vermelho = 1;
uint32_t tempo_ms = 0;
uint16_t num_carro = 0;
enum_selet selet_tela = 0;
uint8_t selet_modo = 0;
uint8_t atualiza_tela = 0;
uint8_t flagLUX = 0;
uint16_t lux = 0;
uint8_t pessoa = 0;
uint8_t emergencia = 0;


void switch_display (uint32_t tempo);
void LCD_nokia (uint8_t *tela_atualizada);
void leituraLUX(uint8_t *flag_lux);

ISR(TIMER0_COMPA_vect)
{
	tempo_ms++;  // Acressento 1 milisegundo
	
	if (tempo_ms % 50){
		atualiza_tela = 1;
	}
		
	if (tempo_ms % 300)
		flagLUX = 1;
		
		
}

ISR(PCINT1_vect)
{
	pessoa = !pessoa;
}

ISR(PCINT2_vect)
{
	//Como a interrupção externa PCINT2 só possui um endereço na memória de programa, é necessário testar qual foi o pino responsável pela interrupção.
	
	if(!tst_bit(PIND,PD1)) // Botão Emergencia
	{
		emergencia = !emergencia;
		if(emergencia)
			selet_tela = Tela_5;
		else
			selet_tela = Tela_1;
		_delay_ms(200);
	}
	
	else if(!tst_bit(PIND,PD2)) // sensor de carros
	{
		static uint32_t carro_anterior = 0;
		num_carro = 60000/((tempo_ms - carro_anterior));
		carro_anterior = tempo_ms;
	}
	else if(!tst_bit(PIND,PD4)) // +
	{
		switch (selet_tela){
			case Tela_1:
				selet_modo = !selet_modo;
			break;
			case Tela_2:
				if(verde<9)
				verde++;
			break;
			case Tela_3:
				if(vermelho<9)
				vermelho++;
			break;
			case Tela_4:
				if(amarelo<9)
				amarelo++;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD5)) // -
	{
		switch (selet_tela){
			case Tela_1:
			selet_modo = !selet_modo;
			break;
			case Tela_2:
			if(verde>1)
			verde--;
			break;
			case Tela_3:
			if(vermelho>1)
			vermelho--;
			break;
			case Tela_4:
			if(amarelo>1)
			amarelo--;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD6)) // S
	{
		static uint8_t flag_tela = 0;
		if (selet_modo)
		{
			if (flag_tela)
			{
				if (selet_tela < (size_enumSelet-2))
				selet_tela++;
				else
				selet_tela = 0;
			}
			flag_tela = !flag_tela;
		}
		_delay_ms(200);
	}	
}

int main(void){
	
	DDRB = 0b11111111; // Habilita os pinos PB0 ao PB7 todos como saida
	DDRD = 0b10001001; // Habilita o pino PD7, PD3 e PD0 como saidas
	PORTD = 0b01110110; // Habilita o resistor de pull up dos pinos PD1, PD2, PD4, PD5 e PD6
	DDRC = 0b01000000; // Habilita o pino PC6 como saida
	PORTC = 0b01000000; // Habilita o resistor de pull up do pino PC6
	
	PCICR  = 0b00000110;
	PCMSK1 = 0b01000000;
	PCMSK2 = 0b01110110;
		
	TCCR0A = 0b00000010; // habilita o CTC
	TCCR0B = 0b00000011; // Liga TC0 com prescaler = 64
	OCR0A  = 249;        // ajusta o comparador para TC0 contar até 249
	TIMSK0 = 0b00000010; // habilita a interrupção na igualdade de comparação com OCR0A. A interupção ocorre a cada 1ms = (65*(249+1))/16MHz

	ADMUX = 0b01000000; // Vcc com referencia canal PC0
	ADCSRA= 0b11100111; // Habilita o AD, habilita interrupção, modo de conversão continua, prescaler = 128
	ADCSRB= 0b00000000; // Modo de conversão contínua
	DIDR0 = 0b00000000; // habilita pino PC0 e PC1 como entrada digitais
	
 	TCCR2A = 0b10100011;
 	TCCR2B = 0b00000011;
 	OCR2B = 0;

	PORTD &= 0b11111110;
	
	sei(); //Habilita interrupção globais, ativando o bit I do SREG
	
	nokia_lcd_init();
	
	while (1){
		
		switch_display (tempo_ms); // Leva os valores do tempo de cada led para ser atualizado
		LCD_nokia (&atualiza_tela);
		leituraLUX (&flagLUX);
	}

}

void switch_display (uint32_t tempo){ // Atualiza os tempo dos leds
	
		const uint16_t estados[9] = {0b000001111, 0b000000111, 0b000000011, 0b000000001, 0b100000000, 0b011110000, 0b001110000, 0b000110000, 0b000010000};
		static int8_t i = 0;
		static uint32_t tempo_anterior_ms = 0;
		static uint8_t emerg = 0;
		
	PORTB = estados[i] & 0b011111111;
	
	if(!emergencia){
		
		PORTD &= 0b11111110;
		if (estados[i] & 0b100000000)
			PORTD |= (1<<7);
		else
			PORTD &= 0b01111111;

		if (i<=3)
		{
			if ((tempo - tempo_anterior_ms) >= (verde*250))
			{

				i++;
				tempo_anterior_ms += (verde*250);
			}
		}
		else
		{
			if(i<=4)
			{
				if((tempo - tempo_anterior_ms) >= (amarelo*1000))
				{
					i++;
					tempo_anterior_ms += (amarelo*1000);
					
				}
			}
			
			else
			{
				if(i<=8)
				{
					
					if((tempo - tempo_anterior_ms) >= (vermelho*250))
					{
						i++;
						tempo_anterior_ms += (vermelho*250);
					}
				}
				else
				{
					i=0;
					tempo_anterior_ms = tempo;
				}
			}
		}
	}
	else{
		if((tempo - tempo_anterior_ms) >= 500)
		{
			if (emerg){
				PORTD |= (1<<7);
				PORTD &= 0b11111110;
				emerg = !emerg;
				i = 4;
			}
			else{
				PORTD &= 0b01111111;
				PORTD |= (1<<0);
				emerg = !emerg;
			}
			tempo_anterior_ms = tempo;
		}
	}
}

void LCD_nokia (uint8_t *tela_atualizada){ // Atualiza o display de acordo com o tempo de cada led
	
	unsigned char verde_strig[3];
	unsigned char verme_strig[3];
	unsigned char amare_strig[3];
	unsigned char num_carro_strig[5];
	unsigned char inten_lux[4];
	static unsigned char modo_string;
	
	if (selet_modo)
		modo_string = 'M';
	else{
		modo_string = 'A';
		verde = 9 - (num_carro/60);
		vermelho = (num_carro/60) + 1;
	}
	
	sprintf (&verde_strig, "%u", verde);
	sprintf (&verme_strig, "%u", vermelho); // Converte as variaveis inteiras em char para imprimir no display
	sprintf (&amare_strig, "%u", amarelo);
	sprintf (&num_carro_strig, "%u", num_carro);
	sprintf (&inten_lux, "%u", lux);

	if(*tela_atualizada)
	{
		switch(selet_tela){
			case Tela_1:
			nokia_lcd_clear();
			nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
			nokia_lcd_set_cursor(0, 5);
			nokia_lcd_write_string("Modo", 1);
			nokia_lcd_set_cursor(30, 5);
			nokia_lcd_write_char(modo_string, 1);
			nokia_lcd_set_cursor(42, 5);
			nokia_lcd_write_string("<", 1);
			
			nokia_lcd_set_cursor(0, 15);
			nokia_lcd_write_string("T.Vd", 1);
			nokia_lcd_set_cursor(30,15);
			nokia_lcd_write_string(verde_strig, 1);
			nokia_lcd_set_cursor(35,15);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string(inten_lux, 2);
			nokia_lcd_set_cursor(50,15);
			nokia_lcd_write_string("lux", 1);
			
			nokia_lcd_set_cursor(50,25);
			nokia_lcd_write_string(num_carro_strig, 2);
			nokia_lcd_set_cursor(50,40);
			nokia_lcd_write_string("c/min", 1);
			
			nokia_lcd_set_cursor(0, 25);
			nokia_lcd_write_string("T.Vm", 1);
			nokia_lcd_set_cursor(30,25);
			nokia_lcd_write_string(verme_strig, 1);
			nokia_lcd_set_cursor(35,25);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(0,35);
			nokia_lcd_write_string("T.Am", 1);
			nokia_lcd_set_cursor(30,35);
			nokia_lcd_write_string(amare_strig, 1);
			nokia_lcd_set_cursor(35,35);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_render();
			break;
			
			case Tela_2:
			nokia_lcd_clear();
			nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
			nokia_lcd_set_cursor(0, 5);
			nokia_lcd_write_string("Modo", 1);
			nokia_lcd_set_cursor(30, 5);
			nokia_lcd_write_char(modo_string, 1);
			
			nokia_lcd_set_cursor(0, 15);
			nokia_lcd_write_string("T.Vd", 1);
			nokia_lcd_set_cursor(30,15);
			nokia_lcd_write_string(verde_strig, 1);
			nokia_lcd_set_cursor(35,15);
			nokia_lcd_write_string("s", 1);
			nokia_lcd_set_cursor(42, 15);
			nokia_lcd_write_string("<", 1);
			
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string(inten_lux, 2);
			nokia_lcd_set_cursor(50,15);
			nokia_lcd_write_string("lux", 1);
			
			nokia_lcd_set_cursor(50,25);
			nokia_lcd_write_string(num_carro_strig, 2);
			nokia_lcd_set_cursor(50,40);
			nokia_lcd_write_string("c/min", 1);
			
			nokia_lcd_set_cursor(0, 25);
			nokia_lcd_write_string("T.Vm", 1);
			nokia_lcd_set_cursor(30,25);
			nokia_lcd_write_string(verme_strig, 1);
			nokia_lcd_set_cursor(35,25);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(0,35);
			nokia_lcd_write_string("T.Am", 1);
			nokia_lcd_set_cursor(30,35);
			nokia_lcd_write_string(amare_strig, 1);
			nokia_lcd_set_cursor(35,35);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_render();
			break;
			
			case Tela_3:
			nokia_lcd_clear();
			nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
			nokia_lcd_set_cursor(0, 5);
			nokia_lcd_write_string("Modo", 1);
			nokia_lcd_set_cursor(30, 5);
			nokia_lcd_write_char(modo_string, 1);

			nokia_lcd_set_cursor(0, 15);
			nokia_lcd_write_string("T.Vd", 1);
			nokia_lcd_set_cursor(30,15);
			nokia_lcd_write_string(verde_strig, 1);
			nokia_lcd_set_cursor(35,15);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string(inten_lux, 2);
			nokia_lcd_set_cursor(50,15);
			nokia_lcd_write_string("lux", 1);
			
			nokia_lcd_set_cursor(50,25);
			nokia_lcd_write_string(num_carro_strig, 2);
			nokia_lcd_set_cursor(50,40);
			nokia_lcd_write_string("c/min", 1);
			
			nokia_lcd_set_cursor(0, 25);
			nokia_lcd_write_string("T.Vm", 1);
			nokia_lcd_set_cursor(30,25);
			nokia_lcd_write_string(verme_strig, 1);
			nokia_lcd_set_cursor(35,25);
			nokia_lcd_write_string("s", 1);
			nokia_lcd_set_cursor(42, 25);
			nokia_lcd_write_string("<", 1);
			
			nokia_lcd_set_cursor(0,35);
			nokia_lcd_write_string("T.Am", 1);
			nokia_lcd_set_cursor(30,35);
			nokia_lcd_write_string(amare_strig, 1);
			nokia_lcd_set_cursor(35,35);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_render();
			break;
			
			case Tela_4:
			nokia_lcd_clear();
			nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)
			
			nokia_lcd_set_cursor(0, 5);
			nokia_lcd_write_string("Modo", 1);
			nokia_lcd_set_cursor(30, 5);
			nokia_lcd_write_char(modo_string, 1);
			
			nokia_lcd_set_cursor(0, 15);
			nokia_lcd_write_string("T.Vd", 1);
			nokia_lcd_set_cursor(30,15);
			nokia_lcd_write_string(verde_strig, 1);
			nokia_lcd_set_cursor(35,15);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(50,0);
			nokia_lcd_write_string(inten_lux, 2);
			nokia_lcd_set_cursor(50,15);
			nokia_lcd_write_string("lux", 1);
			
			nokia_lcd_set_cursor(50,25);
			nokia_lcd_write_string(num_carro_strig, 2);
			nokia_lcd_set_cursor(50,40);
			nokia_lcd_write_string("c/min", 1);
			
			nokia_lcd_set_cursor(0, 25);
			nokia_lcd_write_string("T.Vm", 1);
			nokia_lcd_set_cursor(30,25);
			nokia_lcd_write_string(verme_strig, 1);
			nokia_lcd_set_cursor(35,25);
			nokia_lcd_write_string("s", 1);
			
			nokia_lcd_set_cursor(0,35);
			nokia_lcd_write_string("T.Am", 1);
			nokia_lcd_set_cursor(30,35);
			nokia_lcd_write_string(amare_strig, 1);
			nokia_lcd_set_cursor(35,35);
			nokia_lcd_write_string("s", 1);
			nokia_lcd_set_cursor(42, 35);
			nokia_lcd_write_string("<", 1);
			
			nokia_lcd_render();
			break;
			
			case Tela_5:
			nokia_lcd_clear();
			nokia_lcd_set_cursor(5,0);
			nokia_lcd_write_string("ATENCAO", 2);
			nokia_lcd_set_cursor(5,25);
			nokia_lcd_write_string("PARE!", 3);
			nokia_lcd_render();
			break;
		}
	}
	*tela_atualizada = !*tela_atualizada;
}

void leituraLUX(uint8_t *flag_lux){
	static float testelux = 0;
	
	if(*flag_lux)
	{
		testelux = ADC;
		lux = 1000-((testelux-511)*1.955);
		
		if (lux < 300){
			if(pessoa == 1 || num_carro > 0)
				OCR2B = 255;
			else if(pessoa == 0 || num_carro <= 0)
				OCR2B = 77;
		}	
		else
			OCR2B = 0;
	}
	*flag_lux = 0;
}





