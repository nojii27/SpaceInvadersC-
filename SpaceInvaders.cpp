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
short colonne,nbVies = 3;
int score;
bool endOrnot = true,boolAmiral = false;
bool MAJScore = false;
bool fireOn = true;
S_CASE tab[NB_LIGNE][NB_COLONNE];
int nbAliens = 24, lh = 2, cg = 8, lb = 8,cd = 18;
float delai = 1000;
/**********************/
void Attente(int milli);
void setTab(int l,int c,int type=VIDE,pthread_t tid=0);
void viderCase(int l, int c);

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
void HandlerSIGALRM(int sig);
void HandlerSIGCHLD(int sig);
/////////////MUTEX/////////////////

pthread_mutex_t mutexGrille, mutexFlotteAliens,mutexScore,mutexVies;

///////////COND////////////////////
pthread_cond_t condScore,condVies,condFlotteAliens;

/////////////handle////////////////////
pthread_t pthread_vaisseau, pthread_event, pthread_missile,pthread_TimeOut, pthread_Invader,pthread_FlotteAliens,pthread_Score, pthread_Bombe,pthread_Amiral;

//prototypes de fonctions
void RechercheExtremite();
int DeplacementXY();
void DeleteFlotte();
void SpawnBouclier();
void DeleteBouclier();
void fctFinVaisseau(void * v);

//////////////fct threads//////////////////

void * fctThreadVaisseau();
void * fctThreadEvent();
void * fctThreadTimeOut();
void * fctThreadMissile(S_POSITION*);
void * fctThreadBombe(S_POSITION*);
void * fctThreadInvader();
void * fctThreadFlotteAliens();
void * fctThreadScore();
void * fctThreadAmiral();

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
  pthread_mutex_init(&mutexScore,NULL);
  pthread_cond_init(&condScore,NULL);
  pthread_mutex_init(&mutexVies,NULL);
  pthread_cond_init(&condVies,NULL);
  pthread_cond_init(&condFlotteAliens,NULL);

  // Armement des signaux
  // TO DO
  sigset_t mask;
  struct sigaction sign;
  //SIGUSR1
  sign.sa_handler = HandlerSIGUSR1;
  sigfillset(&sign.sa_mask);           //armement du signal SIGUSR1 sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGUSR1,&sign,NULL);
  //SIGUSR2
  sign.sa_handler = HandlerSIGUSR2;
  sigfillset(&sign.sa_mask);           //armement du signal SIGUSR2 sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGUSR2,&sign,NULL);
  //SIGHUP
  sign.sa_handler = HandlerSIGHUP;
  sigfillset(&sign.sa_mask);           //armement du signal SIGHUP sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGHUP,&sign,NULL);
  //SIGINT
  sign.sa_handler = HandlerSIGINT;
  sigfillset(&sign.sa_mask);           //armement du signal SIGINT sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGINT,&sign,NULL);
  //SIGQUIT
  sign.sa_handler = HandlerSIGQUIT;
  sigfillset(&sign.sa_mask);           //armement du signal SIGQUIT sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGQUIT,&sign,NULL);
  //SIGALRM
  sign.sa_handler = HandlerSIGALRM;
  sigfillset(&sign.sa_mask);           //armement du signal SIGALRM sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGALRM,&sign,NULL);
  //SIGCHLD
  sign.sa_handler = HandlerSIGCHLD;
  sigfillset(&sign.sa_mask);           //armement du signal SIGCHLD sur notre handler
  sign.sa_flags = 0;  
  sigaction(SIGCHLD,&sign,NULL);

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
  Attente(100); // pour etre sur que le vaisseau soit mis en place
  pthread_create(&pthread_Invader,NULL,(void*(*)(void*))fctThreadInvader,NULL); 
  pthread_create(&pthread_Score,NULL,(void*(*)(void*))fctThreadScore,NULL);
  pthread_create(&pthread_Amiral,NULL,(void*(*)(void*))fctThreadAmiral,NULL);  
  //MASK
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGQUIT);
  sigaddset(&mask,SIGCHLD);
  sigaddset(&mask,SIGALRM);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  short compteurErreur = 0;
  pthread_mutex_lock(&mutexVies);
  while(nbVies)
  {
    pthread_cond_wait(&condVies,&mutexVies);
    printf("\nCOND WAIT CONDVIES SORTI\n");
    if(nbVies)
    {
      pthread_create(&pthread_vaisseau,NULL,(void*(*)(void*))fctThreadVaisseau,NULL); //thread_vaisso
    }
    else
    {
      endOrnot = false;
      DessineGameOver(6,11);
      pthread_mutex_lock(&mutexGrille);//on stop les intervenants pour figer l'ecran
    }
    
  }
  

  // Fermeture de la fenetre
  pause();

  exit(0);
}

