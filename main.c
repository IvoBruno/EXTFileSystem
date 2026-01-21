#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ========================================
// CONSTANTES DO SISTEMA
// ========================================
#define TAMANHO_DISCO 100        // Número total de blocos no disco simulado
#define TAMANHO_BLOCO 512        // Tamanho de cada bloco em bytes
#define MAX_INODES 50            // Número máximo de inodes (arquivos/diretórios)
#define MAX_NOME 50              // Tamanho máximo do nome do arquivo
#define MAX_PONTEIROS 10         // Número máximo de blocos que um arquivo pode usar

// ========================================
// ESTRUTURA 1: BLOCO DE DADOS
// ========================================
// Representa um bloco físico do disco
// Cada bloco pode armazenar até TAMANHO_BLOCO bytes de dados
typedef struct {
    char dados[TAMANHO_BLOCO];   // Conteúdo do bloco
    int ocupado;                  // 0 = livre, 1 = ocupado
} Bloco;

// ========================================
// ESTRUTURA 2: INODE
// ========================================
// No EXT, cada arquivo/diretório tem um inode
// O inode contém METADADOS (informações sobre o arquivo)
// mas NÃO contém o conteúdo em si
typedef struct {
    char nome[MAX_NOME];               // Nome do arquivo/diretório
    int tamanho;                       // Tamanho do arquivo em bytes
    int tipo;                          // 0 = arquivo, 1 = diretório
    int ponteiros[MAX_PONTEIROS];      // Índices dos blocos onde o conteúdo está armazenado
    int num_blocos;                    // Quantos blocos o arquivo está usando
    char permissoes[10];               // Permissões (ex: "rwxr-xr-x")
    time_t data_criacao;               // Data de criação
    int ativo;                         // 0 = inode livre, 1 = inode em uso
} Inode;

// ========================================
// ESTRUTURA 3: SISTEMA DE ARQUIVOS
// ========================================
// Esta estrutura representa o sistema de arquivos completo
typedef struct {
    Bloco disco[TAMANHO_DISCO];       // Array de blocos (nosso "disco virtual")
    Inode inodes[MAX_INODES];         // Array de inodes
    int bitmap[TAMANHO_DISCO];        // Bitmap: 0 = bloco livre, 1 = bloco ocupado
    int proximo_inode_livre;          // Índice do próximo inode disponível
} SistemaArquivos;

// ========================================
// VARIÁVEL GLOBAL
// ========================================
SistemaArquivos fs;  // Nosso sistema de arquivos

// ========================================
// PROTÓTIPOS DAS FUNÇÕES
// ========================================
void inicializar_sistema();
void exibir_menu();
void criar_arquivo();
void escrever_arquivo();
void ler_arquivo();
void excluir_arquivo();
void listar_arquivos();
void mostrar_estado_disco();

// ========================================
// FUNÇÃO PRINCIPAL
// ========================================
int main() {
    int opcao;
    
    // Inicializa o sistema de arquivos
    inicializar_sistema();
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════╗\n");
    printf("║     SISTEMA DE ARQUIVOS EXT - SIMULAÇÃO                ║\n");
    printf("║     Trabalho de Sistemas Operacionais                  ║\n");
    printf("╚════════════════════════════════════════════════════════╝\n");
    
    // Loop principal do programa
    do {
        exibir_menu();
        printf("\nEscolha uma opção: ");
        scanf("%d", &opcao);
        getchar(); // Limpa o buffer do teclado
        
        switch(opcao) {
            case 1:
                criar_arquivo();
                break;
            case 2:
                escrever_arquivo();
                break;
            case 3:
                ler_arquivo();
                break;
            case 4:
                excluir_arquivo();
                break;
            case 5:
                listar_arquivos();
                break;
            case 6:
                mostrar_estado_disco();
                break;
            case 0:
                printf("\n✓ Encerrando o sistema...\n\n");
                break;
            default:
                printf("\n✗ Opção inválida!\n");
        }
        
        if (opcao != 0) {
            printf("\nPressione ENTER para continuar...");
            getchar();
        }
        
    } while(opcao != 0);
    
    return 0;
}

