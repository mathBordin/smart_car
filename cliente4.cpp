#include "projeto.hpp"
#include <cekeikon.h>

using namespace Morphology;

int main(int argc, char *argv[])
{
  if(argc!=2)
    erro("fase4 servidorIdAddr\n");
  CLIENT client(argv[1]);

  VideoWriter vo ("car_vision.avi", CV_FOURCC ( 'X','V', 'I', 'D'), 15, Size (320, 240));

  Mat_<FLT> frame, templ;
  Mat_<FLT> templ_placa[8], templ_match_result[8];
  Mat_<COR> output, frame_cor, in, out;
  int size;
  float max_corr;

  // Abre template
  le(templ, "quadrado.png");

  // Redimensiona templates
  for(int i = 0; i < 8; i ++)
  {
    resize(templ,templ_placa[i],Size(round(55*pow(0.81, i)), round(55*pow(0.81, i))),0,0,INTER_AREA);
    templ_placa[i]=somaAbsDois(dcReject(templ_placa[i], 1.0));
  }

  // Cria janela para mostrar saída
  namedWindow("raspcar_cam", WINDOW_NORMAL);

  int ch = -1;
  client.sendUint(11);
  while(ch < 0)
  {
    max_corr = 0.0;
    size = 0;
    float max_corr_aux[8];
    int _l[8], _c[8];
    int centro_l_preto = 0, centro_c_preto = 0, metade_tamanho_preto = 0;

    client.receiveImgComp(frame_cor);
    output = frame_cor.clone();
    converte(output, frame);

    // Faz o template match
    #pragma omp parallel for
    for(int i = 0; i < 8; i++)
    {
      templ_match_result[i] = matchTemplateSame(frame,templ_placa[i],CV_TM_CCORR);
    }

    // Procura resultado com maior correlação
    #pragma omp parallel for
    for (int i = 0; i < 8; i++)
    {
      max_corr_aux[i] = 0.0;
      for(int l=0; l<templ_match_result[i].rows; l++)
        for(int c=0; c<templ_match_result[i].cols; c++)
        {
          if(templ_match_result[i](l,c) > max_corr_aux[i])
          {
            max_corr_aux[i] = templ_match_result[i](l,c);
            _l[i] = l;
            _c[i] = c;
          }
        }
    }

    for(int i = 0; i < 8; i++)
    {
      if(max_corr_aux[i] > max_corr)
      {
        size = i;
        max_corr = max_corr_aux[i];
        centro_c_preto = _c[i];
        centro_l_preto = _l[i];
      }
    }

    if (max_corr < 0.21)
    {
      //NÃO HÁ PLACA!
      client.sendUint(11);
    }
    else
    {
      // Desenha o retangulo amarelo na imagem original
      metade_tamanho_preto = (55*pow(0.81, size))/2;
      Point pt1 (centro_c_preto-metade_tamanho_preto, centro_l_preto-metade_tamanho_preto);
      Point pt2 (centro_c_preto+metade_tamanho_preto, centro_l_preto+metade_tamanho_preto);
      rectangle(output,pt1,pt2,Scalar( 0, 255, 255 ));
      //controla carrinho
      if(size == 0 || size ==1)
      {
        client.sendUint(11);
      }
      else
      {
        float command = (float)(centro_c_preto)/(float)(output.cols);
        command = 10*command;
        client.sendUint((int)round(command));
  			char str[100];
  			sprintf(str, "Dir_x = %.2f", command);
  			putTxt (output, 50, 10, str, COR (255,0,0), 1);
      }
    }

    imshow("raspcar_cam", output);
    vo << output;
    ch = waitKey(30);
  }
  return 0;
}
