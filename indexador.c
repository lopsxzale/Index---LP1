/*
 Indexador - LPI 2026.1
 Tabela Hash com encadeamento para indexacao de arquivos .txt

 Compile: gcc -o indexador indexador.c
 Uso:     ./indexador <caminho_da_pasta>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

//   ==========================================================
//                          Define's
//   ==========================================================

#define TAM_TABELA  10007 // Numero primo grande para evitar colisoes
#define MIN_CHARS     5   // Termos com menos de 5 chars nao sao indexados

//   ==========================================================
//                           Structs
//   ==========================================================

typedef struct ocorrencia {
    char nome_arquivo[71];
    long offset;                  // posicao exata no arquivo
    struct ocorrencia *prox;
} Ocorrencia;

typedef struct {
    char termo[51];
    Ocorrencia *lista;            // lista encadeada de ocorrencias
} RegistroIndice;

// No da tabela hash (para tratar colisoes por encadeamento)
typedef struct no {
    RegistroIndice registro;
    struct no *prox;
} No;

// Tabela hash: vetor de ponteiros para nos
No *tabela[TAM_TABELA];

//   ============================================================
//                     1. MOTOR DE DISPERSAO
//   ============================================================

// Funcao hash: soma dos caracteres mod tamanho da tabela
int hash(const char *termo)
{
    int soma = 0;
    for (int i = 0; termo[i]; i++)
        soma += (unsigned char)termo[i];
    return soma % TAM_TABELA;
}

// Busca um termo na tabela. Retorna o RegistroIndice ou NULL.
RegistroIndice *buscar(const char *termo)
{
    int pos = hash(termo);
    No *atual = tabela[pos];
    while (atual) {
        if (strcmp(atual->registro.termo, termo) == 0)
            return &atual->registro;
        atual = atual->prox;
    }
    return NULL;
}

/* Insere uma ocorrencia na tabela.
   Se o termo nao existe, cria um novo no. Se ja existe, apenas
   adiciona a ocorrencia na lista encadeada. */
void inserir(const char *termo, const char *arquivo, long offset)
{
    int pos = hash(termo);

    // Procura se o termo ja existe no bucket
    No *atual = tabela[pos];
    while (atual) {
        if (strcmp(atual->registro.termo, termo) == 0)
            break;
        atual = atual->prox;
    }

    // Termo nao existe: cria novo no e encadeia no bucket
    if (!atual) {
        atual = malloc(sizeof(No));
        strcpy(atual->registro.termo, termo);
        atual->registro.lista = NULL;
        atual->prox = tabela[pos];   // encadeia na frente
        tabela[pos] = atual;
    }

    // "Pendura" nova ocorrencia na lista do termo
    Ocorrencia *oc = malloc(sizeof(Ocorrencia));
    strcpy(oc->nome_arquivo, arquivo);
    oc->offset = offset;
    oc->prox = atual->registro.lista;
    atual->registro.lista = oc;
}

//  ===========================================================
//                  2. SCANNER DE ARQUIVOS
//  ===========================================================

// Le o arquivo palavra por palavra e popula a tabela hash
void indexar_arquivo(const char *caminho, const char *nome_arquivo)
{
    FILE *fp = fopen(caminho, "r");
    if (!fp) { perror(caminho); return; }

    char palavra[52];    // buffer para a palavra lida
    int i = 0;
    int c;
    long offset_inicio = 0;

    while (1) {
        long pos = ftell(fp);   // posicao atual antes de ler o char
        c = fgetc(fp);

        if (c == EOF) {
            if (i > 0) { palavra[i] = '\0'; goto processa; }
            break;
        }

        if (isalpha(c)) {
            if (i == 0) offset_inicio = pos;   // marca inicio da palavra
            if (i < 51) palavra[i++] = tolower(c);
        } else {
            if (i > 0) {
                palavra[i] = '\0';
processa:
                // So indexa termos com 5 ou mais caracteres
                if (i >= MIN_CHARS)
                    inserir(palavra, nome_arquivo, offset_inicio);
                i = 0;
            }
        }
    }

    fclose(fp);
}

// Percorre a pasta e indexa todos os arquivos .txt encontrados
void indexar_pasta(const char *pasta)
{
    DIR *d = opendir(pasta);
    if (!d) { perror(pasta); exit(1); }

    struct dirent *entrada;
    char caminho[512];

    while ((entrada = readdir(d)) != NULL) {
        // Verifica se termina em .txt
        char *ponto = strrchr(entrada->d_name, '.');
        if (!ponto || strcmp(ponto, ".txt") != 0) continue;

        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, entrada->d_name);
        printf("  Indexando: %s\n", entrada->d_name);
        indexar_arquivo(caminho, entrada->d_name);
    }

    closedir(d);
}

//   ============================================================
//                      3. RECUPERACAO RAPIDA
//   ============================================================

