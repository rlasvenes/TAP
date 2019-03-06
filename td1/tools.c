#include "tools.h"


///////////////////////////////////////////////////////////////////////////
//
// Variables et fonctions internes (static) qui ne doivent pas être
// visibles à l'extérieur de ce fichier. À ne pas mettre dans le .h.
//
///////////////////////////////////////////////////////////////////////////

// nombres d'appels au dessin de la grille attendus par seconde
static unsigned long call_speed = 100;

static bool mouse_ldown = false; // bouton souris gauche, vrai si enfoncé
static bool mouse_rdown = false; // boutons souris droit, vrai si enfoncé
static bool oriented = false;    // pour l'orientation de la tournée
static bool root = false;        // pour le point de départ de la tournée
static int selectedVertex = -1;
static point *vertices;
static int num_vertices;

static SDL_Window *window;
static SDL_GLContext glcontext;
static GLvoid *gridImage;
static GLuint texName;

static void drawLine(point p, point q) {
  glBegin(GL_LINES);
  glVertex2f(p.x, p.y);
  glVertex2f(q.x, q.y);
  glEnd();
}

static void drawEdge(point p, point q) {
  double linewidth = 1;
  glGetDoublev(GL_LINE_WIDTH, &linewidth);
  glBegin(GL_LINES);
  glVertex2f(p.x, p.y);
  glVertex2f(.2*p.x+.8*q.x, .2*p.y+.8*q.y);
  glEnd();
  glLineWidth(linewidth*5);
  glBegin(GL_LINES);
  glVertex2f(.2*p.x+.8*q.x, .2*p.y+.8*q.y);
  glVertex2f(q.x, q.y);
  glEnd();
  glLineWidth(linewidth);
}

static void drawPoint(point p) {
  glPointSize(5.0f);
  glBegin(GL_POINTS);
  glVertex2f(p.x, p.y);
  glEnd();
}

// Convertit les coordonnées pixels en coordonnées dans le dessin
static void pixelToCoord(int pixel_x, int pixel_y, double *x, double *y) {
  GLdouble ray_z;
  GLint viewport[4];
  GLdouble proj[16];
  GLdouble modelview[16];

  // we query OpenGL for the necessary matrices etc.
  glGetIntegerv(GL_VIEWPORT, viewport);
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  glGetDoublev(GL_MODELVIEW_MATRIX, modelview);

  GLdouble _X = pixel_x;
  GLdouble _Y = viewport[3] - pixel_y;

  // using 1.0 for winZ gives u a ray
  gluUnProject(_X, _Y, 1.0f, modelview, proj, viewport, x, y, &ray_z);
}

// Récupère les coordonnées du centre de la fenêtre
static void getCenterCoord(double *x, double *y) {
  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  pixelToCoord((viewport[0] + viewport[2]) / 2, (viewport[1] + viewport[3]) / 2,
               x, y);
}

static int getClosestVertex(double x, double y) {
  // renvoie l'indice i du point le plus proche de (x,y)
  int res = 0;
  double dmin = (x - vertices[0].x) * (x - vertices[0].x) +
                (y - vertices[0].y) * (y - vertices[0].y);

  for (int i = 1; i < num_vertices; i++) {
    double dist = (x - vertices[i].x) * (x - vertices[i].x) +
                  (y - vertices[i].y) * (y - vertices[i].y);
    if (dist < dmin) {
      dmin = dist;
      res = i;
    }
  }

  return res;
}

static char *getTitle(void){
  static char buffer[100];
  sprintf(buffer,"Techniques Algorithmiques et Programmation - %d x %d (%.2lf)",
	  width,height,scale);
  return buffer;
}

// Zoom d'un facteur s centré en (x,y)
static void zoomAt(double s, double x, double y) {
  glTranslatef(x, y, 0);
  glScalef(s, s, 1.0);
  glTranslatef(-x, -y, 0);
}

// Zoom d'un facteur s centré sur la position de la souris, modifie
// la variable globale scale du même facteur
static void zoomMouse(double s){
  int mx, my;
  double x, y;
  SDL_GetMouseState(&mx, &my);
  pixelToCoord(mx, my, &x, &y);
  zoomAt(s, x, y);
  scale *= s;
}

// set drawGrid call speed
void speedUp() {
  if ((call_speed << 1) != 0)
    call_speed <<= 1;
}

void speedDown() {
  call_speed >>= 1;
}

void speedSet(unsigned long speed) {
  call_speed = speed;
}

unsigned long speedMax() {
  return ULONG_MAX;
}


