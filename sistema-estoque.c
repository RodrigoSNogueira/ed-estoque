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