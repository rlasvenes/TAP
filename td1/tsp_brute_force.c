//
//  TSP - BRUTE-FORCE
//
// -> la structure "point" est définie dans tools.h

double dist(point A, point B) {
    double x = A.x - B.x;
    double y = A.y - B.y;
    return sqrt((x * x) + (y * y));
}

double value(point *V, int n, int *P) {
    double res = 0;
    for(int i = 0; i < n -1; i++){
        point A = V[P[i]];
        point B = V[P[i+1]];
        res += dist(A,B);
    }
    point A = V[P[n-1]];
    point B = V[P[0]];
    res += dist(A,B);
    return res;
}

double tsp_brute_force(point *V, int n, int *Q) {
    int P[n];
    for(int i = 0; i < n; i++){
        P[i] = i;
    }
    double wmin = value(V, n, P);
    memcpy(Q, &P, n * sizeof(int));
    while(NextPermutation(P, n) && running){
        double res = value(V, n, P);
        if(res < wmin){
            memcpy(Q, &P, n * sizeof(int));
            wmin = res;
            drawTour(V, n, P);
        }
    }
    return wmin;
}

void MaxPermutation(int *P, int n, int k) {
    int m = (n - k)/2;
    for(int i = 0; i < m; i++){
        int tmp = P[n - 1 - i];
        P[n - 1 - i] = P[k + i];
        P[k + i] = tmp;
    }
}

double value_opt(point *V, int n, int *P, double w) {
    double res = 0;
    for(int i = 0; i < n-1; i++){
        point A = V[P[i]];
        point B = V[P[i+1]];
        res += dist(A,B);
        if(res > w){
            return (-1)*(i + 2);
        }
    }
    point A = V[P[n-1]];
    point B = V[P[0]];
    res += dist(A,B);
    return res;
}

static double value_opt2(point *V, int n, int *P, double w) {
    static double ** D = NULL;
    if(D == NULL){
        D = (double **) malloc (n * sizeof(double*));
        for(int i = 0; i < n; i++){
            D[i] = (double*) malloc (n * sizeof(double));
            for(int j = 0; j < n; j++){
                point A = V[i];
                point B = V[j];
                D[i][j] = dist(A,B);
            }
        }
    }
    double res = 0;
    for(int i = 0; i < n-1; i++){
        int a = P[i];
        int b = P[i+1];
        res += D[a][b];
        if(res > w){
            if(D != NULL){
                for(int j = 0; j < n; j++){
                    free(D[j]);
                }
                free(D);
            }
            return (-1)*(i + 2);
        }
    }
    int a = P[n-1];
    int b = P[0];
    res += D[a][b];
    /*if(D != NULL){
        for(int i = 0; i < n; i++){
            free(D[i]);
        }
        free(D);
    }*/
    return res;
}

double tsp_brute_force_opt(point *V, int n, int *Q) {
    int P[n];
    for(int i = 0; i < n; i++){
        P[i] = i;
    }
    double wmin = value(V, n, P);
    memcpy(Q, &P, n * sizeof(int));
    while(NextPermutation(P, n) && running){
        double res = value_opt(V, n, P, wmin);
        if(res < 0 ){
            MaxPermutation(P, n, (-1)*res);
        }
        else if(res < wmin){
            memcpy(Q, &P, n * sizeof(int));
            wmin = res;
            drawTour(V, n, P);
        }
    }
    return wmin;
}
