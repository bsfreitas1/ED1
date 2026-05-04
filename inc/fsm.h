// fsm.h
#ifndef FSM_H_
#define FSM_H_

#include <stdbool.h>
#include <stdint.h>

// --- Definições Públicas para Flags de Falha (empacotadas em bits) ---
#define FAULT_OVERCURRENT   (1U << 0) // Bit 0: Sobrecorrente detectada
#define FAULT_OVERVOLTAGE   (1U << 1) // Bit 1: Sobretensão detectada
#define FAULT_TEMPERATURE   (1U << 2) // Bit 2: Sobretemperatura detectada
#define FAULT_COMM_ERROR    (1U << 3) // Bit 3: Erro de comunicação

// --- Definições Públicas para Tempos ---
#define TIME_STARTUP        5000U        // Tempo de inicialização (em ciclos de FSM)
#define TIME_RECOVERY       10000U       // Tempo de recuperação (em ciclos de FSM)
#define TIME_DELAY_US       1000U  // Atraso de cada ciclo da FSM em microssegundos (1 segundo)


// --- Enumeração Pública para Estados do Conversor ---
typedef enum
{
    CONVERTER_STATE_INIT,
    CONVERTER_STATE_STANDBY,
    CONVERTER_STATE_OPERATING,
    CONVERTER_STATE_FAULT_OVERCURRENT,
    CONVERTER_STATE_FAULT_OVERVOLTAGE,
    CONVERTER_STATE_FAULT_TEMP,
    CONVERTER_STATE_FAULT_COMM,
    CONVERTER_STATE_RECOVERING
} ConverterState_t;

// --- Variáveis de Estado Globais do Módulo (acessíveis externamente) ---
extern volatile ConverterState_t g_converterState;
extern volatile unsigned int g_faultFlags;
extern volatile unsigned long g_operationCounter;

// --- Protótipos das Funções Públicas do Módulo FSM ---
void FSM_Init(void);     // Inicializa a máquina de estados
void FSM_RunCycle(void); // Executa um ciclo da máquina de estados

#endif /* FSM_H_ */
