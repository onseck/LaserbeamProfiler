/* This file is the light main.cpp. please find more fonctions and detail in mainextend.cpp */
/* Copyright Martial Geiser, Alex Miollany & Christopher Tremblay */
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <ctime>
#include <unistd.h>
#include "pixel.hpp"
#include "cvplot.hpp"
#include <cmath>
#include <raspicam/raspicam_cv.h>
#define CVUI_IMPLEMENTATION
#include "cvui.h"
#define WINDOW_NAME "Laser profiler"
#include <fstream>
#include <dirent.h>

using namespace cv;
using namespace std;
using namespace CvPlot;

#define APP_VERSION 0.9
#define RESOLUTION 960
#define PIXEL_SIZE 1.12   //um  x2 
std::string CAM_MODEL="Camera Pi V2";


//compilation : g++ $(pkg-config --cflags --libs opencv) -lraspicam_cv -lraspicam  main.cpp cvplot.cpp pixel.cpp -o LaserPI -Wno-psabi

Mat frameG;

Mat framePrincipale;


Pixel* maximum(Mat frameG,int bruit){    //Recherche du maximum qui sera enregistré dans un Pixel et suppression du bruit
  std::vector<Pixel> v;
  int hauteur=frameG.rows;
  int largeur=frameG.cols;
  Pixel* max=new Pixel();
  int valMax=0;
  for (int i=0;i<largeur;i++){
    for(int j=0;j<hauteur;j++){
      if( frameG.at<uchar>(j,i)>=bruit) frameG.at<uchar>(j,i)-=bruit;    //On evite les valeurs négatives qui perturbent les mesures
      else frameG.at<uchar>(j,i)=0;
      if (frameG.at<uchar>(j,i)>=valMax){    //Technique typique : On parcourt la liste et on enregistre dès que le maximum temporaire est depassé
        max->setX(i);
        max->setY(j);
        valMax=frameG.at<uchar>(j,i);        //frameG.at<uchar>(j,i) --> Acceder à l'element ligne j et colonne i dans la matrice frameG
      }
    }
  }
  max->setIntensity(valMax);
  return max;
}


Pixel* minimum(Mat frameG){   //Recherche du minimum 
  std::vector<Pixel> v;
  int hauteur=frameG.rows;
  int largeur=frameG.cols;
  Pixel* min=new Pixel();
  int valMin=255;
  for (int i=0;i<largeur;i++){
    for(int j=0;j<hauteur;j++){
      if (frameG.at<uchar>(j,i)<=valMin && frameG.at<uchar>(j,i)>0){
        min->setX(i);
        min->setY(j);
        valMin=frameG.at<uchar>(j,i);
      }
    }
  }
  min->setIntensity(valMin);
  return min;
}


int puissance(Mat frame){    //Calcul de la puissance (relative au filtre) : simple somme de tous les pixels de l'image
  int hauteur=frame.rows;
  int largeur=frame.cols;
  int p=0;
  for (int i=0;i<largeur;i++){
    for(int j=0;j<hauteur;j++){
      p+=frame.at<uchar>(j,i);
    }
  }
  return p;
}


/* Calcul des caracteristiques du laser avec les moments. non utilisée car résultats peu convaincants, à suivre
  Les formules sont dans les annexes du rapport et permettent une exploitation plus rigoureuse des données */
void moment(Mat frame,float* pointeur){    // *(pointeur+i)=x_bar,y_bar,moment_x,moment_y,moment_xy,diam_x,diam_y,diam_xy
  int hauteur=frame.rows;             
  int largeur=frame.cols;
  for(int i=0;i<=4;i++) pointeur[i]=0;
  int p=puissance(frame);
  
  for (int i=0;i<largeur;i++){
    for(int j=0;j<hauteur;j++){
      pointeur[0]+=(i*frame.at<uchar>(j,i));     
      pointeur[1]+=(j*frame.at<uchar>(j,i));
    }
  } 
  pointeur[0]/=p;
  pointeur[1]/=p;
  
  for (int i=0;i<largeur;i++){
    for(int j=0;j<hauteur;j++){
      pointeur[2]+=frame.at<uchar>(j,i)*pow((i-pointeur[0]),2);
      pointeur[3]+=frame.at<uchar>(j,i)*pow((j-pointeur[1]),2);
      pointeur[4]+=frame.at<uchar>(j,i)*(i-pointeur[0])*(j-pointeur[1]);
    }
  } 
  pointeur[2]/=p;
  pointeur[3]/=p;
  pointeur[4]/=p;
  
  pointeur[5]=2*sqrt(2)*sqrt(pointeur[2]+pointeur[3]+2*sqrt(pow(pointeur[4],2))); //Valeur en pixel
  pointeur[6]=2*sqrt(2)*sqrt(pointeur[2]+pointeur[3]-2*sqrt(pow(pointeur[4],2))); //Valeur en pixel
  pointeur[7]=2*sqrt(2)*sqrt(pointeur[2]+pointeur[3]);  //Valeur en pixel
}


