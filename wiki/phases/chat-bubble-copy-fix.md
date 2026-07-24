# Correção do balão e ação de copiar

Status: Concluído

## Objetivo

Eliminar artefatos visuais internos do balão de mensagem e tornar a ação de copiar clara, acessível e funcional.

## Step 1 - Diagnóstico

- [x] Confirmar qual implementação do chat está ativa no `Shell`.
- [x] Reproduzir a falha de compilação causada pelo acesso direto ao clipboard da Raylib.
- [x] Revisar a geometria e os estados visuais do botão.

## Step 2 - Implementação

- [x] Encapsular o acesso ao clipboard no módulo de integração com a Raylib.
- [x] Manter o botão em uma área fixa no rodapé do balão.
- [x] Exibir estados distintos para repouso, hover e cópia concluída.
- [x] Remover linhas internas que possam produzir artefatos no preenchimento arredondado.

## Step 3 - Verificação

- [x] Formatar os módulos C++ alterados com `clang-format`.
- [x] Compilar o frontend com `cmake --build build`.
- [x] Registrar nesta página o comportamento final e o resultado da verificação.

## Resultado

O balão agora usa um único preenchimento arredondado, sem contorno ou divisor sobreposto. O botão permanece dentro do rodapé, com uma área de clique fixa e os estados `Copiar`, hover azul e `Copiado` verde durante dois segundos.

A escrita no clipboard ocorre por `jay::engine::WriteClipboardText`, mantendo o acesso à Raylib concentrado em `jay.engine.render_context`.

Verificações executadas:

- `clang-format -i src/engine/render_context.cppm src/features/chat/message_bubble_widget.cppm`
- `cmake --build build`: concluído com sucesso.
- `git diff --check`: concluído sem erros.
- `ctest --test-dir build --output-on-failure`: o projeto ainda não possui testes configurados.

A inspeção visual e a leitura do clipboard devem ser confirmadas interativamente em uma sessão gráfica.
