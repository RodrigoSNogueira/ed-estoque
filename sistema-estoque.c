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

ProdutoNode *estoque = NULL;

ProdutoNode* criar_no(Produto p){
    ProdutoNode *novo = (ProdutoNode*) malloc(sizeof(ProdutoNode));
    if(!novo){
        printf("Erro de memoria!\n");
        exit(1);
    }
    novo->p = p;
    novo->next = NULL;
    return novo;
}

void inserir_produto(Produto p){
    ProdutoNode *novo = criar_no(p);
    novo->next = estoque;
    estoque = novo;
}

ProdutoNode* buscar_produto(int id){
    ProdutoNode *aux = estoque;
    while(aux){
        if(aux->p.id == id)
            return aux;
        aux = aux->next;
    }
    return NULL;
}

void listar_estoque(){
    ProdutoNode *aux = estoque;
    if(!aux){
        printf("Estoque vazio.\n");
        return;
    }

    while(aux){
        printf("ID: %d | Nome: %s | Qtd: %d | Preco: %.2lf\n",
               aux->p.id,
               aux->p.nome,
               aux->p.qtd,
               aux->p.preco);
        aux = aux->next;
    }
}
