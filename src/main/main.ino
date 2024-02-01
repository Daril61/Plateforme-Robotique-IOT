#include "mouvementType.h"

// Définition des fonctions pour éviter des problèmes de compilation
bool contournement = false;
void AddActionToQueue(MouvementType type, int duree, void (*function)()=nullptr);
void ForceAction(MouvementType type, int duree);
bool EstArreter();
void AppelerFonction(void (*fonction)());
void DebutContournement();
void UpdateContournerObstacle();
void FinContournement();

// Fonction exécutée de base pour initialiser le programme
void setup() 
{
  Serial.begin(9600);

  SetupCapteur();

  Stop();
}

// Fonction exécutée par le système pour mettre à jour le système de mouvement et le système des capteurs
void loop() {
  LoopCapteur();
  LoopMouvement();
}

// Fonction appelée lors de l'appui sur le bouton
void DemarrerProgramme() {
  ForceStop();
  FinContournement();

  // METTRE ICI la mission pour le robot
  Tourner(true, 90);
  Avancer(5000);
}

// Fonction appelée pour faire un carré
void carre() {
  for(int i = 0; i <= 3; i++) {
    Avancer(500);
    Tourner(true, 90);
  } 
}
