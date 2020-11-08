#include "projeto.hpp"
#include "cekeikon.h"

int main(void)
{

  SERVER server;
  server.waitConnection();

  VideoCapture cam(0);
  if (!cam.isOpened())
    erro("Erro: Abertura de webcam.");

  cam.set(CV_CAP_PROP_FRAME_WIDTH,320);
 	cam.set(CV_CAP_PROP_FRAME_HEIGHT,240);

  Mat_<COR> image;
  cam >> image;

  uint32_t comando = 0;

  while (1)
  {
    cam >> image;
  	server.receiveUint(comando);
  	putTxt (image, 10, 10, to_string (comando), COR (0,255,0), 4);
    server.sendImgComp (image);
  }
}
