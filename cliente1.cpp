#include "projeto.hpp"

uint32_t estado = 0; //0 = nao_apertado, 1 = apertou_botao_1, 2 = apertou_botao_2

// mouse callback
void on_mouse(int event, int c, int l, int flags, void* userdata)
{
  if(event == EVENT_LBUTTONDOWN)
  {
    if(0<=l && l<80 && 0<=c && c<80)            estado = 7;
    else if(0<=l && l<80 && 80<=c && c<160)     estado = 8;
    else if(0<=l && l<80 && 160<=c && c<240)    estado = 9;
    else if(80<=l && l<160 && 0<=c && c<80)     estado = 4;
    else if(80<=l && l<160 && 80<=c && c<160)   estado = 0;
    else if(80<=l && l<160 && 160<=c && c<240)  estado = 6;
    else if(160<=l && l<240 && 0<=c && c<80)    estado = 1;
    else if(160<=l && l<240 && 80<=c && c<160)  estado = 2;
    else if(160<=l && l<240 && 160<=c && c<240) estado = 3;
    else estado = 0;
  }
  else if(event == EVENT_LBUTTONUP)
  {
    estado = 0;
  }
}

int main(int argc, char *argv[])
{
  bool video = false;
  String name_video;
  if(argc!=2)
  {
    if (argc=3)
    {
      video = true;
      name_video = argv[2];
    }
    else
    {
      erro("client1 servidorIdAddr\n");
    }
  }

  VideoWriter vo (name_video, CV_FOURCC ( 'X','V', 'I', 'D'), 15, Size (560, 240));

  CLIENT client(argv[1]);

  Mat_<COR> frame;

  COR cinza(128, 128, 128);
  COR vermelho(0, 0, 255);

  Mat_<COR> imagem(240, 560, cinza);

  namedWindow("janela", WINDOW_NORMAL);
  resizeWindow("janela", 2*imagem.cols, 2*imagem.rows);

  setMouseCallback("janela", on_mouse);

  int ch = -1;
  int borda_flecha = 20;

  while(ch < 0)
  {
    client.sendUint(estado);
    client.receiveImgComp(frame);

    imagem.setTo(cinza);

    reta(imagem, 0, 0, 0, 240);
    reta(imagem, 80, 0, 80, 240);
    reta(imagem, 160, 0, 160, 240);
    reta(imagem, 240, 0, 240, 240);
    reta(imagem, 0, 0, 240, 0);
    reta(imagem, 0, 80, 240, 80);
    reta(imagem, 0, 160, 240, 160);
    reta(imagem, 0, 240, 240, 240);

    flecha(imagem,80-borda_flecha ,80-borda_flecha ,borda_flecha    ,borda_flecha    ,COR(0,0,0),2);
    flecha(imagem,80-borda_flecha ,120             ,borda_flecha    ,120             ,COR(0,0,0),2);
    flecha(imagem,80-borda_flecha ,160+borda_flecha,borda_flecha    ,240-borda_flecha,COR(0,0,0),2);
    flecha(imagem,120             ,80-borda_flecha ,120             ,borda_flecha    ,COR(0,0,0),2);
    flecha(imagem,120             ,120             ,120             ,120             ,COR(0,0,0),6);
    flecha(imagem,120             ,160+borda_flecha,120             ,240-borda_flecha,COR(0,0,0),2);
    flecha(imagem,160+borda_flecha,80-borda_flecha ,240-borda_flecha,borda_flecha    ,COR(0,0,0),2);
    flecha(imagem,160+borda_flecha,120             ,240-borda_flecha,120             ,COR(0,0,0),2);
    flecha(imagem,160+borda_flecha,160+borda_flecha,240-borda_flecha,240-borda_flecha,COR(0,0,0),2);

    if(estado == 7)
    {
      flecha(imagem, 80-borda_flecha, 80-borda_flecha, borda_flecha, borda_flecha, COR(0,0,255), 2);
    }
    else if(estado == 8)
    {
      flecha(imagem, 80-borda_flecha, 120, borda_flecha, 120, COR(0,0,255), 2);
    }
    else if(estado == 9)
    {
      flecha(imagem, 80-borda_flecha, 160+borda_flecha,borda_flecha, 240-borda_flecha, COR(0,0,255), 2);
    }
    else if(estado == 4)
    {
      flecha(imagem, 120, 80-borda_flecha, 120, borda_flecha, COR(0,0,255), 2);
    }
    else if(estado == 0)
    {
      flecha(imagem, 120, 120, 120, 120, COR(0,0,255), 6);
    }
    else if(estado == 6)
    {
      flecha(imagem, 120, 160+borda_flecha, 120, 240-borda_flecha, COR(0,0,255), 2);
    }
    else if(estado == 1)
    {
      flecha(imagem, 160+borda_flecha, 80-borda_flecha, 240-borda_flecha, borda_flecha, COR(0,0,255), 2);
    }
    else if(estado == 2)
    {
      flecha(imagem, 160+borda_flecha, 120, 240-borda_flecha, 120, COR(0,0,255), 2);
    }else if(estado == 3)
    {
      flecha(imagem, 160+borda_flecha, 160+borda_flecha, 240-borda_flecha, 240-borda_flecha, COR(0,0,255), 2);
    }

    for(int l=0; l<240; l++)
      for(int c=0; c<320; c++)
        imagem (l, c+240) = frame (l,c);

    if (video)
    {
      vo << imagem;
    }

    imshow("janela", imagem);

    ch = waitKey(30);
  }

  return 0;
}
