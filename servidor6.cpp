//compila -w -c
// Versao final do programa da experiencia 2
// do servidor (carrinho)
#include "projeto.hpp"
#include "cekeikon.h"
#include <wiringPi.h>
#include <softPwm.h>

int main(void)
{
  // Conecta ao servidor
  SERVER server;
  server.waitConnection();

  // Inicializa GPIO utilizada para controlar os motores
  wiringPiSetup();
  if (softPwmCreate(8, 0, 100))
    erro("erro");
  if (softPwmCreate(9, 0, 100))
    erro("erro");
  if (softPwmCreate(21, 0, 100))
    erro("erro");
  if (softPwmCreate(22, 0, 100))
    erro("erro");

  // Carrinho começa parado
  softPwmWrite (8, 0);
  softPwmWrite (9, 0);
  softPwmWrite (21, 0);
  softPwmWrite (22, 0);

  // Inicializa camera
  VideoCapture cam(0);
  if (!cam.isOpened())
    erro("Erro: Abertura de webcam.");
 	cam.set(CV_CAP_PROP_FRAME_WIDTH,320);
 	cam.set(CV_CAP_PROP_FRAME_HEIGHT,240);
  Mat_<COR> image;
  cam >> image;

  uint32_t comando = 0;
  int acao = 0;
  int mov = 0;
  int frame = 0;
  int ch = -1;

  // laco principal
  while (ch <0)
  {
    // descarta frames passados e pega atual
    cam.grab();
    cam.grab();
    cam >> image;
    // inverte imagem que foi recebida de ponta cabeça
    flip(image, image, -1);

    // recebe comando enviado pelo cliente para comandar carrinho
	  server.receiveUint(comando);
    //envia frame atual para o cliente processar e retornar comando
    server.sendImgComp(image);

    // a temporizacao das acoes do carrinho sao feitas com base na quantidade
    // de frames recebidos.
    // quando a quantidade de frames determinada para cada movimento é atingida
    // o carrinho para e sai do modo de acao e zera a contagem de frames.
    // essa solucao é melhor do que wait ou sleep, pois não trava envio de
    // imagens, porem fica dependente da velocidade da transmissao de dados.
    if(acao == 1)
    {
      frame ++;
      if((mov == 12 || mov == 13) && frame == 30) // giro de 180
      {
        softPwmWrite (9, 0);
        softPwmWrite (9, 0);
        softPwmWrite (21, 0);
        softPwmWrite (22, 0);
        acao = 0;
        frame = 0;
      }
      else if((mov == 14 || mov == 15) && frame == 16) //giro de 90
      {
        softPwmWrite (8, 0);
        softPwmWrite (9, 0);
        softPwmWrite (21, 0);
        softPwmWrite (22, 0);
        acao = 0;
        frame = 0;
      }
      else if((mov == 16) && frame == 60) //seguir em frente
      {
        softPwmWrite (8, 0);
        softPwmWrite (9, 0);
        softPwmWrite (21, 0);
        softPwmWrite (22, 0);
        acao = 0;
        frame = 0;
      }
    }

    // realiza as acoes do carrinho de acorda com
    // os comandos recebidos, de acordo com os digitos lidos ou com
    // a posicao da placa
    if (comando == 11)
    {
      softPwmWrite (8, 100);
      softPwmWrite (9, 100);
      softPwmWrite (21, 100);
      softPwmWrite (22, 100);
    }
    else if(comando > 5 && comando < 10)
    {
      softPwmWrite (8, 20 + 10*(comando - 5));
      softPwmWrite (9, 0);
      softPwmWrite (21, 80);
      softPwmWrite (22, 0);
    }
    else if(comando < 5)
    {
      softPwmWrite (8, 80);
      softPwmWrite (9, 0);
      softPwmWrite (21, 20 + 10*comando);
      softPwmWrite (22, 0);
    }
    else if(comando == 5)
    {
      softPwmWrite (8, 75);
      softPwmWrite (9, 0);
      softPwmWrite (21, 80);
      softPwmWrite (22, 0);
    }
    else if(comando == 12)
    {
      printf("180 left\n");
      softPwmWrite (8, 80);
      softPwmWrite (9, 0);
      softPwmWrite (21, 0);
      softPwmWrite (22, 80);
      acao = 1;
      mov = comando;
    }
    else if(comando == 13)
    {
      printf("180 right\n");
      softPwmWrite (8, 0);
      softPwmWrite (9, 80);
      softPwmWrite (21, 80);
      softPwmWrite (22, 0);
      acao = 1;
      mov = comando;
    }
    else if(comando == 14)
    {
      printf("90 left\n");
      softPwmWrite (8, 80);
      softPwmWrite (9, 0);
      softPwmWrite (21, 0);
      softPwmWrite (22, 80);
      acao = 1;
      mov = comando;
    }
    else if(comando == 15)
    {
      printf("90 right\n");
      softPwmWrite (8, 0);
      softPwmWrite (9, 80);
      softPwmWrite (21, 80);
      softPwmWrite (22, 0);
      acao = 1;
      mov = comando;
    }
    else if(comando == 16)
    {
      printf("forward\n");
      softPwmWrite (8, 75);
      softPwmWrite (9, 0);
      softPwmWrite (21, 80);
      softPwmWrite (22, 0);
      acao = 1;
      mov = comando;
    }
  }
}