static int NextPerm(int *P, const int n, const int *C) {
  /*
    Génère, à partir d'une permutation P, la prochaine dans l'ordre
    lexicographique suivant les contraintes définies par le tableau C
    (voir ci-après). Mettre C=NULL s'il n'y a pas de contrainte
    particulière. On renvoie 1 si la prochaine permutation à pu être
    déterminée et 0 si la dernière permutation a été atteinte. Dans ce
    cas la permutation la plus petite selon l'ordre lexicographique est
    renvoyée. On permute les éléments de P que si leurs positions sont
    entre C[j] et C[j+1] (exclu) pour un certain indice j.

    Ex: si C={2,3,5}. Les permutations sous la contrainte C sont:
    (on peut permuter les indices {0,1}{2}{3,4})

                   0 1 2 3 4 (positions dans P)
                P={a,b,c,d,e}
                  {b,a,c,d,e}
                  {a,b,c,e,d}
                  {b,a,c,e,d}

    Evidemment, il y a beaucoup moins de permutations dès que le nombre
    de contraintes augmente. Par exemple, si C contient k intervalles de
    même longueur, alors le nombre de permutations sera de (n/k)!^k au
    lieu de n!. Le rapport des deux est d'environ k^n.

    Concrêtement, pour:
    - n=9 et k=3, on a 216 permutations au lieu de 362.880 (k^n=19.683)
    - n=12 et k=3, on a 13.824 permutations au lieu de 479.001.600 (k^n=531.441)

    Le dernier élément de C doit être égale à n-1 (sentinelle), le
    premier étant omis car toujours = 0. Donc C est un tableau à au plus
    n éléments. Si C=NULL, alors il n'y a pas de contrainte
    particulière, ce qui est identique à poser C[0]=n.

    On se base sur l'algorithme classique (lorsque C=NULL, sinon on
    l'applique sur l'intervalle de positions [C[j],C[j+1][):

    1. Trouver le plus grand index i tel que P[i] < P[i+1].
       S'il n'existe pas, la dernière permutation est atteinte.
    2. Trouver le plus grand indice j tel que P[i] < P[j].
    3. Echanger P[i] avec P[j].
    4. Renverser la suite de P[i+1] jusqu'au dernier élément.

  */
  int i, j, a, b, c, T[1];

  if (C == NULL) {
    T[0] = n;
    C = T;
  }

  b = C[i = j = 0]; /* j=indice de la prochaine valeur à lire dans C */
  c = -1;

  /* étape 1: on cherche l'intervalle [a,b[ avec i tq P[i]<P[i+1] */
etape1:
  for (a = i; i < b - 1; i++)
    if (P[i] < P[i + 1])
      c = i;   /* on a trouvé un i tq P[i]<P[i+1] */
  if (c < 0) { /* le plus grand i tq P[i]<[i+1] n'existe pas */
    for (i = a; i < b; i++)
      P[i] = i; /* on réinitialise P[a]...P[b-1] */
    if (b == n)
      return 0; /* alors on a fini d'examiner C */
    b = C[++j]; /* b=nouvelle borne */
    goto etape1;
  }
  i = c; /* i=le plus grand tq P[i]<P[i+1] avec a<=i,i+1<b */

  /* étape 2: cherche j=le plus grand tq P[i]<P[j] */
  for (j = i + 1; j < b; j++)
    if (P[i] < P[j])
      c = j;
  j = c;

  /* étape 3: échange P[i] et P[j] */
  SWAP(P[i], P[j], c);

  /* étape 4: renverse P[i+1]...P[b-1] */
  for (++i, --b; i < b; i++, b--)
    SWAP(P[i], P[b], c);

  return 1;
}

// Vrai ssi (i,j) est sur le bord de la grille G.
static inline int onBorder(grid *G, int i, int j) {
  return (i == 0) || (j == 0) || (i == G->X - 1) || (j == G->Y - 1);
}

// Distance Lmax entre s et t.
static inline double distLmax(position s, position t) {
  return hypot(t.x - s.x, t.y - s.y);
}

typedef struct {
  // l'ordre de la déclaration est important
  GLubyte R;
  GLubyte G;
  GLubyte B;
} RGB;

static RGB color[] = {
    {0xE0, 0xE0, 0xE0}, // V_FREE
    {0x00, 0x00, 0x00}, // V_WALL
    {0xF0, 0xD8, 0xA8}, // V_SAND
    {0x00, 0x6D, 0xBA}, // V_WATER
    {0x7C, 0x70, 0x56}, // V_MUD
    {0x00, 0xA0, 0x60}, // V_GRASS
    {0x70, 0xE0, 0xD0}, // V_TUNNEL
    {0x80, 0x80, 0x80}, // M_NULL
    {0x12, 0x66, 0x66}, // M_USED
    {0x08, 0xF0, 0xF0}, // M_FRONT
    {0x90, 0x68, 0xF8}, // M_PATH
    {0xFF, 0x00, 0x00}, // C_START
    {0xFF, 0x88, 0x28}, // C_END
    {0x99, 0xAA, 0xCC}, // C_FINAL
    {0xFF, 0xFF, 0x80}, // C_END_WALL
    {0xC0, 0x4F, 0x16}, // M_USED2
    {0x66, 0x12, 0x66}, // C_FINAL2
};

// nombre de couleurs dans color[]
static const int NCOLOR = (int)(sizeof(color) / sizeof(*color));

// Vrai ssi p est une position de la grille. Attention ! cela ne veut
// pas dire que p est un sommet du graphe, car la case peut contenir
// V_WALL.
static inline int inGrid(grid *G, position p) {
  return (0 <= p.x) && (p.x < G->X) && (0 <= p.y) && (p.y < G->Y);
}

