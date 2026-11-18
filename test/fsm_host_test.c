/* Teste host opcional: gcc -std=c11 -Wall -Wextra -Werror \
 *   src/fsm.c test/fsm_host_test.c -Iinclude -o /tmp/fsm_test && /tmp/fsm_test
 */
#include <assert.h>
#include <stdio.h>
#include "fsm.h"

static void expect(Estado_t from, Evento_t event, Estado_t to, Acao_t expected_action)
{
    Acao_t action = NENHUMA_ACAO;
    const Estado_t result = fsm_process_event(from, event, &action);
    assert(result == to);
    assert(action == expected_action);
}

int main(void)
{
    fsm_init();

    expect(RETRAIDO, TEMPO, ESPERA, A01);
    expect(RETRAIDO, BOTAO, ESTENDENDO, A02);
    expect(ESPERA, TEMPO, ESPERA, NENHUMA_ACAO);
    expect(ESPERA, MOLHADO, ESPERA, NENHUMA_ACAO);
    expect(ESPERA, UMIDO, ESPERA, NENHUMA_ACAO);
    expect(ESPERA, UMIDADE_OK, ESPERA, NENHUMA_ACAO);
    expect(ESPERA, SECO, ESTENDENDO, A02);
    expect(ESPERA, BOTAO, ESTENDENDO, A02);
    expect(ESTENDENDO, MOVIMENTO_CONCLUIDO, ESTENDIDO, A03);
    expect(ESTENDENDO, MOLHADO, RETRAINDO, A04);
    expect(ESTENDENDO, UMIDO, RETRAINDO, A06);
    expect(ESTENDENDO, BOTAO, RETRAINDO, A04);
    expect(ESTENDIDO, MOLHADO, RETRAINDO, A04);
    expect(ESTENDIDO, UMIDO, RETRAINDO, A06);
    expect(ESTENDIDO, BOTAO, RETRAINDO, A04);
    expect(RETRAINDO, MOVIMENTO_CONCLUIDO, RETRAIDO, A05);

    /* Toda combinacao valida deve permanecer dentro dos limites dos enums. */
    for (int state = 0; state < NUM_ESTADOS; ++state) {
        for (int event = 0; event < NUM_EVENTOS; ++event) {
            Acao_t action = NENHUMA_ACAO;
            const Estado_t next = fsm_process_event((Estado_t)state, (Evento_t)event, &action);
            assert(next >= RETRAIDO && next < NUM_ESTADOS);
            assert(action >= A01 && action <= NENHUMA_ACAO);
        }
    }

    /* Sentinela e indices invalidos nunca podem acessar a matriz. */
    expect(ESPERA, EVENTO_NENHUM, ESPERA, NENHUMA_ACAO);
    expect((Estado_t)99, SECO, (Estado_t)99, NENHUMA_ACAO);
    expect(ESPERA, (Evento_t)99, ESPERA, NENHUMA_ACAO);
    assert(fsm_process_event(ESTENDIDO, SECO, NULL) == ESTENDIDO);

    puts("FSM host tests: OK");
    return 0;
}
