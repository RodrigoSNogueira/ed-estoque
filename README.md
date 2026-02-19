# Sistema de Estoque em C (Lista + Fila + Pilha + Array)

## Tema escolhido
**Sistema de Estoque** (gerenciamento de produtos com processamento de pedidos).


## Descrição do sistema
Este projeto implementa um sistema de estoque em **C puro**, executado em terminal, com operações principais:

- **Cadastrar produto** (ID, nome, quantidade e preço)
- **Listar produtos** (exibição ordenada por ID)
- **Buscar produto por ID**
- **Remover produto**
- **Entrada/Saída de estoque** (movimentações de quantidade)
- **Atualizar preço**
- **Criar pedidos** (venda ou reposição) e armazenar em uma **fila FIFO**
- **Processar pedidos** (sempre processa o pedido mais antigo primeiro)
- **Desfazer última operação (UNDO)** usando uma **pilha LIFO**
- **Histórico** das ações usando um **array fixo** (buffer circular com deslocamento simples)

O sistema funciona durante a execução do programa, permitindo inserir/remover e manipular produtos e pedidos dinamicamente, com controle de memória usando `malloc/free`.


## Justificativa do uso das estruturas de dados

### 1) `struct`
Foi utilizada para modelar as entidades do sistema:
- `Produto`: dados do item em estoque
- `Pedido`: dados de uma venda ou reposição
- `Operacao`: registro para permitir desfazer ações (UNDO)

Isso torna o código organizado e coerente com o domínio do problema.


### 2) Lista Encadeada (Estoque)
O estoque é representado por uma **lista encadeada** de nós (`ProdutoNode`), permitindo:
- Inserção e remoção dinâmicas durante a execução
- Alocação sob demanda (sem tamanho fixo)

**Decisão de desempenho**:  
- Inserção de produto é feita em **O(1)** (no início da lista), tornando o cadastro rápido.
- Para manter a listagem “bonita”, a listagem é feita **ordenada por ID** usando um **array auxiliar de ponteiros**, ordenado no momento da impressão (sem alterar a lista).


### 3) Fila (FIFO) para pedidos
Pedidos (venda/reposição) entram em uma **fila FIFO**, garantindo:
- O primeiro pedido inserido é o primeiro processado
- Modela o comportamento real de atendimento por ordem de chegada

Campos:
- `inicio` (antigo front) e `final` (antigo rear)


### 4) Pilha (LIFO) para desfazer (UNDO)
A pilha armazena operações para permitir **desfazer**:
- Cadastro → desfazer remove o produto cadastrado
- Remoção → desfazer reinsere o produto removido
- Entrada/Saída/Preço → desfazer restaura o estado anterior do produto (snapshot)

A pilha é a estrutura ideal porque o último evento realizado é o primeiro que deve ser desfeito.


### 5) Array (Histórico fixo)
Um array fixo armazena as últimas ações como mensagens:
- Simples, rápido e direto para log
- Quando atinge o limite, mantém apenas as últimas `HIST_MAX`


## Arquivos do projeto
- `sistema-estoque.c` → código-fonte do sistema
- `Makefile` → automação de compilação e execução
- `README.md` → documentação


## Como compilar e executar

### Opção A) Usando Makefile (recomendado)
Na pasta do projeto:
bash
make
./estoque

### Opção B) Compilação manual 
clang -Wall -Wextra -O2 sistema-estoque.c -o estoque
./estoque
