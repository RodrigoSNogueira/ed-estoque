#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CONFIGURAÇÕES GERAIS (constantes simples) */
#define NAME_MAX 64
#define HIST_MAX 200

/*  1) MODELAGEM (structs do sistema)
   - Produto: o que existe no estoque
   - Nó do estoque: lista encadeada de produtos
   - Pedido: evento que vai pra fila (venda/reposição)
   - Operacao: registro para desfazer (pilha/UNDO) */

typedef struct {
    int id;
    char nome[NAME_MAX];
    int qtd;
    double preco;
} Produto;

/* Lista encadeada: cada nó guarda 1 produto e aponta para o próximo */
typedef struct ProdutoNode {
    Produto p;
    struct ProdutoNode *next;
} ProdutoNode;

/* Tipos de operação que entram no UNDO */
typedef enum {
    OP_CADASTRO = 1,
    OP_REMOCAO  = 2,
    OP_ENTRADA  = 3,
    OP_SAIDA    = 4,
    OP_ATUALIZA_PRECO = 5
} TipoOperacao;

/* Operacao guarda um “snapshot” (estado anterior) para desfazer */
typedef struct {
    TipoOperacao tipo;
    Produto snapshot;     // estado ANTES (ou produto removido)
    int delta_qtd;        // informativo (+x/-x) se necessário
} Operacao;

/* 2) PILHA (LIFO) - "DESFAZER"
   - LIFO: último que entrou é o primeiro que sai
   - Cada ação importante empilha uma Operacao */

typedef struct NoPilha {
    Operacao op;
    struct NoPilha *next;
} NoPilha;

typedef struct {
    NoPilha *topo;
    int tamanho;
} Pilha;

static void pilha_init(Pilha *p) {
    p->topo = NULL;
    p->tamanho = 0;
}

static int pilha_vazia(const Pilha *p) {
    return p->topo == NULL;
}

static void pilha_push(Pilha *p, Operacao op) {
    NoPilha *novo = (NoPilha*)malloc(sizeof(NoPilha));
    if (!novo) {
        printf("ERRO: sem memoria (pilha_push)\n");
        return;
    }
    novo->op = op;
    novo->next = p->topo;
    p->topo = novo;
    p->tamanho++;
}

static int pilha_pop(Pilha *p, Operacao *out) {
    if (pilha_vazia(p)) return 0;

    NoPilha *rem = p->topo;
    *out = rem->op;

    p->topo = rem->next;
    free(rem);
    p->tamanho--;
    return 1;
}

static void pilha_clear(Pilha *p) {
    Operacao lixo;
    while (pilha_pop(p, &lixo)) {}
}

/* 3) FILA (FIFO) - "PEDIDOS"
   - FIFO: primeiro a entrar é o primeiro a ser processado
   - Você pediu: front->inicio e rear->final */

typedef enum {
    PED_VENDA = 1,
    PED_REPOSICAO = 2
} TipoPedido;

typedef struct {
    int produto_id;
    int qtd;
    TipoPedido tipo;
} Pedido;

typedef struct NoFila {
    Pedido ped;
    struct NoFila *next;
} NoFila;

typedef struct {
    NoFila *inicio;   
    NoFila *final;   
    int tamanho;
} Fila;

static void fila_init(Fila *f) {
    f->inicio = NULL;
    f->final  = NULL;
    f->tamanho = 0;
}

static int fila_vazia(const Fila *f) {
    return f->inicio == NULL;
}

static void fila_enqueue(Fila *f, Pedido p) {
    NoFila *novo = (NoFila*)malloc(sizeof(NoFila));
    if (!novo) {
        printf("ERRO: sem memoria (fila_enqueue)\n");
        return;
    }

    novo->ped = p;
    novo->next = NULL;

    if (f->final) {
        f->final->next = novo;
    } else {
        f->inicio = novo;
    }
    f->final = novo;
    f->tamanho++;
}

static int fila_dequeue(Fila *f, Pedido *out) {
    if (fila_vazia(f)) return 0;

    NoFila *rem = f->inicio;
    *out = rem->ped;

    f->inicio = rem->next;
    if (!f->inicio) f->final = NULL;

    free(rem);
    f->tamanho--;
    return 1;
}

static void fila_clear(Fila *f) {
    Pedido lixo;
    while (fila_dequeue(f, &lixo)) {}
}

/* 4) HISTÓRICO (ARRAY FIXO)
   - Estrutura simples e rápida: armazena mensagens de log
   - Quando enche, “empurra” e mantém só as últimas HIST_MAX */