Mat profilX(Mat* frame,Pixel* pixel,float* mean,float* ecart_type,float* dataProfil,float* largeurProfilX,double* seuilLargeur,int* finesseFiltrage){   
  float normalizer=pixel->getIntensity();      // Normalisation des valeurs par rapport au pixel maximum 
  float newVal=0.f;
  int width=frame->cols;
  if(normalizer==0.f){
    normalizer=1.f;
  }
  int ligne=pixel->getY();
  float* array = (float*)malloc(frame->cols*sizeof(float));
  float* arrayMean = (float*)malloc(frame->cols*sizeof(float));
  for(int i=0;i<frame->cols;i++){
    array[i]=((double)frame->at<uchar>(ligne,i))/normalizer;
    dataProfil[i]=array[i];
  }
  
  float variance=0;   // Calcul moyenne, variance et ecart-type
  for(int j=0;j<width;j++){
    *mean+=array[j];
  }
  *mean=*mean/width;
  for(int j=0;j<width;j++){
    variance+=(array[j]-*mean)*(array[j]-*mean);
  }
  variance=variance/width;
  *ecart_type=sqrt(variance);
  
  for(int i=0;i<width; i++){
  arrayMean[i]=0;
  }
  
  for(int i=*finesseFiltrage/2;i<width-(*finesseFiltrage/2)-1; i++){    //Lissage de la courbe avec finesseFiltrage
		newVal=0;
  	for(int j=i-*finesseFiltrage/2;j<i+*finesseFiltrage/2;j++){
  		newVal+=array[j];
  	}
 		newVal=newVal/(*finesseFiltrage);
 		arrayMean[i]=newVal;
  }
  
  //Recherche du max et du min
  float maxTemp=0;
  float minTemp=1;
  int positionMax;
  for(int i=0;i<width;i++){
  	if(arrayMean[i]>maxTemp){
  		maxTemp=arrayMean[i];
  		positionMax=i;}
  	if(arrayMean[i]<minTemp){
  		minTemp=arrayMean[i];}
  	}
  float seuil=round((*seuilLargeur/100)*(maxTemp-minTemp)*100)/100;
  ////Recherche des barres 
  int positionX1=0;
  int positionX2=0;
  for(int i=0;i<positionMax;i++){
  	if(round(arrayMean[i]*100)/100==seuil){
  		positionX1=i;
  		}
  }
  for(int i=positionMax;i<width;i++){
  	if(round(arrayMean[i]*100)/100==seuil){
  		positionX2=i;
 		}
  }	
  
  if(positionX1!=0 && positionX2!=0){
    *largeurProfilX=PIXEL_SIZE*(positionX2-positionX1)*2;
  }
 	 
  CvPlot::Series serie= CvPlot::Series();
  serie.SetData(frame->cols,array);
  
  CvPlot::Series serieMean= CvPlot::Series();
  serieMean.SetData(frame->cols,arrayMean);
  
  CvPlot::Figure profilX = CvPlot::Figure("Profil X");
  profilX.Add(serie);
  profilX.Add(serieMean);

  IplImage* p = profilX.Show();
  
  cv::Mat m = cv::cvarrToMat(p,true);
  cvReleaseImage(&p);
  
  return m;  
}

