#ifndef FSM_H
#define FSM_H

#define NUM_ESTADOS 5
#define NUM_EVENTOS 6

typedef enum {RETRAIDO, ESPERA, ESTENDIDO, ESTENDENDO, RETRAINDO
} Estado_t;
typedef enum {SECO, MOLHADO, UMIDO, TEMPO, BOTAO, LIMIT_SWITCH} Evento_t;
typedef enum {A01, A02, A03, A04, A05, A06, NENHUMA_ACAO} Acao_t;

typedef struct {
    Estado_t prox_estado;
    Acao_t prox_acao;
} Transicao_t;

void fsm_init(void);

Estado_t fsm_process_event(Estado_t estado_atual, Evento_t evento, Acao_t *acao);

#endif