// Abre o arquivo, vai ate o offset e exibe 50 chars de contexto
void exibir_contexto(const char *pasta, const char *arquivo, long offset)
{
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s", pasta, arquivo);

    FILE *fp = fopen(caminho, "r");
    if (!fp) { printf("  [erro ao abrir %s]\n", arquivo); return; }

    // Recua um pouco para mostrar contexto antes do termo
    long inicio = offset > 20 ? offset - 20 : 0;
    fseek(fp, inicio, SEEK_SET);

    // Le 50 caracteres de contexto
    char contexto[51];
    int lido = 0;
    int c;
    while (lido < 50 && (c = fgetc(fp)) != EOF) {
        if (c == '\n' || c == '\r') c = ' ';
        contexto[lido++] = c;
    }
    contexto[lido] = '\0';

    printf("  %s - \"...%s...\"\n", arquivo, contexto);
    fclose(fp);
}

// Busca linear nos arquivos (para termos com menos de 5 chars)
void busca_linear(const char *pasta, const char *termo)
{
    DIR *d = opendir(pasta);
    if (!d) return;

    struct dirent *entrada;
    char caminho[512], linha[1024];
    int encontrados = 0, pagina = 0;

    while ((entrada = readdir(d)) != NULL) {
        char *ponto = strrchr(entrada->d_name, '.');
        if (!ponto || strcmp(ponto, ".txt") != 0) continue;

        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, entrada->d_name);
        FILE *fp = fopen(caminho, "r");
        if (!fp) continue;

        long pos_linha = 0;
        while (fgets(linha, sizeof(linha), fp)) {
            // Busca o termo na linha (case-insensitive manualmente)
            char linha_lower[1024];
            for (int i = 0; linha[i]; i++)
                linha_lower[i] = tolower((unsigned char)linha[i]);
            linha_lower[strlen(linha)] = '\0';

            char *encontrado = strstr(linha_lower, termo);
            if (encontrado) {
                long offset = pos_linha + (encontrado - linha_lower);
                exibir_contexto(pasta, entrada->d_name, offset);
                encontrados++;
                pagina++;

                if (pagina == 10) {
                    // Limpeza de buffer e aviso sobre o Enter
                    printf("Enter para prosseguir ou Esc (+ Enter) para parar: ");
                    int k = getchar();
                    if (k == 27) { // 27 eh o ASCII do Esc
                        while ((k = getchar()) != '\n' && k != EOF); // limpa buffer
                        fclose(fp); closedir(d); return; 
                    }
                    if (k != '\n' && k != EOF) {
                        while ((k = getchar()) != '\n' && k != EOF); // limpa se digitou lixo
                    }
                    pagina = 0;
                }
            }
            pos_linha = ftell(fp);
        }
        fclose(fp);
    }
    closedir(d);
    if (encontrados == 0) printf("  Nenhuma ocorrencia encontrada.\n");
}


// ============================================================
//                     PROGRAMA PRINCIPAL
// ============================================================


int main(int argc, char *argv[])
{
    // Se o usuario esquecer a pasta, avisa como usar
    if (argc < 2) {
        printf("ERRO: Voce esqueceu de informar a pasta!\n");
        printf("Uso correto: %s <caminho_da_pasta>\n", argv[0]);
        printf("Exemplo: ./indexador ./meus_textos\n");
        return 1;
    }

    const char *pasta = argv[1];

    // Inicializa tabela com NULL
    memset(tabela, 0, sizeof(tabela));

    // Indexa todos os .txt da pasta
    printf("Indexando arquivos em \"%s\"...\n", pasta);
    indexar_pasta(pasta);
    printf("Indexacao concluida!\n\n");

    // Loop de busca
    char termo[51];
    while (1) {
        printf("Pesquise o termo (ou digite 'sair'): ");
        if (!fgets(termo, sizeof(termo), stdin)) break;
        termo[strcspn(termo, "\n")] = '\0';   /* remove '\n' */

        if (strcmp(termo, "sair") == 0) break;
        if (termo[0] == '\0') continue;

        // Converte para minusculo
        for (int i = 0; termo[i]; i++)
            termo[i] = tolower((unsigned char)termo[i]);

        // Termo curto: busca linear sob demanda
        if ((int)strlen(termo) < MIN_CHARS) {
            printf("  (termo curto, buscando nos arquivos...)\n");
            busca_linear(pasta, termo);
            printf("\n");
            continue;
        }

        // Busca na tabela hash
        RegistroIndice *reg = buscar(termo);
        if (!reg) {
            printf("  Nenhuma ocorrencia encontrada.\n\n");
            continue;
        }

        // Exibe ocorrencias com contexto usando fseek
        Ocorrencia *oc = reg->lista;
        int pagina = 0;
        while (oc) {
            exibir_contexto(pasta, oc->nome_arquivo, oc->offset);
            pagina++;

            if (pagina == 10) {
                printf("Enter para prosseguir ou Esc (+ Enter) para parar: ");
                int k = getchar();
                if (k == 27) { // 27 = ESC (ASCII)
                    while ((k = getchar()) != '\n' && k != EOF); 
                    break; 
                }
                if (k != '\n' && k != EOF) {
                    while ((k = getchar()) != '\n' && k != EOF); 
                }
                pagina = 0;
            }

            oc = oc->prox;
        }
        printf("\n");
    }

    return 0;
}