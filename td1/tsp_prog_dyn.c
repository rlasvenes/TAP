//
//  TSP - PROGRAMMATION DYNAMIQUE
//
// -> compléter uniquement tsp_prog_dyn()

#include <float.h>

int NextSet(int S, int n) {
    /*
    Cette fonction permet de lister les sous-ensembles de {0,...,n-1}
    par taille croissante, les sous-ensembles étant vu comme des mots
    binaires codés par des entiers (int généralement sur 32 bits dont
    1 bit de signe). Ainsi NextSet(S,n) donne le prochain
    sous-ensemble celui codé par S qui lui est supérieur.

    Plus précisément, la fonction renvoie le plus petit entier
    immédiatement supérieur à S>0 et qui a: (1) soit le même poids que
    S, c'est-à-dire le même nombre de 1 dans son écriture binaire que S;
    (2) soit le poids de S plus 1 s'il n'y en a pas de même poids. La
    fonction renvoie 0 si S est le plus grand entier sur n bits, soit
    S=2^n-1.

    Le principe de l'algorithme est le suivant (NB: S>0): dans S on
    décale complètement à droite le premier bloc de 1 (celui contenant
    le bit de poids faible) sauf le bit le plus à gauche de ce bloc qui
    lui est décalé d'une position à gauche de sa position d'origine. Par
    exemple 1001110 devient 1010011. Si on ne peut pas décaler ce bit à
    gauche c'est que ou bien on est arrivé à 2^n-1 (et on renvoie 0) ou
    bien on doit passer à un poids plus élevé. Dans ce cas on renvoie le
    plus petit entier de poids p, soit 2^p-1, où p est le poids de S
    plus 1.

    Résultats obtenus en itérant S -> NextSet(S,4) à partir de S=1:
    (taille 1) S = 0001 -> 0010 -> 0100 -> 1000 ->
    (taille 2)     0011 -> 0101 -> 0110 -> 1001 -> 1010 -> 1100 ->
    (taille 3)     0111 -> 1011 -> 1101 -> 1110
    (taille 4)     1111 ->
    (taille 0)     0000

    Ainsi, les 16 entiers sur n=4 bits sont parcourus dans l'ordre: 1 ->
    2 -> 4 -> 8 -> 3 -> 5 -> 6 -> 9 -> 10 -> 12 -> 7 -> 11 -> 13 -> 14
    -> 15 -> 0

    L'algorithme prend un temps O(1) car les opérations de shifts (<<)
    et la fonction ffs() sur des int correspondent à des instructions
    machines sur des registres.
    */

    // ~(110010) = (001101)

    int p1 = ffs(S); // position du bit le plus à droit (commence à 1)
    int p2 = ffs(~(S >> p1)) + p1 - 1; // position du bit le plus gauche du 1er bloc de 1
    if (p2 - p1 + 1 == n)
    return 0; // cas S=2^n-1
    if (p2 == n)
    return (1 << (p2 - p1 + 2)) - 1; // cas: poids(S)+1

    return (S & ((-1) << p2)) | (1 << p2) |
    ((1 << (p2 - p1)) - 1); // cas: poids(S)
}

/* renvoie l'ensemble S\{i} */
int DeleteSet(int S, int i) { return (S & ~(1 << (i))); }

/* une cellule de la table */
typedef struct {
    double length; // longueur du chemin minimum D[t][S]
    int pred;      // point précédant t dans la solution D[t][S]
} cell;

int ExtractPath(cell **D, int t, int S, int n, int *Q) {
    // Construit dans Q le chemin extrait de la table D depuis la case
    // D[t][S] jusqu'au sommet V[n-1]. Il faut que Q[] soit assez grand
    // Renvoie la taille du chemin
    if(Q[0]<0) return 0; // si Q n'est pas défini

    Q[0] = t;                   // écrit le dernier point
    int k = 1;                  // k=taille de Q=nombre de points écrits dans Q
    while (Q[k - 1] != n - 1) { // on s'arrête si le dernier point est V[n-1]
    Q[k] = D[Q[k - 1]][S].pred;
    S = DeleteSet(S, Q[k - 1]);
    k++;
}
return k;
}

void PrintSet(int S) {
    printf("{ %d }\n", S);
}