// ========================================
// IMPLEMENTAÇÃO: Inicializar Sistema
// ========================================
void inicializar_sistema() {
    int i;
    
    // Inicializa todos os blocos do disco como livres
    for (i = 0; i < TAMANHO_DISCO; i++) {
        fs.disco[i].ocupado = 0;
        fs.bitmap[i] = 0;  // 0 = bloco livre
        memset(fs.disco[i].dados, 0, TAMANHO_BLOCO);
    }
    
    // Inicializa todos os inodes como livres
    for (i = 0; i < MAX_INODES; i++) {
        fs.inodes[i].ativo = 0;  // 0 = inode livre
        fs.inodes[i].tamanho = 0;
        fs.inodes[i].num_blocos = 0;
        strcpy(fs.inodes[i].nome, "");
    }
    
    fs.proximo_inode_livre = 0;
    
    printf("\n✓ Sistema de arquivos inicializado com sucesso!\n");
    printf("  - Blocos disponíveis: %d\n", TAMANHO_DISCO);
    printf("  - Inodes disponíveis: %d\n", MAX_INODES);
}

// ========================================
// IMPLEMENTAÇÃO: Exibir Menu
// ========================================
void exibir_menu() {
    printf("\n");
    printf("┌────────────────────────────────────────┐\n");
    printf("│           MENU PRINCIPAL               │\n");
    printf("├────────────────────────────────────────┤\n");
    printf("│ 1. Criar arquivo                       │\n");
    printf("│ 2. Escrever em arquivo                 │\n");
    printf("│ 3. Ler arquivo                         │\n");
    printf("│ 4. Excluir arquivo                     │\n");
    printf("│ 5. Listar arquivos                     │\n");
    printf("│ 6. Mostrar estado do disco             │\n");
    printf("│ 0. Sair                                │\n");
    printf("└────────────────────────────────────────┘\n");
}

// ========================================
// FUNÇÃO AUXILIAR: Buscar Inode por Nome
// ========================================
int buscar_inode(char *nome) {
    int i;
    for (i = 0; i < MAX_INODES; i++) {
        if (fs.inodes[i].ativo == 1 && strcmp(fs.inodes[i].nome, nome) == 0) {
            return i;  // Retorna o índice do inode encontrado
        }
    }
    return -1;  // Retorna -1 se não encontrar
}

// ========================================
// FUNÇÃO AUXILIAR: Encontrar Bloco Livre
// ========================================
int encontrar_bloco_livre() {
    int i;
    for (i = 0; i < TAMANHO_DISCO; i++) {
        if (fs.bitmap[i] == 0) {  // Bloco livre
            return i;
        }
    }
    return -1;  // Disco cheio
}

// ========================================
// IMPLEMENTAÇÃO: Criar Arquivo
// ========================================
void criar_arquivo() {
    char nome[MAX_NOME];
    int i;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         CRIAR NOVO ARQUIVO             ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Verifica se há inodes disponíveis
    if (fs.proximo_inode_livre >= MAX_INODES) {
        printf("✗ Erro: Número máximo de arquivos atingido!\n");
        return;
    }
    
    printf("Digite o nome do arquivo: ");
    fgets(nome, MAX_NOME, stdin);
    nome[strcspn(nome, "\n")] = 0;  // Remove o \n
    
    // Verifica se o arquivo já existe
    if (buscar_inode(nome) != -1) {
        printf("✗ Erro: Arquivo '%s' já existe!\n", nome);
        return;
    }
    
    // Encontra um inode livre
    for (i = 0; i < MAX_INODES; i++) {
        if (fs.inodes[i].ativo == 0) {
            // Configura o novo inode
            strcpy(fs.inodes[i].nome, nome);
            fs.inodes[i].tamanho = 0;
            fs.inodes[i].tipo = 0;  // 0 = arquivo
            fs.inodes[i].num_blocos = 0;
            strcpy(fs.inodes[i].permissoes, "rw-r--r--");
            fs.inodes[i].data_criacao = time(NULL);
            fs.inodes[i].ativo = 1;  // Marca como ativo
            
            // Inicializa os ponteiros
            int j;
            for (j = 0; j < MAX_PONTEIROS; j++) {
                fs.inodes[i].ponteiros[j] = -1;
            }
            
            fs.proximo_inode_livre++;
            
            printf("\n✓ Arquivo '%s' criado com sucesso!\n", nome);
            printf("  - Inode #%d alocado\n", i);
            printf("  - Permissões: %s\n", fs.inodes[i].permissoes);
            return;
        }
    }
    
    printf("✗ Erro ao criar arquivo!\n");
}

