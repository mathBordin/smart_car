//compila -w -c
#include "projeto.hpp"
#include "cekeikon.h"
#include <wiringPi.h>
#include <softPwm.h>

int main(void)
{
  SERVER server;
  server.waitConnection();

  wiringPiSetup();
  if (softPwmCreate(0, 0, 100))
    erro("erro");
  if (softPwmCreate(1, 0, 100))
    erro("erro");
  if (softPwmCreate(2, 0, 100))
    erro("erro");
  if (softPwmCreate(3, 0, 100))
    erro("erro");

  VideoCapture cam(0);
  if (!cam.isOpened())
    erro("Erro: Abertura de webcam.");
 	cam.set(CV_CAP_PROP_FRAME_WIDTH,320);
 	cam.set(CV_CAP_PROP_FRAME_HEIGHT,240);
  Mat_<COR> image;
  cam >> image;

  uint32_t comando = 0;
  int ch = -1;

  while (ch <0)
  {
    cam.grab();
    cam.grab();
    cam >> image;
    flip(image, image, -1);

	  server.receiveUint(comando);
	  putTxt (image, 10, 10, to_string (comando), COR (0,255,0), 4);

	  server.sendImgComp(image);

    if (comando == 11)
    {
      softPwmWrite (0, 100);
      softPwmWrite (1, 100);
      softPwmWrite (2, 100);
      softPwmWrite (3, 100);
    }
    else if(comando >= 5)
    {
      softPwmWrite (0, 20 + 10*(comando - 5));
      softPwmWrite (1, 0);
      softPwmWrite (2, 100);
      softPwmWrite (3, 0);
    }
    else
    {
      softPwmWrite (0, 100);
      softPwmWrite (1, 0);
      softPwmWrite (2, 20 + 10*comando);
      softPwmWrite (3, 0);
    }
  }
}