typedef struct {
    char msg[180];
} HistoricoItem;

static HistoricoItem historico[HIST_MAX];
static int historico_qtd = 0;

static void historico_add(const char *msg) {
    if (historico_qtd < HIST_MAX) {
        strncpy(historico[historico_qtd].msg, msg, sizeof(historico[historico_qtd].msg) - 1);
        historico[historico_qtd].msg[sizeof(historico[historico_qtd].msg) - 1] = '\0';
        historico_qtd++;
    } else {
        for (int i = 1; i < HIST_MAX; i++) historico[i - 1] = historico[i];
        strncpy(historico[HIST_MAX - 1].msg, msg, sizeof(historico[HIST_MAX - 1].msg) - 1);
        historico[HIST_MAX - 1].msg[sizeof(historico[HIST_MAX - 1].msg) - 1] = '\0';
    }
}

static void historico_print(void) {
    printf("\n=== HISTORICO (ultimos %d) ===\n", historico_qtd);
    if (historico_qtd == 0) {
        printf("(vazio)\n");
        return;
    }
    for (int i = 0; i < historico_qtd; i++) {
        printf("%2d) %s\n", i + 1, historico[i].msg);
    }
}

/* 5) ENTRADA/SAÍDA (helpers de leitura)
   - Evita bugs de scanf deixando buffer sujo
   - Torna o menu mais resistente a entradas erradas */

static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static int ler_int(const char *prompt) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) == 1) {
            flush_stdin();
            return x;
        }
        printf("Entrada invalida. Tente novamente.\n");
        flush_stdin();
    }
}

static double ler_double(const char *prompt) {
    double x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%lf", &x) == 1) {
            flush_stdin();
            return x;
        }
        printf("Entrada invalida. Tente novamente.\n");
        flush_stdin();
    }
}

static void ler_linha(const char *prompt, char *out, size_t cap) {
    printf("%s", prompt);
    if (!fgets(out, (int)cap, stdin)) {
        out[0] = '\0';
        return;
    }
    size_t len = strlen(out);
    if (len && out[len - 1] == '\n') out[len - 1] = '\0';
}

/* 6) LISTA ENCADEADA (ESTOQUE)
   - Inserção em O(1) (insere no início)
   - Sem global (ponta por parâmetro)
   - Sem ID duplicado
   - Listagem ordenada via array auxiliar (não altera a lista) */

static ProdutoNode* criar_no_produto(Produto p) {
    ProdutoNode *novo = (ProdutoNode*)malloc(sizeof(ProdutoNode));
    if (!novo) {
        printf("Erro de memoria!\n");
        exit(1);
    }
    novo->p = p;
    novo->next = NULL;
    return novo;
}

static ProdutoNode* buscar_produto_por_id(ProdutoNode *ponta, int id) {
    ProdutoNode *aux = ponta;
    while (aux) {
        if (aux->p.id == id) return aux;
        aux = aux->next;
    }
    return NULL;
}

/* Inserção O(1): sempre no início (mas bloqueia duplicado) */
static int inserir_produto_o1(ProdutoNode **ponta, Produto p) {
    if (!ponta) return 0;

    if (buscar_produto_por_id(*ponta, p.id)) {
        printf("ID %d ja existe. Nao vou duplicar.\n", p.id);
        return 0;
    }

    ProdutoNode *novo = criar_no_produto(p);
    novo->next = *ponta;
    *ponta = novo;
    return 1;
}

static int remover_produto_por_id(ProdutoNode **ponta, int id, Produto *out_removido) {
    if (!ponta || !*ponta) return 0;

    ProdutoNode *anterior = NULL;
    ProdutoNode *atual = *ponta;

    while (atual) {
        if (atual->p.id == id) {
            if (out_removido) *out_removido = atual->p;

            if (anterior) anterior->next = atual->next;
            else *ponta = atual->next;

            free(atual);
            return 1;
        }
        anterior = atual;
        atual = atual->next;
    }
    return 0;
}

static int contar_produtos(ProdutoNode *ponta) {
    int n = 0;
    while (ponta) { n++; ponta = ponta->next; }
    return n;
}

/* Ordenação simples(insertion sort):
   - Não usa bibliotecas proibidas
   - Boa para N pequeno/médio */
static void ordenar_array_de_nos_por_id(ProdutoNode **v, int n) {
    for (int i = 1; i < n; i++) {
        ProdutoNode *chave = v[i];
        int j = i - 1;
        while (j >= 0 && v[j]->p.id > chave->p.id) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = chave;
    }
}