Mat profilY(Mat* frame,Pixel* pixel,float* mean,float* ecart_type,float* dataProfil,float* largeurProfilY,double* seuilLargeur,int* finesseFiltrage){
  float normalizer=pixel->getIntensity();
  int width=frame->rows;
  float newVal;
  if(normalizer==0.f){
    normalizer=1.f;
  }
  int colonne=pixel->getX();
  float* array = (float*)malloc(frame->rows*sizeof(float));
  float* arrayMean = (float*)malloc(frame->rows*sizeof(float));
  for(int i=0;i<frame->rows;i++){
    array[i]=((double)frame->at<uchar>(i,colonne))/normalizer;
    dataProfil[i]=array[i];
  }
  
  float variance=0;
  for(int j=0;j<width;j++){
    *mean+=array[j];
  }
  *mean=*mean/width;
  for(int j=0;j<width;j++){
    variance+=(array[j]-*mean)*(array[j]-*mean);
  }
  variance=variance/width;
  *ecart_type=sqrt(variance);
  
  for(int i=0;i<width; i++){
    arrayMean[i]=0;
  }

	for(int i=*finesseFiltrage/2;i<width-(*finesseFiltrage/2)-1; i++){
	  newVal=0;
		for(int j=i-*finesseFiltrage/2;j<i+*finesseFiltrage/2;j++){
			newVal+=array[j];
		}
    newVal=newVal/(*finesseFiltrage);
		arrayMean[i]=newVal;
	}
	
	//Recherche du max et du min
	float maxTemp=0;
	float minTemp=1;
	int positionMax;
	for(int i=0;i<width;i++){
	  if(arrayMean[i]>maxTemp){
			maxTemp=arrayMean[i];
			positionMax=i;
    }
		if(arrayMean[i]<minTemp){
			minTemp=arrayMean[i];
    }
	}
	float seuil=round((*seuilLargeur/100)*(maxTemp-minTemp)*100)/100;
	////Recherche des barres 
	int positionX1=0;
	int positionX2=0;
	for(int i=0;i<positionMax;i++){
		if(round(arrayMean[i]*100)/100==seuil){
			positionX1=i;
		}
	}
	for(int i=positionMax;i<width;i++){
		if(round(arrayMean[i]*100)/100==seuil){
			positionX2=i;
		}
	}	

	if(positionX1!=0 && positionX2!=0){
		*largeurProfilY=PIXEL_SIZE*(positionX2-positionX1)*2;
	}

  CvPlot::Series serie= CvPlot::Series();
  serie.SetData(frame->rows,array);
  CvPlot::Series serieMean= CvPlot::Series();
  serieMean.SetData(frame->rows,arrayMean);
  CvPlot::Figure profilY = CvPlot::Figure("Profil Y");
  profilY.Add(serie);
  profilY.Add(serieMean);
    
	IplImage* p = profilY.Show();
	
	cv::Mat m = cv::cvarrToMat(p,true);
    cvReleaseImage(&p);

  return m;    
}


int add_cmp( int theX,int theY,int theWidth,int theHeight,int finesseFiltrage){ //Fonction d'affichage : permet de dessiner un compteur pour l'utilisateur : avec deux boutons de chaque coté
  cvui::rect(framePrincipale,theX,theY,theWidth,theHeight,0xAfAfAf);         //Tracé d'un rectangle tout autour
  if(cvui::button(framePrincipale, theX, theY,theWidth/6,theHeight, "|<")) finesseFiltrage=2;     //Bouton "retour au minimum"
  if(cvui::button(framePrincipale, theX+theWidth/6, theY,theWidth/6,theHeight, "-")) finesseFiltrage--;   //Bouton de décrementation
  cvui::printf(framePrincipale,theX+2*theWidth/6,theY+4,theWidth/(6*20)*0.5,0xffffff,"%d",finesseFiltrage);     //Compteur de finesseFiltrage
  if(cvui::button(framePrincipale, theX+4*theWidth/6, theY,theWidth/6,theHeight, "+")) finesseFiltrage++;    //Bouton d'incrémentation
  if(cvui::button(framePrincipale, theX+5*theWidth/6, theY,theWidth/6,theHeight, ">|")) finesseFiltrage=256;  // Bouton "avancer au maximum"
  return finesseFiltrage;
}
    
double add_switch(int theX,int theY,int theWidth,int theHeight,double seuil){  //Fonction d'affichage : permet de dessiner un switch 2 positions
  cvui::rect(framePrincipale,theX,theY,theWidth,theHeight,0xAfAfAf);         //Rectangle autour du futur switch
  cvui::printf(framePrincipale,theX-55,theY+4,0.5,0xffffff,"13.5%%");        //premier choix à gauche du switch
  cvui::printf(framePrincipale,theX+theWidth+5,theY+4,0.5,0xffffff,"50.0%%");  //second choix à droite du switch
  if(seuil==13.5){
    if(cvui::button(framePrincipale, theX, theY,2*theWidth/5,theHeight, "")) seuil=50;    //Le curseur est du coté de 13.5, si l'utilisateur appuie on le passe à 50
  }else{
    if(cvui::button(framePrincipale, theX+3*theWidth/5, theY,2*theWidth/5,theHeight, "")) seuil=13.5;  //Inversement
  }
  return seuil;   // On retourne 13.5 ou 50 selon l'emplacement du switch
}