///////////////////////////////////////////////////////////////////////////////////////////////////


void * fctThreadVaisseau()
{

  sigset_t mask;sigemptyset(&mask);                     
  sigaddset(&mask,SIGINT);   //masquage signal SIGINT
  sigaddset(&mask,SIGCHLD);
  sigaddset(&mask,SIGALRM);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  printf("THREAD VAISSEAU CREE, TID = %d\n",pthread_self());
  int i = cg;
  DessineChiffre(16,4,nbVies);
  DessineChiffre(16,3,0);
  pthread_mutex_lock(&mutexGrille);
  if(tab[NB_LIGNE - 1][15].type == VIDE)
  {
    tab[NB_LIGNE - 1][15].type = VAISSEAU;
    tab[NB_LIGNE - 1][15].tid = pthread_self();
    DessineVaisseau(NB_LIGNE - 1,15);
    
    colonne = 15;
  }
  else
  {
    while(i < 22 && tab[NB_LIGNE - 1][i].type != VIDE)
      i++;
    if(i < 22){
      
      tab[NB_LIGNE - 1][i].type = VAISSEAU;
      tab[NB_LIGNE - 1][i].tid = pthread_self();
      DessineVaisseau(NB_LIGNE - 1,i);
      colonne = i;
    }
  }
  pthread_mutex_unlock(&mutexGrille);
  while(endOrnot)
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
  printf("THREAD EVENT TID %d\n",pthread_self());
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGALRM);
  sigaddset(&mask,SIGCHLD);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  char boolean = 1;
  EVENT_GRILLE_SDL event;
  while(boolean)
  {
    event = ReadEvent();
    if(event.type == CROIX) boolean = 0;
    else if(event.type == CLAVIER)
    {
      if(tab[LIGNE_BAS][colonne].tid != 0)
      {
        switch(event.touche)
        {
          case KEY_LEFT:
            pthread_kill(tab[LIGNE_BAS][colonne].tid,SIGUSR1);
            break;
          case KEY_RIGHT:
            pthread_kill(tab[LIGNE_BAS][colonne].tid,SIGUSR2);
            break;
          case KEY_SPACE:
            if(fireOn == true)
              pthread_kill(tab[LIGNE_BAS][colonne].tid,SIGHUP);
            break;
          default:
            break;        
        }
      }
      
    }
    else;
  }
  printf("\n\nLe thread EVENT se termine!\n\n");
  FermetureFenetreGraphique();
  pthread_mutex_destroy(&mutexGrille);
  pthread_mutex_destroy(&mutexFlotteAliens);
  pthread_mutex_destroy(&mutexScore);
  pthread_mutex_destroy(&mutexVies);
  exit(0);
}


