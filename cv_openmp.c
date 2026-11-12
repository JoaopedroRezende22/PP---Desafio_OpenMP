/*
 * Desafio Final: Coeficiente de Variação com dados agrupados em classes (OpenMP)
 * Estatura: classes de 8 cm | Peso: classes de 4 kg
 *
 * Compilar: gcc -O2 -fopenmp cv_openmp.c -o cv_openmp -lm
 * Executar: ./cv_openmp dados.txt [num_threads] [repeticoes]
 *
 * Padrões: num_threads = máximo disponível, repeticoes = 20000
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

#define N_MAX       1000
#define N_ESPERADO  1000   /* avisa se o arquivo tiver menos registros */
#define MAX_CLASSES  256   /* limite de segurança para o vetor de classes */
#define REPS_PADRAO 20000  /* repetições padrão do benchmark            */

typedef struct {
    double mu, sigma, cv;
    int k;
    int f[MAX_CLASSES];
    double lim_inf;
    double h;
} Resultado;

/* Versão paralela com OpenMP */
/* Retorna 0 em sucesso, -1 se o número de classes exceder MAX_CLASSES. */
static int calcula_par(const double *v, int n, double h, Resultado *r)
{
    double mn = v[0], mx = v[0];

    /* Mínimo e máximo: reduction */
    #pragma omp parallel for reduction(min:mn) reduction(max:mx)
    for (int i = 0; i < n; i++) {
        if (v[i] < mn) mn = v[i];
        if (v[i] > mx) mx = v[i];
    }
    double base = floor(mn / h) * h;
    int k = (int)floor((mx - base) / h) + 1;

    if (k > MAX_CLASSES) {
        fprintf(stderr,
            "ERRO: numero de classes (%d) excede MAX_CLASSES (%d).\n"
            "      Aumente MAX_CLASSES em tempo de compilacao ou use um intervalo maior.\n",
            k, MAX_CLASSES);
        return -1;
    }

    r->h = h; r->lim_inf = base; r->k = k;
    int f[MAX_CLASSES] = {0};

    /* Histograma: reduction em array (cada thread tem cópia privada) */
    #pragma omp parallel for reduction(+:f[:MAX_CLASSES])
    for (int i = 0; i < n; i++)
        f[(int)floor((v[i] - base) / h)]++;

    for (int i = 0; i < k; i++) r->f[i] = f[i];

    /* Soma ponderada -> média */
    double soma = 0.0;
    #pragma omp parallel for reduction(+:soma)
    for (int i = 0; i < k; i++)
        soma += (base + (i + 0.5) * h) * f[i];
    r->mu = soma / n;

    /* Soma dos desvios quadráticos -> desvio padrão */
    double ss = 0.0, mu = r->mu;
    #pragma omp parallel for reduction(+:ss)
    for (int i = 0; i < k; i++) {
        double d = (base + (i + 0.5) * h) - mu;
        ss += f[i] * d * d;
    }
    r->sigma = sqrt(ss / n);
    r->cv = r->sigma / r->mu * 100.0;
    return 0;
}

/* Versão sequencial também retorna int para manter interface simétrica */
static int calcula_seq_checked(const double *v, int n, double h, Resultado *r)
{
    double mn = v[0];
    for (int i = 1; i < n; i++) if (v[i] < mn) mn = v[i];
    double base = floor(mn / h) * h;
    double mx = v[0];
    for (int i = 1; i < n; i++) if (v[i] > mx) mx = v[i];
    int k = (int)floor((mx - base) / h) + 1;

    if (k > MAX_CLASSES) {
        fprintf(stderr,
            "ERRO: numero de classes (%d) excede MAX_CLASSES (%d).\n", k, MAX_CLASSES);
        return -1;
    }

    r->h = h; r->lim_inf = base; r->k = k;
    for (int i = 0; i < k; i++) r->f[i] = 0;
    for (int i = 0; i < n; i++) r->f[(int)floor((v[i] - base) / h)]++;

    double soma = 0.0;
    for (int i = 0; i < k; i++) soma += (base + (i + 0.5) * h) * r->f[i];
    r->mu = soma / n;

    double ss = 0.0;
    for (int i = 0; i < k; i++) {
        double d = (base + (i + 0.5) * h) - r->mu;
        ss += r->f[i] * d * d;
    }
    r->sigma = sqrt(ss / n);
    r->cv = r->sigma / r->mu * 100.0;
    return 0;
}

