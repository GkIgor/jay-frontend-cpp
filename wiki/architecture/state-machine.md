# Máquina de Estados Visual do Avatar

A interface da Jay não lida com "memória profunda" ou "raciocínio". Ela recebe comandos IPC que dizem explicitamente o que ela deve aparentar estar fazendo.

A Máquina de Estados (State Machine) visual mapeia as mensagens do Core para as seguintes aparências na interface:

1. `Offline / Erro`:
   - Conexão caiu ou o Core não está rodando. Tela ou Avatar adota cor neutra/vermelha, exibindo aviso de "Desconectado".
2. `Idle` (Ocioso):
   - A IA está rodando, conectada, mas sem fazer nada. Animação de repouso suave (ondas baixas, "respirando").
3. `Listening` (Ouvindo):
   - O microfone ou captação ativou (seja no Core ou detectado). Animação acompanha volumes altos, com cores receptivas (ex: azul/verde claro).
4. `Thinking / Processing` (Pensando):
   - A requisição foi para a LLM (Gemini) e aguardamos. Animação giratória pulsante ou "engrenagens", demonstrando computação.
5. `Speaking` (Falando):
   - A IA está reproduzindo som/TTS de resposta. A onda responde ao sinal de áudio, com textos de legendas na tela.

> O Core pode emitir um JSON `{"event": "state_changed", "data": "speaking"}` e o C++ simplesmente fará uma transição suave da animação atual para a de "speaking".
