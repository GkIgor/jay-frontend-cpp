module;
#include <string>
#include <memory>
#include <iostream>
export module jay.usecases.base;

import jay.state.actions;
import jay.state.state_store;
import ipc_client;

export namespace jay::usecases {

// ─────────────────────────────────────────────────────────────────
// BaseUseCase — Contrato Base para todos os Casos de Uso
//
// Regras Arquiteturais (Review 3):
//   1. Workflow Encapsulado: Cada UseCase representa um workflow ou
//      intenção direta do usuário (ex: SendMessageUseCase).
//   2. Não-Proprietário: Recebe referências não-proprietárias para
//      StateStore& e IPCClient& no construtor.
//   3. Tratamento de Erros Isolado: Captura falhas de rede/socket e
//      as traduz em mutações de domínio na StateStore.
//   4. Sem Código Gráfico: Não importa nada de UI, Widgets ou Raylib.
// ─────────────────────────────────────────────────────────────────
class BaseUseCase {
public:
    BaseUseCase(jay::state::StateStore& store, jay::IPCClient& ipc)
        : m_store(store), m_ipc(ipc) {}

    virtual ~BaseUseCase() = default;

protected:
    jay::state::StateStore& m_store;
    jay::IPCClient&         m_ipc;
};

} // namespace jay::usecases