void * fctThreadMissile(S_POSITION* posMissile)
{
  sigset_t mask;sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGALRM);
  sigaddset(&mask,SIGCHLD);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  printf("\nTHREAD MISSILE TID = %d\n",pthread_self());
  S_POSITION pos;
  memcpy(&pos,posMissile,sizeof(S_POSITION));
  free(posMissile);
  pthread_mutex_lock(&mutexGrille);
  if(tab[pos.L][pos.C].type == BOUCLIER1)
  {
    tab[pos.L][pos.C].type = BOUCLIER2;
    EffaceCarre(pos.L,pos.C);
    DessineBouclier(pos.L,pos.C,2);             //si le missile spawn sur un bouclier1 alors bouclier1 -> bouclier2
    pthread_mutex_unlock(&mutexGrille);
    
    pthread_exit(NULL);
  }
  else if(tab[pos.L][pos.C].type == BOUCLIER2)
  {
    tab[pos.L][pos.C].type = VIDE; //si le missile spawn sur un bouclier2 alors bouclier2 -> Vide
    EffaceCarre(pos.L,pos.C);
    pthread_mutex_unlock(&mutexGrille);
    
    pthread_exit(NULL);
  }
  else if(tab[pos.L][pos.C].type == BOMBE)
  {
    pthread_kill(tab[pos.L][pos.C].tid,SIGINT);
    setTab(pos.L,pos.C);
    EffaceCarre(pos.L,pos.C);
    pthread_mutex_unlock(&mutexGrille);
    pthread_exit(NULL);
  }
  else if(tab[pos.L][pos.C].type == VIDE)
  {
    tab[pos.L][pos.C].type = MISSILE;
    tab[pos.L][pos.C].tid = pthread_self();     // si le missile spawn sur VIDE alors il monte...
    DessineMissile(pos.L,pos.C);
    pthread_mutex_unlock(&mutexGrille);
    while(pos.L > 0)
    {
      Attente(80);
      pthread_mutex_lock(&mutexGrille);
      setTab(pos.L,pos.C);
      EffaceCarre(pos.L,pos.C);
      if(tab[pos.L - 1][pos.C].type == ALIEN)
      {
        setTab(pos.L - 1,pos.C);  //le missile a touché
        EffaceCarre(pos.L - 1,pos.C);
        RechercheExtremite();
        pthread_mutex_unlock(&mutexGrille);

        pthread_mutex_lock(&mutexScore);
        MAJScore = true;
        score++;
        pthread_mutex_unlock(&mutexScore);
        pthread_cond_signal(&condScore);
        
        pthread_mutex_lock(&mutexFlotteAliens);
        nbAliens--;
        pthread_mutex_unlock(&mutexFlotteAliens);
        
        pthread_exit(NULL);
      }
      else if(tab[pos.L - 1][pos.C].type == BOMBE)
      {
        printf("pthread kill dans threadMissile sur bombe tid %d \n",tab[pos.L - 1][pos.C].tid);
        pthread_kill(tab[pos.L - 1][pos.C].tid,SIGINT);
        setTab(pos.L - 1,pos.C);
        EffaceCarre(pos.L - 1,pos.C);
        pthread_mutex_unlock(&mutexGrille);
        pthread_exit(NULL);
      }
      else if(tab[pos.L - 1][pos.C].type == AMIRAL)
      {
        pthread_kill(tab[pos.L - 1][pos.C].tid,SIGCHLD);
        pthread_mutex_unlock(&mutexGrille);
        pthread_exit(NULL);
      }
      else
      {
        pos.L--;         //le missile monte...
        tab[pos.L][pos.C].type = MISSILE;
        tab[pos.L][pos.C].tid = pthread_self();
        DessineMissile(pos.L,pos.C);
        pthread_mutex_unlock(&mutexGrille);
      }
    }
    pthread_mutex_lock(&mutexGrille);
    setTab(pos.L,pos.C);
    EffaceCarre(pos.L,pos.C);
    pthread_mutex_unlock(&mutexGrille);
    pthread_exit(NULL);
  }  
  
  pthread_mutex_unlock(&mutexGrille);
  pthread_exit(NULL);

}

void * fctThreadTimeOut()
{
  Attente(600);
  fireOn = true;                                
  pthread_exit(NULL);
}

void * fctThreadInvader()
{
  printf("THREAD INVADER TID %d\n",pthread_self());
  sigset_t mask;sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x) SIGINT
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGCHLD);
  sigaddset(&mask,SIGALRM);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  bool continueOrNot = true;
  int level = 1;
  int tmp = 0;
  DessineChiffre(13,4,level);
  DessineChiffre(13,3,0);
  while(continueOrNot)
  {
    pthread_create(&pthread_FlotteAliens,NULL,(void*(*)(void*))fctThreadFlotteAliens,NULL);
    pthread_join(pthread_FlotteAliens,(void **)NULL);
    if(nbAliens == 0)
    {
      ++level;
      tmp = level;
      delai -= (delai * 0.3);

      DessineChiffre(13,4,tmp % 10);
      tmp /= 10;
      DessineChiffre(13,3,tmp % 10);
      DeleteBouclier();
      SpawnBouclier();
    }
    else
    {
      DeleteFlotte();
      DeleteBouclier();
      pthread_mutex_lock(&mutexGrille);
      //pthread_kill(tab[LIGNE_BAS][colonne].tid,SIGQUIT);  //pas besoin car on fige l'ecran
      DessineGameOver(6,11);
      continueOrNot = false;
    }
  }
pthread_exit(NULL);

}

