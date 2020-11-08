#include <cekeikon.h>
#include <time.h>

using namespace Morphology;

Mat_<FLT> bbox(Mat_<FLT> a, int nlado)
{
	int esq=a.cols, dir=0, cima=a.rows, baixo=0; // primeiro pixel diferente de 255.
	for (int l=0; l<a.rows; l++)
		for (int c=0; c<a.cols; c++)
		{
			if (a(l,c)<=0.5)
			{
				if (c<esq) esq=c;
				if (dir<c) dir=c;
				if (l<cima) cima=l;
				if (baixo<l) baixo=l;
			}
		}

	if (!(esq<dir && cima<baixo))
  {
		printf("Erro MNIST::bbox");
    return a;
  }

	Mat_<FLT> roi(a, Rect(esq,cima,dir-esq+1,baixo-cima+1));
	Mat_<FLT> d;
	resize(roi,d,Size(nlado,nlado),0, 0, INTER_AREA);
	return d;
}

int main(int argc, char** argv)
{
  if(argc!=4)
		erro("fase5 capturado.avi quadrado.png localiza.avi\n");

  Mat_<FLT> frame, templ;
  Mat_<FLT> templ_placa[8];
  Mat_<FLT> templ_match_result[8];
  Mat_<GRY> melhor_frame;
  Mat_<COR> output, frame_cor, in, out;

  int size;
  float max_corr;

	namedWindow("cropped", WINDOW_NORMAL);
	namedWindow("output", WINDOW_NORMAL);

  VideoCapture vi(argv[1]);
  if (!vi.isOpened())
		erro("Erro na abertura do video de entrada");
  int nc=vi.get(CV_CAP_PROP_FRAME_WIDTH);
  int nl=vi.get(CV_CAP_PROP_FRAME_HEIGHT);
  int fps=vi.get(CV_CAP_PROP_FPS);

  VideoWriter vo(argv[3], CV_FOURCC('X','V','I','D'),	fps, Size(nc,nl));
  if (!vo.isOpened())
		erro("Erro na abertura do video de saida");

  // Abre template e cria vetor de templates
  le(templ, argv[2]);
  for(int i = 0; i < 8; i ++){
    resize(templ,templ_placa[i],Size(55*pow(0.82, i), 55*pow(0.82, i)),0,0,INTER_AREA);
    templ_placa[i]=somaAbsDois(dcReject(templ_placa[i], 1.0));
  }

	float deltam_n = 121.0/200.0;
	float delta_n = 121.0/400.0;
	int size_digit = round((55*pow(0.82, size))-2*delta_n*(55*pow(0.82, size)));

  ///////////////////////////////////////

	int sz = 14;
	MNIST mnist(sz, true, true); //redimensiona imagens
															 //inverte preto/branco=true,
															 //crop bounding box=true
	mnist.le("/home/matheus/cekeikon5/tiny_dnn/data"); //Mudar path para executar em outro pc
	flann::Index ind(mnist.ax, flann::KDTreeIndexParams(4));
	vector<int> indices(1);
	vector<float> dists(1);



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

    // Procura pelo melhor template em tamanhos entre 20 e 55
    max_corr = 0.0;
    size = 0;
    float max_corr_aux[8];
    int _l[8], _c[8];
    Mat_<GRY> binarizado;

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

		int centro_l_preto = 0, centro_c_preto = 0, metade_tamanho = 0;
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
    }
		else
		{
      metade_tamanho = round(55*pow(0.81, size))/2;

			if(size == 0)
			{
				Mat_<COR> placa(size_digit, size_digit);
				for(int i = 0; i < placa.rows; i++)
				{
					for(int j = 0; j < placa.cols; j++)
					{
						placa(i,j) = output(centro_l_preto-metade_tamanho+round(deltam_n*metade_tamanho)+i,centro_c_preto-metade_tamanho+round(deltam_n*metade_tamanho)+j);
					}
				}

				imshow("cropped",placa);
				Mat_<FLT> digit(placa.rows, placa.cols);
				Mat_<FLT> resized(sz, sz);

				// Preprocessa placa
				converte(placa, digit);
				for(int i = 0; i < placa.rows; i++)
				{
					for(int k = 0; k < placa.cols; k++)
					{
						digit(i,k)=4*(digit(i,k)-0.5)+0.5;
						if(digit(i,k)<0) digit(i,k)=0;
						if(digit(i,k)>1) digit(i,k)=1;
					}
				}

				resized = bbox(digit, sz);

				for(int i = 0; i < resized.rows; i++)
				{
					for(int k = 0; k < resized.cols; k++)
					{
						resized(i,k)=4*(resized(i,k)-0.5)+0.5;
						if(resized(i,k)<0) resized(i,k)=0;
						if(resized(i,k)>1) resized(i,k)=1;
					}
				}

				ind.knnSearch(resized.reshape(0,1), indices, dists, 1);
				int prediction = mnist.ay(indices[0]);
				char str[100];
				sprintf(str, "Prediction = %d", prediction);
				putTxt (output, 10, 10, str, COR (0,255,0), 1);
			}

      // Desenha o retangulo amarelo na imagem original
      Point pt1 (centro_c_preto-metade_tamanho, centro_l_preto-metade_tamanho);
      Point pt2 (centro_c_preto+metade_tamanho, centro_l_preto+metade_tamanho);
      rectangle(output,pt1,pt2,Scalar( 0, 255, 255 ));
			cv::drawMarker (output, Point(centro_c_preto, centro_l_preto), Scalar( 0, 0, 255 ), MARKER_CROSS, 40, 1, 8);
    }

		vo<<output;
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