//
// Construit l'image (pixels) de la grille à partir de la grille G. Le
// point (0,0) de G correspond au coin en haut à gauche.
//
// +--x
// |
// y
//
static void makeImage(grid *G) {
  static int cpt; // compteur d'étape lorsqu'on reconstruit le chemin

  RGB *I = gridImage, c;
  int k = 0, v, m, f;
  int fin = (G->mark[G->start.x][G->start.y] ==
             M_PATH); // si le chemin a fini d'être construit
  int debut = (G->mark[G->end.x][G->end.y] ==
               M_PATH); // si le chemin commence à être construit

  if (fin)
    update = false;
  if (debut == 0)
    cpt = 0;
  if (debut)
    cpt++;
  if (debut && cpt == 1) {
    update = true;
    speedSet(16);
  }

  for (int j = 0; j < G->Y; j++)
    for (int i = 0; i < G->X; i++) {
      m = G->mark[i][j];
      if ((m < 0) || (m >= NCOLOR))
        m = M_NULL;
      v = G->value[i][j];
      if ((v < 0) || (v >= NCOLOR))
        v = V_FREE;
      do {
        if (m == M_PATH) {
          c = color[m];
          break;
        }
        if (fin && erase) {
          c = color[v];
          break;
        } // affiche la grille d'origine à la fin
        if (m == M_NULL) {
          c = color[v];
          break;
        } // si pas de marquage
        if (m == M_USED || m == M_USED2) {
          // interpolation de couleur entre les couleurs M_USED(2) et
          // C_FINAL(2) ou bien M_USED(2) et v si on est en train de
          // reconstruire le chemin
          position p = {.x = i, .y = j};
          double t = distLmax(G->start, G->end);
          if (t == 0)
            t = 1E-10; // pour éviter la division par 0
          if (debut && erase) {
            t = 0.5 * cpt / t;
            f = v;
          } else {
            t = distLmax(G->start, p) / t;
            f = (m == M_USED) ? C_FINAL : C_FINAL2;
          }
          t = fmax(t, 0.0), t = fmin(t, 1.0);
          c = color[m];
          c.R += t * (color[f].R - color[m].R);
          c.G += t * (color[f].G - color[m].G);
          c.B += t * (color[f].B - color[m].B);
          break;
        }
        c = (m == M_NULL) ? color[v] : color[m];
        break;
      } while (0);
      I[k++] = c;
    }

  if (inGrid(G, G->start)) {
    k = G->start.y * G->X + G->start.x;
    I[k] = color[C_START];
  }

  if (inGrid(G, G->end)) {
    v = (G->value[G->end.x][G->end.y] == V_WALL) ? C_END_WALL : C_END;
    k = G->end.y * G->X + G->end.x;
    I[k] = color[v];
  }
}

//
// Alloue une grille aux dimensions x,y ainsi que son image. On force
// x,y>=3 pour avoir au moins un point qui n'est pas sur le bord.
//
static grid allocGrid(int x, int y) {
  grid G;
  position p = {-1, -1};
  G.start = G.end = p;
  if (x < 3)
    x = 3;
  if (y < 3)
    y = 3;
  G.X = x;
  G.Y = y;
  G.value = malloc(x * sizeof(*(G.value)));
  G.mark = malloc(x * sizeof(*(G.mark)));

  for (int i = 0; i < x; i++) {
    G.value[i] = malloc(y * sizeof(*(G.value[i])));
    G.mark[i] = malloc(y * sizeof(*(G.mark[i])));
    for (int j = 0; j < y; j++)
      G.mark[i][j] = M_NULL; // initialise
  }

  gridImage = malloc(3 * x * y * sizeof(GLubyte));
  return G;
}

//
// Renvoie une position aléatoire de la grille qui est uniforme parmi
// toutes les valeurs de la grille du type t (hors les bords de la
// grille). Si aucune case de type t n'est pas trouvée, la position
// (-1,-1) est renvoyée.
//
static position randomPosition(grid G, int t) {
  int i, j, c;
  int n;                      // n=nombre de cases de type t hors le bord
  int r = -1;                 // r=numéro aléatoire dans [0,n[
  position p = {-1, -1};      // position par défaut
  const int stop = G.X * G.Y; // pour sortir des boucles
  const int x1 = G.X - 1;
  const int y1 = G.Y - 1;

  // On fait deux parcours: un 1er pour compter le nombre n de cases
  // de type t, et un 2e pour tirer au hasard la position parmi les
  // n. A la fin du premier parcours on connaît le nombre n de cases
  // de type t. On tire alors au hasard un numéro r dans [0,n[. Puis
  // on recommence le comptage (n=0) de cases de type t et on s'arrête
  // dès qu'on arrive à la case numéro r.

  c = 0;
  do {
    n = 0;
    for (i = 1; i < x1; i++)
      for (j = 1; j < y1; j++)
        if (G.value[i][j] == t) {
          if (n == r)
            p.x = i, p.y = j,
            i = j = stop; // toujours faux
                          // au 1er parcours
          n++;
        }
    c = 1 - c;
    if (c)
      r = random() % n;
  } while (c); // vrai la 1ère fois, faux la 2e

  return p;
}


