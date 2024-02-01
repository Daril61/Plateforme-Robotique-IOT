#include <MKRMotorCarrier.h>
#include "QueueList.h"

struct Action {
  MouvementType type;
  int duree;
  void (*function)();
};

QueueList<Action> actionsQueue;

unsigned long derniereAction = 0;

unsigned long lastSpeedUpdateTOMAX = 0;   // ms
int dureeSpeedUpdateTOMAX = 250;

unsigned long lastSpeedCompensing = 0;
int intervalSpeedCompensing = 200;

int dureeSpeedCompensing = 25;
bool compensing = false;

int vitesseMaximum = 25;
int vitesseActuel = 0;

String lastTypePrint;
int tempsAvancerPourContourner = 0;

// Fonction exécutée constamment par le système, elle gère le déplacement
void LoopMouvement() {
  // S'il n'y a aucune action à faire
  if (actionsQueue.isEmpty()) return;

  Action currentAction = actionsQueue.peek();

  if(currentAction.type == MouvementType::FONCTION) {
    printMoveType("Fonction");
    currentAction.function();

    derniereAction = millis();
    lastTypePrint = "";
    actionsQueue.pop();
    return;
  }

  // Vérification que l'action en cours est fini
  if(millis() - derniereAction >= currentAction.duree) {
    derniereAction = millis();
    lastTypePrint = "";
    actionsQueue.pop();
    return;
  }

  switch(currentAction.type) {
    case AVANCER:
      printMoveType("Avancer");
      // Si on n'a pas atteint la vitesse maximum
      if(vitesseActuel != vitesseMaximum) {
        if(millis() - lastSpeedUpdateTOMAX >= dureeSpeedUpdateTOMAX) {
          lastSpeedUpdateTOMAX = millis();

          vitesseActuel += calculateIncrement(millis() - derniereAction);
          Serial.println(vitesseActuel);
          if(vitesseActuel > vitesseMaximum) vitesseActuel = vitesseMaximum;

          M3.setDuty(vitesseActuel);
          M4.setDuty(-vitesseActuel);
        }
      }

      // Changement de vitesse pour compenser
      if(millis() - lastSpeedCompensing >= intervalSpeedCompensing) {   
        lastSpeedCompensing = millis();

        compensing = true;
      }

      // S'il y a une compensation, vérification qu'il est pas fini
      if(compensing) {
        // Fin de la compensation
        if((millis() - lastSpeedCompensing) - dureeSpeedCompensing >= 0) { 
           M3.setDuty(vitesseActuel);
           compensing = false;
        } else {
          M3.setDuty((vitesseActuel + 1));
        }
      }

      break;
    
    case TOURNER_DROITE:
      printMoveType("Tourner Droite");
      M3.setDuty(25);
      M4.setDuty(25);
      break;
    case TOURNER_GAUCHE:
      printMoveType("Tourner Gauche");
      M3.setDuty(-25);
      M4.setDuty(-25);
      break;
    case STOP:
      printMoveType("STOP");
      vitesseActuel = 0;
      M3.setDuty(0);
      M4.setDuty(0);
      break;
  }
}

// Fonction qui permet de faire incrémenter une variable de façon rapide au début, et plus lente à la fin grâce au temps
int calculateIncrement(int elapsedTime) {
    // La fonction décroît l'incrément en fonction du temps écoulé
    float maxIncrement = 10.0; // Incrément maximal initial
    float minIncrement = 1; // Valeur minimale de l'incrément
    float timeThreshold = 1000.0; // Seuil de temps pour atteindre l'incrément minimal (en millisecondes)
    
    // Calculer un facteur d'atténuation en fonction du temps écoulé
    float attenuationFactor = max(1.0 - (float)elapsedTime / timeThreshold, 0.0);
    
    // Calculer l'incrément en fonction du facteur d'atténuation
    float calculatedIncrement = maxIncrement * attenuationFactor;
    
    // Assurer que l'incrément est supérieur ou égal à la valeur minimale
    return max(calculatedIncrement, minIncrement);
}

// Fonction qui permet d'afficher l'action en cours par le système
void printMoveType(String type) {
  if(type == lastTypePrint) return;

  Serial.println(type);
  lastTypePrint = type;
}

// Fonction qui permet de forcer un arrêt, et donc de supprimer les actions en cours
void ForceStop() {
  while(!actionsQueue.isEmpty()) actionsQueue.pop();

  Stop();
}

// Fonction qui permet d'ajouter une action à la queue
void AddActionToQueue(MouvementType type, int duree, void (*function)()) {
  if(actionsQueue.isEmpty())
    derniereAction = millis();
  actionsQueue.push({type, duree, function});
}

// Fonction qui permet de faire faire au robot l'action d'avancer
void Avancer(int duree) {
  AddActionToQueue(MouvementType::AVANCER, duree);

  Stop();
}

// Fonction qui permet de faire faire au robot l'action de tourner
void Tourner(bool droite, int degree) {
  if(droite)
    AddActionToQueue(MouvementType::TOURNER_DROITE, (256 * degree) / 90);
  else
    AddActionToQueue(MouvementType::TOURNER_GAUCHE, (256 * degree) / 90);

  Stop();
}

// Fonction qui permet de faire faire au robot l'action de s'arrêter
void Stop() {
  AddActionToQueue(MouvementType::STOP, 500);
}

// Fonction qui permet de faire faire au robot l'action d'appeler une fonction
void AppelerFonction(void (*fonction)()) {
  AddActionToQueue(MouvementType::FONCTION, 1, fonction);
}

// Fonction pour voir si le robot est arrêté
bool EstArrete() {
  return actionsQueue.isEmpty();
}

// Fonction appelée quand un obstacle est trop proche pour démarrer la procédure d'évitement
void DebutContournement() {
  ForceStop();
  tempsAvancerPourContourner = 0;
  contournement = true;

  UpdateContournerObstacle();
}

// Fonction appelée quand le contournement est en cours pour faire bouger le robot
void UpdateContournerObstacle() {
  if(VerifierObstacle()) {
    Tourner(true, 90);
    Avancer(600);
    tempsAvancerPourContourner += 600;
    Tourner(false, 90);
    AppelerFonction(UpdateContournerObstacle);
    return;
  }

  ForceStop();

  Avancer(1000);
  Tourner(false, 90);
  Avancer(tempsAvancerPourContourner);
  Tourner(true, 90);
  AppelerFonction(FinContournement);
}

// Fonction qui arrête le contournement
void FinContournement() {
  contournement = false;
}