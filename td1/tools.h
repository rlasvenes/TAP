#ifndef _TOOLS_H
#define _TOOLS_H

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <float.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <limits.h>

#ifdef __APPLE__
#include <OpenGL/glu.h>
#else
#include <GL/glu.h>
#endif
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

// échange les variables x et y, via z
#define SWAP(x, y, z)  (z) = (x), (x) = (y), (y) = (z)


////////////////////////
//
// SPÉCIFIQUE POUR TSP
//
////////////////////////


// Un point (x,y).
typedef struct {
  double x, y;
} point;

// Permet de construire la prochaine permutation de P sur 0..n-1 dans
// l'ordre lexicographique. Le résultat est écrit dans P. Si à l'appel
// P est la dernière permutation, soit {n-1..0}, alors P repasse à la
// plus petite, soit {0..n-1}, et "true" est renvoyé. Dans les autres
// cas "false" est renvoyée.
bool NextPermutation(int *P, const int n);

// Primitives de dessin.
point *generatePoints(int n); // n points au hasard
point *generateCircles(int n, int k); // n points au hasard sur n cercles
void drawTour(point *V, int n, int *P); // affichage de la tournée P
void drawPath(point *V, int n, int *P, int k); // affiche les k premier points

// Un graphe G (pour MST).
typedef struct {
  int n;         // n=nombre de sommets
  int *deg;      // deg[u]=nombre de voisins du sommet u
  int **list;    // list[u][i]=i-ème voisin de u
} graph;

void drawGraph(point *V, int n, int *P, graph G); // affiche l'arbre et la tournée


////////////////////////
//
// SPÉCIFIQUE POUR A*
//
////////////////////////


// Une position entière dans la grille.
typedef struct {
  int x, y;
} position;

// Une grille.
typedef struct {
  int X, Y;       // dimensions: X et Y
  int **value;    // valuation des cases: value[i][j], 0<=i<X, 0<=j<Y
  int **mark;     // marquage des cases: mark[i][j], 0<=i<X, 0<=j<Y
  position start; // position de la source
  position end;   // position de la destination
} grid;

// Valeurs possibles des cases d'une grille pour les champs .value et
// .mark. L'ordre est important: il doit être cohérent avec les
// tableaux color[] (tools.c) et weight[] (a_star.c).
enum {

  // pour .value
  V_FREE = 0, // case vide
  V_WALL,     // Mur
  V_SAND,     // Sable
  V_WATER,    // Eau
  V_MUD,      // Boue
  V_GRASS,    // Herbe
  V_TUNNEL,   // Tunnel

  // pour .mark
  M_NULL,  // sommet non marqué
  M_USED,  // sommet marqué dans P
  M_FRONT, // sommet marqué dans Q
  M_PATH,  // sommet dans le chemin

  // divers
  C_START,    // couleur de la position de départ
  C_END,      // idem pour la destination
  C_FINAL,    // couleur de fin du dégradé pour M_USED
  C_END_WALL, // couleur de destination si sur V_WALL

  // extra
  M_USED2,  // sommet marqué dans P_t
  C_FINAL2, // couleur de fin du dégradé pour M_USED2
};

// Routine de dessin et de construction de grilles. Le point (0,0) de
// la grille est le coin en haut à gauche.

void drawGrid(grid); // affiche une grille
grid initGridLaby(int,int,int w); // construit un labyrithne x,y,w
grid initGridPoints(int,int,int t,double p); // point aléatoires d'un type et proba donnés
grid initGridFile(char*); // construit une grille depuis un fichier

// ajoute à une grille n "blobs" de type donné t
void addRandomBlob(grid,int t,int n);

void freeGrid(grid); // libère la mémoire alouée par une grille


////////////////////////
//
// SPÉCIFIQUE POUR SDL
//
////////////////////////


// Quelques variables globales.
bool update;        // si vrai, force le dessin dans les fonctions drawXXX()
int width, height;  // taille de la fenêtre, mise à jour si redimensionnée
bool running;       // devient faux si 'q' est pressé
bool hover;         // si vrai, permet de déplacer un sommet
bool erase;         // pour A*: efface les couleurs à la fin ou pas
int delay;          // pour A*: délais d'affichage pour drawGrid(), unité = 0"01
double scale;       // zoom courrant, 1 par défaut

// Initialisation de SDL et OpenGL.
void init_SDL_OpenGL(void);

// Libération de la mémoire.
void cleaning_SDL_OpenGL(void);

// Gestion des évènements (souris, touche clavier, redimensionnement
// de la fenêtre, etc.). Renvoie true ssi la position d'un point a
// changé. Si wait_event = true, alors on attend qu'un évènement se
// produise (et on le gère). Sinon, on n'attend pas et on gère
// l'évènement courant.
//
//  q -> passe running à false
//  o -> indique l'orientation de la tournée dans drawXxx()
//  r -> indique le point de départ de la tournée dans drawXxx()
//  p -> pause de 0"5, maintenir pour pause plus longue
//  c -> maintient ou supprime les sommets visités à la fin de A*
//  + ou a -> accélère drawGrid() en diminuant delay, pour A*
//  - ou z -> ralentis drawGrid() en augmentant delay, pour A*
//
bool handleEvent(bool wait_event);

// fonction de chronométrage. Renvoie une durée (écrite sous forme de
// texte) depuis le dernier apppel à la fonction TopChrono(i) où i est
// le numéro de chronomètre souhaité.
char *TopChrono(const int i);

#endif
