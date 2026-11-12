FROM gcc:13

WORKDIR /app

COPY cv_openmp.c .
COPY gerar_dados.py .

# Gera os dados e compila
RUN python3 gerar_dados.py && \
    gcc -O2 -fopenmp cv_openmp.c -o cv_openmp -lm

# Padrão: threads automático, 20000 repetições
CMD ["./cv_openmp", "dados.txt"]
# Para benchmark com threads específico:  docker run --rm cv-openmp ./cv_openmp dados.txt 4 20000
