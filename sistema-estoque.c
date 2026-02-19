#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NAME_MAX 64
#define HIST_MAX 200

//Modeladgem da Struct

typedef struct {
    int id;
    char nome[NAME_MAX];
    int qtd; 
    double preco;
} Produto;

typedef struct ProdutoNode{
    Produto p;
    struct ProdutoNode *next;
} ProdutoNode;

typedef enum {
    OP_CADASTRO = 1,
    OP_REMOCAO = 2,
    OP_ENTRADA = 3,
    OP_SAIDA = 4,
    OP_ATUALIZAR_PRECO = 5
} TipoOperacao;

typedef struct {
    TipoOperacao tipo;
    Produto snapshot;
    int delta_qtd;
} Operacao;

// Estrutura da Pilha 

typedef struct StackNode {
    Operacao op;
    struct StackNode *next;
} StackNode;

typedef struct {
    StackNode *top;
    int tamanho;
} Stack;

static void stack_init (Stack *s) {
    s -> top = NULL;
    s -> tamanho = 0;
}

static int stack_empty (const Stack *s, Operacao op){
    return s -> top == NULL;
}

static void stack_push (Stack *s, Operacao op) {
    StackNode *n = (StackNode*) malloc(sizeof(StackNode));

    if (!n) {
        printf ("Erro: sem memória\n");
        return;
      }
    n -> op = op;
    n -> next = s -> top;
    s -> top = n;
    s -> tamanho++;
}

static int stack_pop (Stack *s, Operacao *out) {
    if (s -> top == NULL)
    return 0;

    StackNode *t = s-> top;
    *out = t -> op;
    s -> top = t-> next;
    free(t);
    s-> tamanho--;
    return 1;
}

static void stack_clear (Stack *s) {
    Operacao tmp;
    while (stack_pop(s, &tmp)) {}
}

//Estrutura da fila - Funcao Pedidos

typedef enum {
    PED_VENDA = 1,
    PED_REPOSICAO = 2
} TipoPedido;

typedef struct {
    int produto_id;
    int qtd;
    TipoPedido tipo;
} Pedido;

typedef struct QueueNode {
    Pedido ped ; 
    struct QueueNode *next;
} QueueNode;

typedef struct {
    QueueNode *inicio;
    QueueNode *fim;
    int tamanho;
} Queue;

static void queue_init (Queue *q) {
    q -> inicio = q -> fim = NULL;
    q -> tamanho = 0;
}

static int queue_empty (const Queue *q) {
    return q-> inicio == NULL;
}

static void enqueue (Queue *q, Pedido p) {
    QueueNode *n = (QueueNode*)malloc(sizeof(QueueNode));

    if (!n) {
        printf("Erro: Sem memória\n");
        return;
    }
    n -> ped = p;
    n -> next = NULL;

    if (q -> fim) q ->fim->next = n;
    else q->inicio = n;
    q -> tamanho++;
}

static int dequeue (Queue *q, Pedido *out) {

    if (queue_empty(q)) 
    return 0;

    QueueNode *f = q-> inicio;
    *out = f -> ped;
    q -> inicio = f -> next;
    if (!q -> inicio)
    q -> fim = NULL;

    free (f);
    q-> tamanho--;
    return 1;
}

static void queue_clear (Queue *q) {
    Pedido tmp; 
    while (dequeue(q, &tmp)) {}

}

// Array para Histórico 

typedef struct {
    char msg [160]
}HistoricoItem;

static HistoricoItem hist[HIST_MAX];
static int hist_count = 0;

static void hist_add (const char *msg) {
    if (hist_count < HIST_MAX){
        strncpy (hist[hist_count].msg, msg, sizeof(hist[hist_count].msg - 1));
        hist[hist_count].msg[sizeof(hist[hist_count].msg)-1] = '\0';
        hist_count++;
    }else {
        for (int i = 1; i < HIST_MAX; i++) hist[i - 1] = hist[i];
        strncpy(hist[HIST_MAX - 1].msg, msg, sizeof(hist[HIST_MAX - 1].msg) - 1);
        hist[HIST_MAX - 1].msg[sizeof(hist[HIST_MAX - 1].msg) - 1] = '\0';
    }
}

static void hist_print(void) {
    printf("\n=== HISTORICO ===\n", hist_count);

    if (hist_count == 0) {
        printf ("Vazio\n");
        return;
    }
    for (int i = 0; i < hist_count; i++) {
        printf("%2d) %s", i + 1, hist[i].msg);
    }
}

