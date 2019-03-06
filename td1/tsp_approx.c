//
//  TSP - APPROXIMATION / HEURISTIQUES
//

#include <float.h>

void printTab(int* T, int n) {
    printf("{");
    for (int i = 0; i < n; i++) {
        printf(" %d, ", T[i]);
    }
    printf(" }\n");
}

void initPermutation(int* P, int n) {
    for (int i = 0; i < n; i++) {
        P[i] = i;
    }
}

void reverse(int *T, int p, int q) {
    // renverse la partie T[p]...T[q] du tableau T avec p<q
    // si T={0,1,2,3,4,5,6} et p=2 et q=5, alors le nouveau
    // tableau sera {0,1, 5,4,3,2, 6}

    int tmp;
    while (p < q) {
        SWAP(T[p], T[q], tmp);
        p++; q--;
    }
    return;
}

double first_flip(point *V, int n, int *P) {
    // renvoie le gain>0 du premier flip réalisable, tout en réalisant
    // le flip, et 0 s'il n'y en a pas
    double res = 0;
    for (int i = 0; i < n; i++) {
        for (int j = i + 2; j < n; j++) {
            if ((dist(V[P[i]], V[P[j]]) + dist(V[P[i + 1]], V[P[(j + 1) % n]]))
            < (dist(V[P[i]], V[P[i + 1]]) + dist(V[P[j]], V[P[(j + 1) % n]]))) {
                res = value(V, n, P);
                reverse(P, i + 1, j);
                return res;
            }
        }
    }

    return res;
}

double tsp_flip(point *V, int n, int *P) {
    // la fonction doit renvoyer la valeur de la tournée obtenue. Pensez
    // à initialiser P, par exemple à P[i]=i. Pensez aussi faire
    // drawTour() pour visualiser chaque flip
    initPermutation(P, n); // P[i] = i
    bool a = true;
    double min = value(V, n, P);
    while (a && running) {
        drawTour(V, n, P);
        double b = first_flip(V, n, P);
        min -= b;
        a = (b != 0);
    }
    return value(V, n, P);
}

double tsp_greedy(point *V, int n, int *P) {
    double min = DBL_MAX;
    for(int i = 0; i < n - 1; i++) {
        int t = i;
        for(int j = i + 1; j < n; j++) {
            if (dist(V[i], V[j]) < min) {
                min = dist(V[i], V[j]);
                t = j;
            }
        }
        P[i + 1] = t;
    }
    return min;
}
