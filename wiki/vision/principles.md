# Princípios do Frontend

Ao desenvolver no repositório `jay-frontend-cpp`, todos os mantenedores (humanos ou agentes) devem respeitar os seguintes princípios inegociáveis:

## 1. Responsividade e Performance (60 FPS)
O Frontend é focado em experiência visual em tempo real.
A thread principal (`main.cpp` contendo o Raylib) nunca deve ser bloqueada por operações síncronas de rede ou disco. Todo I/O bloqueante (como chamadas ao IPC do Core) deve rodar em threads separadas (background worker threads) para garantir taxa de quadros (framerate) constante de pelo menos 60 FPS.

## 2. Burro e Autocontido (Dumb UI)
A interface é uma marionete do Core.
O frontend jamais deve possuir inteligência de negócio, tomar decisões sobre o conteúdo da fala, executar inferência de LLM ou persistir dados em banco. Ele apenas **recebe comandos do Core** (via IPC) e os renderiza na tela, além de **enviar eventos do usuário** (cliques, atalhos, etc.) de volta ao Core.

## 3. Resiliência IPC
Como o frontend opera em um processo isolado e o Core pode travar, fechar ou reiniciar, o IPC Client do Frontend deve ser resiliente. Se a conexão via socket cair, o cliente deve tentar se reconectar indefinidamente sem causar o encerramento abrupto (crash) da interface gráfica.

## 4. Separação Estrita C++
Deve-se manter clareza e legibilidade no código em C++:
- Uso do clang-format obrigatório (evitar código superdenso ou sem espaçamento vertical apropriado).
- Utilização estrita de C++20 Modules para diminuir tempos de build e isolar cabeçalhos.
- Separação da lógica de apresentação (Views) da lógica de manipulação de dados (Modelos/Parsers JSON).
