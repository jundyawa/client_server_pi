# Projet Final ELE4205

Ce projet a pour objectif de mettre en communication sous une structure client/serveur une Webcam connectee a un Odroid et un ordinateur de bureau. Lorsque le client 
connecte a l'odroid sur le port 4099 et qu'il envoie la bonne sequence de 32 bit, il va commence a recevoir le streaming. L'odroid s'assure qu'il y a suffisament
de lumiere ambiante avant de lancer le streaming. De plus, un bouton se trouve sur l'odroid pour donner la commande au client d'enregistrer une capture. 
Le client a ensuite des fonctions de reconnaissance de visages qui sont entrainables.

Le projet s'est deroule en six sceances, donc si vous voulez la version finale utilisez seulement le dossier sceance6/

## Instruction d'installation

Quand on rentre a l'interieur du dossier d'une sceance and a deux dossiers, un s'intitulant client et l'autre serveur. A l'interieur de chacun de ces dossiers
vous trouverez un .cpp, un CMakeLists et un executable. Vous n'avez qu'a importer l'executable "server" sur l'odroid et l'executable "client" sur votre ordinateur
de bureau. Vous pourrez ensuite les lancer avec un terminal avec les commandes ./server et ./client.

### Prerequis

Vous aurez besoin de la librairie OpenCV si vous souhaitez modifier et recompiler le code.

## Modification

Si vous souhaitez modifier le code, vous trouverez a l'interieur du dossier client et server un .cpp et un CMakeLists. Vous pouvez modifier ce que vous voulez
dans le .cpp. Ensuite, vous n'avez qu'a lancer un cmake/make et vous obtiendrez vos executables que vous pouvez lancer en ecrivant dans la console 
./client sur l'ordi et ./server sur l'odroid.

## Fait Avec

* [OpenCV](https://github.com/opencv/opencv) - La librairie de vision


## Auteur

* **Jean-Romain Roy** - *Initial work* - [jeanromainroy](https://github.com/jeanromainroy)


## Remerciement

* J'aimerais remercier Jeremie Pilon pour son mentorat tout au long de se projet.