double tsp_prog_dyn(point *V, int n, int *Q) {
    /*
    Version programmation dynamique du TSP. Le résultat (la tournée
    optimale) doit être écrit dans la permutation Q, tableau qui doit
    être alloué avant l'appel. On renvoie la valeur de la tournée
    optimale ou -1 s'il y a eut une erreur (pression de 'q' pour sortir
    de l'affichage). Une fois que votre programme arrivera à calculer
    une tournée optimale (à comparer avec tsp_brute_force() ou
    tsp_brute_force_opt()), il sera intéressant de dessiner à chaque
    fois que possible le chemin courant avec drawPath(V,n,Q). La
    variable "running" indique si l'affichage est actif (ce qui peut
    ralentir le programme), la pression de 'q' faisant passer running à
    faux.

    Pour résumer, D est un tableau de cellules à deux dimensions indexé
    par un sommet t (int) et un ensemble S (un int aussi):

    - D[t][S].length = longueur minimum d'un chemin allant de V[n-1] à
    V[t] qui visite tous les points d'indice dans S

    - D[t][S].pred = l'indice du point précédant V[t] dans le chemin
    ci-dessus de longueur D[t][S].length

    NB: ne pas lancer tsp_prog_dyn() avec n>31 car sinon les entiers
    (int sur 32 bits) ne seront pas assez grands pour stocker les
    sous-ensembles. De plus, pour n=32, n*2^n / 10^9 = 137 secondes
    est un peu long pour un ordinateur à 1 GHz. On pourrait
    outrepasser cette limitation en utilisant des "long" sur 64 bits
    pour coder les ensembles. Mais, outre le temps de calcul, l'espace
    mémoire (le malloc() pour la table D) risque d'être problématique:
    32 * 2^32 * sizeof(cell) représente déjà 1,536 To de mémoire.
    */

    int t, S;

    // déclaration de la table D[t][S] qui comportent (n-1)*2^(n-1) cellules
    cell **D = malloc((n - 1) * sizeof(cell *)); // n-1 lignes
    for (t = 0; t < n - 1; t++)
    D[t] = malloc((1 << (n - 1)) * sizeof(cell)); // 2^{n-1} colonnes
    D[0][1].length=-1; // pour savoir si la table a été remplie

    // cas de base, lorsque S={t}, pour t=0..n-2: D[t][S]=d(V[n-1],V[t])
    S = 1; // S=00...001 et pour l'incrémenter utiliser NextSet(S,n-1)

    for (t = 0; t < n - 1; t++) {
        D[t][S].length = dist(V[n - 1], V[t]); // Pour |S| = 1
        D[t][S].pred = n - 1;
        S = NextSet(S, n - 1);
    }


    // cas |S|>1 (cardinalité de S > 1): D[t][S] = min_x { D[x][S\{t}] +
    // d(V[x],V[t]) } avec t dans S et x dans S\{t}. NB: pour faire T =
    // S\{t}, utilisez T=DeleteSet(S,t); et pour savoir si x appartient
    // à l'ensemble S testez tout simplement si (S != DeleteSet(S,x)).

    double d_min = DBL_MAX;
    int pred;
    int K;

    do {
        if (S > 1) {
            for (t = 0; t < n - 1; t++) {
                int T = DeleteSet(S, t); // S \ {t}
                for (int x = 0; x < n - 1; x++) { // Pour tout x appartenant à S
                    int T2 = DeleteSet(T, x);
                    if (T != T2) { // Si x est dans S \ {t}
                    double tmp = D[x][T].length + dist(V[x], V[t]);
                    if (tmp < d_min) {
                        d_min = tmp;
                        pred = t - 1;
                        D[x][S].length = d_min;
                        D[x][S].pred = pred;
                    }
                }
            }
        }
    }


    // ici D[t][S].length et D[t][S].pred viennent d'être calculés.
    // Il reste a extraire le chemin de V[t] à V[n-1] et le mettre
    // dans Q, c'est le rôle de la fonction ExtractPath().

    int k = ExtractPath(D, t, S, n, Q);
    drawPath(V, n, Q, k);
    // }

    PrintSet(S);
    K = S;
    S = NextSet(S, n - 1);

} while (S && running);

double w = 0; // valeur de la tournée optimale, 0 par défaut

if (running) {
    // on a terminé correctement le calcul complet de la table D. Il
    // faut alors calculer la longueur w de la tournée optimale et
    // éventuellement construire la tournée pour la mettre dans Q. NB:
    // si le calcul a été interrompu (pression de 'q' pendant
    // l'affichage) alors ne rien faire et renvoyer 0

    for (int x = 0; x < n - 1; x++) {
        int value = D[x][K].length;
        if (value < d_min)
            d_min = value;
        printf("w = %f\n", w);
    }
} else {
    return 0; // On a appuyé sur la touche 'q'
}

for (t = 0; t < n - 1; t++) {
    free(D[t]);
}
return w;
}
