# Especificação de Integração IPC do Core — Frontend C++ (`jay-frontend-cpp`)

**Documento:** Especificação Técnica de Integração Frontend <-> Core  
**Status:** Aprovado  
**Alvo:** `jay-frontend-cpp` (Cliente Gráfico C++23 / Raylib)  
**Versão do Protocolo:** `1`

---

## 1. Visão Geral da Integração

O **Jay Core** opera como um serviço de backend orientado a recursos, 100% reativo. Esta especificação define tudo o que o **Frontend C++** precisa implementar para comunicar-se com o Core.

### 1.1. Modelo de Comunicação Estritamente Reativo (Pull-Only)
- **O Core NUNCA envia mensagens não solicitadas (push / broadcast).**
- Toda comunicação é iniciada pelo Frontend C++ no modelo **Requisição-Resposta (*Request-Response / Pull*)**.
- Para atualizar a interface (chats, mensagens, ferramentas), o Frontend consulta o Core ativamente enviando mensagens de requisição (ex: `GetMessages`, `ListChats`).

```
+--------------------------+                     +-------------------+
|   Frontend C++ (Raylib)  | --- Solicitacao --> |     Jay Core      |
|  client_id:jay_client_cpp | <-- Resposta ------ | (Resource Engine) |
+--------------------------+                     +-------------------+
```

### 1.2. Desacoplamento de Transporte
- O transporte (Unix Domain Socket / UDS em `/tmp/jay/jay.sock` ou `XDG_RUNTIME_DIR/jay/jay.sock`) é efêmero.
- Conectar ou desconectar o socket **NÃO** remove a identidade do Frontend no Core.
- Ao reconectar, o Frontend re-envia o pacote de registro `RegisterClient`.

---

## 2. Envelope do Protocolo IPC (JSON-over-Socket)

Toda mensagem trafegada no socket deve utilizar a estrutura de envelope JSON abaixo.

### 2.1. Request Envelope (Frontend -> Core)
```json
{
  "protocol_version": 1,
  "request_id": "req-uuid-12345",
  "client_id": "jay_client_cpp",
  "type": 300,
  "payload": {}
}
```

- `protocol_version` (Integer): Sempre `1`.
- `request_id` (String UUID): Identificador único da requisição gerado pelo Frontend para correlacionar a resposta.
- `client_id` (String): Identificador da identidade lógica do Frontend (`"jay_client_cpp"`).
- `type` (Integer Enum `MessageType`): Código numérico da ação solicitada.
- `payload` (Object): Objeto JSON com os parâmetros do comando.

### 2.2. Response Envelope (Core -> Frontend)
```json
{
  "protocol_version": 1,
  "request_id": "req-uuid-12345",
  "type": 300,
  "status": 0,
  "error": null,
  "payload": {}
}
```

- `request_id` (String): O mesmo UUID enviado no request.
- `status` (Integer `ErrorCode`): `0` para Sucesso (`ErrSuccess`), maior que `0` para erros.
- `error` (Object nullable): Detalhes do erro quando `status != 0`.
- `payload` (Object): Resultado da operação em caso de sucesso.

### 2.3. Estrutura de Erros
```json
{
  "code": 4004,
  "message": "Chat com o ID informado não foi encontrado ou não está acessível.",
  "details": "chat_id=chat-uuid-9999"
}
```

#### Tabela de Códigos de Erro (`ErrorCode`)
| Código | Nome Enum | Descrição |
|---|---|---|
| `0` | `ErrSuccess` | Operação concluída com sucesso. |
| `4000` | `ErrInvalidFormat` | JSON malformado ou campos obrigatórios ausentes. |
| `4001` | `ErrUnauthorized` | Cliente não registrado. |
| `4003` | `ErrForbidden` | Acesso negado pelo motor de permissões. |
| `4004` | `ErrNotFound` | Recurso (Chat, Message, Tool) não encontrado. |
| `4009` | `ErrConflict` | Conflito de ID ou estado. |
| `5000` | `ErrInternalDatabase` | Erro interno de banco/persistência no Core. |

---

## 3. Comandos e Contratos da API (JSON Payloads)

### 3.1. Módulo de Registro (Handshake)

#### `RegisterClient` (`type = 100`)
Enviado logo após a abertura da conexão com o socket para registrar/confirmar a identidade do Frontend.
- **Request Payload:**
  ```json
  {
    "client_id": "jay_client_cpp",
    "metadata": {
      "version": "1.0.0",
      "platform": "linux_x86_64"
    }
  }
  ```
