# Indexador de Alta Performance – LPI 2026.1

## Alexandre Lopes Silva

Sistema de indexação de arquivos `.txt` com **Tabela Hash e encadeamento**.

---

## Compilar e Executar

```bash
gcc -o indexador indexador.c
./indexador <caminho_da_pasta>
```

**Exemplo:**
```bash
./indexador ./textos
```

---

## Entrada de Dados

O programa recebe o **caminho de uma pasta** contendo arquivos `.txt`.

```
textos/
├── livro1.txt
├── artigo.txt
└── noticias.txt
```

Ao executar, ele indexa todos automaticamente:

```
Indexando arquivos em "textos"...
  Indexando: livro1.txt
  Indexando: artigo.txt
  Indexando: noticias.txt
Indexacao concluida!

Pesquise o termo:
```

---

## Uso Interativo

- Digite um **termo** e pressione Enter para buscar
- **Enter** → próxima página (10 resultados por vez)
- **Esc** → volta ao prompt de busca
- Digite **`sair`** para encerrar

---

## Regras

| Situação | Comportamento |
|---|---|
| Termo ≥ 5 caracteres | Indexado na tabela hash — busca O(1) |
| Termo < 5 caracteres | Busca linear nos arquivos sob demanda |
| Colisão de hash | Resolvida por encadeamento externo |

---

## Exemplo de Saída

```
Pesquise o termo: turing
  historia.txt - "...o trabalho de Alan Turing. O algoritmo de Turing..."
  historia.txt - "...algoritmo de Turing foi fundamental para a teoria..."
  
Pesquise o termo: sair
```