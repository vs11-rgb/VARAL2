#ifndef FSM_H
#define FSM_H

/*
 * FSM do varal sem sensores de fim de curso.
 * Os movimentos do servo sao considerados concluidos por temporizacao.
 */
#define NUM_ESTADOS 5
#define NUM_EVENTOS 7

typedef enum {
    RETRAIDO = 0,
    ESPERA,
    ESTENDIDO,
    ESTENDENDO,
    RETRAINDO
} Estado_t;

typedef enum {
    SECO = 0,
    MOLHADO,
    UMIDO,
    TEMPO,
    BOTAO,
    MOVIMENTO_CONCLUIDO,
    UMIDADE_OK,
    EVENTO_NENHUM /* sentinela; nao e um evento indexavel na matriz */
} Evento_t;

typedef enum {
    A01 = 0,
    A02,
    A03,
    A04,
    A05,
    A06,
    NENHUMA_ACAO
} Acao_t;

typedef struct {
    Estado_t prox_estado;
    Acao_t prox_acao;
} Transicao_t;

void fsm_init(void);
Estado_t fsm_process_event(Estado_t estado_atual, Evento_t evento, Acao_t *acao);

#endif
