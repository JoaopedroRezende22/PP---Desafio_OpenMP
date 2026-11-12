# Desafio Final — Análise de Dados com OpenMP

Ciência da Computação · IESB  
Disciplina: Programação Paralela · Prof. Rodrigo

---

## Descrição

Programa em C que calcula o **coeficiente de variação (CV)** da estatura e do peso de 1000 pessoas, com dados agrupados em intervalos de classe, usando **OpenMP** para paralelizar os principais laços de cálculo.

| Variável | Intervalo de classe | CV obtido |
|----------|-------------------|-----------|
| Estatura | 8 cm              | 5,53 %    |
| Peso     | 4 kg              | 17,74 %   |

---

## Estrutura do projeto

desafio_openmp/
├── cv_openmp.c # Código principal (versão sequencial + paralela)
├── gerar_dados.py # Gera o arquivo dados.txt com 1000 registros sintéticos
├── dados.txt # Entrada: 1000 linhas “estatura(cm) peso(kg)”
├── Dockerfile # Imagem Docker com gcc:13 para compilar e executar
├── Makefile # Atalhos para compilar e rodar localmente
├── benchmark.ps1 # Script PowerShell para medir speedup com 1–12 threads
└── README.md # Este arquivo


---

## Como executar com Docker (recomendado)

### 1. Construir a imagem

```bash
docker build -t cv-openmp .
```

### 2. Executar com padrões (threads automático, 20 000 repetições)

```bash
docker run --rm cv-openmp
```

### 3. Especificar threads e repetições

```bash
docker run --rm cv-openmp ./cv_openmp dados.txt <threads> <repeticoes>

# Exemplos:
docker run --rm --cpus=1  cv-openmp ./cv_openmp dados.txt 1  20000
docker run --rm --cpus=2  cv-openmp ./cv_openmp dados.txt 2  20000
docker run --rm --cpus=4  cv-openmp ./cv_openmp dados.txt 4  20000
docker run --rm --cpus=8  cv-openmp ./cv_openmp dados.txt 8  20000
docker run --rm            cv-openmp ./cv_openmp dados.txt 12 20000
```

> A flag `--cpus=N` limita os núcleos visíveis ao container e é recomendada para o benchmark.

---

## Como compilar e executar localmente (Linux/macOS)

```bash
# Compilar
gcc -O2 -fopenmp cv_openmp.c -o cv_openmp -lm

# Gerar dados (caso não exista o dados.txt)
python3 gerar_dados.py

# Executar
./cv_openmp dados.txt
```

Ou simplesmente:

```bash
make run
```

---

## Formato do arquivo de entrada

Cada linha contém a estatura (cm) e o peso (kg) separados por espaço:
172.3 68.5
165.0 55.2
...


O programa aceita arquivos com menos de 1000 linhas (exibe aviso), mas espera exatamente esse formato por linha.

---

## Fórmulas utilizadas

Todas para **dados agrupados em classes**, conforme o enunciado:

- **Média ponderada:** μ = Σ(xᵢ · fᵢ) / N  
- **Desvio padrão populacional:** σ = √( Σ fᵢ · (xᵢ − μ)² / N )  
- **Coeficiente de variação:** CV = (σ / μ) × 100 %

Onde xᵢ é o ponto médio da classe i e fᵢ é a sua frequência.

---

## Partes paralelizadas com OpenMP

| Laço                        | Diretiva                                      |
|-----------------------------|-----------------------------------------------|
| Mínimo e máximo             | `reduction(min:mn) reduction(max:mx)`         |
| Histograma de frequências   | `reduction(+:f[:256])`                        |
| Soma ponderada → média      | `reduction(+:soma)`                           |
| Soma dos desvios → σ        | `reduction(+:ss)`                             |