void * fctThreadFlotteAliens()
{
  printf("THREAD FLOTTEALIENS TID %d\n",pthread_self());
  char i,j;
  int tidThread = pthread_self();
  pthread_mutex_lock(&mutexGrille);
  pthread_mutex_lock(&mutexFlotteAliens);
  nbAliens = 24;lh = 2; cg = 8; lb = 8;cd = 18;
  for(i = lh;i <= lb; i+=2)     // i < 4 mais on commence a ligne 2 + ligne vide -> 10
  {
    
    for(j = cg;j <= cd;j+=2) // on commence aek j = 8 ; or 6 aliens par ligne 6 + 8 -> 14
    {
      tab[i][j].type = ALIEN;
      tab[i][j].tid = tidThread;
      DessineAlien(i,j);
    }

  }
  pthread_mutex_unlock(&mutexFlotteAliens);
  pthread_mutex_unlock(&mutexGrille);
  i = DeplacementXY();
  if(i == 1)
  {
    printf("\nVous avez Remporte le niveau! nbAliens = %d\n",nbAliens);
    pthread_exit(NULL);
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
  pthread_exit(NULL);
}



int DeplacementXY()
{
  short lAlien, cAlien;
  char k = 1,i,j;
  int tidThread = pthread_self();
  int tmpAlien;
  int unAlienSurDeux = 0;
  int compteurAlien = 0;
  int randAlien;
  S_POSITION *positiondelabombe = NULL; 

  
  pthread_mutex_lock(&mutexFlotteAliens);
  while(nbAliens > 0  && lb < LIGNE_BAS - 2)
  {    
    pthread_mutex_unlock(&mutexFlotteAliens);

    tmpAlien = nbAliens;
    pthread_mutex_lock(&mutexFlotteAliens);
    while(nbAliens > 0 && cd < 22 )
    {
      pthread_mutex_unlock(&mutexFlotteAliens);
      if(nbAliens % 6 == 0 && nbAliens && boolAmiral == false)
      {
        printf("\n------ENVOI COND SIGNAL-------\n");
        pthread_cond_signal(&condFlotteAliens);
      }
        
      
      
      compteurAlien = 0;

      //Deplacement droite
      Attente(delai);   
      unAlienSurDeux++;
      
      if(unAlienSurDeux % 2 && nbAliens)
      {
        randAlien = rand() % (nbAliens) +1;
        printf("Numero Alien random %d\n", randAlien );
      }

      if(nbAliens > 0)
      {
        
        pthread_mutex_lock(&mutexGrille);
        pthread_mutex_lock(&mutexFlotteAliens);
        
    
        for(i = lh;i <= lb;i+=k)
        {
          for(j = cd; j >= cg; j-=k)
          {
            if(tab[i][j].type == ALIEN) // check si alien
            {
              tab[i][j].type = VIDE;
              tab[i][j].tid = 0;
              EffaceCarre(i,j);
              if(tab[i][j+k].type == MISSILE) // si alien fonce vers missile alors on l'efface
              {
                
                nbAliens--;
                
                

                pthread_mutex_lock(&mutexScore);    //maj score
                MAJScore = true;
                score++;
                pthread_mutex_unlock(&mutexScore);
                pthread_cond_signal(&condScore);

                EffaceCarre(i,j+k);              
                pthread_kill(tab[i][j+k].tid,SIGINT); //efface missile
                setTab(i,j+k);

              }
              else if(tab[i][j+k].type == BOMBE)    // si bombe alors on le remplace avec un alien et on recree un thread bombe aek les nouvelles coordonnees 
              {
                pthread_kill(tab[i][j+k].tid,SIGINT);
                setTab(i,j+k,ALIEN,tidThread);
                EffaceCarre(i,j+k);
                DessineAlien(i,j+k);
                positiondelabombe = (S_POSITION*)malloc(sizeof(S_POSITION));
                positiondelabombe->L = i + 1;
                positiondelabombe->C = j + k;
                pthread_create(&pthread_Bombe, NULL,(void*(*)(void*))fctThreadBombe,positiondelabombe);
                
              }else    // si pas missile alors on déplace alien
              {
                setTab(i,j+k,ALIEN,tidThread);
                DessineAlien(i,j+k);
                if(compteurAlien == randAlien)
                {
                  lAlien = i + 1;
                  cAlien = j + k;
                }
              }
              compteurAlien++;
            }
          }
        }
        cg += k;
        cd += k;
        pthread_mutex_unlock(&mutexFlotteAliens);
        if(tmpAlien != nbAliens)RechercheExtremite();
        pthread_mutex_unlock(&mutexGrille);
        tmpAlien = nbAliens;
      }
      if(unAlienSurDeux % 2)
      {
        positiondelabombe = (S_POSITION*)malloc(sizeof(S_POSITION));
        positiondelabombe->L = lAlien;
        positiondelabombe->C = cAlien;
        pthread_create(&pthread_Bombe, NULL,(void*(*)(void*))fctThreadBombe,positiondelabombe);
      }
      pthread_mutex_lock(&mutexFlotteAliens);
      
    }
    if(nbAliens <=0)    // au cas ou ^ ce lock a lieu mais n'est jamais unlock car conditions non remplies
    {
      pthread_mutex_unlock(&mutexFlotteAliens);      
    }
    
    while(cg > 8 && nbAliens > 0) //deplacement gauche
    {
      pthread_mutex_unlock(&mutexFlotteAliens);
      if(nbAliens % 6 == 0 && nbAliens && boolAmiral == false)
      {
        printf("\n------ENVOI COND SIGNAL-------\n");
        pthread_cond_signal(&condFlotteAliens);
      }
      
      compteurAlien = 0;
      
      unAlienSurDeux++;
      
      if(unAlienSurDeux % 2)
      {
        randAlien = rand() % (nbAliens) +1;
        printf("Numero Alien random %d\n", randAlien );
      }
      if(nbAliens > 0)
      {
        
        Attente(delai);
        pthread_mutex_lock(&mutexGrille);
        pthread_mutex_lock(&mutexFlotteAliens);
        
        for(i = lh;i <= lb;i+=k)
        {
          for(j = cg; j <= cd; j+=k)
          {
            if(tab[i][j].type == ALIEN) // on check si alien
            {
              tab[i][j].type = VIDE;    // efface alien
              tab[i][j].tid = 0;
              EffaceCarre(i,j);
              if(tab[i][j-k].type == MISSILE) // si alien fonce vers missile alors on efface missile
              {
                
                nbAliens--;
                

                pthread_mutex_lock(&mutexScore);    // maj score 
                MAJScore = true;
                score++;
                pthread_mutex_unlock(&mutexScore);
                pthread_cond_signal(&condScore);

                EffaceCarre(i,j-k);
                
                pthread_kill(tab[i][j-k].tid,SIGINT); // efface missile 
                setTab(i,j-k);
              }else if(tab[i][j-k].type == BOMBE)    // si bombe alors on le remplace avec un alien et on recree un thread bombe aek les nouvelles coordonnees 
              {
                pthread_kill(tab[i][j-k].tid,SIGINT);
                setTab(i,j-k,ALIEN,tidThread);
                EffaceCarre(i,j-k);
                DessineAlien(i,j-k);
                positiondelabombe = (S_POSITION*)malloc(sizeof(S_POSITION));
                positiondelabombe->L = i + 1;
                positiondelabombe->C = j - k;
                pthread_create(&pthread_Bombe, NULL,(void*(*)(void*))fctThreadBombe,positiondelabombe);
              }
              else
              {
                tab[i][j-k].type = ALIEN; // si pas missile alors deplacer alien
                tab[i][j-k].tid = tidThread;
                DessineAlien(i,j-k);
                if(compteurAlien == randAlien)
                {
                  lAlien = i + 1;
                  cAlien = j - k;
                }
              }

              compteurAlien++;
            }
          }
        }
        cg -= k;
        cd -= k;
        pthread_mutex_unlock(&mutexFlotteAliens);
        if(tmpAlien != nbAliens)RechercheExtremite();
        pthread_mutex_unlock(&mutexGrille);
        tmpAlien = nbAliens;
      }
      if(unAlienSurDeux % 2)
      {
        positiondelabombe = (S_POSITION*)malloc(sizeof(S_POSITION));
        positiondelabombe->L = lAlien;
        positiondelabombe->C = cAlien;
        pthread_create(&pthread_Bombe, NULL,(void*(*)(void*))fctThreadBombe,positiondelabombe);
      }    
      pthread_mutex_lock(&mutexFlotteAliens);
    }

    if(nbAliens <=0 || cg <= 8)    // au cas ou ^ ce lock a lieu mais n'est jamais unlock car conditions non remplies
    {
      pthread_mutex_unlock(&mutexFlotteAliens);
      
    }
    if(nbAliens > 0)
    {
      Attente(delai);
      pthread_mutex_lock(&mutexGrille);
      pthread_mutex_lock(&mutexFlotteAliens);
      
      if(nbAliens % 6 == 0 && nbAliens && boolAmiral == false)
      {
        printf("\n------ENVOI COND SIGNAL-------\n");
        pthread_cond_signal(&condFlotteAliens);
      }
      for(i = lb;i >= lh;i-=k) //saut de ligne
      {
        for(j = cg;j <= cd;j+=k)
        {
          if(tab[i][j].type == ALIEN)
          {
            tab[i][j].type = VIDE;      // si alien alors efface
            tab[i][j].tid = 0;
            EffaceCarre(i,j);
            if(tab[i+k][j].type == MISSILE)   // si alien fonce vers missile on efface le missile
            {
              
              nbAliens--;
              
              pthread_mutex_lock(&mutexScore);  // maj score
              MAJScore = true;
              score++;
              pthread_mutex_unlock(&mutexScore);
              pthread_cond_signal(&condScore);

              EffaceCarre(i+k,j);
              pthread_kill(tab[i+k][j].tid,SIGINT);//efface missile
              setTab(i+k,j);
            }
            else if(tab[i + k][j].type == BOMBE)    // si bombe alors on le remplace avec un alien et on recree un thread bombe aek les nouvelles coordonnees 
            {
              pthread_kill(tab[i + k][j].tid,SIGINT);
              setTab(i+k,j,ALIEN,tidThread);
              EffaceCarre(i+k,j);
              DessineAlien(i+k,j);
              positiondelabombe = (S_POSITION*)malloc(sizeof(S_POSITION));
              positiondelabombe->L = i + 1 + k;
              positiondelabombe->C = j;
              pthread_create(&pthread_Bombe, NULL,(void*(*)(void*))fctThreadBombe,positiondelabombe);
            }
            else        //si pas missile alors on deplace l'alien
            {
              tab[i+k][j].type = ALIEN;
              tab[i+k][j].tid = tidThread;
              DessineAlien(i+k,j);
            }

          }
        }
      }
      lb += k;
      lh += k;
      pthread_mutex_unlock(&mutexFlotteAliens);
      if(tmpAlien != nbAliens)RechercheExtremite();
      pthread_mutex_unlock(&mutexGrille);
      tmpAlien = nbAliens;
    }

    
    pthread_mutex_lock(&mutexFlotteAliens);
  }
  if(nbAliens <= 0  || lb >= LIGNE_BAS - 2)
  {
    pthread_mutex_unlock(&mutexFlotteAliens);
    
  }
  printf("\nFINI\n");
  if(nbAliens == 0)
  {
    return 1;
  }
  return 0;
}


void * fctThreadBombe(S_POSITION* posBombe)
{
  printf("THREAD BOOBME TID %d\n",pthread_self());
  S_POSITION tmp;
  tmp.L = posBombe->L;
  tmp.C = posBombe->C;
  free(posBombe);
  bool stop = false;
  pthread_mutex_lock(&mutexGrille);
  
  switch(tab[tmp.L][tmp.C].type)
  {
    case BOMBE:
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case MISSILE:
      printf("\npthread kill dans threadBombe sur missile tid %d\n",tab[tmp.L][tmp.C].tid);
      pthread_kill(tab[tmp.L][tmp.C].tid,SIGINT);
      setTab(tmp.L,tmp.C);
      EffaceCarre(tmp.L,tmp.C);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case BOUCLIER1:
      setTab(tmp.L,tmp.C,BOUCLIER2);
      EffaceCarre(tmp.L,tmp.C);
      DessineBouclier(tmp.L,tmp.C,2);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case BOUCLIER2:
      setTab(tmp.L,tmp.C);
      EffaceCarre(tmp.L,tmp.C);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case VIDE:
      setTab(tmp.L, tmp.C, BOMBE, pthread_self());
      DessineBombe(tmp.L,tmp.C);
      break;
  }
  pthread_mutex_unlock(&mutexGrille);

  while(tmp.L < LIGNE_BAS)
  {
    
    Attente(160);
    pthread_mutex_lock(&mutexGrille);
    if(stop == false)
    {
      EffaceCarre(tmp.L, tmp.C);
      setTab(tmp.L,tmp.C);
    }
    stop = false;
    tmp.L++;
    switch(tab[tmp.L ][tmp.C].type)
    {
    case BOMBE:
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case MISSILE:
      printf("\npthread kill dans threadBombe sur missile tid %d a la case tab[%d][%d]\n",tab[tmp.L][tmp.C].tid,tmp.L,tmp.C);
      pthread_kill(tab[tmp.L][tmp.C].tid,SIGINT);
      setTab(tmp.L,tmp.C);
      EffaceCarre(tmp.L,tmp.C);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case BOUCLIER1:
      setTab(tmp.L,tmp.C,BOUCLIER2);
      EffaceCarre(tmp.L,tmp.C);
      DessineBouclier(tmp.L,tmp.C,2);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case BOUCLIER2:
      setTab(tmp.L,tmp.C);
      EffaceCarre(tmp.L,tmp.C);
      pthread_mutex_unlock(&mutexGrille);
      pthread_exit(NULL);
      break;
    case ALIEN:
      pthread_mutex_unlock(&mutexGrille);
      stop = true;
      continue;
    case VAISSEAU:
      printf("\npthread kill dans threadBombe sur vaisseau tid %d\n",tab[tmp.L][tmp.C].tid);
      pthread_kill(tab[tmp.L][tmp.C].tid,SIGQUIT);
      pthread_mutex_unlock(&mutexGrille);
      break;
    case VIDE:
      setTab(tmp.L, tmp.C, BOMBE, pthread_self());
      DessineBombe(tmp.L,tmp.C);
      pthread_mutex_unlock(&mutexGrille);
      break;
    default:
      printf("\nDEFAULT DNAS THREADBOMB\n");
      pthread_mutex_unlock(&mutexGrille);
      break;
    }
  }
  setTab(tmp.L,tmp.C);
  EffaceCarre(tmp.L,tmp.C);
  pthread_exit(NULL);
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
      if(tab[i][j].type == ALIEN)
      {
        tab[i][j].type = VIDE;
        tab[i][j].tid = 0;
        EffaceCarre(i,j); 
      }
    }
  }
  pthread_mutex_unlock(&mutexGrille);
  pthread_mutex_unlock(&mutexFlotteAliens);
}


