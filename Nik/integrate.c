#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include<fcntl.h>

double f (double a) {
	return a * a;
}

double integrate (double a, double b, double dx, double(*f)(double)) {
	double x = a, result = 0.;
	for ( ; x < b; x += dx) {
		result += ((f(x) + f(x + dx))/2) * dx;
	}
	return result;
}

void fullIntegrate (double a, double b, double dx, double(*f)(double), int numProc) {
	double delta = (b - a) / (double)numProc;
	int i = 0;
	for ( ; i < numProc; ++i) {
		int pid = fork();
		if (pid == 0) {
			int fd = open("temporary", O_WRONLY | O_CREAT | O_APPEND, 0666);
			double res = integrate (a + delta * (double)i, a + delta * (double)(i + 1), dx, f);
			write(fd, &res, sizeof(res));
			close(fd);
			exit(0);
		}
		else {
			if (pid == -1) {
				printf ("Error\n");
			}
		}
	}
	double res, totalResult = 0.;
	int fd = open("temporary", O_RDONLY);
	i = 0;
	for ( ; i < numProc; ++i) {
		while (read(fd, &res, sizeof(res)) != sizeof(res))
			sleep(1);
		totalResult += res;
	}
	close(fd);
	unlink("temporary");
	printf("%f\n", totalResult);
}

int main (int argc, char **argv) {
	fullIntegrate (0., 5., 0.1, f, 4);
	return 0;
}