// I/O
static void flush_stdin(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
}

static int read_int(const char *prompt) {
    int x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%d", &x) == 1) { flush_stdin(); return x; }
        printf("Entrada invalida.\n");
        flush_stdin();
    }
}

static double read_double(const char *prompt) {
    double x;
    while (1) {
        printf("%s", prompt);
        if (scanf("%lf", &x) == 1) { flush_stdin(); return x; }
        printf("Entrada invalida.\n");
        flush_stdin();
    }
}

static void read_line(const char *prompt, char *out, size_t cap) {
    printf("%s", prompt);
    if (!fgets(out, (int)cap, stdin)) { out[0] = '\0'; return; }
    size_t len = strlen(out);
    if (len && out[len - 1] == '\n') out[len - 1] = '\0';
}

static ProdutoNode criar_no(Produto p) {
    ProdutoNode *novo = (ProdutoNode*)malloc(sizeof(ProdutoNode));
    if (!novo){
        printf("Erro de Memoria\n");
        exit(1);
    }
    novo -> p = p;
    novo ->next = NULL;
    return novo;
}

static ProdutoNode* buscar_produto (ProdutoNode *ponta, int id) {
    ProdutoNode *aux = ponta;
    while (aux){
        if (aux -> p.id == id)
        return aux;
        aux = aux -> next;
    }
    return NULL;
}

static int inserir_produto_sem_duplicar (ProdutoNode **ponta, Produto p) {
    if (!ponta) 
    return 0;

    if (buscar_produto(*ponta, p.id)) {
        printf ("ID %d já existe. Cadastro cancelado\n", p.id);
        return 0;
    }

    ProdutoNode *novo = criar_no(p);
    novo -> next = *ponta
    *ponta = novo;
    return 1;
}

static int remover_produto_por_id(ProdutoNode **ponta, int id, Produto *out_removed) {
    if (!ponta || !*ponta) return 0;

    ProdutoNode *prev = NULL;
    ProdutoNode *cur = *ponta;

    while (cur) {
        if (cur->p.id == id) {
            if (out_removed) *out_removed = cur->p;
            if (prev) prev->next = cur->next;
            else *ponta = cur->next;
            free(cur);
            return 1;
        }
        prev = cur;
        cur = cur->next;
    }
    return 0;
}

static int contar_produtos(ProdutoNode *ponta) {
    int n = 0;
    while (ponta) { n++; ponta = ponta->next; }
    return n;
}

static void insertion_sort_nodes_por_id(ProdutoNode **v, int n) {
    for (int i = 1; i < n; i++) {
        ProdutoNode *key = v[i];
        int j = i - 1;
        while (j >= 0 && v[j]->p.id > key->p.id) {
            v[j + 1] = v[j];
            j--;
        }
        v[j + 1] = key;
    }
}

static void listar_estoque_ordenado(ProdutoNode *ponta) {
    printf("\n=== PRODUTOS NO ESTOQUE (ordenado por ID) ===\n");
    if (!ponta) { printf("(vazio)\n"); return; }

    int n = contar_produtos(ponta);
    ProdutoNode **arr = (ProdutoNode**)malloc(n * sizeof(ProdutoNode*));
    if (!arr) { printf("ERRO: sem memoria (listar)\n"); return; }

    ProdutoNode *aux = ponta;
    for (int i = 0; i < n; i++) {
        arr[i] = aux;
        aux = aux->next;
    }

    insertion_sort_nodes_por_id(arr, n);

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
        ProdutoNode *n = cur->next;
        free(cur);
        cur = n;
    }
    *ponta = NULL;
}

// Operações do Sistema 

static void cadastrar_produto(ProdutoNode **head, Stack *undo) {
    Produto p;
    p.id = read_int("ID do produto: ");
    read_line("Nome: ", p.nome, sizeof(p.nome));
    p.qtd = read_int("Quantidade inicial: ");
    p.preco = read_double("Preco (ex: 19.90): ");

    if (p.id <= 0 || p.qtd < 0 || p.preco < 0 || strlen(p.nome) == 0) {
        printf("Dados invalidos.\n");
        return;
    }

    if (!inserir_produto_o1_sem_duplicado(head, p)) return;

    Operacao op = { OP_CADASTRO, p, 0 };
    stack_push(undo, op);

    char msg[160];
    snprintf(msg, sizeof(msg), "CADASTRO: id=%d nome=%s qtd=%d preco=%.2f", p.id, p.nome, p.qtd, p.preco);
    hist_add(msg);

    printf("Produto cadastrado.\n");
}