void HandlerSIGQUIT(int sig)
{
  pthread_cleanup_push(fctFinVaisseau,NULL);
  tab[LIGNE_BAS][colonne].type = VIDE;
  tab[LIGNE_BAS][colonne].tid = 0;
  EffaceCarre(LIGNE_BAS,colonne);
  pthread_cleanup_pop(1);
  if(nbVies == 0)
  {
    DessineChiffre(16,4,nbVies);
    pthread_exit(NULL);
  }
}

void SpawnBouclier()
{
  int j = 12,tmp = 0;
  pthread_mutex_lock(&mutexGrille);
  for(int i = LIGNE_BAS - 1; j <= 16;j+=2)
  {
    for(tmp=j;j < tmp + 2;j++)
    {
      if(tab[i][j].type == VIDE)
      {
        tab[i][j].type = BOUCLIER1;
        DessineBouclier(i,j,1);
      }

    }
  }
  pthread_mutex_unlock(&mutexGrille);
}

void DeleteBouclier()
{
  int j = 8;
  pthread_mutex_lock(&mutexGrille);
  for(int i = LIGNE_BAS - 1; j <= 22;j++)
  {
    if(tab[i][j].type == BOUCLIER1 || tab[i][j].type == BOUCLIER2)
    {
      tab[i][j].type = VIDE;
      tab[i][j].tid = 0;
      EffaceCarre(i,j);
    }
  }
  pthread_mutex_unlock(&mutexGrille);
}