/* Listagem ordenada sem mexer na lista:
   1) conta
   2) cria array de ponteiros
   3) ordena o array
   4) imprime
   Isso preserva inserção O(1) e dá exibição */
static void listar_estoque_ordenado(ProdutoNode *ponta) {
    printf("\n=== PRODUTOS NO ESTOQUE (ordenado por ID) ===\n");
    if (!ponta) {
        printf("(vazio)\n");
        return;
    }

    int n = contar_produtos(ponta);

    ProdutoNode **arr = (ProdutoNode**)malloc(n * sizeof(ProdutoNode*));
    if (!arr) {
        printf("ERRO: sem memoria (listar_estoque_ordenado)\n");
        return;
    }

    ProdutoNode *aux = ponta;
    for (int i = 0; i < n; i++) {
        arr[i] = aux;
        aux = aux->next;
    }

    ordenar_array_de_nos_por_id(arr, n);

    printf("%-6s | %-30s | %-6s | %-10s\n", "ID", "NOME", "QTD", "PRECO");
    printf("---------------------------------------------------------------\n");
    for (int i = 0; i < n; i++) {
        printf("%-6d | %-30s | %-6d | R$ %-8.2f\n",
               arr[i]->p.id, arr[i]->p.nome, arr[i]->p.qtd, arr[i]->p.preco);
    }

    free(arr);
}

static void liberar_estoque(ProdutoNode **ponta) {
    if (!ponta) return;

    ProdutoNode *cur = *ponta;
    while (cur) {
        ProdutoNode *prox = cur->next;
        free(cur);
        cur = prox;
    }
    *ponta = NULL;
}

/* 7) OPERAÇÕES DO SISTEMA (menu)
   - Cada ação registra no histórico
   - As ações reversíveis empilham no UNDO */

static void cadastrar_produto(ProdutoNode **ponta, Pilha *undo) {
    Produto p;
    p.id = ler_int("ID do produto: ");
    ler_linha("Nome: ", p.nome, sizeof(p.nome));
    p.qtd = ler_int("Quantidade inicial: ");
    p.preco = ler_double("Preco (ex: 19.90): ");

    if (p.id <= 0 || p.qtd < 0 || p.preco < 0 || strlen(p.nome) == 0) {
        printf("Dados invalidos.\n");
        return;
    }

    if (!inserir_produto_o1(ponta, p)) return;

    Operacao op = { OP_CADASTRO, p, 0 };
    pilha_push(undo, op);

    char msg[180];
    snprintf(msg, sizeof(msg),
             "CADASTRO: id=%d | nome=%s | qtd=%d | preco=%.2f",
             p.id, p.nome, p.qtd, p.preco);
    historico_add(msg);

    printf("Produto cadastrado com sucesso.\n");
}

static void remover_produto_menu(ProdutoNode **ponta, Pilha *undo) {
    int id = ler_int("ID do produto para remover: ");
    Produto removido;

    if (!remover_produto_por_id(ponta, id, &removido)) {
        printf("Produto nao encontrado.\n");
        return;
    }

    Operacao op = { OP_REMOCAO, removido, 0 };
    pilha_push(undo, op);

    char msg[180];
    snprintf(msg, sizeof(msg), "REMOCAO: id=%d | nome=%s", removido.id, removido.nome);
    historico_add(msg);

    printf("Produto removido.\n");
}

static void buscar_produto_menu(ProdutoNode *ponta) {
    int id = ler_int("ID para buscar: ");
    ProdutoNode *n = buscar_produto_por_id(ponta, id);

    if (!n) {
        printf("Produto nao encontrado.\n");
        return;
    }

    printf("\nEncontrado:\n");
    printf("ID: %d\nNome: %s\nQuantidade: %d\nPreco: R$ %.2f\n",
           n->p.id, n->p.nome, n->p.qtd, n->p.preco);
}

