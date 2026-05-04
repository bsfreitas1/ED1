// fsm.c
#include "fsm.h"
#include "driverlib.h"
#include "device.h"
#include <stdbool.h>

// --- Definições para os LEDs da LaunchPadXL ---
#define LED_AZUL_GPIO      31  
#define LED_VERMELHO_GPIO  34  

// Lógica invertida (nível baixo liga o LED)
#define LED_LIGADO     0
#define LED_APAGADO    1

// Define a taxa de piscada do LED no estado RECOVERING (ajuste conforme necessário)
#define BLINK_CYCLES   500U

// --- Variáveis de Estado Globais do Módulo (Definição) ---
volatile ConverterState_t g_converterState = CONVERTER_STATE_INIT;
volatile unsigned int g_faultFlags = 0U;
volatile unsigned long g_operationCounter = 0UL;

// --- Variáveis Internas (simulação de eventos) ---
static unsigned int startup_counter = 0U;
static bool enable_cmd = false;
static bool oc_fault_sim = false;
static bool ov_fault_sim = false;
static bool ot_fault_sim = false;
static bool comm_err_sim = false;
static unsigned int recovery_counter = 0U;

// Protótipos das Funções Handler de Estado (Internas)
static void state_init_handler(void);
static void state_standby_handler(void);
static void state_operating_handler(void);
static void state_fault_overcurrent_handler(void);
static void state_fault_overvoltage_handler(void);
static void state_fault_temp_handler(void);
static void state_fault_comm_handler(void);
static void state_recovering_handler(void);

// Funções de Evento Simuladas (Internas)
static bool check_startup_complete(void);
static bool check_enable_command(void);
static bool check_overcurrent_fault(void);
static bool check_overvoltage_fault(void);
static bool check_overtemp_fault(void);
static bool check_comm_error(void);
static bool check_recovery_complete(void);


// --- Implementações das Funções Públicas do Módulo FSM ---

void FSM_Init(void)
{
    g_converterState = CONVERTER_STATE_INIT;
    g_faultFlags = 0U;
    g_operationCounter = 0UL;

    // Resetar contadores das funções de evento simuladas
    startup_counter = 0U;
    enable_cmd = false;
    oc_fault_sim = false;
    ov_fault_sim = false;
    ot_fault_sim = false;
    comm_err_sim = false;
    recovery_counter = 0U;
}

void FSM_RunCycle(void)
{
    // Despacho baseado no estado atual usando switch-case
    switch (g_converterState)
    {
        case CONVERTER_STATE_INIT:
            state_init_handler();
            break;

        case CONVERTER_STATE_STANDBY:
            state_standby_handler();
            break;

        case CONVERTER_STATE_OPERATING:
            state_operating_handler();
            break;

        case CONVERTER_STATE_FAULT_OVERCURRENT:
            state_fault_overcurrent_handler();
            break;

        case CONVERTER_STATE_FAULT_OVERVOLTAGE:
            state_fault_overvoltage_handler();
            break;

        case CONVERTER_STATE_FAULT_TEMP:
            state_fault_temp_handler();
            break;

        case CONVERTER_STATE_FAULT_COMM:
            state_fault_comm_handler();
            break;

        case CONVERTER_STATE_RECOVERING:
            state_recovering_handler();
            break;

        default:
            // Estado inválido: força entrada em estado de falta
            g_converterState = CONVERTER_STATE_FAULT_OVERCURRENT;
            break;
    }
}


// --- Implementações das Funções Handler de Estado (Internas) ---

void state_init_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_APAGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_APAGADO);


    if (check_startup_complete())
    {
        g_converterState = CONVERTER_STATE_STANDBY;
    }
}

void state_standby_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_APAGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_APAGADO);


    if (check_enable_command())
    {
        g_converterState = CONVERTER_STATE_OPERATING;
    }
}