///////////////////////////////////////////////////////////////////////////
//
// Variables et fonctions utilisées depuis l'extérieur (non static). À
// mettre dans le .h.
//
///////////////////////////////////////////////////////////////////////////


// valeurs par défaut
int    width   = 640;
int    height  = 480;
bool   update  = true;
bool   running = true;
bool   hover   = true;
bool   erase   = true;
double scale   = 1;

char *TopChrono(const int i) {
#define CHRONOMAX 10
  /*
    Met à jour le chronomètre interne numéro i (i=0..CHRNONMAX-1) et
    renvoie sous forme de char* le temps écoulé depuis le dernier appel
    à la fonction pour le même chronomètre. La précision dépend du temps
    mesuré. Il varie entre la seconde et le 1/1000 de seconde. Plus
    précisément le format est le suivant:

    1d00h00'  si le temps est > 24h (précision: 1')
    1h00'00"  si le temps est > 60' (précision: 1s)
    1'00"0    si le temps est > 1'  (précision: 1/10s)
    1"00      si le temps est > 1"  (précision: 1/100s)
    0"000     si le temps est < 1"  (précision: 1/1000s)

    Pour initialiser et mettre à jour tous les chronomètres (dont le
    nombre vaut CHRONOMAX), il suffit d'appeler une fois la fonction,
    par exemple avec TopChrono(0). Si i<0, alors les pointeurs alloués
    par l'initialisation sont désalloués. La durée maximale est limitée
    à 100 jours. Si une erreur se produit (durée supérieure ou erreur
    avec gettimeofday()), alors on renvoie la chaîne "--error--".
  */
  if (i >= CHRONOMAX)
    return "--error--";

  /* variables globales, locale à la fonction */
  static int first =
      1; /* =1 ssi c'est la 1ère fois qu'on exécute la fonction */
  static char *str[CHRONOMAX];
  static struct timeval last[CHRONOMAX], tv;
  int j;

  if (i < 0) {  /* libère les pointeurs */
    if (!first) /* on a déjà alloué les chronomètres */
      for (j = 0; j < CHRONOMAX; j++)
        free(str[j]);
    first = 1;
    return NULL;
  }

  /* tv=temps courant */
  int err = gettimeofday(&tv, NULL);

  if (first) { /* première fois, on alloue puis on renvoie TopChrono(i) */
    first = 0;
    for (j = 0; j < CHRONOMAX; j++) {
      str[j] = malloc(
          10); // assez grand pour "--error--", "99d99h99'" ou "23h59'59""
      last[j] = tv;
    }
  }

  /* t=temps en 1/1000" écoulé depuis le dernier appel à TopChrono(i) */
  long t = (tv.tv_sec - last[i].tv_sec) * 1000L +
           (tv.tv_usec - last[i].tv_usec) / 1000L;
  last[i] = tv; /* met à jour le chrono interne i */
  if ((t < 0L) || (err))
    t = LONG_MAX; /* temps erroné */

  /* écrit le résultat dans str[i] */
  for (;;) { /* pour faire un break */
    /* ici t est en millième de seconde */
    if (t < 1000L) { /* t<1" */
      sprintf(str[i], "0\"%03li", t);
      break;
    }
    t /= 10L;        /* t en centième de seconde */
    if (t < 6000L) { /* t<60" */
      sprintf(str[i], "%li\"%02li", t / 100L, t % 100L);
      break;
    }
    t /= 10L;         /* t en dixième de seconde */
    if (t < 36000L) { /* t<1h */
      sprintf(str[i], "%li'%02li\"%li", t / 360L, (t / 10L) % 60L, t % 10L);
      break;
    }
    t /= 10L;         /* t en seconde */
    if (t < 86400L) { /* t<24h */
      sprintf(str[i], "%lih%02li'%02li\"", t / 3600L, (t / 60L) % 60L, t % 60L);
      break;
    }
    t /= 60L;         /* t en minute */
    if (t < 144000) { /* t<100 jours */
      sprintf(str[i], "%lid%02lih%02li'", t / 1440L, (t / 60L) % 24L, t % 60L);
      break;
    }
    /* error ... */
    sprintf(str[i], "--error--");
  }

  return str[i];
#undef CHRONOMAX
}

bool NextPermutation(int *P, const int n) {
  return ( NextPerm(P, n, NULL) == 1 ); };

//
// Libère les pointeurs alloués par allocGrid().
//
void freeGrid(grid G) {
  for (int i = 0; i < G.X; i++) {
    free(G.value[i]);
    free(G.mark[i]);
  }
  free(G.value);
  free(G.mark);
  free(gridImage);
}

