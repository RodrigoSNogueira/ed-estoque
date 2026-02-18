#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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
