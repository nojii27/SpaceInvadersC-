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
double delai = 1000;
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
void HandlerSIGINT(int sig);
void HandlerSIGQUIT(int sig);

/////////////MUTEX/////////////////

pthread_mutex_t mutexGrille, mutexFlotteAliens;


/////////////handle////////////////////
pthread_t pthread_vaisseau, pthread_event, pthread_missile,pthread_TimeOut, pthread_Invader,pthread_FlotteAliens;

//prototypes de fonctions
void RechercheExtremite();
int DeplacementXY();
void DoubleCheck();
void DeleteFlotte();
void SpawnBouclier();
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
  //SIGINT
  sign.sa_handler = HandlerSIGINT;
  sigemptyset(&sign.sa_mask);           //armement du signal SIGINT sur notre handler
  sign.sa_flags = 2;  
  sigaction(SIGINT,&sign,NULL);
  sign.sa_handler = HandlerSIGQUIT;
  sigemptyset(&sign.sa_mask);           //armement du signal SIGINT sur notre handler
  sign.sa_flags = 2;  
  sigaction(SIGQUIT,&sign,NULL);


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
  sigaddset(&mask,SIGINT);
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

  sigset_t mask;sigemptyset(&mask);                     
  sigaddset(&mask,SIGINT);   //masquage signal SIGINT
  sigprocmask(SIG_SETMASK, &mask, NULL);
  printf("THREAD VAISSEAU CREE, TID = %d\n",pthread_self());
  int i = cg;

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
   
  i = 0;
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

      printf("Movement Effectué! -> Colonne = %d\n",colonne);
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
  printf("\nSIGNAL SIGHUP RECU PAR LE THREAD %d\n",pthread_self());
  fireOn = false;
  pthread_create(&pthread_TimeOut,NULL,(void*(*)(void*))fctThreadTimeOut,NULL);
  
  S_POSITION *pos;
  pos = (S_POSITION*) malloc(sizeof(S_POSITION));
  pos->L = LIGNE_BAS - 1;
  pos->C = colonne ;
  pthread_create(&pthread_missile,NULL,(void*(*)(void*))fctThreadMissile,pos);  //FREE
}


void * fctThreadEvent()
{
  sigset_t mask;sigemptyset(&mask);
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK, &mask, NULL);
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
  printf("\nnbALIENS %d\n",nbAliens);
  sigset_t mask;sigemptyset(&mask);
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigprocmask(SIG_SETMASK, &mask, NULL);
  printf("\nTHREAD MISSILE TID = %d\n",pthread_self());

  pthread_mutex_lock(&mutexGrille);
  if(tab[pos->L][pos->C].type == BOUCLIER1)
  {
    tab[pos->L][pos->C].type = BOUCLIER2;
    EffaceCarre(pos->L,pos->C);
    DessineBouclier(pos->L,pos->C,2);             //si le missile spawn sur un bouclier1 alors bouclier1 -> bouclier2
    pthread_mutex_unlock(&mutexGrille);
    free(pos);
    pthread_exit(NULL);
  }
  else if(tab[pos->L][pos->C].type == BOUCLIER2)
  {
    tab[pos->L][pos->C].type = VIDE;
    tab[pos->L][pos->C].tid = 0;                  //si le missile spawn sur un bouclier2 alors bouclier2 -> Vide
    EffaceCarre(pos->L,pos->C);
    pthread_mutex_unlock(&mutexGrille);
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
      EffaceCarre(pos->L,pos->C);
      if(tab[pos->L - 1][pos->C].type == ALIEN)
      {
        tab[pos->L - 1][pos->C].type = VIDE;
        tab[pos->L - 1][pos->C].tid = 0;        // le missile a touché..
        EffaceCarre(pos->L - 1,pos->C);
        nbAliens--;
        free(pos);
        pthread_mutex_unlock(&mutexGrille);
        RechercheExtremite();
        pthread_exit(NULL);
      }
      else
      {
        pos->L--;         //le missile monte...
        tab[pos->L][pos->C].type = MISSILE;
        tab[pos->L][pos->C].tid = pthread_self();
        DessineMissile(pos->L,pos->C);
      }
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
  sigset_t mask;sigemptyset(&mask);
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x) SIGINT
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigprocmask(SIG_SETMASK, &mask, NULL);
  bool continueOrNot = true;
  float level = 0;
  int b = 0;
  DessineChiffre(13,4,(int)level);
  DessineChiffre(13,3,(int)level);
  while(continueOrNot)
  {
    nbAliens = 24;
    pthread_create(&pthread_FlotteAliens,NULL,(void*(*)(void*))fctThreadFlotteAliens,NULL);
    pthread_join(pthread_FlotteAliens,(void **)NULL);
    printf("CACA");
    if(nbAliens == 0)
    {
      printf("DEBUT IF APRES THREADALIEN ");
      ++level;
      delai -= delai * 0.3;
      if((level + 1 / 10) > 1)                    // affichage deux chiffres
      {
        b = ((level * 10) - ((int)(level / 10) * 100 ) ) / 10;
        DessineChiffre(13,3,(int)level / 10);
        DessineChiffre(13,4,b);
      }
      else                                    // affichage 1 chiffre
        DessineChiffre(13,4,(int)level);
    }
    else
    {
      pthread_mutex_lock(&mutexGrille);
      pthread_kill(tab[LIGNE_BAS][colonne].tid,SIGQUIT);
      pthread_mutex_unlock(&mutexGrille);
      continueOrNot = false;
    }
    printf("FIN FIN AVANT DELE");
    DeleteFlotte();
    SpawnBouclier();
  }


}

