all: cv_openmp
cv_openmp: cv_openmp.c
	gcc -O2 -fopenmp cv_openmp.c -o cv_openmp -lm
dados.txt:
	python3 gerar_dados.py
run: cv_openmp dados.txt
	./cv_openmp dados.txt
clean:
	rm -f cv_openmp