//
// Renvoie une grille de dimensions x,y rempli de points aléatoires de
// type et de densité donnés. Le départ et la destination sont
// initialisées aléatroirement dans une case V_FREE.
//
grid initGridPoints(int x, int y, int type, double density) {
#define RAND01 ((double)random() / RAND_MAX) // réel aléatoire dans [0,1[
  grid G = allocGrid(x, y); // alloue la grille et son image

  // vérifie que le type est correct, M_NULL par défaut
  if ((type < 0) || (type >= NCOLOR))
    type = M_NULL;

  // met les bords et remplit l'intérieur
  for (int i = 0; i < x; i++)
    for (int j = 0; j < y; j++)
      G.value[i][j] =
          onBorder(&G, i, j) ? V_WALL : ((RAND01 <= density) ? type : V_FREE);

  // position start/end aléatoires
  G.start = randomPosition(G, V_FREE);
  G.end = randomPosition(G, V_FREE);

#undef RAND01
  return G;
}

//
// Renvoie une grille aléatoire de dimensions x,y (au moins 3)
// correspondant à partir un labyrinthe qui est un arbre couvrant
// aléatoire uniforme. On fixe le point start = en bas à droit et end
// = en haut à gauche. La largeur des couloirs est donnée par w>0.
//
// Il s'agit de l'algorithme de Wilson par "marches aléatoires avec
// effacement de boucle" (cf. https://bl.ocks.org/mbostock/11357811)
//
grid initGridLaby(int x, int y, int w) {

  // vérifie les paramètres
  if (x < 3)
    x = 3;
  if (y < 3)
    y = 3;
  if (w <= 0)
    w = 1;

  // alloue la grille et son image
  int *value = malloc(x * y * sizeof(*value));

  // alloue la grille et son image
  grid Gw = allocGrid(x * (w + 1) + 1, y * (w + 1) + 1);

  // position par défaut
  Gw.start.x = Gw.X - 2;
  Gw.start.y = Gw.Y - 2;
  Gw.end.x = 1;
  Gw.end.y = 1;

  // au début des murs seulement sur les bords
  for (int i = 0; i < Gw.X; i++) {
    for (int j = 0; j < Gw.Y; j++) {
      Gw.value[i][j] =
          ((i % (w + 1) == 0) || (j % (w + 1) == 0)) ? V_WALL : V_FREE;
    }
  }

  for (int i = 0; i < x; i++)
    for (int j = 0; j < y; j++)
      value[i * y + j] = -1;

  int count = 1;
  value[0] = 0;
  while (count < x * y) {
    int i0 = 0;
    for (i0 = 0; i0 < x * y && value[i0] != -1; i0++)
      ;
    value[i0] = i0 + 1;
    while (i0 < x * y) {
      int x0 = i0 / y;
      int y0 = i0 % y;
      while (true) {
        int dir = rand() % 4;
        switch (dir) {
        case 0:
          if (x0 <= 0)
            continue;
          x0--;
          break;
        case 1:
          if (y0 <= 0)
            continue;
          y0--;
          break;
        case 2:
          if (x0 >= x - 1)
            continue;
          x0++;
          break;
        case 3:
          if (y0 >= y - 1)
            continue;
          y0++;
          break;
        }
        break;
      }
      if (value[x0 * y + y0] == -1) {
        value[x0 * y + y0] = i0 + 1;
        i0 = x0 * y + y0;
      } else {
        if (value[x0 * y + y0] > 0) {
          while (i0 != x0 * y + y0 && i0 > 0) {
            int i1 = value[i0] - 1;
            value[i0] = -1;
            i0 = i1;
          }
        } else {
          int i1 = i0;
          i0 = x0 * y + y0;
          do {
            int x0 = i0 / y;
            int y0 = i0 % y;
            int x1 = i1 / y;
            int y1 = i1 % y;
            if (x0 < x1)
              for (int i = 0; i < w; ++i)
                Gw.value[x1 * (w + 1)][y0 * (w + 1) + i + 1] = V_FREE;
            if (x0 > x1)
              for (int i = 0; i < w; ++i)
                Gw.value[x0 * (w + 1)][y0 * (w + 1) + i + 1] = V_FREE;
            if (y0 < y1)
              for (int i = 0; i < w; ++i)
                Gw.value[x1 * (w + 1) + i + 1][y1 * (w + 1)] = V_FREE;
            if (y0 > y1)
              for (int i = 0; i < w; ++i)
                Gw.value[x1 * (w + 1) + i + 1][y0 * (w + 1)] = V_FREE;
            i0 = i1;
            i1 = value[i0] - 1;
            value[i0] = 0;
            count++;
          } while (value[i1] != 0);
          break;
        }
      }
    }
  }

  free(value);

  return Gw;
}

