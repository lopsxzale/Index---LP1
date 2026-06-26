#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dirent.h>

#define TAM_TABELA 10007
#define MIN_CHARS 5

typedef struct ocorrencia {
    char nome_arquivo[71];
    long offset;
    struct ocorrencia *prox;
} Ocorrencia;

typedef struct {
    char termo[51];
    Ocorrencia *lista;
} RegistroIndice;

typedef struct no {
    RegistroIndice registro;
    struct no *prox;
} No;

No *tabela[TAM_TABELA];

int hash(const char *termo)
{
    int soma = 0;
    for (int i = 0; termo[i]; i++)
        soma += (unsigned char)termo[i];
    return soma % TAM_TABELA;
}

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

void inserir(const char *termo, const char *arquivo, long offset)
{
    int pos = hash(termo);

    No *atual = tabela[pos];
    while (atual) {
        if (strcmp(atual->registro.termo, termo) == 0)
            break;
        atual = atual->prox;
    }

    if (!atual) {
        atual = malloc(sizeof(No));
        strcpy(atual->registro.termo, termo);
        atual->registro.lista = NULL;
        atual->prox = tabela[pos];
        tabela[pos] = atual;
    }

    Ocorrencia *oc = malloc(sizeof(Ocorrencia));
    strcpy(oc->nome_arquivo, arquivo);
    oc->offset = offset;
    oc->prox = atual->registro.lista;
    atual->registro.lista = oc;
}

void indexar_arquivo(const char *caminho, const char *nome_arquivo)
{
    FILE *fp = fopen(caminho, "r");
    if (!fp) { perror(caminho); return; }

    char palavra[52];
    int i = 0;
    int c;
    long offset_inicio = 0;

    while (1) {
        long pos = ftell(fp);
        c = fgetc(fp);

        if (c == EOF) {
            if (i > 0) { palavra[i] = '\0'; goto processa; }
            break;
        }

        if (isalpha(c)) {
            if (i == 0) offset_inicio = pos;
            if (i < 51) palavra[i++] = tolower(c);
        } else {
            if (i > 0) {
                palavra[i] = '\0';
processa:
                if (i >= MIN_CHARS)
                    inserir(palavra, nome_arquivo, offset_inicio);
                i = 0;
            }
        }
    }

    fclose(fp);
}

void indexar_pasta(const char *pasta)
{
    DIR *d = opendir(pasta);
    if (!d) { perror(pasta); exit(1); }

    struct dirent *entrada;
    char caminho[512];

    while ((entrada = readdir(d)) != NULL) {
        char *ponto = strrchr(entrada->d_name, '.');
        if (!ponto || strcmp(ponto, ".txt") != 0) continue;

        snprintf(caminho, sizeof(caminho), "%s/%s", pasta, entrada->d_name);
        printf("  Indexando: %s\n", entrada->d_name);
        indexar_arquivo(caminho, entrada->d_name);
    }

    closedir(d);
}

void exibir_contexto(const char *pasta, const char *arquivo, long offset)
{
    char caminho[512];
    snprintf(caminho, sizeof(caminho), "%s/%s", pasta, arquivo);

    FILE *fp = fopen(caminho, "r");
    if (!fp) { printf("  [erro ao abrir %s]\n", arquivo); return; }

    long inicio = offset > 20 ? offset - 20 : 0;
    fseek(fp, inicio, SEEK_SET);

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
                    printf("Enter para prosseguir ou Esc (+ Enter) para parar: ");
                    int k = getchar();
                    if (k == 27) {
                        while ((k = getchar()) != '\n' && k != EOF);
                        fclose(fp); closedir(d); return; 
                    }
                    if (k != '\n' && k != EOF) {
                        while ((k = getchar()) != '\n' && k != EOF);
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

int main(int argc, char *argv[])
{
    if (argc < 2) {
        printf("ERRO: Voce esqueceu de informar a pasta!\n");
        printf("Uso correto: %s <caminho_da_pasta>\n", argv[0]);
        printf("Exemplo: ./indexador ./meus_textos\n");
        return 1;
    }

    const char *pasta = argv[1];

    memset(tabela, 0, sizeof(tabela));

    printf("Indexando arquivos em \"%s\"...\n", pasta);
    indexar_pasta(pasta);
    printf("Indexacao concluida!\n\n");

    char termo[51];
    while (1) {
        printf("Pesquise o termo (ou digite 'sair'): ");
        if (!fgets(termo, sizeof(termo), stdin)) break;
        termo[strcspn(termo, "\n")] = '\0';

        if (strcmp(termo, "sair") == 0) break;
        if (termo[0] == '\0') continue;

        for (int i = 0; termo[i]; i++)
            termo[i] = tolower((unsigned char)termo[i]);

        if ((int)strlen(termo) < MIN_CHARS) {
            printf("  (termo curto, buscando nos arquivos...)\n");
            busca_linear(pasta, termo);
            printf("\n");
            continue;
        }

        RegistroIndice *reg = buscar(termo);
        if (!reg) {
            printf("  Nenhuma ocorrencia encontrada.\n\n");
            continue;
        }

        Ocorrencia *oc = reg->lista;
        int pagina = 0;
        while (oc) {
            exibir_contexto(pasta, oc->nome_arquivo, oc->offset);
            pagina++;

            if (pagina == 10) {
                printf("Enter para prosseguir ou Esc (+ Enter) para parar: ");
                int k = getchar();
                if (k == 27) {
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