void * fctThreadScore()
{
  printf("THREAD SCORE TID %d\n",pthread_self());
  sigset_t mask;sigemptyset(&mask);
  sigaddset(&mask,SIGUSR1);             //masquage signaux SIGHUP SIGUSR(x)
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGALRM);
  sigaddset(&mask,SIGCHLD);
  pthread_sigmask(SIG_SETMASK, &mask, NULL);
  int tmp = score;
  int i;
  for(i=5;i >=2;i--)
    DessineChiffre(10,i,0);
  pthread_mutex_lock(&mutexScore);
  while(endOrnot)
  {
    pthread_cond_wait(&condScore,&mutexScore);
    if(MAJScore)
    {
      tmp = score;
      i = 5;
      while(tmp && i >= 2)
      {
        DessineChiffre(10,i,tmp % 10);
        tmp /= 10;
        i--;
      }
    }
    
    MAJScore = false;

  }
    pthread_mutex_unlock(&mutexScore);
  pthread_exit(NULL);
}




void viderCase(int l, int c)
{
  tab[l][c].type = VIDE;
  tab[l][c].tid = 0;
  EffaceCarre(l,c);
}




void fctFinVaisseau(void * v)
{
  printf("FONCTION DE FIN DE VAISSEAU\n");
  pthread_mutex_lock(&mutexVies);
  nbVies--;
  pthread_cond_signal(&condVies);
  pthread_mutex_unlock(&mutexVies);
  printf("VIES = %d\n",nbVies);
}

