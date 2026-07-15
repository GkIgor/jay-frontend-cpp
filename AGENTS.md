# Repository Guidelines - Jay Frontend (C++)

## Wiki-First Rule

Este repositório contém o frontend (corpo visual e auditivo) da Jay IA. 

A documentação local (arquitetura C++, build, ADRs de UI e Raylib) está na wiki do próprio Frontend:
- [Frontend Wiki (`wiki/`)](wiki/index.md)

Para arquitetura sistêmica macro, protocolos JSON-IPC e I/O, a fonte oficial continua sendo a wiki na subpasta do Core:
- [Core Wiki (`jay-ia/wiki/`)](../jay-ia/wiki/index.md)

## Como Agentes Devem Trabalhar no C++

Sempre que alterar qualquer arquivo de código neste repositório (arquivos `.cpp`, `.cppm`, `.h`, etc.), o agente **deve** obrigatoriamente:

1. **Testar a compilação**: Executar `cmake --build build` (ou configurar primeiro com `cmake -B build -S .` se necessário) para garantir que o código compila perfeitamente sem avisos ou erros.
2. **Formatar o arquivo**: Formatar o arquivo usando a ferramenta padrão do repositório:
   ```bash
   clang-format -i <arquivo>
   ```
3. **Respeitar a legibilidade e espaçamento**: A configuração do `clang-format` deve sempre respeitar a legibilidade e a semântica de espaçamento em blocos e namespaces. Certifique-se de que o código não fique aglomerado, mantendo quebras de linha lógicas e espaços verticais adequados para leitura humana.
4. **Desacoplamento Rigoroso**: Nunca tente acessar de forma direta arquivos de fonte ou memória interna do Core. Toda a comunicação deve ocorrer estritamente através de mensagens JSON via Socket UNIX (IPC).