- **Response Payload:**
  ```json
  {
    "registration": {
      "id": "jay_client_cpp",
      "status": 1,
      "metadata": { "version": "1.0.0", "platform": "linux_x86_64" },
      "created_at": "2026-07-20T21:00:00Z"
    }
  }
  ```

---

### 3.2. Módulo de Chats

#### 1. `CreateChat` (`type = 200`)
Cria uma nova conversa no Core.
- **Request Payload:**
  ```json
  {
    "title": "Nova Conversa",
    "metadata": {}
  }
  ```
- **Response Payload:**
  ```json
  {
    "chat": {
      "id": "chat-uuid-0001",
      "owner_registration_id": "jay_client_cpp",
      "title": "Nova Conversa",
      "status": 1,
      "created_at": "2026-07-20T21:05:00Z"
    }
  }
  ```

#### 2. `ListChats` (`type = 204`)
Consulta a lista de conversas do usuário ou compartilhadas.
- **Request Payload:**
  ```json
  {
    "include_shared": true,
    "limit": 50,
    "offset": 0
  }
  ```
- **Response Payload:**
  ```json
  {
    "chats": [
      {
        "id": "chat-uuid-0001",
        "owner_registration_id": "jay_client_cpp",
        "title": "Nova Conversa",
        "status": 1,
        "is_owner": true,
        "created_at": "2026-07-20T21:05:00Z"
      }
    ],
    "total": 1
  }
  ```

---

### 3.3. Módulo de Mensagens & Processamento de Conversas

#### 1. `CreateMessage` (`type = 300`)
Envia uma nova mensagem do usuário para o chat. Se `trigger_agent` for `true`, solicita ao Core que processe e responda na mesma chamada.
- **Request Payload:**
  ```json
  {
    "chat_id": "chat-uuid-0001",
    "author_type": 1,
    "author_id": "jay_client_cpp",
    "role": 1,
    "content": "Olá Jay!",
    "content_type": 1,
    "trigger_agent": true,
    "metadata": {}
  }
  ```
- **Response Payload (com `trigger_agent = true`):**
  ```json
  {
    "created_message": {
      "id": "msg-uuid-0101",
      "chat_id": "chat-uuid-0001",
      "author_type": 1,
      "author_id": "jay_client_cpp",
      "role": 1,
      "content": "Olá Jay!",
      "sequence_no": 1,
      "created_at": "2026-07-20T21:10:00Z"
    },
    "processed_message": {
      "id": "msg-uuid-0102",
      "chat_id": "chat-uuid-0001",
      "author_type": 2,
      "author_id": "gemini_pro_agent",
      "role": 2,
      "content": "Olá! Como posso ajudar você hoje?",
      "sequence_no": 2,
      "created_at": "2026-07-20T21:10:02Z"
    }
  }
  ```

#### 2. `GetMessages` (`type = 303`)
Consulta o histórico de mensagens de um chat para renderização na tela.
- **Request Payload:**
  ```json
  {
    "chat_id": "chat-uuid-0001",
    "since_sequence_no": 0,
    "limit": 100
  }
  ```
- **Response Payload:**
  ```json
  {
    "chat_id": "chat-uuid-0001",
    "messages": [
      {
        "id": "msg-uuid-0101",
        "chat_id": "chat-uuid-0001",
        "author_type": 1,
        "author_id": "jay_client_cpp",
        "role": 1,
        "content": "Olá Jay!",
        "sequence_no": 1,
        "created_at": "2026-07-20T21:10:00Z"
      },
      {
        "id": "msg-uuid-0102",
        "chat_id": "chat-uuid-0001",
        "author_type": 2,
        "author_id": "gemini_pro_agent",
        "role": 2,
        "content": "Olá! Como posso ajudar você hoje?",
        "sequence_no": 2,
        "created_at": "2026-07-20T21:10:02Z"
      }
    ],
    "has_more": false
  }
  ```

#### 3. `MsgProcessChat` (`type = 350`)
Solicita que o Core processe a conversa e gere uma nova resposta da IA sem enviar uma nova mensagem de texto do usuário (ex: para re-tentar ou continuar a resposta).
- **Request Payload:**
  ```json
  {
    "chat_id": "chat-uuid-0001"
  }
  ```