void gradient_relative(int value,int* rgb,int max){
    if(max>10){    //Oblige sinon "exception en point flottant"
      int pas=int(max/5);
      if(value<1){
        rgb[0]=0;
        rgb[1]=0;
        rgb[2]=0;
      }
      else if(value<=pas){
        rgb[0]=255-int(255/pas*value);
        rgb[1]=0;
        rgb[2]=255;
        }
      else if(value<=2*pas){
        rgb[0]=0;
        rgb[1]=-255+int(255/pas*value);
        rgb[2]=255;
      }
      else if(value<=3*pas){
        rgb[0]=0;
        rgb[1]=255;
        rgb[2]=255*3-int(255/pas*value);
      }
      else if(value<=4*pas){
        rgb[0]=-255*3+int(255/pas*value);
        rgb[1]=255;
        rgb[2]=0;
      }
      else{
        rgb[0]=255;
        rgb[1]=255*5-int(255/pas*value);
        rgb[2]=0;
      }
    }
    else{
      rgb[0]=0;
      rgb[1]=0;
      rgb[2]=0;
    }

}

void gradient_absolute(int value,int* rgb){
  int pas=51;
    if(value<1){
      rgb[0]=0;
      rgb[1]=0;
      rgb[2]=0;
    }
    else if(value<=pas){
      rgb[0]=255-int(255/pas*value);
      rgb[1]=0;
      rgb[2]=255;
      }
    else if(value<=2*pas){
      rgb[0]=0;
      rgb[1]=-255+int(255/pas*value);
      rgb[2]=255;
    }
    else if(value<=3*pas){
      rgb[0]=0;
      rgb[1]=255;
      rgb[2]=255*3-int(255/pas*value);
    }
    else if(value<=4*pas){
      rgb[0]=-255*3+int(255/pas*value);
      rgb[1]=255;
      rgb[2]=0;
    }
    else{
      rgb[0]=255;
      rgb[1]=255*5-int(255/pas*value);
      rgb[2]=0;
    }

}



