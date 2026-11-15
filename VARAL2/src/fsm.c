#include "fsm.h"

static Transicao_t matriz[NUM_ESTADOS][NUM_EVENTOS];

void fsm_init(void){
    int i,j;
    for (i=0; i<NUM_ESTADOS; i++){
        for(j=0; j< NUM_EVENTOS; j++){
            matriz[i][j].prox_estado = (Estado_t)i;
            matriz[i][j].prox_acao = NENHUMA_ACAO;
        }
    }

    matriz[RETRAIDO][TEMPO].prox_estado = ESPERA;
    matriz[RETRAIDO][TEMPO].prox_acao = A01;
    
    matriz[ESPERA][SECO].prox_estado = ESTENDENDO;
    matriz[ESPERA][SECO].prox_acao = A02;

    matriz[ESTENDENDO][LIMIT_SWITCH].prox_estado = ESTENDIDO;
    matriz[ESTENDENDO][LIMIT_SWITCH].prox_acao = A03;

    matriz[ESTENDIDO][MOLHADO].prox_estado = RETRAINDO;
    matriz[ESTENDIDO][MOLHADO].prox_acao = A04;

    matriz[RETRAINDO][MOLHADO].prox_estado = RETRAIDO;
    matriz[RETRAINDO][MOLHADO].prox_acao = A05;

    matriz[ESTENDENDO][MOLHADO].prox_estado = RETRAIDO;
    matriz[ESTENDENDO][MOLHADO].prox_acao = A04;

    matriz[ESPERA][UMIDO].prox_estado = RETRAIDO;
    matriz[ESPERA][UMIDO].prox_acao = A06;
}

Estado_t fsm_process_event(Estado_t estado_atual, Evento_t evento, Acao_t *acao){
    *acao = matriz[estado_atual][evento].prox_acao;
    return matriz[estado_atual][evento].prox_estado;

}