- **Response Payload:**
  ```json
  {
    "processed_message": {
      "id": "msg-uuid-0103",
      "chat_id": "chat-uuid-0001",
      "author_type": 2,
      "author_id": "gemini_pro_agent",
      "role": 2,
      "content": "Continuando nosso papo...",
      "sequence_no": 3,
      "created_at": "2026-07-20T21:12:00Z"
    }
  }
  ```

---

### 3.4. Módulo de Ferramentas Nativas (`Tools`)

#### `RegisterTool` (`type = 400`)
Se o Frontend C++ oferecer ferramentas locais (ex: caixa de diálogo, manipulação de clipboard, modais da UI), ele deve registrá-las no Core.
- **Request Payload:**
  ```json
  {
    "id": "ui_prompt_dialog",
    "name": "Caixa de Diálogo do Frontend",
    "description": "Exibe uma caixa de diálogo nativa para o usuário confirmar ações.",
    "version": "1.0.0",
    "schema": {
      "type": "object",
      "properties": {
        "title": { "type": "string" },
        "message": { "type": "string" }
      },
      "required": ["title", "message"]
    }
  }
  ```
- **Response Payload:**
  ```json
  {
    "tool": {
      "id": "ui_prompt_dialog",
      "registration_id": "jay_client_cpp",
      "name": "Caixa de Diálogo do Frontend",
      "version": "1.0.0",
      "status": 1,
      "created_at": "2026-07-20T21:15:00Z"
    }
  }
  ```

---

## 4. Tipos e Enums Relevantes para o C++ Frontend

### 4.1. Enums de Mensagem do Protocolo (`MessageType`)
```cpp
enum class MessageType : int {
    RegisterClient       = 100,
    UnregisterClient     = 101,
    UpdateRegistration   = 102,
    GetRegistration      = 103,
    ListRegistrations    = 104,
    UpdateSharedRules    = 105,

    CreateChat           = 200,
    DeleteChat           = 201,
    RenameChat           = 202,
    GetChat              = 203,
    ListChats            = 204,

    CreateMessage        = 300,
    UpdateMessage        = 301,
    DeleteMessage        = 302,
    GetMessages          = 303,

    ProcessChat          = 350,

    RegisterTool         = 400,
    UnregisterTool       = 401,
    GetTool              = 402,
    ListTools            = 403
};
```

### 4.2. Enums do Domínio de Mensagens
```cpp
enum class AuthorType : int {
    Registration = 1,
    Agent        = 2,
    Tool         = 3,
    System       = 4
};

enum class MessageRole : int {
    User      = 1,
    Assistant = 2,
    System    = 3,
    Tool      = 4
};
```

---

## 5. Arquitetura do Cliente IPC em C++23 (`IPCClient`)

### 5.1. Responsabilidades do `IPCClient`
1. **Thread de Socket Assíncrona**: Manter conexão persistente e não-bloqueante no Unix Domain Socket (`/tmp/jay/jay.sock` ou `$XDG_RUNTIME_DIR/jay/jay.sock`).
2. **Correlacionador de Requisições (`RequestMap`)**: Mapear `request_id` (UUID) para callbacks (`std::function<void(nlohmann::json)>`) ou `std::promise<nlohmann::json>`.
3. **Reconexão Automática**: Caso o daemon `jayd` reinicie, o `IPCClient` reconecta e re-envia `RegisterClient`.

### 5.2. Exemplo de Loop do Frontend C++ (Pull & Render)
```cpp
// 1. Ao iniciar o app:
ipcClient.SendRegister("jay_client_cpp");

// 2. Ao carregar a tela de Chat:
ipcClient.SendRequest(MessageType::GetMessages, {{"chat_id", currentChatId}, {"limit", 50}}, [](json res) {
    chatMessages = ParseMessages(res["payload"]["messages"]);
});

// 3. Ao enviar mensagem pelo campo de texto:
ipcClient.SendRequest(MessageType::CreateMessage, {
    {"chat_id", currentChatId},
    {"author_type", 1},
    {"author_id", "jay_client_cpp"},
    {"role", 1},
    {"content", userText},
    {"trigger_agent", true}
}, [](json res) {
    // Adiciona mensagem do usuario e resposta da IA a lista de exibição
    AppendUserAndAssistantMessages(res["payload"]);
});
```
