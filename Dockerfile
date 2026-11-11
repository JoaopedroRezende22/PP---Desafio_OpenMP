FROM gcc:latest

WORKDIR /app

# Copia os arquivos necessários para dentro do container
COPY cv_openmp.c dados.txt ./

# Compila o código
RUN gcc -O2 -fopenmp cv_openmp.c -o cv_openmp -lm

# Comando padrão ao rodar o container
CMD ["./cv_openmp", "dados.txt"]