void state_operating_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_LIGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_APAGADO);

    g_operationCounter++;

    if (check_overcurrent_fault())
    {
        g_faultFlags |= FAULT_OVERCURRENT;
        g_converterState = CONVERTER_STATE_FAULT_OVERCURRENT;
    }
    else if (check_overvoltage_fault())
    {
        g_faultFlags |= FAULT_OVERVOLTAGE;
        g_converterState = CONVERTER_STATE_FAULT_OVERVOLTAGE;
    }
    // Outras verificações de falta podem ser adicionadas aqui se desejado
}

void state_fault_overcurrent_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_LIGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_LIGADO);


    if (check_recovery_complete())
    {
        g_faultFlags &= ~FAULT_OVERCURRENT;
        g_converterState = CONVERTER_STATE_RECOVERING;
    }
}

void state_fault_overvoltage_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_LIGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_LIGADO);


    if (check_recovery_complete())
    {
        g_faultFlags &= ~FAULT_OVERVOLTAGE;
        g_converterState = CONVERTER_STATE_RECOVERING;
    }
}

void state_fault_temp_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_LIGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_LIGADO);


    if (check_recovery_complete())
    {
        g_faultFlags &= ~FAULT_TEMPERATURE;
        g_converterState = CONVERTER_STATE_RECOVERING;
    }
}

void state_fault_comm_handler(void)
{

    GPIO_writePin(LED_AZUL_GPIO, LED_LIGADO);
    GPIO_writePin(LED_VERMELHO_GPIO, LED_LIGADO);


    if (check_recovery_complete())
    {
        g_faultFlags &= ~FAULT_COMM_ERROR;
        g_converterState = CONVERTER_STATE_RECOVERING;
    }
}

void state_recovering_handler(void)
{
    GPIO_writePin(LED_AZUL_GPIO, LED_APAGADO);

    // Lógica não-bloqueante para piscar o LED Azul
    static unsigned int blink_counter = 0;
    if (++blink_counter >= BLINK_CYCLES)
    {
        blink_counter = 0;
        GPIO_togglePin(LED_VERMELHO_GPIO); // Inverte o estado do pino
    }

    if (check_recovery_complete())
    {
        // Ao sair do estado de recuperação, reseta o contador e apaga o LED
        blink_counter = 0; 
        GPIO_writePin(LED_VERMELHO_GPIO, LED_APAGADO);
        g_converterState = CONVERTER_STATE_STANDBY;
    }
}


// --- Implementações das Funções de Evento Simuladas (Internas) ---

static bool check_startup_complete(void) {
    // Verifica se o estado é INIT e se o contador de inicialização excedeu o limite
    if (g_converterState == CONVERTER_STATE_INIT) {
        if (++startup_counter > TIME_STARTUP) {
            startup_counter = 0U;
            return true;
        }
    }
    return false;
}

static bool check_enable_command(void) {
    // Simula comando de habilitação externo (ativado via depurador)
    if (g_converterState == CONVERTER_STATE_STANDBY && enable_cmd) {
        enable_cmd = false;
        return true;
    }
    return false;
}

static bool check_overcurrent_fault(void) {
    if (g_converterState == CONVERTER_STATE_OPERATING && oc_fault_sim) {
        oc_fault_sim = false;
        return true;
    }
    return false;
}

static bool check_overvoltage_fault(void) {
    if (g_converterState == CONVERTER_STATE_OPERATING && ov_fault_sim) {
        ov_fault_sim = false;
        return true;
    }
    return false;
}

static bool check_overtemp_fault(void) {
    if (g_converterState == CONVERTER_STATE_OPERATING && ot_fault_sim) {
        ot_fault_sim = false;
        return true;
    }
    return false;
}

static bool check_comm_error(void) {
    if (g_converterState == CONVERTER_STATE_OPERATING && comm_err_sim) {
        comm_err_sim = false;
        return true;
    }
    return false;
}

static bool check_recovery_complete(void) {
    // Verifica se está em algum estado de falta ou recuperação e se o contador excedeu o limite
    if (g_converterState >= CONVERTER_STATE_FAULT_OVERCURRENT) {
        if (++recovery_counter > TIME_RECOVERY) {
            recovery_counter = 0U;
            return true;
        }
    }
    return false;
}
