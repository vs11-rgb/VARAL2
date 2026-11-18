#include "fsm.h"

#include <stddef.h>

/*
 * A matriz tem 7 colunas: EVENTO_NENHUM e apenas uma sentinela (valor 7)
 * usada pela task ao inicializar sua variavel local, nunca e enfileirada.
 * Eventos sem transicao mantem o estado atual e nao executam acao.
 */
static Transicao_t matriz[NUM_ESTADOS][NUM_EVENTOS];

void fsm_init(void)
{
    for (int estado = 0; estado < NUM_ESTADOS; estado++) {
        for (int evento = 0; evento < NUM_EVENTOS; evento++) {
            matriz[estado][evento].prox_estado = (Estado_t)estado;
            matriz[estado][evento].prox_acao = NENHUMA_ACAO;
        }
    }

    /* Ciclo de avaliacao: apos o intervalo recolhido, entra em ESPERA. */
    matriz[RETRAIDO][TEMPO] = (Transicao_t){ESPERA, A01};

    /* ESPERA e um estado estavel: TEMPO reavalia o ambiente periodicamente. */
    matriz[ESPERA][TEMPO] = (Transicao_t){ESPERA, NENHUMA_ACAO};
    matriz[ESPERA][SECO] = (Transicao_t){ESTENDENDO, A02};
    matriz[ESPERA][BOTAO] = (Transicao_t){ESTENDENDO, A02};

    /* Se chuva/umidade persistirem, o varal permanece recolhido/em espera. */
    matriz[ESPERA][MOLHADO] = (Transicao_t){ESPERA, NENHUMA_ACAO};
    matriz[ESPERA][UMIDO] = (Transicao_t){ESPERA, NENHUMA_ACAO};
    matriz[ESPERA][UMIDADE_OK] = (Transicao_t){ESPERA, NENHUMA_ACAO};

    /* Botao pode solicitar extensao manual a partir da posicao recolhida. */
    matriz[RETRAIDO][BOTAO] = (Transicao_t){ESTENDENDO, A02};

    /* Conclusao temporizada do movimento de extensao/retracao. */
    matriz[ESTENDENDO][MOVIMENTO_CONCLUIDO] = (Transicao_t){ESTENDIDO, A03};
    matriz[RETRAINDO][MOVIMENTO_CONCLUIDO] = (Transicao_t){RETRAIDO, A05};

    /* Chuva, umidade alta ou botao durante a extensao iniciam recolhimento. */
    matriz[ESTENDENDO][MOLHADO] = (Transicao_t){RETRAINDO, A04};
    matriz[ESTENDENDO][UMIDO] = (Transicao_t){RETRAINDO, A06};
    matriz[ESTENDENDO][BOTAO] = (Transicao_t){RETRAINDO, A04};

    /* As mesmas condicoes recolhem o varal que ja esta estendido. */
    matriz[ESTENDIDO][MOLHADO] = (Transicao_t){RETRAINDO, A04};
    matriz[ESTENDIDO][UMIDO] = (Transicao_t){RETRAINDO, A06};
    matriz[ESTENDIDO][BOTAO] = (Transicao_t){RETRAINDO, A04};
}

Estado_t fsm_process_event(Estado_t estado_atual, Evento_t evento, Acao_t *acao)
{
    if (acao == NULL) {
        return estado_atual;
    }

    *acao = NENHUMA_ACAO;

    if ((int)estado_atual < 0 || estado_atual >= NUM_ESTADOS ||
        (int)evento < 0 || evento >= NUM_EVENTOS) {
        return estado_atual;
    }

    const Transicao_t transicao = matriz[estado_atual][evento];
    *acao = transicao.prox_acao;
    return transicao.prox_estado;
}