// ========================================
// IMPLEMENTAÇÃO: Escrever em Arquivo
// ========================================
void escrever_arquivo() {
    char nome[MAX_NOME];
    char conteudo[TAMANHO_BLOCO];
    int inode_idx, bloco_idx;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║       ESCREVER EM ARQUIVO              ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    printf("Digite o nome do arquivo: ");
    fgets(nome, MAX_NOME, stdin);
    nome[strcspn(nome, "\n")] = 0;
    
    // Busca o inode do arquivo
    inode_idx = buscar_inode(nome);
    if (inode_idx == -1) {
        printf("✗ Erro: Arquivo '%s' não encontrado!\n", nome);
        return;
    }
    
    printf("Digite o conteúdo (máx %d caracteres):\n", TAMANHO_BLOCO - 1);
    fgets(conteudo, TAMANHO_BLOCO, stdin);
    conteudo[strcspn(conteudo, "\n")] = 0;
    
    // Encontra um bloco livre
    bloco_idx = encontrar_bloco_livre();
    if (bloco_idx == -1) {
        printf("✗ Erro: Disco cheio!\n");
        return;
    }
    
    // Escreve o conteúdo no bloco
    strcpy(fs.disco[bloco_idx].dados, conteudo);
    fs.disco[bloco_idx].ocupado = 1;
    fs.bitmap[bloco_idx] = 1;
    
    // Atualiza o inode
    fs.inodes[inode_idx].ponteiros[0] = bloco_idx;
    fs.inodes[inode_idx].num_blocos = 1;    
    fs.inodes[inode_idx].tamanho = strlen(conteudo);
    
    printf("\n✓ Conteúdo escrito com sucesso!\n");
    printf("  - Bytes escritos: %d\n", fs.inodes[inode_idx].tamanho);
    printf("  - Bloco utilizado: #%d\n", bloco_idx);
}

// ========================================
// IMPLEMENTAÇÃO: Ler Arquivo
// ========================================
void ler_arquivo() {
    char nome[MAX_NOME];
    int inode_idx, bloco_idx;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║           LER ARQUIVO                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    printf("Digite o nome do arquivo: ");
    fgets(nome, MAX_NOME, stdin);
    nome[strcspn(nome, "\n")] = 0;
    
    // Busca o inode
    inode_idx = buscar_inode(nome);
    if (inode_idx == -1) {
        printf("✗ Erro: Arquivo '%s' não encontrado!\n", nome);
        return;
    }
    
    // Verifica se o arquivo tem conteúdo
    if (fs.inodes[inode_idx].tamanho == 0) {
        printf("⚠ Arquivo vazio!\n");
        return;
    }
    
    // Lê o conteúdo do primeiro bloco
    bloco_idx = fs.inodes[inode_idx].ponteiros[0];
    
    printf("\n┌────────────────────────────────────────┐\n");
    printf("│ Conteúdo do arquivo '%s':\n", nome);
    printf("├────────────────────────────────────────┤\n");
    printf("|%s\n", fs.disco[bloco_idx].dados);
    printf("└────────────────────────────────────────┘\n");
    printf("\nInformações:\n");
    printf("  - Tamanho: %d bytes\n", fs.inodes[inode_idx].tamanho);
    printf("  - Blocos utilizados: %d\n", fs.inodes[inode_idx].num_blocos);
    printf("  - Permissões: %s\n", fs.inodes[inode_idx].permissoes);
}

