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
