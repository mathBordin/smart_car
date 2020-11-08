#include <cekeikon.h>
#include <time.h>

using namespace Morphology;

int main(int argc, char** argv)
{
  if(argc!=4)
    erro("fase3 capturado.avi quadrado.png localiza.avi\n");

  Mat_<FLT> frame, templ;
  Mat_<FLT> templ_placa[8], templ_match_result[8];
  Mat_<GRY> melhor_frame;
  Mat_<COR> output, frame_cor, in, out;

  int size;
  float max_corr;

  VideoCapture vi(argv[1]);
  if (!vi.isOpened())
    erro("Erro na abertura do video de entrada");
  int nc=vi.get(CV_CAP_PROP_FRAME_WIDTH);
  int nl=vi.get(CV_CAP_PROP_FRAME_HEIGHT);
  int fps=vi.get(CV_CAP_PROP_FPS);

	namedWindow("output", WINDOW_NORMAL);

  VideoWriter vo(argv[3], CV_FOURCC('X','V','I','D'),	fps, Size(nc,nl));
  if (!vo.isOpened())
    erro("Erro na abertura do video de saida");

  // Abre template e cria vetor de templates
  le(templ, argv[2]);
  for(int i = 0; i < 8; i ++)
  {
    resize(templ,templ_placa[i],Size(55*pow(0.81, i), 55*pow(0.81, i)),0,0,INTER_AREA);
    templ_placa[i]=somaAbsDois(dcReject(templ_placa[i], 1.0));
  }

  // Start and end times
  time_t start, end;

  // Start time
  time(&start);
  while (true)
  {
    vi >> frame_cor;
    if (!frame_cor.data)
      break;
    output = frame_cor.clone();
    converte(frame_cor, frame);

    // Procura pelo melhor template
    max_corr = 0.0;
    size = 0;

    float max_corr_aux[8];
    int _l[8], _c[8];
    Mat_<GRY> binarizado;

    // Faz template matching e procura a max correlação de pra cada template
    #pragma omp parallel for
    for(int i = 0; i < 8; i++)
    {
      templ_match_result[i] = matchTemplateSame(frame,templ_placa[i],CV_TM_CCORR);
      //Procura ponto de máxima correlação
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

    // Acha a max correlação entre todas
    int centro_l_preto = 0, centro_c_preto = 0, metade_tamanho_preto = 0;
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

    // Testa se há placa ou não de acordo com a max correlação
    if (max_corr < 0.21)
    {
      //NÃO HÁ PLACA!
      vo << output;
    }
    else
    {
      metade_tamanho_preto = (55*pow(0.81, size))/2;
      // Desenha o retangulo amarelo na imagem original
      Point pt1 (centro_c_preto-metade_tamanho_preto, centro_l_preto-metade_tamanho_preto);
      Point pt2 (centro_c_preto+metade_tamanho_preto, centro_l_preto+metade_tamanho_preto);
      rectangle(output,pt1,pt2,Scalar( 0, 255, 255 ));
      vo << output;
    }
		imshow("output", output);
		int ch = (signed char)(waitKey(30));
  }
  // End Time
  time(&end);
  // Time elapsed
  double seconds = difftime (end, start);
  float est_fps = vi.get(CV_CAP_PROP_FRAME_COUNT)/seconds;
  cout << "Estimated frames per second : " << est_fps << endl;

  return 0;
}