static void imprime(const char *nome, const char *un, const Resultado *r)
{
    printf("\n=== %s (classes de %.0f %s) ===\n", nome, r->h, un);
    printf("%-4s %-16s %-10s %-5s\n", "i", "Classe", "Ponto med.", "fi");
    for (int i = 0; i < r->k; i++) {
        double a = r->lim_inf + i * r->h;
        printf("%-4d [%6.1f, %6.1f) %-10.1f %-5d\n", i + 1, a, a + r->h, a + r->h / 2, r->f[i]);
    }
    printf("Media (mu)        = %.4f %s\n", r->mu, un);
    printf("Desvio padrao     = %.4f %s\n", r->sigma, un);
    printf("CV                = %.2f %%\n", r->cv);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Uso: %s arquivo.txt [threads] [repeticoes]\n", argv[0]);
        return 1;
    }

    /* --- validação dos argumentos opcionais --- */
    int nthreads = omp_get_max_threads();
    int reps     = REPS_PADRAO;

    if (argc > 2) {
        nthreads = atoi(argv[2]);
        if (nthreads <= 0) {
            fprintf(stderr, "AVISO: numero de threads invalido (%s), usando 1.\n", argv[2]);
            nthreads = 1;
        }
    }
    if (argc > 3) {
        reps = atoi(argv[3]);
        if (reps <= 0) {
            fprintf(stderr, "AVISO: numero de repeticoes invalido (%s), usando %d.\n",
                    argv[3], REPS_PADRAO);
            reps = REPS_PADRAO;
        }
    }
    omp_set_num_threads(nthreads);

    /* --- leitura do arquivo --- */
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror("fopen"); return 1; }
    static double est[N_MAX], peso[N_MAX];
    int n = 0;
    while (n < N_MAX && fscanf(fp, "%lf %lf", &est[n], &peso[n]) == 2) n++;
    fclose(fp);

    if (n == 0) { fprintf(stderr, "ERRO: nenhum dado lido de '%s'.\n", argv[1]); return 1; }
    if (n < N_ESPERADO)
        fprintf(stderr, "AVISO: esperados %d registros, lidos apenas %d.\n", N_ESPERADO, n);
    printf("Pessoas lidas: %d | Threads: %d\n", n, nthreads);

    /* --- cálculos e impressão --- */
    static Resultado re, rp, re_s, rp_s;

    if (calcula_par(est, n, 8.0, &re)   < 0) return 1;
    if (calcula_par(peso, n, 4.0, &rp)  < 0) return 1;
    if (calcula_seq_checked(est, n, 8.0, &re_s) < 0) return 1;
    if (calcula_seq_checked(peso, n, 4.0, &rp_s) < 0) return 1;

    imprime("ESTATURA", "cm", &re);
    imprime("PESO", "kg", &rp);
    printf("\nConferencia seq x par: dif CV estatura = %.2e | dif CV peso = %.2e\n",
           fabs(re.cv - re_s.cv), fabs(rp.cv - rp_s.cv));

    /* --- benchmark --- */
    double sink = 0, t0, ts, tp;

    t0 = omp_get_wtime();
    for (int r = 0; r < reps; r++) {
        calcula_seq_checked(est, n, 8.0, &re_s);
        calcula_seq_checked(peso, n, 4.0, &rp_s);
        sink += re_s.cv + rp_s.cv;
    }
    ts = omp_get_wtime() - t0;

    t0 = omp_get_wtime();
    for (int r = 0; r < reps; r++) {
        calcula_par(est, n, 8.0, &re);
        calcula_par(peso, n, 4.0, &rp);
        sink += re.cv + rp.cv;
    }
    tp = omp_get_wtime() - t0;

    printf("\n=== DESEMPENHO (%d repeticoes) ===\n", reps);
    printf("Sequencial : %.4f s  (%.2f us/rep)\n", ts, ts / reps * 1e6);
    printf("Paralelo   : %.4f s  (%.2f us/rep)  [%d threads]\n", tp, tp / reps * 1e6, nthreads);
    printf("Speedup    : %.2fx | Eficiencia: %.1f %%\n", ts / tp, 100.0 * ts / tp / nthreads);
    if (sink < 0) printf("%f\n", sink); /* impede eliminação do laço pelo compilador */
    return 0;
}
