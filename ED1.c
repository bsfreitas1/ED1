//#############################################################################
//
// ARQUIVO:   ED1.c
// TÍTULO:    Estudo Dirigido - Filtragem, FSM e PWM Integrados
// ALUNO:     Bernardo Silveira Freitas
//
//#############################################################################

#include "driverlib.h"
#include "device.h"
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>

// --- Definições do Sistema ---
#define BUFFER_SIZE     16
#define PWM_PERIOD      1000   // Resolução do PWM (0 a 1000)
#define LED_LIGADO      0      
#define LED_APAGADO     1

// --- Variáveis Globais ---
volatile uint16_t adc_raw = 0;
volatile uint16_t adc_filtered = 0;
volatile int16_t  sinal_ac = 0;           
volatile bool g_enableModulation = false; // Flag controlada via Debug


//VARIÁVEIS PARA A WATCH WINDOW ---
typedef enum { ESTADO_IDLE = 0, ESTADO_POSITIVE = 1, ESTADO_NEGATIVE = 2 } FSM_State;
volatile FSM_State g_estado_atual = ESTADO_IDLE;
volatile uint16_t g_duty_gpio31 = 0;
volatile uint16_t g_duty_gpio34 = 0;


// --- Variáveis Internas do Filtro ---
static uint16_t buffer[BUFFER_SIZE] = {0};
static uint16_t buffer_index = 0;
static uint32_t buffer_sum = 0;

// --- Variáveis do Simulador de Sinal e PWM ---
static float theta = 0.0f;
static uint16_t pwm_counter = 0;

void main(void)
{
    // 1. Inicialização do Sistema
    Device_init();
    Device_initGPIO();

    // Configuração dos LEDs (iniciando apagados)
    GPIO_setDirectionMode(31, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(31, GPIO_PIN_TYPE_STD);
    GPIO_writePin(31, LED_APAGADO);

    GPIO_setDirectionMode(34, GPIO_DIR_MODE_OUT);
    GPIO_setPadConfig(34, GPIO_PIN_TYPE_STD);
    GPIO_writePin(34, LED_APAGADO);

    Interrupt_initModule();
    Interrupt_initVectorTable();
    EINT;
    ERTM;

    // Inicializa o buffer com o valor de offset (2048) para evitar degrau inicial
    for(int i=0; i<BUFFER_SIZE; i++) {
        buffer[i] = 2048;
        buffer_sum += 2048;
    }

    // 2. Loop Principal (Amostragem, Filtragem e Controle)
    for(;;)
    {
        // ---------------------------------------------------------
        // A. SIMULADOR DO SINAL (ADC + Ruído)
        // ---------------------------------------------------------
        // Incrementa o ângulo lentamente para o efeito visual dos LEDs ser perceptível
        theta += 0.000005f; 
        if(theta >= 6.283185f) {
            theta = 0.0f;
        }
        
        // Gera ruído entre -50 e +50
        int16_t noise = (rand() % 101) - 50; 
        
        // Sinal: Offset 2048 + Senoide de amplitude 1000 + Ruído
        adc_raw = (uint16_t)(2048 + (1000.0f * sinf(theta)) + noise);

        // ---------------------------------------------------------
        // B. FILTRO DE MÉDIA MÓVEL (Circular)
        // ---------------------------------------------------------
        buffer_sum -= buffer[buffer_index];     // Remove a amostra mais velha
        buffer[buffer_index] = adc_raw;         // Insere a nova amostra
        buffer_sum += buffer[buffer_index];     // Adiciona à soma
        
        buffer_index++;
        if(buffer_index >= BUFFER_SIZE) {
            buffer_index = 0;
        }
        
        adc_filtered = buffer_sum / BUFFER_SIZE; // Calcula a média

        // Centraliza o sinal para análise da FSM (-1000 a +1000)
        sinal_ac = (int16_t)adc_filtered - 2048;

        // ---------------------------------------------------------
        // C. MÁQUINA DE ESTADOS (FSM) 
        // ---------------------------------------------------------
        if (g_enableModulation == false)
        {
            g_estado_atual = ESTADO_IDLE;
            g_duty_gpio31 = 0;
            g_duty_gpio34 = 0;
        }
        else
        {
            if (sinal_ac > 0)
            {
                g_estado_atual = ESTADO_POSITIVE;
                g_duty_gpio31 = sinal_ac; 
                g_duty_gpio34 = 0;        
            }
            else
            {
                g_estado_atual = ESTADO_NEGATIVE;
                g_duty_gpio31 = 0;
                g_duty_gpio34 = abs(sinal_ac); 
            }
        }

        // ---------------------------------------------------------
        // D. PWM 
        // ---------------------------------------------------------
        pwm_counter++;
        if(pwm_counter >= PWM_PERIOD) {
            pwm_counter = 0;
        }

        if (pwm_counter < g_duty_gpio31 && g_duty_gpio31 > 0) {
            GPIO_writePin(31, LED_LIGADO);
        } else {
            GPIO_writePin(31, LED_APAGADO);
        }

        if (pwm_counter < g_duty_gpio34 && g_duty_gpio34 > 0) {
            GPIO_writePin(34, LED_LIGADO);
        } else {
            GPIO_writePin(34, LED_APAGADO);
        }

    }
}