grid initGridFile(char *file) {
  FILE *f = fopen(file, "r");
  if (f == NULL) {
    printf("Cannot open file \"%s\"\n", file);
    exit(1);
  }

  char *L = NULL; // L=buffer pour la ligne de texte à lire
  size_t b = 0;   // b=taille du buffer L utilisé (nulle au départ)
  ssize_t n;      // n=nombre de caractères lus dans L, sans le '\0'

  // Etape 1: on évalue la taille de la grille. On s'arrête si c'est
  // la fin du fichier ou si le 1ère caractère n'est pas un '#'

  int x = 0; // x=nombre de caractères sur une ligne
  int y = 0; // y=nombre de lignes

  while ((n = getline(&L, &b, f)) > 0) {
    if (L[0] != '#')
      break;
    if (L[n - 1] == '\n')
      n--; // se termine par '\n' sauf si fin de fichier
    if (n > x)
      x = n;
    y++;
  }

  rewind(f);

  if (x < 3)
    x = 3;
  if (y < 3)
    y = 3;
  grid G = allocGrid(x, y);

  // met des bords et remplit l'intérieur
  for (int i = 0; i < x; i++)
    for (int j = 0; j < y; j++)
      G.value[i][j] = onBorder(&G, i, j) ? V_WALL : V_FREE;

  // Etape 2: on relie le fichier et on remplit la grille

  int v;
  for (int j = 0; j < y; j++) {
    n = getline(&L, &b, f);
    if (L[n - 1] == '\n')
      n--;                        // enlève le '\n' éventuelle
    for (int i = 0; i < n; i++) { // ici n<=x
      switch (L[i]) {
      case ' ':
        v = V_FREE;
        break;
      case '#':
        v = V_WALL;
        break;
      case ';':
        v = V_SAND;
        break;
      case '~':
        v = V_WATER;
        break;
      case ',':
        v = V_MUD;
        break;
      case '.':
        v = V_GRASS;
        break;
      case '+':
        v = V_TUNNEL;
        break;
      case 's':
        v = V_FREE;
        G.start.x = i;
        G.start.y = j;
        break;
      case 't':
        v = V_FREE;
        G.end.x = i;
        G.end.y = j;
        break;
      default:
        v = V_FREE;
      }
      G.value[i][j] = v;
    }
  }

  free(L);
  fclose(f);
  return G;
}