void * fctThreadAmiral()
{
  printf("\nTHREAD AMIRAL TID %d\n",pthread_self());
  bool twoInARow = false;
  char tmpsAttente;
  short direction,k=0,i,j;
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask,SIGALRM);
  sigaddset(&mask,SIGCHLD);
  sigaddset(&mask,SIGHUP);
  sigaddset(&mask,SIGINT);
  sigaddset(&mask,SIGQUIT);
  sigaddset(&mask,SIGUSR2);
  sigaddset(&mask,SIGUSR1);
  pthread_sigmask(SIG_UNBLOCK,&mask,NULL);  //démasquage signal SIGALRM*/
  pthread_mutex_lock(&mutexFlotteAliens);
  while(nbAliens)
  {
    pthread_cond_wait(&condFlotteAliens,&mutexFlotteAliens);
    pthread_mutex_unlock(&mutexFlotteAliens);
    boolAmiral = true;
    printf("\n----AMIRAL PASSE----\n");
    pthread_mutex_lock(&mutexGrille);
    for(i =0,j = 8; k < 30 && twoInARow == false;k++)   // ici k fonctionne comme un compteur
    {
      j = rand() % (22 - 8 + 1) + 8;
      if( (tab[i][j].type == VIDE && tab[i][j + 1].type == VIDE) )
      {
        twoInARow = true;
      }
    }
    twoInARow = false;
    setTab(i,j,AMIRAL,pthread_self());
    setTab(i,j + 1,AMIRAL,pthread_self());
    DessineVaisseauAmiral(i,j);
    pthread_mutex_unlock(&mutexGrille);
    direction = rand() % (11 - 10 + 1) + 10;
    
    tmpsAttente = rand() % (12 - 4 + 1) + 4;
    alarm(tmpsAttente);

    while(boolAmiral)
    {
      if(j == 8)direction = DROITE;
      else if(j == 21) direction = GAUCHE;
      if(direction == GAUCHE)k = -1;else k = 2;
      Attente(200);
      pthread_mutex_lock(&mutexGrille);
      if(tab[i][j+k].type == MISSILE)
        ;
      else if(tab[i][j+k].type == VIDE)  //faire les deplacements
      {
        setTab(i,j);
        setTab(i,j+1);
        EffaceCarre(i,j);
        EffaceCarre(i,j+1);
        if(direction == DROITE )j++;else j--;
        setTab(i,j,AMIRAL,pthread_self());
        setTab(i,j+1,AMIRAL,pthread_self());
        
        if(direction == DROITE)
          DessineVaisseauAmiral(i,j);
        else
          DessineVaisseauAmiral(i,j);
        
      }
      pthread_mutex_unlock(&mutexGrille);
      
    }
    pthread_mutex_lock(&mutexGrille);
    setTab(i,j);
    setTab(i,j+1);
    EffaceCarre(i,j);
    EffaceCarre(i,j+1);
    pthread_mutex_unlock(&mutexGrille);

  }
  pthread_exit(NULL);
}

void HandlerSIGALRM(int sig)
{
  printf("RECEPTION SIGNAL SIGALRM PAR %d\n",pthread_self()); // n est pas reçu par AMIRAL
  boolAmiral =false;
}
void HandlerSIGCHLD(int sig)
{
  printf("RECEPTION SIGNAL SIGCHLD PAR %d\n",pthread_self());
  pthread_mutex_lock(&mutexScore);
  MAJScore = true;
  score+= 10;
  pthread_mutex_unlock(&mutexScore);
  pthread_cond_signal(&condScore);
  alarm(0);
  boolAmiral = false;
}