static void entrada_estoque(ProdutoNode *ponta, Pilha *undo) {
    int id = ler_int("ID do produto: ");
    int qtd = ler_int("Quantidade de ENTRADA: ");
    if (qtd <= 0) { printf("Quantidade deve ser > 0.\n"); return; }

    ProdutoNode *n = buscar_produto_por_id(ponta, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    Operacao op = { OP_ENTRADA, n->p, qtd };
    pilha_push(undo, op);

    n->p.qtd += qtd;

    char msg[180];
    snprintf(msg, sizeof(msg), "ENTRADA: id=%d | +%d | nova_qtd=%d", id, qtd, n->p.qtd);
    historico_add(msg);

    printf("Entrada aplicada.\n");
}

static void saida_estoque(ProdutoNode *ponta, Pilha *undo) {
    int id = ler_int("ID do produto: ");
    int qtd = ler_int("Quantidade de SAIDA: ");
    if (qtd <= 0) { printf("Quantidade deve ser > 0.\n"); return; }

    ProdutoNode *n = buscar_produto_por_id(ponta, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    if (n->p.qtd < qtd) {
        printf("Estoque insuficiente. Qtd atual: %d\n", n->p.qtd);
        return;
    }

    Operacao op = { OP_SAIDA, n->p, -qtd };
    pilha_push(undo, op);

    n->p.qtd -= qtd;

    char msg[180];
    snprintf(msg, sizeof(msg), "SAIDA: id=%d | -%d | nova_qtd=%d", id, qtd, n->p.qtd);
    historico_add(msg);

    printf("Saida aplicada.\n");
}

static void atualizar_preco(ProdutoNode *ponta, Pilha *undo) {
    int id = ler_int("ID do produto: ");
    double novo = ler_double("Novo preco: ");
    if (novo < 0) { printf("Preco invalido.\n"); return; }

    ProdutoNode *n = buscar_produto_por_id(ponta, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    Operacao op = { OP_ATUALIZA_PRECO, n->p, 0 };
    pilha_push(undo, op);

    n->p.preco = novo;

    char msg[180];
    snprintf(msg, sizeof(msg), "PRECO: id=%d | novo_preco=%.2f", id, novo);
    historico_add(msg);

    printf("Preco atualizado.\n");
}

/* 8) FILA DE PEDIDOS (menu)
   - Cria pedido e coloca na fila
   - Processa o primeiro pedido (FIFO)*/

static void criar_pedido_menu(Fila *f) {
    Pedido p;
    p.produto_id = ler_int("ID do produto: ");
    p.qtd = ler_int("Quantidade: ");
    if (p.produto_id <= 0 || p.qtd <= 0) {
        printf("Pedido invalido.\n");
        return;
    }

    int t = ler_int("Tipo (1=VENDA / 2=REPOSICAO): ");
    p.tipo = (t == 2) ? PED_REPOSICAO : PED_VENDA;

    fila_enqueue(f, p);

    char msg[180];
    snprintf(msg, sizeof(msg), "PEDIDO NA FILA: %s | id=%d | qtd=%d",
             (p.tipo == PED_VENDA ? "VENDA" : "REPOSICAO"),
             p.produto_id, p.qtd);
    historico_add(msg);

    printf("Pedido enfileirado. Total na fila: %d\n", f->tamanho);
}

static void listar_fila(const Fila *f) {
    printf("\n=== FILA DE PEDIDOS (FIFO) ===\n");
    if (fila_vazia(f)) {
        printf("(vazia)\n");
        return;
    }

    const NoFila *cur = f->inicio;
    int i = 1;
    while (cur) {
        printf("%2d) %s | id=%d | qtd=%d\n",
               i,
               (cur->ped.tipo == PED_VENDA ? "VENDA" : "REPOSICAO"),
               cur->ped.produto_id,
               cur->ped.qtd);
        cur = cur->next;
        i++;
    }
}

static void processar_proximo_pedido(Fila *f, ProdutoNode *ponta, Pilha *undo) {
    Pedido p;
    if (!fila_dequeue(f, &p)) {
        printf("Fila vazia.\n");
        return;
    }

    ProdutoNode *n = buscar_produto_por_id(ponta, p.produto_id);
    if (!n) {
        printf("Pedido descartado: produto id=%d nao existe.\n", p.produto_id);
        return;
    }

    if (p.tipo == PED_VENDA) {
        if (n->p.qtd < p.qtd) {
            printf("Venda NAO processada: estoque insuficiente (atual=%d).\n", n->p.qtd);
            return;
        }

        Operacao op = { OP_SAIDA, n->p, -p.qtd };
        pilha_push(undo, op);

        n->p.qtd -= p.qtd;

        char msg[180];
        snprintf(msg, sizeof(msg), "PROCESSADO: VENDA | id=%d | -%d | nova_qtd=%d",
                 p.produto_id, p.qtd, n->p.qtd);
        historico_add(msg);

        printf("Pedido de VENDA processado.\n");
    } else {
        Operacao op = { OP_ENTRADA, n->p, p.qtd };
        pilha_push(undo, op);

        n->p.qtd += p.qtd;

        char msg[180];
        snprintf(msg, sizeof(msg), "PROCESSADO: REPOSICAO | id=%d | +%d | nova_qtd=%d",
                 p.produto_id, p.qtd, n->p.qtd);
        historico_add(msg);

        printf("Pedido de REPOSICAO processado.\n");
    }
}

/* 9) UNDO (desfazer)
   - POP na pilha
   - Reverte conforme o tipo da operação */

static void desfazer_ultima(ProdutoNode **ponta, Pilha *undo) {
    Operacao op;
    if (!pilha_pop(undo, &op)) {
        printf("Nada para desfazer.\n");
        return;
    }

    /*
       - CADASTRO: desfazer = remover o produto cadastrado
       - REMOCAO: desfazer = reinserir o produto removido
       - ENTRADA/SAIDA/PRECO: desfazer = restaurar snapshot (estado anterior)
    */

    if (op.tipo == OP_CADASTRO) {
        Produto lixo;
        if (remover_produto_por_id(ponta, op.snapshot.id, &lixo)) {
            char msg[180];
            snprintf(msg, sizeof(msg), "UNDO: desfez CADASTRO (removeu id=%d)", op.snapshot.id);
            historico_add(msg);
            printf("Desfeito: cadastro removido.\n");
        } else {
            printf("UNDO falhou: produto nao encontrado.\n");
        }
        return;
    }

    if (op.tipo == OP_REMOCAO) {
        if (inserir_produto_o1(ponta, op.snapshot)) {
            char msg[180];
            snprintf(msg, sizeof(msg), "UNDO: desfez REMOCAO (recolocou id=%d)", op.snapshot.id);
            historico_add(msg);
            printf("Desfeito: produto reinserido.\n");
        } else {
            printf("UNDO falhou: nao foi possivel reinserir (ID duplicado?).\n");
        }
        return;
    }

    ProdutoNode *n = buscar_produto_por_id(*ponta, op.snapshot.id);
    if (!n) {
        printf("UNDO falhou: produto nao encontrado.\n");
        return;
    }

    n->p = op.snapshot;

    char msg[180];
    snprintf(msg, sizeof(msg),
             "UNDO: restaurou estado (id=%d | qtd=%d | preco=%.2f)",
             n->p.id, n->p.qtd, n->p.preco);
    historico_add(msg);

    printf("Desfeito: estado restaurado.\n");
}

/*
   10) MENU
   - Menu de seleção da operação desejada
   - Parte final do código */

static void print_menu(void) {
    printf("\n========== SISTEMA DE ESTOQUE ==========\n");
    printf("1) Cadastrar produto\n");
    printf("2) Listar produtos (ordenado por ID)\n");
    printf("3) Buscar produto por ID\n");
    printf("4) Remover produto\n");
    printf("5) Entrada de estoque\n");
    printf("6) Saida de estoque\n");
    printf("7) Atualizar preco\n");
    printf("8) Criar pedido (entra na FILA)\n");
    printf("9) Listar fila de pedidos\n");
    printf("10) Processar proximo pedido (FIFO)\n");
    printf("11) Desfazer ultima operacao (PILHA/UNDO)\n");
    printf("12) Ver historico (ARRAY)\n");
    printf("0) Sair\n");
}

/* 11) MAIN
   - Inicializa estruturas
   - Loop do menu
   - Limpa malloc/free ao sair */

int main(void) {
    ProdutoNode *ponta = NULL;  // era "estoque/head"
    Pilha undo;
    Fila fila;

    pilha_init(&undo);
    fila_init(&fila);

    int opc;
    do {
        print_menu();
        opc = ler_int("Escolha: ");

        switch (opc) {
            case 1:  cadastrar_produto(&ponta, &undo); break;
            case 2:  listar_estoque_ordenado(ponta); break;
            case 3:  buscar_produto_menu(ponta); break;
            case 4:  remover_produto_menu(&ponta, &undo); break;
            case 5:  entrada_estoque(ponta, &undo); break;
            case 6:  saida_estoque(ponta, &undo); break;
            case 7:  atualizar_preco(ponta, &undo); break;
            case 8:  criar_pedido_menu(&fila); break;
            case 9:  listar_fila(&fila); break;
            case 10: processar_proximo_pedido(&fila, ponta, &undo); break;
            case 11: desfazer_ultima(&ponta, &undo); break;
            case 12: historico_print(); break;
            case 0:  break;
            default: printf("Opcao invalida.\n"); break;
        }
    } while (opc != 0);

    /* Limpeza final:
       - liberar lista do estoque
       - limpar pilha do undo
       - limpar fila de pedidos */
    liberar_estoque(&ponta);
    pilha_clear(&undo);
    fila_clear(&fila);

    printf("Encerrado.\n");
    return 0;
}