// ========================================
// IMPLEMENTAÇÃO: Excluir Arquivo
// ========================================
void excluir_arquivo() {
    char nome[MAX_NOME];
    int inode_idx, i;
    char confirmacao;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║         EXCLUIR ARQUIVO                ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    printf("Digite o nome do arquivo: ");
    fgets(nome, MAX_NOME, stdin);
    nome[strcspn(nome, "\n")] = 0;
    
    // Busca o inode
    inode_idx = buscar_inode(nome);
    if (inode_idx == -1) {
        printf("✗ Erro: Arquivo '%s' não encontrado!\n", nome);
        return;
    }
    
    printf("⚠ Tem certeza que deseja excluir '%s'? (s/n): ", nome);
    scanf(" %c", &confirmacao);
    getchar();  // Limpa buffer
    
    if (confirmacao != 's' && confirmacao != 'S') {
        printf("✓ Operação cancelada.\n");
        return;
    }
    
    // Libera os blocos ocupados pelo arquivo
    for (i = 0; i < fs.inodes[inode_idx].num_blocos; i++) {
        int bloco = fs.inodes[inode_idx].ponteiros[i];
        if (bloco != -1) {
            fs.disco[bloco].ocupado = 0;
            fs.bitmap[bloco] = 0;
            memset(fs.disco[bloco].dados, 0, TAMANHO_BLOCO);
        }
    }
    
    // Libera o inode
    fs.inodes[inode_idx].ativo = 0;
    fs.inodes[inode_idx].tamanho = 0;
    fs.inodes[inode_idx].num_blocos = 0;
    strcpy(fs.inodes[inode_idx].nome, "");
    
    fs.proximo_inode_livre--;
    
    printf("\n✓ Arquivo '%s' excluído com sucesso!\n", nome);
}

// ========================================
// IMPLEMENTAÇÃO: Listar Arquivos
// ========================================
void listar_arquivos() {
    int i, total = 0;
    
    printf("\n");
    printf("╔════════════════════════════════════════════════════════════════════╗\n");
    printf("║                    ARQUIVOS NO SISTEMA                             ║\n");
    printf("╚════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("┌────────┬──────────────────────┬──────────┬───────────┬────────────┐\n");
    printf("│ Inode  │ Nome                 │ Tamanho  │ Blocos    │ Permissões │\n");
    printf("├────────┼──────────────────────┼──────────┼───────────┼────────────┤\n");
    
    for (i = 0; i < MAX_INODES; i++) {
        if (fs.inodes[i].ativo == 1) {
            printf("│ %-6d │ %-20s │ %-8d │ %-9d │ %-10s │\n",
                   i,
                   fs.inodes[i].nome,
                   fs.inodes[i].tamanho,
                   fs.inodes[i].num_blocos,
                   fs.inodes[i].permissoes);
            total++;
        }
    }
    
    printf("└────────┴──────────────────────┴──────────┴───────────┴────────────┘\n");
    printf("\nTotal de arquivos: %d / %d\n", total, MAX_INODES);
}

// ========================================
// IMPLEMENTAÇÃO: Mostrar Estado do Disco
// ========================================
void mostrar_estado_disco() {
    int i, ocupados = 0, livres = 0;
    
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║       ESTADO DO DISCO                  ║\n");
    printf("╚════════════════════════════════════════╝\n\n");
    
    // Conta blocos ocupados e livres
    for (i = 0; i < TAMANHO_DISCO; i++) {
        if (fs.bitmap[i] == 1) {
            ocupados++;
        } else {
            livres++;
        }
    }
    
    printf("Mapa de blocos (■ = ocupado, □ = livre):\n\n");
    
    // Exibe o mapa visual
    for (i = 0; i < TAMANHO_DISCO; i++) {
        if (fs.bitmap[i] == 1) {
            printf("■ ");
        } else {
            printf("□ ");
        }
        
        // Quebra linha a cada 20 blocos
        if ((i + 1) % 20 == 0) {
            printf("\n");
        }
    }
    
    printf("\n\n");
    printf("┌────────────────────────────────────────┐\n");
    printf("│ Estatísticas do Disco:                 │\n");
    printf("├────────────────────────────────────────┤\n");
    printf("│ Total de blocos:      %-16d │\n", TAMANHO_DISCO);
    printf("│ Blocos ocupados:      %-16d │\n", ocupados);
    printf("│ Blocos livres:        %-16d │\n", livres);
    printf("│ Uso do disco:         %02.1f%%            │\n", (ocupados * 100.0) / TAMANHO_DISCO);
    printf("└────────────────────────────────────────┘\n");
    
    printf("\n");
    printf("┌────────────────────────────────────────┐\n");
    printf("│ Estatísticas de Inodes:                │\n");
    printf("├────────────────────────────────────────┤\n");
    printf("│ Total de inodes:      %-16d │\n", MAX_INODES);
    printf("│ Inodes em uso:        %-16d │\n", fs.proximo_inode_livre);
    printf("│ Inodes livres:        %-16d │\n", MAX_INODES - fs.proximo_inode_livre);
    printf("└────────────────────────────────────────┘\n");
}