void * fctThreadFlotteAliens()
{
  char i,j;
  int tidThread = pthread_self();
  pthread_mutex_lock(&mutexGrille);
  for(i = lh;i <= lb; i+=2)     // i < 4 mais on commence a ligne 2 + ligne vide -> 10
  {
    
    for(j = cg;j <= cd;j+=2) // on commence aek j = 8 ; or 6 aliens par ligne 6 + 8 -> 14
    {
      tab[i][j].type = ALIEN;
      tab[i][j].tid = tidThread;
      DessineAlien(i,j);

    }

  }
  pthread_mutex_unlock(&mutexGrille);

  if(DeplacementXY() == 1)
  {
    printf("\nVous avez Remporte le niveau!\n");
    //pthread_exit(NULL);
  }
  else
  {
    printf("\nVous avez perdu!\n");
    pthread_exit(NULL);
  }


}


void RechercheExtremite()
{
  int minVertical = NB_LIGNE ,maxVertical = 0,minHorizontal = NB_COLONNE,maxHorizontal = 0;
  int i,j;
  if(nbAliens > 0)
  {
    for(i = lh; i <= lb;i+=2)
    {
      for(j = cg;j <= cd;j+=2)
      {
        if(tab[i][j].type == ALIEN)
        {
          if(minHorizontal > j)
            minHorizontal = j;
          if(maxHorizontal < j)
            maxHorizontal = j;
          
          if(minVertical > i)
            minVertical = i;
          if(maxVertical < i)
            maxVertical = i;
        }  
      }
    }
    pthread_mutex_lock(&mutexFlotteAliens);
    lh = minVertical;
    lb = maxVertical;
    cg = minHorizontal;
    cd = maxHorizontal;
    pthread_mutex_unlock(&mutexFlotteAliens);
  }
}


void HandlerSIGINT(int sig)
{
  printf("\nSIGNAL SIGINT RECU PAR %d\n",pthread_self());
  pthread_mutex_unlock(&mutexGrille);
  pthread_exit(NULL);
}



