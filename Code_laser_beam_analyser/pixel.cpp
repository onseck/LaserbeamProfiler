#include "pixel.hpp"

 Pixel::Pixel(){
  this->x=0;
  this->y=0;
}

int Pixel::getX(){
  return this->x;
}
int Pixel::getY(){
  return this->y;
}
void Pixel::setX(int x){
  this->x=x;
}
void Pixel::setY(int y){
  this->y=y;
}
int Pixel::getIntensity(){
  return this->intensity;
}

void Pixel::setIntensity(int intensity){
  this->intensity=intensity;
}