static void remover_produto_menu(ProdutoNode **head, Stack *undo) {
    int id = read_int("ID do produto para remover: ");
    Produto removed;

    if (!remover_produto_por_id(head, id, &removed)) {
        printf("Produto nao encontrado.\n");
        return;
    }

    Operacao op = { OP_REMOCAO, removed, 0 };
    stack_push(undo, op);

    char msg[160];
    snprintf(msg, sizeof(msg), "REMOCAO: id=%d nome=%s", removed.id, removed.nome);
    hist_add(msg);

    printf("Produto removido.\n");
}

static void buscar_produto_menu(ProdutoNode *head) {
    int id = read_int("ID para buscar: ");
    ProdutoNode *n = buscar_produto(head, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    printf("\nEncontrado:\n");
    printf("ID: %d\nNome: %s\nQuantidade: %d\nPreco: R$ %.2f\n",
           n->p.id, n->p.nome, n->p.qtd, n->p.preco);
}

static void entrada_estoque(ProdutoNode *head, Stack *undo) {
    int id = read_int("ID do produto: ");
    int qtd = read_int("Quantidade de ENTRADA: ");
    if (qtd <= 0) { printf("Quantidade deve ser > 0.\n"); return; }

    ProdutoNode *n = buscar_produto(head, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    Operacao op = { OP_ENTRADA, n->p, qtd };
    stack_push(undo, op);

    n->p.qtd += qtd;

    char msg[160];
    snprintf(msg, sizeof(msg), "ENTRADA: id=%d +%d (qtd=%d)", id, qtd, n->p.qtd);
    hist_add(msg);

    printf("Entrada aplicada.\n");
}

static void saida_estoque(ProdutoNode *head, Stack *undo) {
    int id = read_int("ID do produto: ");
    int qtd = read_int("Quantidade de SAIDA: ");
    if (qtd <= 0) { printf("Quantidade deve ser > 0.\n"); return; }

    ProdutoNode *n = buscar_produto(head, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    if (n->p.qtd < qtd) {
        printf("Estoque insuficiente. Qtd atual: %d\n", n->p.qtd);
        return;
    }

    Operacao op = { OP_SAIDA, n->p, -qtd };
    stack_push(undo, op);

    n->p.qtd -= qtd;

    char msg[160];
    snprintf(msg, sizeof(msg), "SAIDA: id=%d -%d (qtd=%d)", id, qtd, n->p.qtd);
    hist_add(msg);

    printf("Saida aplicada.\n");
}

static void atualizar_preco(ProdutoNode *head, Stack *undo) {
    int id = read_int("ID do produto: ");
    double novo = read_double("Novo preco: ");
    if (novo < 0) { printf("Preco invalido.\n"); return; }

    ProdutoNode *n = buscar_produto(head, id);
    if (!n) { printf("Produto nao encontrado.\n"); return; }

    Operacao op = { OP_ATUALIZA_PRECO, n->p, 0 };
    stack_push(undo, op);

    n->p.preco = novo;

    char msg[160];
    snprintf(msg, sizeof(msg), "PRECO: id=%d atualizado para %.2f", id, novo);
    hist_add(msg);

    printf("Preco atualizado.\n");
}


// Estrutura da Fila para pedidos

static void criar_pedido_menu(Queue *q) {
    Pedido p;
    p.produto_id = read_int("ID do produto: ");
    p.qtd = read_int("Quantidade: ");
    if (p.produto_id <= 0 || p.qtd <= 0) { printf("Pedido invalido.\n"); return; }

    int t = read_int("Tipo (1=VENDA / 2=REPOSICAO): ");
    p.tipo = (t == 2) ? PED_REPOSICAO : PED_VENDA;

    enqueue(q, p);

    char msg[160];
    snprintf(msg, sizeof(msg), "PEDIDO ENFILEIRADO: %s id=%d qtd=%d",
             (p.tipo == PED_VENDA ? "VENDA" : "REPOSICAO"),
             p.produto_id, p.qtd);
    hist_add(msg);

    printf("Pedido adicionado na fila. Tamanho: %d\n", q->size);
}

static void listar_fila(const Queue *q) {
    printf("\n=== FILA DE PEDIDOS (FIFO) ===\n");
    if (queue_empty(q)) { printf("(vazia)\n"); return; }

    const QueueNode *cur = q->front;
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

static void processar_proximo_pedido(Queue *q, ProdutoNode *head, Stack *undo) {
    Pedido p;
    if (!dequeue(q, &p)) { printf("Fila vazia.\n"); return; }

    ProdutoNode *n = buscar_produto(head, p.produto_id);
    if (!n) {
        printf("Pedido descartado: produto id=%d nao existe.\n", p.produto_id);
        return;
    }

    if (p.tipo == PED_VENDA) {
        if (n->p.qtd < p.qtd) {
            printf("Venda NAO processada (estoque insuficiente). Qtd atual: %d\n", n->p.qtd);
            return;
        }
        Operacao op = { OP_SAIDA, n->p, -p.qtd };
        stack_push(undo, op);

        n->p.qtd -= p.qtd;

        char msg[160];
        snprintf(msg, sizeof(msg), "PROCESSADO (VENDA): id=%d -%d (qtd=%d)", p.produto_id, p.qtd, n->p.qtd);
        hist_add(msg);

        printf("Pedido de VENDA processado.\n");
    } else {
        Operacao op = { OP_ENTRADA, n->p, p.qtd };
        stack_push(undo, op);

        n->p.qtd += p.qtd;

        char msg[160];
        snprintf(msg, sizeof(msg), "PROCESSADO (REPOSICAO): id=%d +%d (qtd=%d)", p.produto_id, p.qtd, n->p.qtd);
        hist_add(msg);

        printf("Pedido de REPOSICAO processado.\n");
    }
}

static void desfazer_ultima(ProdutoNode **head, Stack *undo) {
    Operacao op;
    if (!stack_pop(undo, &op)) { printf("Nada para desfazer.\n"); return; }

    if (op.tipo == OP_CADASTRO) {
        Produto tmp;
        if (remover_produto_por_id(head, op.snapshot.id, &tmp)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "UNDO: desfez CADASTRO (removeu id=%d)", op.snapshot.id);
            hist_add(msg);
            printf("Desfeito: cadastro removido.\n");
        } else {
            printf("UNDO falhou: produto nao encontrado.\n");
        }
        return;
    }

    if (op.tipo == OP_REMOCAO) {
        // Reinsere como era, sem duplicado (mas aqui ID já deveria estar livre)
        if (inserir_produto_o1_sem_duplicado(head, op.snapshot)) {
            char msg[160];
            snprintf(msg, sizeof(msg), "UNDO: desfez REMOCAO (reinsert id=%d)", op.snapshot.id);
            hist_add(msg);
            printf("Desfeito: produto reinserido.\n");
        } else {
            printf("UNDO falhou: nao reinseriu (ID duplicado?).\n");
        }
        return;
    }

    // Movimentações / preço: restaura snapshot
    ProdutoNode *n = buscar_produto(*head, op.snapshot.id);
    if (!n) { printf("UNDO falhou: produto nao encontrado.\n"); return; }

    n->p = op.snapshot;

    char msg[160];
    snprintf(msg, sizeof(msg), "UNDO: restaurou estado do produto id=%d (qtd=%d preco=%.2f)",
             n->p.id, n->p.qtd, n->p.preco);
    hist_add(msg);

    printf("Desfeito: estado restaurado.\n");
}


// Estrutura do Menu de Operações 

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

int main(void) {
    ProdutoNode *estoque = NULL;
    Stack undo;
    Queue fila;

    stack_init(&undo);
    queue_init(&fila);

    int opc;
    do {
        print_menu();
        opc = read_int("Escolha: ");

        switch (opc) {
            case 1:  cadastrar_produto(&estoque, &undo); break;
            case 2:  listar_estoque_ordenado(estoque); break;
            case 3:  buscar_produto_menu(estoque); break;
            case 4:  remover_produto_menu(&estoque, &undo); break;
            case 5:  entrada_estoque(estoque, &undo); break;
            case 6:  saida_estoque(estoque, &undo); break;
            case 7:  atualizar_preco(estoque, &undo); break;
            case 8:  criar_pedido_menu(&fila); break;
            case 9:  listar_fila(&fila); break;
            case 10: processar_proximo_pedido(&fila, estoque, &undo); break;
            case 11: desfazer_ultima(&estoque, &undo); break;
            case 12: hist_print(); break;
            case 0:  break;
            default: printf("Opcao invalida.\n"); break;
        }
    } while (opc != 0);

    // limpeza (malloc/free)
    liberar_estoque(&estoque);
    stack_clear(&undo);
    queue_clear(&fila);

    printf("Encerrado.\n");
    return 0;
}