void addRandomBlob(grid G, int type, int nb) {
  // Ne touche pas au bord de la grille.
  int neighs[8][2] = {{0, -1},  {1, 0},  {0, 1},  {-1, 0},
                      {-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
  for (int i = 0; i < nb; i++)
    G.value[1 + random() % (G.X - 2)][1 + random() % (G.Y - 2)] = type;

  for (int it = 0; it < G.X && it < G.Y; it++)
    for (int i = 2; i < G.X - 2; i++)
      for (int j = 2; j < G.Y - 2; j++) {
        int n = 0;
        for (int k = 0; k < 4; k++)
          if (G.value[i + neighs[k][0]][j + neighs[k][1]] == type)
            n++;
        if (n && random() % ((4 - n) * 20 + 1) == 0)
          G.value[i][j] = type;
      }
}

// Initialisation de SDL
void init_SDL_OpenGL(void) {

  SDL_Init(SDL_INIT_VIDEO);
  window = SDL_CreateWindow(getTitle(), SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, width, height,
      SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

  if (window == NULL) { // échec lors de la création de la fenêtre
    printf("Could not create window: %s\n", SDL_GetError());
    SDL_Quit();
    exit(1);
  }

  SDL_GetWindowSize(window, &width, &height);
  // Contexte OpenGL
  glcontext = SDL_GL_CreateContext(window);

  // Projection de base, un point OpenGL == un pixel
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  glOrtho(0.0, width, height, 0.0, 0.0f, 1.0f);
  glScalef(scale, scale, 1.0);

  // Some GL options
  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glGenTextures(1, &texName);
  glBindTexture(GL_TEXTURE_2D, texName);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
}

// Fermeture de SDL
void cleaning_SDL_OpenGL() {
  SDL_GL_DeleteContext(glcontext);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

// Génère n points aléatoires du rectangle [0,width] × [0,height] et
// renvoie le tableau des n points (type double) ainsi générés. Met à
// jour les variables globales vertices[] et num_vertices.
point *generatePoints(int n) {

  vertices = malloc(n * sizeof(point));
  const double rx = (double)width / RAND_MAX;
  const double ry = (double)height / RAND_MAX;
  for (int i = 0; i < n; i++) {
    vertices[i].x = random() * rx;
    vertices[i].y = random() * ry;
  }
  num_vertices = n;
  return vertices;
}

// Génère n points du rectangle [0,width] × [0,height] répartis
// aléatoirement sur k cercles concentriques.
point *generateCircles(int n, int k) {

  point c = { width / 2.0, height / 2.0 }; // centre
  vertices = malloc(n * sizeof(point));
  const double r0 = ((width<height)? width:height)/(2.2*k);

  for (int i = 0; i < n; i++) { // on place les n points
    int j = random()%k; // j=numéro du cercle
    double r = (j+1)*r0;
    const double K = 2.0 * M_PI / RAND_MAX;
    double a = K * random();
    vertices[i].x = c.x + r * cos(a);
    vertices[i].y = c.y + r * sin(a);
  }

  num_vertices = n;
  return vertices;
}

#define ORANGE .99,.8,.3

void drawTour(point *V, int n, int *P) {
  static unsigned int last_tick = 0;

  // saute le dessin si le précédent a été fait il y a moins de 20ms
  // ou si update est faux
  if ((!update) && (last_tick + 20 > SDL_GetTicks()))
    return;
  last_tick = SDL_GetTicks();

  // gestion de la file d'event
  handleEvent(false);

  // efface la fenêtre
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // dessine le cycle
  if (V && P && (P[0]>=0)) {
    glColor3f(1, 1, 1); // Blanc
    for (int i = 0; i < n; i++){
      if(oriented)
        drawEdge(V[P[i]], V[P[(i + 1) % n]]);
      else
        drawLine(V[P[i]], V[P[(i + 1) % n]]);
    }
    if (root) {
      glColor3f(ORANGE); // Orange
      if(oriented && (n>0))
        drawEdge(V[P[0]], V[P[1]]);
      else
        drawLine(V[P[0]], V[P[1]]);
    }
  }

  // dessine les points
  if (V) {
    glColor3f(1, 0, 0); // Rouge
    for (int i = 0; i < n; i++)
      drawPoint(V[i]);
    if (root && P && (P[0]>=0) ){
      glColor3f(ORANGE); // Orange
      drawPoint(V[P[0]]);
    }
  }

  // affiche le dessin
  SDL_GL_SwapWindow(window);
}

void drawPath(point *V, int n, int *P, int k) {
  static unsigned int last_tick = 0;

  // saute le dessin si le précédent a été fait il y a moins de 20ms
  // ou si update est faux
  if ((!update) && (last_tick + 20 > SDL_GetTicks()))
    return;
  last_tick = SDL_GetTicks();

  // gestion de la file d'event
  handleEvent(false);

  // efface la fenêtre
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // dessine le chemin
  if (V && P && (P[0]>=0)) {
    glColor3f(0, 1, 0); // Vert
    for (int i = 0; i < k - 1; i++){
      if(oriented)
        drawEdge(V[P[i]], V[P[(i + 1) % n]]);
      else
        drawLine(V[P[i]], V[P[(i + 1) % n]]);
    }
    if (root) {
      glColor3f(ORANGE); // Orange
      if(oriented && (n>0))
        drawEdge(V[P[0]], V[P[1]]);
      else
        drawLine(V[P[0]], V[P[1]]);
    }
  }

  // dessine les points
  if (V) {
    glColor3f(1, 0, 0); // Rouge
    for (int i = 0; i < n; i++)
      drawPoint(V[i]);
    if (root && P && (P[0]>=0) ){
      glColor3f(ORANGE); // Orange
      drawPoint(V[P[0]]);
    }
  }

  // affiche le dessin
  SDL_GL_SwapWindow(window);
}

// Dessine le graphe G, les points V et la tournée définie par Q
void drawGraph(point *V, int n, int *P, graph G) {
  static unsigned int last_tick = 0;

  // saute le dessin si le précédent a été fait il y a moins de 20ms
  // ou si update est faux
  if ((!update) && (last_tick + 20 > SDL_GetTicks()))
    return;
  last_tick = SDL_GetTicks();

  // gestion de la file d'event
  handleEvent(false);

  // efface la fenêtre
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // dessine G
  if (G.list) {
    glLineWidth(5.0);
    glColor3f(0, 0.4, 0); // Vert foncé
    for (int i = 0; i < n; i++)
      for (int j = 0; j < G.deg[i]; j++)
        if (i < G.list[i][j])
          drawLine(V[i], V[G.list[i][j]]);
    glLineWidth(1.0);
  }

  // dessine la tournée
  if (V && P && (P[0]>=0)) {
    glColor3f(1, 1, 1); // Blanc
    for (int i = 0; i < n; i++){
      if(oriented)
        drawEdge(V[P[i]], V[P[(i + 1) % n]]);
      else
        drawLine(V[P[i]], V[P[(i + 1) % n]]);
    }
    if (root) {
      glColor3f(ORANGE); // Orange
      if(oriented && (n>0))
        drawEdge(V[P[0]], V[P[1]]);
      else
        drawLine(V[P[0]], V[P[1]]);
    }
  }

  // dessine les points
  if (V) {
    glColor3f(1, 0, 0); // Rouge
    for (int i = 0; i < n; i++)
      drawPoint(V[i]);
    if (root && P && (P[0]>=0) ){
      glColor3f(ORANGE); // Orange
      drawPoint(V[P[0]]);
    }
  }

  // affiche le dessin
  SDL_GL_SwapWindow(window);
}
#undef ORANGE

static void drawGridImage(grid G){
  // Efface la fenêtre
  glClearColor(0, 0, 0, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  // Dessin
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, G.X, G.Y, 0, GL_RGB, GL_UNSIGNED_BYTE, gridImage);
  glEnable(GL_TEXTURE_2D);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
  glBindTexture(GL_TEXTURE_2D, texName);
  glBegin(GL_QUADS);
  glTexCoord2f(0.0, 0.0);
  glVertex3f(0, 0, 0);
  glTexCoord2f(0.0, 1.0);
  glVertex3f(0, G.Y, 0);
  glTexCoord2f(1.0, 1.0);
  glVertex3f(G.X, G.Y, 0);
  glTexCoord2f(1.0, 0.0);
  glVertex3f(G.X, 0, 0);
  glEnd();
  glFlush();
  glDisable(GL_TEXTURE_2D);
}

void drawGrid(grid G) {
  static unsigned int last_tick = 0;

  static unsigned int last_call = 0;
  static unsigned int call_count = 0;

  const unsigned int frame_rate = 50;

  unsigned int call_per_frame = call_speed / frame_rate;

  call_count++;

  unsigned int current_tick = SDL_GetTicks();

  if (!call_speed || frame_rate < call_speed)
  {
    if(!update){
      if(call_speed){
        if(call_count - last_call > call_per_frame) {
          SDL_Delay(last_tick + 1000/frame_rate - current_tick);
          current_tick = SDL_GetTicks();
        }
      }
    }
    if(update || last_tick + 1000/frame_rate <= current_tick) {
      makeImage(&G);
      handleEvent(false);

      drawGridImage(G);

      // Affiche le résultat puis attend un certain délais
      SDL_GL_SwapWindow(window);
      last_tick = current_tick;
      last_call = call_count;
    }
  }
  else{
    int delay = 1000/call_speed;
    makeImage(&G);
    while(current_tick - last_tick < delay) {
      handleEvent(false);
      drawGridImage(G);

      // Affiche le résultat puis attend un certain délais
      SDL_GL_SwapWindow(window);
      SDL_Delay(1000/frame_rate);
      current_tick = SDL_GetTicks();
    }
    last_tick = current_tick;
  }
}

bool handleEvent(bool wait_event) {
  bool vertices_have_changed = false;
  SDL_Event e;

  if (wait_event)
    SDL_WaitEvent(&e);
  else if (!SDL_PollEvent(&e))
    return false;

  do {
    switch (e.type) {

    case SDL_QUIT:
      running = false;
      update = false;
      speedSet(speedMax());
      break;

    case SDL_KEYDOWN:
      if (e.key.keysym.sym == SDLK_q) {
        running = false;
        update = false;
        speedSet(speedMax());
        break;
      }
      if (e.key.keysym.sym == SDLK_p) {
        SDL_Delay(500);
        SDL_WaitEvent(&e);
        break;
      }
      if (e.key.keysym.sym == SDLK_c) {
        erase = !erase;
        break;
      }
      if (e.key.keysym.sym == SDLK_o) {
        oriented = !oriented;
        break;
      }
      if (e.key.keysym.sym == SDLK_r) {
        root = !root;
        break;
      }
      if (e.key.keysym.sym == SDLK_z || e.key.keysym.sym == SDLK_KP_MINUS) {
        speedDown();
        break;
      }
      if (e.key.keysym.sym == SDLK_a || e.key.keysym.sym == SDLK_KP_PLUS) {
        speedUp();
        break;
      }
      break;
    case SDL_WINDOWEVENT:
      if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
        double x, y;
        getCenterCoord(&x, &y);
        glViewport(0, 0, e.window.data1, e.window.data2);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, e.window.data1, e.window.data2, 0.0f, 0.0f, 1.0f);
        glTranslatef(e.window.data1 / 2 - x, e.window.data2 / 2 - y, 0.0f);
        zoomAt(scale, x, y);
	SDL_GetWindowSize(window, &width, &height);
	SDL_SetWindowTitle(window, getTitle());
      }
      break;

    case SDL_MOUSEWHEEL:
      if (e.wheel.y > 0) zoomMouse(2.0);
      if (e.wheel.y < 0) zoomMouse(0.5);
      SDL_GetWindowSize(window, &width, &height);
      SDL_SetWindowTitle(window, getTitle());
      break;

    case SDL_MOUSEBUTTONDOWN:
      if (e.button.button == SDL_BUTTON_LEFT) {
        double x, y;
        pixelToCoord(e.motion.x, e.motion.y, &x, &y);
        if (hover) {
          int v = getClosestVertex(x, y);
          if ((x - vertices[v].x) * (x - vertices[v].x) +
                  (y - vertices[v].y) * (y - vertices[v].y) <
              30.0f)
            selectedVertex = v;
        }
        mouse_ldown = true;
      }
      if (e.button.button == SDL_BUTTON_RIGHT)
        mouse_rdown = true;
      break;

    case SDL_MOUSEBUTTONUP:
      if (e.button.button == SDL_BUTTON_LEFT) {
        selectedVertex = -1;
        mouse_ldown = false;
      }
      if (e.button.button == SDL_BUTTON_RIGHT)
        mouse_rdown = false;
      break;

    case SDL_MOUSEMOTION:
      if (hover && !mouse_rdown && mouse_ldown &&
          selectedVertex >= 0) {
        pixelToCoord(e.motion.x, e.motion.y, &(vertices[selectedVertex].x),
                     &(vertices[selectedVertex].y));
        vertices_have_changed = true;
      }
      if (mouse_rdown) {
        glTranslatef(e.motion.xrel / scale, e.motion.yrel / scale, 0);
      }
      break;

    }
  } while (SDL_PollEvent(&e));

  return vertices_have_changed;
}

