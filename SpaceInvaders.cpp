#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <time.h>
#include "GrilleSDL.h"
#include "Ressources.h"

// Dimensions de la grille de jeu
#define NB_LIGNE   18
#define NB_COLONNE 23 

#define LIGNE_BAS NB_LIGNE - 1

// Direction de mouvement
#define GAUCHE       10
#define DROITE       11
#define BAS          12

// Intervenants du jeu (type)
#define VIDE        0
#define VAISSEAU    1
#define MISSILE     2
#define ALIEN       3
#define BOMBE       4
#define BOUCLIER1   5
#define BOUCLIER2   6
#define AMIRAL      7

typedef struct
{
  int type;
  pthread_t tid;
} S_CASE;

typedef struct 
{
  int L;
  int C;
} S_POSITION;
/*VARIABLES GLOBALES*/
short colonne;
bool fireOn = true;
S_CASE tab[NB_LIGNE][NB_COLONNE];
int nbAliens = 24, lh = 2, cg = 8, lb = 8,cd = 18;
short delai = 1000;
/**********************/
void Attente(int milli);
void setTab(int l,int c,int type=VIDE,pthread_t tid=0);

///////////////////////////////////////////////////////////////////////////////////////////////////
void Attente(int milli)
{
  struct timespec del;
  del.tv_sec = milli/1000;
  del.tv_nsec = (milli%1000)*1000000;
  nanosleep(&del,NULL);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void setTab(int l,int c,int type,pthread_t tid)
{
  tab[l][c].type = type;
  tab[l][c].tid = tid;
}
/////////////HANDLERS DE SIGNAUX///////////

void HandlerSIGUSR1(int sig);
void HandlerSIGUSR2(int sig);
void HandlerSIGHUP(int sig);


/////////////MUTEX/////////////////

pthread_mutex_t mutexGrille, mutexFlotteAliens;


/////////////handle////////////////////
pthread_t pthread_vaisseau, pthread_event, pthread_missile,pthread_TimeOut, pthread_Invader,pthread_FlotteAliens;


//////////////fct threads//////////////////

void * fctThreadVaisseau();
void * fctThreadEvent();
void * fctThreadTimeOut();
void * fctThreadMissile(S_POSITION*);
void * fctThreadInvader();
void * fctThreadFlotteAliens();
///////////////////////////////////////////////////////////////////////////////////////////////////
int main(int argc,char* argv[])
{
  EVENT_GRILLE_SDL event;
 
  srand((unsigned)time(NULL));

  // Ouverture de la fenetre graphique
  printf("(MAIN %d) Ouverture de la fenetre graphique\n",pthread_self()); fflush(stdout);
  if (OuvertureFenetreGraphique() < 0)
  {
    printf("Erreur de OuvrirGrilleSDL\n");
    fflush(stdout);
    exit(1);
  }

  // Initialisation des mutex et variables de condition
  // TO DO
  pthread_mutex_init(&mutexGrille,NULL);
  pthread_mutex_init(&mutexFlotteAliens,NULL);

  // Armement des signaux
  // TO DO
  sigset_t mask;
  struct sigaction sign;
  //SIGUSR1
  sign.sa_handler = HandlerSIGUSR1;
  sigemptyset(&sign.sa_mask);           //armement du signal SIGUSR1 sur notre handler
  sign.sa_flags = 2;  
  sigaction(SIGUSR1,&sign,NULL);
  //SIGUSR2
  sign.sa_handler = HandlerSIGUSR2;
  sigemptyset(&sign.sa_mask);           //armement du signal SIGUSR2 sur notre handler
  sign.sa_flags = 2;  
  sigaction(SIGUSR2,&sign,NULL);
  //SIGHUP
  sign.sa_handler = HandlerSIGHUP;
  sigemptyset(&sign.sa_mask);           //armement du signal SIGHUP sur notre handler
  sign.sa_flags = 2;  
  sigaction(SIGHUP,&sign,NULL);


  // Initialisation de tab
  for (int l=0 ; l<NB_LIGNE ; l++)
    for (int c=0 ; c<NB_COLONNE ; c++)
      setTab(l,c);

  // Initialisation des boucliers
  setTab(NB_LIGNE-2,11,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,11,1);
  setTab(NB_LIGNE-2,12,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,12,1);
  setTab(NB_LIGNE-2,13,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,13,1);
  setTab(NB_LIGNE-2,17,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,17,1);
  setTab(NB_LIGNE-2,18,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,18,1);
  setTab(NB_LIGNE-2,19,BOUCLIER1,0);  DessineBouclier(NB_LIGNE-2,19,1);

  // Creation des threads
  // TO DO

  pthread_create(&pthread_vaisseau,NULL,(void*(*)(void*))fctThreadVaisseau,NULL); //thread_vaisso
  pthread_create(&pthread_event,NULL,(void*(*)(void*))fctThreadEvent,NULL); 
  pthread_create(&pthread_Invader,NULL,(void*(*)(void*))fctThreadInvader,NULL); 
  //MASK
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigprocmask(SIG_SETMASK, &mask, NULL);

  // Exemples d'utilisation du module Ressources --> a supprimer


  printf("(MAIN %d) Attente du clic sur la croix\n",pthread_self());  
  bool ok = false;
  while(!ok)
  {
    event = ReadEvent();
    if (event.type == CROIX) ok = true;
    if (event.type == CLIC_GAUCHE)
    {
      if (tab[event.ligne][event.colonne].type == VIDE) 
      {
        DessineVaisseau(event.ligne,event.colonne);
        setTab(event.ligne,event.colonne,VAISSEAU,pthread_self());
      }
    }
    if (event.type == CLAVIER)
    {
      if (event.touche == KEY_RIGHT) printf("Flèche droite enfoncée\n");
      printf("Touche enfoncee : %c\n",event.touche);
    }
  }

  // Fermeture de la fenetre
  printf("(MAIN %d) Fermeture de la fenetre graphique...",pthread_self()); fflush(stdout);
  FermetureFenetreGraphique();
  printf("OK\n");

  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////


void * fctThreadVaisseau()
{
  printf("THREAD VAISSEAU CREE, TID = %d\n",pthread_self());
  int i = 8;

  pthread_mutex_lock(&mutexGrille);
  if(tab[NB_LIGNE - 1][15].type == VIDE)
  {
    tab[NB_LIGNE - 1][15].type = VAISSEAU;
    tab[NB_LIGNE - 1][15].tid = pthread_self();
    DessineVaisseau(NB_LIGNE - 1,15);
    pthread_mutex_unlock(&mutexGrille); 
    colonne = 15;
  }
  else
  {
    while(i < 22 && tab[NB_LIGNE - 1][i].type != VIDE)
      i++;
    pthread_mutex_unlock(&mutexGrille);  
    if(i < 22){
      pthread_mutex_lock(&mutexGrille);
      tab[NB_LIGNE - 1][i].type = VAISSEAU;
      tab[NB_LIGNE - 1][i].tid = pthread_self();
      DessineVaisseau(NB_LIGNE - 1,i);
      pthread_mutex_unlock(&mutexGrille);
      colonne = i;
    }
  }
   
  i=0;
  while(true)
    pause();
  pthread_exit(NULL);
}


void HandlerSIGUSR1(int sig)
{
  printf("SIGNAL SIGUSR1 RECU PAR LE THREAD %d \n",pthread_self());
  
  if(colonne - 1 >= 8)
  {
    pthread_mutex_lock(&mutexGrille);
    if(tab[LIGNE_BAS][colonne - 1].type == VIDE)
    {

      tab[LIGNE_BAS][colonne].type = VIDE;
      tab[LIGNE_BAS][colonne].tid = 0;    // efface
      EffaceCarre(LIGNE_BAS,colonne);

      tab[LIGNE_BAS][colonne - 1].type = VAISSEAU; 
      tab[LIGNE_BAS][colonne - 1].tid = pthread_self();   //new
      colonne -= 1;
      DessineVaisseau(LIGNE_BAS,colonne);

      printf("Movement Effectué! -> COloonne = %d\n",colonne);
    }
    pthread_mutex_unlock(&mutexGrille);
  }
}
void HandlerSIGUSR2(int sig)
{
  printf("SIGNAL SIGUSR2 RECU PAR LE THREAD %d \n",pthread_self());
  
  if(colonne + 1 <= 22 )
  {
    pthread_mutex_lock(&mutexGrille);
    if(tab[LIGNE_BAS][colonne + 1 ].type == 0)
    {
      tab[LIGNE_BAS][colonne].type = VIDE;    // efface
      tab[LIGNE_BAS][colonne].tid = 0;
      EffaceCarre(LIGNE_BAS,colonne);

      tab[LIGNE_BAS][colonne + 1].type = VAISSEAU;  // nouveau 
      tab[LIGNE_BAS][colonne + 1].tid = pthread_self();
      colonne += 1;
      DessineVaisseau(LIGNE_BAS,colonne);
    }
    pthread_mutex_unlock(&mutexGrille);
  }
  
}
void HandlerSIGHUP(int sig)
{
  
  
  S_POSITION *pos;
  pos = (S_POSITION*) malloc(sizeof(S_POSITION));
  pos->L = LIGNE_BAS - 1;
  pos->C = colonne ;
  pthread_create(&pthread_missile,NULL,(void*(*)(void*))fctThreadMissile,pos);  //FREE
  fireOn = false;
  pthread_create(&pthread_TimeOut,NULL,(void*(*)(void*))fctThreadTimeOut,NULL); //?


}


void * fctThreadEvent()
{
  char boolean = 1;
  EVENT_GRILLE_SDL event;
  while(boolean)
  {
    event = ReadEvent();
    if(event.type == CROIX) boolean = 0;
    else if(event.type == CLAVIER)
    {
      switch(event.touche)
      {
        case KEY_LEFT:
          kill(getpid(),SIGUSR1);
          break;
        case KEY_RIGHT:
          kill(getpid(),SIGUSR2);
          break;
        case KEY_SPACE:
          kill(getpid(),SIGHUP);
          break;
      }
    }
    else;
  }
  printf("\n\nLe thread EVENT se termine!\n\n");
  FermetureFenetreGraphique();
  exit(0);
}


void * fctThreadMissile(S_POSITION* pos)
{

  pthread_mutex_lock(&mutexGrille);
  if(tab[pos->L][pos->C].type == BOUCLIER1)
  {
    tab[pos->L][pos->C].type = BOUCLIER2;
    EffaceCarre(pos->L,pos->C);
    DessineBouclier(pos->L,pos->C,2);             //si le missile spawn sur un bouclier1 alors bouclier1 -> bouclier2
    free(pos);
    pthread_exit(NULL);
  }
  else if(tab[pos->L][pos->C].type == BOUCLIER2)
  {
    tab[pos->L][pos->C].type = VIDE;
    tab[pos->L][pos->C].tid = 0;                  //si le missile spawn sur un bouclier2 alors bouclier2 -> Vide
    EffaceCarre(pos->L,pos->C);
    free(pos);
    pthread_exit(NULL);
  }
  else if(tab[pos->L][pos->C].type == VIDE)
  {
    tab[pos->L][pos->C].type = MISSILE;
    tab[pos->L][pos->C].tid = pthread_self();     // si le missile spawn sur VIDE alors il monte...
    DessineMissile(pos->L,pos->C);
    pthread_mutex_unlock(&mutexGrille);
    while(pos->L > 0)
    {
      Attente(80);
      pthread_mutex_lock(&mutexGrille);
      tab[pos->L][pos->C].type = VIDE;
      tab[pos->L][pos->C].tid = 0;
      EffaceCarre(pos->L,pos->C);                   //le missile monte...
      pos->L--;
      tab[pos->L][pos->C].type = MISSILE;
      tab[pos->L][pos->C].tid = pthread_self();
      DessineMissile(pos->L,pos->C);
      pthread_mutex_unlock(&mutexGrille);
    }
    pthread_mutex_lock(&mutexGrille);
    tab[pos->L][pos->C].type = VIDE;
    tab[pos->L][pos->C].tid = 0;
    EffaceCarre(pos->L,pos->C);
    pthread_mutex_unlock(&mutexGrille);

    free(pos);
    pthread_exit(NULL);
  }  

}

void * fctThreadTimeOut()
{
  Attente(600);
  fireOn = true;                                // je ne sais point..
  pthread_exit(NULL);
}

void * fctThreadInvader()
{
  pthread_create(&pthread_FlotteAliens,NULL,(void*(*)(void*))fctThreadFlotteAliens,NULL);
  pthread_join(pthread_FlotteAliens,(void **)NULL);

}

void * fctThreadFlotteAliens()
{
  char i = 2,j = 8,k = 0;
  int tidThread = pthread_self();
  for(;i < 10; i++)     // i < 4 mais on commence a ligne 2 + ligne vide -> 10
  {
    pthread_mutex_lock(&mutexGrille);
    for(;j < 14;j++) // on commence aek j = 8 ; or 6 aliens par ligne 6 + 8 -> 14
    {
      tab[i][j + k].type = ALIEN;
      tab[i][j + k].tid = tidThread;
      DessineAlien(i,j + k);
      k++;
    }
    pthread_mutex_unlock(&mutexGrille);
    i++;
    j = 8;
    k = 0;
  }

  

  while(nbAliens && lb < LIGNE_BAS - 2)
  {
    
    //nbAliens--;
    
    
    while(cd < 22)  //22 car on avance par pas de deux
    {
      Attente(delai);
      pthread_mutex_lock(&mutexGrille);
      for(i = lh,j = cg; i < lb;i++)
      {
        
        tab[i][j].type = VIDE;
        tab[i][j].tid = 0;
        EffaceCarre(i,j);
        i++;
      }
      pthread_mutex_unlock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      cg = cg + 2;
      pthread_mutex_unlock(&mutexFlotteAliens);
      pthread_mutex_lock(&mutexGrille);
      for(i = lh, j = cd; i < lb;i++)
      {
        tab[i][j].type = ALIEN;
        tab[i][j].tid = tidThread;
        DessineAlien(i,j);
        i++;
      }
      pthread_mutex_unlock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      cd = cd + 2;
      pthread_mutex_unlock(&mutexFlotteAliens);
    }

    
    while(cg > 8)  
    {
      Attente(delai);
      pthread_mutex_lock(&mutexGrille);
      for(i = lh,j = cd; i < lb;i++)
      {
        
        tab[i][j].type = VIDE;
        tab[i][j].tid = 0;
        EffaceCarre(i,j);
        i++;
      }
      pthread_mutex_unlock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      cd = cd - 2;
      pthread_mutex_unlock(&mutexFlotteAliens);
      pthread_mutex_lock(&mutexGrille);
      for(i = lh, j = cg; i < lb ;i++)
      {
        tab[i][j].type = ALIEN;
        tab[i][j].tid = tidThread;
        DessineAlien(i,j);
        i++;
      }
      pthread_mutex_unlock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      cg = cg - 2;
      pthread_mutex_unlock(&mutexFlotteAliens);
    }
    
    
    pthread_mutex_lock(&mutexGrille);
    for(i = lh, j = cg; j < cd;j+=2)
    {
      tab[i][j].type = VIDE;
      tab[i][j].tid = 0;
      EffaceCarre(i,j);
    }
    pthread_mutex_unlock(&mutexGrille);
    pthread_mutex_lock(&mutexFlotteAliens);
    lh += 2;
    pthread_mutex_unlock(&mutexFlotteAliens);
    pthread_mutex_lock(&mutexGrille);
    for(i = lb, j = cg; j < cd;j+=2)
    {
      tab[i][j].type = ALIEN;
      tab[i][j].tid = tidThread;
      DessineAlien(i,j);
    }
    pthread_mutex_unlock(&mutexGrille);
    pthread_mutex_lock(&mutexFlotteAliens);
    lb += 2;
    pthread_mutex_unlock(&mutexFlotteAliens);
    

  }
  printf("\n\nlh = %d lb = %d cg = %d cd = %d\n\n",lh,lb,cg,cd);
  for(i=lh;i < lb;i+=2)
  {
    for(j=cg;j < cd;j+=2)
    {
      tab[i][j].type = VIDE;
      tab[i][j].tid = 0;
      EffaceCarre(i,j);    
    }
  }
  printf("\nLe thread FlotteAliens se termine\n");
  pthread_exit(NULL);

}