int DeplacementXY()
{
  char k = 1,i,j;
  int tidThread = pthread_self();
  int tmpAlien = nbAliens;
  while(nbAliens && lb <= LIGNE_BAS - 1)
  {
    tmpAlien = nbAliens;
    while(cd < 22)
    {
      //Deplacement droite
      Attente(delai);
      pthread_mutex_lock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      if(nbAliens > 0)
      {
        for(i = lh;i <= lb;i+=k)
        {
          for(j = cd; j >= cg; j-=k)
          {
            if(tab[i][j+k].type == MISSILE)
            {
              nbAliens--;
              tab[i][j].type = VIDE;
              tab[i][j].tid = 0;
              EffaceCarre(i,j);
              EffaceCarre(i,j+k);
              pthread_kill(tab[i][j+k].tid,SIGINT);
              tab[i][j+k].type = VIDE;
              tab[i][j+k].tid = 0;
            }
            
            if(tab[i][j].type == ALIEN)
            {
              tab[i][j].type = VIDE;
              tab[i][j].tid = 0;
              EffaceCarre(i,j);

              tab[i][j+k].type = ALIEN;
              tab[i][j+k].tid = tidThread;
              DessineAlien(i,j+k); 
            }
          }
        }
      }

      pthread_mutex_unlock(&mutexGrille);
      cg += k;
      cd += k;
      pthread_mutex_unlock(&mutexFlotteAliens);
      RechercheExtremite();
      tmpAlien = nbAliens;
    }
    while(cg > 8) //deplacement gauche
    {
      Attente(delai);
      pthread_mutex_lock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      if(nbAliens > 0)
      {
        for(i = lh;i <= lb;i+=k)
        {
          for(j = cg; j <= cd; j+=k)
          {

            if(tab[i][j-k].type == MISSILE)
            {
              nbAliens--;
              tab[i][j].type = VIDE;
              tab[i][j].tid = 0;
              EffaceCarre(i,j);
              EffaceCarre(i,j-k);
              pthread_kill(tab[i][j-k].tid,SIGINT);
              tab[i][j-k].type = VIDE;
              tab[i][j-k].tid = 0;
            }
            if(tab[i][j].type == ALIEN)
            {
              tab[i][j].type = VIDE;
              tab[i][j].tid = 0;
              EffaceCarre(i,j);

              tab[i][j-k].type = ALIEN;
              tab[i][j-k].tid = tidThread;
              DessineAlien(i,j-k); 
            }
          }
        }
      }

    
      pthread_mutex_unlock(&mutexGrille);

      cg -= k;
      cd -= k;
      pthread_mutex_unlock(&mutexFlotteAliens);
      RechercheExtremite();
      tmpAlien = nbAliens;
    }

    Attente(delai);
    pthread_mutex_lock(&mutexGrille);
    pthread_mutex_lock(&mutexFlotteAliens);
    if(nbAliens > 0)
    {
      for(i = lb;i >= lh;i-=k) //saut de ligne
      {
        for(j = cg;j <= cd;j+=k)
        {

          if(tab[i+k][j].type == MISSILE)
          {
            nbAliens--;
            tab[i][j].type = VIDE;
            tab[i][j].tid = 0;
            EffaceCarre(i,j);
            EffaceCarre(i+k,j);
            pthread_kill(tab[i+k][j].tid,SIGINT);
            tab[i+k][j].type = VIDE;
            tab[i+k][j].tid = 0;
          }
          if(tab[i][j].type == ALIEN)
          {
            tab[i][j].type = VIDE;
            tab[i][j].tid = 0;
            EffaceCarre(i,j);
            
            tab[i+k][j].type = ALIEN;
            tab[i+k][j].tid = tidThread;
            DessineAlien(i+k,j);
          }
        }
      }
    }

    pthread_mutex_unlock(&mutexGrille);
    lb += k;
    lh += k;
    pthread_mutex_unlock(&mutexFlotteAliens);
    RechercheExtremite();
    printf("\nNombre Alien : %d\n",nbAliens);
  }
  if(nbAliens == 0)
    return 1;
  else
    return 0;

}

void DeleteFlotte()
{
  short i,j,k = 2;
  pthread_mutex_lock(&mutexFlotteAliens);
  pthread_mutex_lock(&mutexGrille);
  for(i=lh;i <= LIGNE_BAS;i+=k)
  {
    for(j=cg;j <= cd;j+=k)
    {
      tab[i][j].type = VIDE;
      tab[i][j].tid = 0;
      EffaceCarre(i,j);    
    }
  }
  pthread_mutex_unlock(&mutexGrille);
  pthread_mutex_unlock(&mutexFlotteAliens);
}

void DoubleCheck()
{
  int i,j;
  for(i = lh; i < lb; i+=2)
  {
    for(j = cg; j < cd; j+=2)
    {
      if(tab[i][j].type == VIDE)
        EffaceCarre(i,j);
    }
  }
}


void HandlerSIGQUIT(int sig)
{
  pthread_mutex_lock(&mutexGrille);
  tab[LIGNE_BAS][colonne].type = VIDE;
  tab[LIGNE_BAS][colonne].tid = 0;
  EffaceCarre(LIGNE_BAS,colonne);
  pthread_mutex_unlock(&mutexGrille);
  pthread_exit(NULL);
}

void SpawnBouclier()
{
  int j = 12;
  pthread_mutex_lock(&mutexGrille);
  for(int i = LIGNE_BAS - 1; j <= 16;j+=2)
    for(;j < j + 2;j++)
    {
      tab[i][j].type = BOUCLIER1;
      DessineBouclier(i,j,1);
    }
  pthread_mutex_unlock(&mutexGrille);
}