#include <stdio.h>
#include <bitset>
#include <stdlib.h>
#include <fstream>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <string>

using namespace cv;
using namespace std;

// GPIO
#define PHOTO_PATH "/sys/class/saradc/ch0"
#define BUTTON_PATH "/sys/class/gpio/gpio228/value"

// Webcam
VideoCapture cam(0);

// Image Attributes
int width = 0;
int height = 0;
Mat img;
int imgSize;

// Information Carrier
uint32_t clientStatus;
uint32_t serverStatus;
int bytes = 0;

/**
	\fn changeVidFormat()
	\brief Fait la lecture du uint32_t envoye par le client et si un changement du format de l'image est observe on ajuste les attributs.
	**/
void changeVidFormat(){

	if(((clientStatus >> (32-1)) & 1U) == 1){

		// si on avait deja cette largeur
		if(width == 320){	
			return;		
		}

		width = 320;
		height = 240;
	}else if(((clientStatus >> (32-2)) & 1U) == 1){

		// si on avait deja cette largeur
		if(width == 800){	
			return;		
		}

		width = 800;
		height = 600;
	}else if(((clientStatus >> (32-3)) & 1U) == 1){

		// si on avait deja cette largeur
		if(width == 960){	
			return;		
		}

		width = 960;
		height = 720;
	}else if(((clientStatus >> (32-4)) & 1U) == 1){

		// si on avait deja cette largeur
		if(width == 1280){	
			return;		
		}

		width = 1280;
		height = 960;
	}

	// On formate notre nouveau canva d'image
	img = Mat::zeros(height,width,CV_8UC3);
	img = (img.reshape(0,1));
	imgSize = img.total()*img.elemSize();
	
	// On change les parametres de la webcam
	cam.set(CV_CAP_PROP_FRAME_WIDTH,width);
	cam.set(CV_CAP_PROP_FRAME_HEIGHT,height);
}

/**
	\fn updateServerStatus(bool isThereLight, bool isButtonPressed)
	\brief Met a jour le uint32_t du serveur si il n'y a pas suffisament de lumiere ou le bouton a ete pese.
	\param isThereLight 
	Si le niveau de lumiere est suffisant = vrai
	\param isThereLight 
	Si le bouton est pese = vrai
	**/
void updateServerStatus(bool isThereLight, bool isButtonPressed){

	serverStatus = 0;

	if(isThereLight){
		serverStatus |= 1UL << (32-1);
	}else{
		serverStatus &= ~(1UL << 32-1);
	}

	if(isButtonPressed){
		serverStatus |= 1UL << (32-2);
	}else{
		serverStatus &= ~(1UL << 32-2);
	}
}

/**
\mainpage
Ce programme est roule sur un Odroid qui est configure comme serveur. On commence par creer un socket sur lequel 
un client peut venir se connecter. Ensuite, la webcam est demarree et on commence a envoyer son stream si la bonne
commande de 32 bit est envoye. On peut changer le format de l'image de quatre maniere differente.
**/
int main(int argc, char *argv[]){

	// ------------------------------------------------------------------------
	// ------------------ On initie la connection au socket -------------------
	// ------------------------------------------------------------------------
	int sockfd, newsockfd, n;
	socklen_t clilen;
	struct sockaddr_in serv_addr, cli_addr;
	int portno = 4099;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0){
		cout << "ERROR opening socket" << endl;
		return -1;
	}
	bzero((char *) &serv_addr, sizeof(serv_addr));

	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(portno);
	if (bind(sockfd, (struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0){
		cout << "ERROR on binding" << endl;
	}

	listen(sockfd,5);
	clilen = sizeof(cli_addr);

	newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr,&clilen);
	
	if (newsockfd < 0){
        	cout << "ERROR on accept" << endl;
		return -1;
	}
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------
	// ------------------------------------------------------------------------


	// On s'assure que la webcam est fonctionnelle
	if(!cam.isOpened()){
		cout << "Failed to connect to the camera." << endl;
		return -1;
	}

	// Init le registre du bouton
	system("echo 228 > /sys/class/gpio/export");
	system("echo in > /sys/class/gpio/gpio228/direction");

	// Ouvre le fichier contenant le voltage sur la Photo Diode
	ifstream photoFile(PHOTO_PATH);
	if (!photoFile.is_open()) {                 
		cout << " Failed to open photo diode voltage file." << endl;
		return -1;
	}

	// Definit les variables reliees au voltage de la photo diode
	int voltage = 0;
	int voltageThreshold = 850;

	// Ouvre le fichier contenant l'etat du bouton
	ifstream buttonFile(BUTTON_PATH);
	if (!buttonFile.is_open()) {                 
		cout << " Failed to open button pressed file." << endl;
		return -1;
	}
	
	// Definit la variable reliee a l'etat du bouton
	bool buttonReleased = false;
		
	while(1){

		// Check voltage Photo Diode
		photoFile.clear();	// clear flags
		photoFile.seekg(0);	// fait revenir le pointeur au debut
		photoFile >> voltage;

		// Check l'etat du bouton
		buttonFile.clear();
		buttonFile.seekg(0);
		buttonFile >> buttonReleased;

		if(voltage > voltageThreshold && buttonReleased){	
			// il fait noir
			updateServerStatus(false,!buttonReleased);
		}else if(voltage > 0){
			updateServerStatus(true,!buttonReleased);
		}
		
		// Utile pour deboggage permet de voir le uint32
		//bitset<32> x(serverStatus);
		//cout << " Server : " << x << endl;

		// envoie le uint32_t contenant l'etat du serveur
		n = send(newsockfd, &serverStatus, sizeof(uint32_t), 0);
		if(n < 0){
			cout << "Couldnt send server status" << endl;
			return -1; // error
		}

		// Attend de recevoir le status du client
		if(recv(newsockfd, &clientStatus, sizeof(uint32_t), 0) < 0){
				cout << "Couldnt receive start command" << endl;
				return -1; // error
		}

		// Utile pour deboggage permet de voir le uint32
		//bitset<32> y(clientStatus);
		//cout << " Client : " << y << endl;

		if(((clientStatus >> (32-6)) & 1U) == 0){
			// Si le client ne veut pas recevoir l'image en ce moment
			waitKey(30);
			continue;
			
		}else if(((clientStatus >> (32-5)) & 1U) == 0){
			// Si le client demande d'arreter la capture
			cout << "Aborting Capture" << endl;
			break;
		}

		// Sinon on envoi l'image

		// Check si le format de l'image a change
		changeVidFormat();

		// Capture l'image
		cam >> img;
		img = (img.reshape(0,1));
		
		// Envoi l'image sur le socket vers le client
		bytes = send(newsockfd, img.data, imgSize, 0);				            
	}

	// On ferme tous les services ouvert
	buttonFile.close();
	photoFile.close();
	cam.release();
	close(sockfd);
	return 0;
}