int main(int argc, char** argv){
  Mat frame;   // frame=framec  --> affichage du maximum sans perturber les mesures suivantes
  Mat framec;    
  
  Mat frameT;    // frameT=frameTc  --> affichage du maximum sans perturber les mesures suivantes
  Mat frameTc;

  Pixel* min;    //Pixel minimum
  Pixel* max;    //Pixel maximum
  int puis;       //Puissance
  raspicam::RaspiCam_Cv cap;   //Initialisation de la camera
  float* meanY;      //Moyenne selon les lignes
  float* ecart_typeY;   
  float* meanX;         //Moyenne selon les colonnes
  float* largeurProfilX;    //resultat traitement : diametre selon X
  float* largeurProfilY;    // diametre selo Y
  float* diam=(float *)malloc(sizeof(float) *8 );   //  x_bar,y_bar,moment_x,moment_y,moment_xy,diam_x,diam_y
  int* rgb=(int *)malloc(sizeof(int) *3 );
  largeurProfilX=(float*)malloc(sizeof(float));
  largeurProfilY=(float*)malloc(sizeof(float));
  float* ecart_typeX;
  int hauteur=0;
  int largeur=0;
  int finesseFiltrage=32;
  int numeroReleve=0;
  meanY=(float*)malloc(sizeof(float));
  ecart_typeY=(float*)malloc(sizeof(float));
  meanX=(float*)malloc(sizeof(float));
  ecart_typeX=(float*)malloc(sizeof(float));
  float* dataProfilX;
  float* dataProfilY;
  dataProfilX=(float*)malloc(RESOLUTION*sizeof(float));
  dataProfilY=(float*)malloc(RESOLUTION*sizeof(float));
  double seuilLargeur=13.5;
  *largeurProfilX=0;
  *largeurProfilY=0;
  int luminosite=50;
  int saturation=50;
  int contraste=50;
  int gain=50;
  int tempsExpo=-1;
  String sTempsExpo;
  bool modeExpert=false;
  int affichage=0;
  int langue=0;
  Point textOrg;
  std::string name="";  
  
  cvNamedWindow(WINDOW_NAME, CV_WINDOW_NORMAL);
  cvSetWindowProperty(WINDOW_NAME, CV_WND_PROP_FULLSCREEN, CV_WINDOW_FULLSCREEN);
  cvui::init(WINDOW_NAME);
  
  Mat frameA = Mat(320,320,CV_8UC3); //Affichage du retour camera
  Mat frameAc = Mat(320,320,CV_8UC3); //Affichage du retour camera
  Mat frameScale=Mat(20,320,CV_8U);
  Mat frameP1,frameP11,frameP2,frameP22,framePG1,framePG2;
  frameP11=Mat(160,320,CV_8U);
  frameP22=Mat(320,160,CV_8U);
  cap.set( CV_CAP_PROP_FORMAT, CV_8UC3 );
  
  ofstream myfile;
  
  if (!cap.open()) {cerr<<"Error opening the camera"<<endl;return -1;}
  cout << cap.get(CV_CAP_PROP_FRAME_WIDTH)<<"x"<<cap.get(CV_CAP_PROP_FRAME_HEIGHT) << endl;
  cout << "Start grabbing" << endl
      << "Press any key to terminate" << endl;
  
  int x=0;
  int y=0;
  bool pause=false;
  bool calc_moment=false;
  bool about=false;
  
  Mat frametest;
  Mat image = imread("Echelle.PNG", CV_LOAD_IMAGE_COLOR);  
  
  for (;;){
 	
  	framePrincipale = cv::Mat(480, 800, CV_8UC3);    //Creation de la frame principale
  	framePrincipale.setTo( cv::Scalar(49,52,49) );
   
   if(!pause && !about){      //Aucun bouton appuye, mode Play
    calc_moment=false;
  	cap.set(CV_CAP_PROP_EXPOSURE,tempsExpo);   //Mise à jour des réglages de la caméra à chaque tour
  	cap.set (CV_CAP_PROP_CONTRAST ,contraste);
  	cap.set ( CV_CAP_PROP_SATURATION, saturation);
  	cap.set ( CV_CAP_PROP_GAIN, gain);
  	cap.set(CV_CAP_PROP_BRIGHTNESS,luminosite);


  	
  	cap.grab();  // wait for a new frame from camera and store it into 'frame'
    cap.retrieve(frameT);//Capture de l'image
    frame=frameT(cv::Rect(159,0,RESOLUTION,RESOLUTION));
    rotate(frame,frame,2);
    resize(frame,frameA,frameA.size(),0,0,INTER_NEAREST);
    cvtColor(frame,frameG, CV_BGR2GRAY);
     
  /*//save image of laser to extract
    vector<int> compression_params;
    compression_params.push_back(CV_IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(9);
    imwrite("image.png", frameT, compression_params);   */
     
    min=minimum(frameG);   //chercher min et max et calcul de la puissance
    max=maximum(frameG,min->getIntensity());
    if(max->getX()<960 && max->getX()>0 && 960-max->getY()<960 && 960-max->getY()>0){     //Controle que le max soit dans l'image
      x=max->getX();
      y=max->getY();
    } 
    cap.retrieve(frameTc);          //Capture de l'image
    framec=frameTc(cv::Rect(159,0,RESOLUTION,RESOLUTION));
    
    //rotate(framec,frametest,2);
   /* int c;
    for(int i=0;i<framec.rows;i++){
      for(int j=0;j<framec.cols*3;j=j+3){
      c=frametest.at<uchar>(i,j/3);
      gradient(c,rgb);
      framec.at<uchar>(i,j)=rgb[2];
      framec.at<uchar>(i,j+1)=rgb[1];
      framec.at<uchar>(i,j+2)=rgb[0];
    }
    }*/
    
    
    
    
    
    textOrg.x=x;    // Point d'origine de la croix
    textOrg.y=y;
 /*   line(framec, textOrg,textOrg + Point(960, 0),Scalar(255, 255, 255),3);    //On trace une croix sur le max branche par branche
    line(framec, textOrg,textOrg + Point(-960,0),Scalar(255, 255, 255),3);
    line(framec, textOrg,textOrg + Point(0, 960),Scalar(255, 255, 255),3);
    line(framec, textOrg,textOrg + Point(0,-960),Scalar(255, 255, 255),3);*/
    rotate(framec,framec,2);
    if(affichage!=2){
      int c;
      for(int i=0;i<framec.rows;i++){
        for(int j=0;j<framec.cols*3;j=j+3){
        c=frameG.at<uchar>(i,j/3);
        if(affichage==0) gradient_relative(c,rgb,max->getIntensity());
        else if(affichage==1) gradient_absolute(c,rgb);      
        framec.at<uchar>(i,j)=rgb[2];
        framec.at<uchar>(i,j+1)=rgb[1];
        framec.at<uchar>(i,j+2)=rgb[0];
        }
      }
    }
    line(framec, textOrg,textOrg + Point(960, 0),Scalar(255, 255, 255),3);    //On trace une croix sur le max branche par branche
    line(framec, textOrg,textOrg + Point(-960,0),Scalar(255, 255, 255),3);
    line(framec, textOrg,textOrg + Point(0, 960),Scalar(255, 255, 255),3);
    line(framec, textOrg,textOrg + Point(0,-960),Scalar(255, 255, 255),3);
    resize(framec,frameAc,frameA.size(),0,0,INTER_NEAREST);
    //cvtColor(frameAc,frameAc, CV_BGR2GRAY);
    cvui::image(framePrincipale,0,0,frameAc);     //Ajout de la frame à l'interface
    if(affichage!=2){
      resize(image,frameScale,frameScale.size(),0,0,INTER_NEAREST);
      cvui::image(framePrincipale,0,0,frameScale);
    }
    frameP1=profilX(&frameG,max,meanX,ecart_typeX,dataProfilX,largeurProfilX,&seuilLargeur,&finesseFiltrage);
    resize(frameP1,frameP11,frameP11.size(),0,0,INTER_NEAREST);
    cvui::image(framePrincipale,0,320,frameP11);//PROFIL X
        
    frameP2=profilY(&frameG,max,meanY,ecart_typeY,dataProfilY,largeurProfilY,&seuilLargeur,&finesseFiltrage);
    rotate(frameP2,frameP2,2);
    resize(frameP2,frameP22,frameP22.size(),0,0,INTER_NEAREST);
    flip(frameP22,frameP22,0);
    cvui::image(framePrincipale,320,0,frameP22);//PROFIL Y
        
    if(*meanY>=0 && *meanY<=1){
      cvui::printf(framePrincipale,500,50, 0.5, 0xffffff, "Y Profile:");
  	  cvui::printf(framePrincipale,500,70,0.5,0xffffff,"Width: ~%d microns @ Y=%.0fum",int(*largeurProfilY),-(max->getY()-RESOLUTION/2)*2*PIXEL_SIZE);
  	}
  	if(*meanX>=0 && *meanX<=1){
      cvui::printf(framePrincipale,330,430, 0.5, 0xffffff, "X Profile:");
  	  cvui::printf(framePrincipale,330,450,0.5,0xffffff,"Width: ~%d microns @ X=%.0fum",int(*largeurProfilX),(max->getX()-RESOLUTION/2)*2*PIXEL_SIZE);
  	}
  	cvui::checkbox(framePrincipale,330,335,"Expert mode",&modeExpert);
   
   
    if(modeExpert){
    cvui::trackbar(framePrincipale,500,130,200,&contraste,(int)0,(int)100);
    cvui::printf(framePrincipale,550,130,0.3,0xffffff,"Contrast:");
  	
  	cvui::trackbar(framePrincipale,500,180,200,&luminosite,(int)0,(int)100);
  	cvui::printf(framePrincipale,550,180,0.3,0xffffff,"Luminosity:");
  	
  	cvui::trackbar(framePrincipale, 500,330,200,&seuilLargeur,(double)13.5,(double)50,1,"%.1f",cvui::TRACKBAR_DISCRETE | cvui::TRACKBAR_HIDE_VALUE_LABEL,(double)36.5);
  	cvui::printf(framePrincipale,550,330, 0.3, 0xffffff, "Threshold value:");
  	
  	cvui::trackbar(framePrincipale,330,380,400,&tempsExpo,(int)1,(int)30);
  	cvui::printf(framePrincipale,425,380,0.4,0xffffff,"Shutter speed: %.2f ms",tempsExpo*(100/99)*0.033);
  	
  	cvui::trackbar(framePrincipale, 500,280,200,&finesseFiltrage,(int)0,(int)256,1,"%.0Lf",cvui::TRACKBAR_DISCRETE,2);
  	cvui::printf(framePrincipale,515,280, 0.3, 0xffffff, "Number of samples:(Smoothing)");
  	
  	cvui::trackbar(framePrincipale, 500,230,200,&gain,(int)0,(int)100);
  	cvui::printf(framePrincipale,550,230, 0.3, 0xffffff, "Gain");
    }else{
     
      cvui::printf(framePrincipale,500,180, 0.5, 0xffffff, "Number of samples:(Smoothing)");
      finesseFiltrage=add_cmp(530,200,120,20,finesseFiltrage);
    
      cvui::printf(framePrincipale,500,250, 0.5, 0xffffff, "Threshold value:");
      seuilLargeur=add_switch(580,270,50,20,seuilLargeur);
      
      
      if(affichage==0){
        cvui::printf(framePrincipale,500,330, 0.5, 0xffffff, "Display mode: relative");
        if(cvui::button(framePrincipale,500,350,"Change Display Mode")) affichage=1;
        }
      else if(affichage==1){
        cvui::printf(framePrincipale,500,330, 0.5, 0xffffff, "Display mode: absolute");
        if(cvui::button(framePrincipale,500,350,"Change Display Mode")) affichage=2;
        }
      else if(affichage==2){
        cvui::printf(framePrincipale,500,330, 0.5, 0xffffff, "Display mode: normal");
        if(cvui::button(framePrincipale,500,350,"Change Display Mode")) affichage=0;
        }
    }
   }
    else if(pause && !about){   //Mode Pause : affichage fige et exportation de l'interface
    if(!calc_moment){    //test pour calculer les moments que la première fois (pas a chaque raffraichissement de la page)
      puis=puissance(frameG);
      moment(frameG,diam);
      }

    circle(framec,Point(diam[0],diam[1]),diam[7]/2,Scalar( 0, 0, 255 ),3);      //Cercle autour du laser
    circle(framec,Point(diam[0],diam[1]),500/32.0,Scalar( 0, 0,255 ),2);        //Cercle entourant le pixel maximum (selon les normes)
    resize(framec,frameAc,frameA.size(),0,0,INTER_NEAREST);
    cvui::image(framePrincipale,0,0,frameAc);     //Ajout de la frame à l'interface
    if(affichage!=2){
      resize(image,frameScale,frameScale.size(),0,0,INTER_NEAREST);
      cvui::image(framePrincipale,0,0,frameScale);
    }
    cvui::image(framePrincipale,0,320,frameP11);//PROFIL X
    cvui::image(framePrincipale,320,0,frameP22);//PROFIL Y
    cvui::printf(framePrincipale,500,60,0.5,0xffffff,"File: %s.png",name.c_str());
    cvui::printf(framePrincipale,500,80,0.5,0xffffff,"App version: %.1f",APP_VERSION);
    cvui::printf(framePrincipale,500,120,0.5,0xffffff,"Position of the max:");
    cvui::printf(framePrincipale,510,140,0.5,0xffffff,"X=%.0fum      Y=%.0fum",(diam[0]-RESOLUTION/2)*2*PIXEL_SIZE,(diam[1]-RESOLUTION/2)*2*PIXEL_SIZE);
    cvui::printf(framePrincipale,500,180,0.5,0xffffff,"Diameter at 13.5%%:");
    cvui::printf(framePrincipale,510,200,0.5,0xffffff,"X=%.0fum      Y=%.0fum",diam[5]*2*PIXEL_SIZE,diam[6]*2*PIXEL_SIZE);
    cvui::printf(framePrincipale,500,220,0.5,0xffffff,"     Mean=%.0fum",diam[7]*2*PIXEL_SIZE);
    cvui::printf(framePrincipale,500,260,0.5,0xffffff,"Power=%d [a.u.i]",puis);
    cvui::printf(framePrincipale,500,280,0.5,0xffffff,"Noise=%.1f%%",float(min->getIntensity()*100)/255);
    
    cvui::printf(framePrincipale,330,350,0.5,0xffffff,"Camera features: %s",CAM_MODEL.c_str());
    cvui::printf(framePrincipale,350,380,0.5,0xffffff,"Contrast=%d",contraste);
    cvui::printf(framePrincipale,350,400,0.5,0xffffff,"Saturation=%d",saturation);
    cvui::printf(framePrincipale,350,420,0.5,0xffffff,"Luminosity=%d",luminosite);
    cvui::printf(framePrincipale,520,380,0.5,0xffffff,"Gain=%d",gain);
    cvui::printf(framePrincipale,520,400,0.5,0xffffff,"Smoothing=%d",finesseFiltrage);
    cvui::printf(framePrincipale,520,420,0.5,0xffffff,"Shutter speed=%.2fus",abs(tempsExpo*(100/99)*0.033));
    
    cvui::printf(framePrincipale,330,460,0.5,0xffffff,"Laser Beam Analyser by HESSO (Geiser Miollany Tremblay)");
    sleep(0.5);
      if(!calc_moment){
        imwrite("./measurement/"+name+".png",framePrincipale);   //Une fois tous les calculs faits on exporte la frame
        calc_moment=true;
      }
    }

    else if(about){           //Bouton About presse : Affichage d'un texte
        if(cvui::button(framePrincipale,40,15,"English")) langue=0;
        if(cvui::button(framePrincipale,140,15,"Francais")) langue=1;
        if(cvui::button(framePrincipale,240,15,"Deutsch")) langue=2;
        
        ifstream fichier_about;
        if(langue==0) fichier_about.open("about_english.txt");  //Ouverture d'un fichier en lecture
        else if(langue==1) fichier_about.open("about_francais.txt");  //Ouverture d'un fichier en lecture
        else if(langue==2) fichier_about.open("about_deutsch.txt");  //Ouverture d'un fichier en lecture
        
        if(fichier_about){   //Oblige d'ouvrir le fichier a chaque passage ou alors creation d'un pointeur dynamique de taille = nb ligne
          string ligne;
          getline(fichier_about, ligne);   //Pas d'affichage pour la premiere phrase du fichier
          int initX=20;
          int initY=50;
          int pas=20;   //Affichage toutes les 20 lignes
          int k=0;
          while(getline(fichier_about, ligne)){
            cvui::printf(framePrincipale,initX,initY+k*pas,0.5,0xffffff,ligne.c_str());
            k++;
             }
          fichier_about.close();
        }else  cout << "ERREUR: Impossible d'ouvrir le fichier en lecture." << endl ;
      sleep(0.5);
    }
      
    
  	if(cvui::button(framePrincipale,500,15,"Play/Pause")){
       if(about) pause=true;      //L'appui sur Play/Pause dans le mode about met toujours le mode play (sans enregistrer les frames)
       about=false;
      if(!pause){
        time_t timer1;
        time(&timer1);
        
        struct tm *newTime1;
        newTime1 = localtime(&timer1); //recuperation de l'heure du systeme
        
        int annees,mois,jours,secondes, minutes, heures;
        annees = 1900+newTime1->tm_year; 
        mois = 1+newTime1->tm_mon; 
        jours = newTime1->tm_mday;
        heures = newTime1->tm_hour;		
        minutes = newTime1->tm_min;		
        secondes = newTime1->tm_sec;	
        name=std::to_string(annees)+"_"+std::to_string(mois)+"_"+std::to_string(jours)+"_"+std::to_string(heures)+":"+std::to_string(minutes)+":"+std::to_string(secondes);

  			std::string nomCsv=name+".csv";          //CSV des profils
        std::string nomImage=name+"_image.png";         //Image du laser  
    		
    		myfile.open ("./measurement/"+nomCsv);
    		if(&myfile!=NULL){
    		  myfile << "X "<<","<< "Y\n";
    		  myfile << *largeurProfilX << "," << *largeurProfilY <<","<< "Seuil"<<"\n";
    		  for(int i=0;i<frame.cols;i++){
    			  myfile << dataProfilX[i]<<","<< dataProfilY[i]<<"\n";
    			}
    			myfile.close();
    			imwrite("./measurement/"+nomImage,frameT);     //Exportation de l'image du laser dans releves
    			cvui::printf(framePrincipale,500,110,0.5,0xffffff,"Exported in /measurement"); 
    		}else cvui::printf(framePrincipale,500,110,0.5,0xffffff,"Fichier non trouve");
       }
       pause=!pause;
 		} 
    
    
    
    if(cvui::button(framePrincipale,630,15,"About")){
      if(about) pause=false;          //On repasse en mode play a chaque fois que le mode about est quitte
      about=!about;
     }
    
  	if(cvui::button(framePrincipale,720,15,"Quit")) break;
  	
    cvui::update();
  	cv::imshow(WINDOW_NAME, framePrincipale);
  	
    if (frame.empty()) {
      cerr << "ERROR! blank frame grabbed\n";
      break;
    }
    if (waitKey(16) >= 0) break;
  }
}
