#include "projeto.hpp"
#include <cekeikon.h>

using namespace Morphology;

int bbox_error = 0;

Mat_<FLT> bbox(Mat_<FLT> a, int nlado)
{
	int esq=a.cols, dir=0, cima=a.rows, baixo=0; // primeiro pixel diferente de 255.
	for (int l=0; l<a.rows; l++)
		for (int c=0; c<a.cols; c++)
		{
			if (a(l,c)<=0.5)
			{
				if (c<esq)   esq=c;
				if (dir<c)   dir=c;
				if (l<cima)  cima=l;
				if (baixo<l) baixo=l;
			}
		}

	if (!(esq<dir && cima<baixo))
  {
		printf("Erro MNIST::bbox");
		bbox_error = 1;
    return a;
  }
	Mat_<FLT> roi(a, Rect(esq,cima,dir-esq+1,baixo-cima+1));
	Mat_<FLT> d;
	resize(roi,d,Size(nlado,nlado),0, 0, INTER_AREA);
	return d;
}

int main(int argc, char *argv[])
{
	if(argc!=2)
		erro("fase4 servidorIdAddr\n");

	Mat_<FLT> frame, templ;
	Mat_<FLT> templ_placa[9], templ_match_result[9];
	Mat_<COR> output, frame_cor, in, out;
	int size;
	float max_corr;
	int comando = 0;

	// Conecta com servidor
	CLIENT client(argv[1]);

	// Cria vídeo de saída
  VideoWriter vo ("car_vision.avi", CV_FOURCC ( 'X','V', 'I', 'D'), 15, Size (320, 240));

	// Cria janela para visualizar vídeo de saída
	namedWindow("raspcar_cam", WINDOW_NORMAL);

  // Abre template e redimensiona
  le(templ, "quadrado.png");
  for(int i = 0; i < 9; i ++)
	{
    resize(templ,templ_placa[i],Size(round(55*pow(0.81, i)), round(55*pow(0.81, i))),0,0,INTER_AREA);
    templ_placa[i]=somaAbsDois(dcReject(templ_placa[i], 1.0));
  }

	// Define tamanho da imagem do dígito
	float deltam_n = 121.0/200.0;
	float delta_n = deltam_n/2.0;
  int size_digit = round((55*pow(0.81, size))-2*delta_n*(55*pow(0.81, size)));

  // Lê mnist e treina modelo p/ reconhecer digitos
  int sz = 14;
  MNIST mnist(sz, true, true); //redimensiona imagens
                               //inverte preto/branco=true,
                               //crop bounding box=true
  mnist.le("/home/matheus/cekeikon5/tiny_dnn/data"); // mudar path p/ executar em outro pc
  flann::Index ind(mnist.ax, flann::KDTreeIndexParams(4));
  vector<int> indices(1);
  vector<float> dists(1);

	// Inicializa carrinho parado e sincroniza as transmissões/recepções de dados
	client.sendUint(11);

	int estado = 0;
	int ch;
	int stopped = 0;
	int frm = 0;
	int mov = 0;

	// Laço Principal, roda de acordo com o estado da máquina de estados
	do
	{
		float max_corr_aux[9];
		int _l[9], _c[9];
		int centro_l_preto = 0, centro_c_preto = 0, metade_tamanho = 0;
		max_corr = 0.0;
		size = 0;

		// Recebe e prepara novo frame
		client.receiveImgComp(frame_cor);
		output = frame_cor.clone();
		converte(output, frame);


		if(estado == 0) // segue placa
		{
			// Faz o template match
			#pragma omp parallel for
			for(int i = 0; i < 9; i++)
			{
				templ_match_result[i] = matchTemplateSame(frame,templ_placa[i],CV_TM_CCORR);
			}

			// Procura resultado com maior correlação
			#pragma omp parallel for
			for (int i = 0; i < 9; i++)
			{
				max_corr_aux[i] = 0.0;
				for(int l=0; l<templ_match_result[i].rows; l++)
				{
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
			}
			for(int i = 0; i < 9; i++)
			{
				if(max_corr_aux[i] > max_corr)
				{
					size = i;
					max_corr = max_corr_aux[i];
					centro_c_preto = _c[i];
					centro_l_preto = _l[i];
					}
				}

			// Verifica valor da máxima correlação para descobrir se há placa
			if (max_corr < 0.21) // Não há placa
			{
				client.sendUint(11);;
			}
			else // Há placa
			{
				metade_tamanho = round(55*pow(0.81, size))/2;

				//controla carrinho
				if(size == 0 )
				{
					estado = 1;
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

				// Desenha o retangulo amarelo na imagem original
				Point pt1 (centro_c_preto-metade_tamanho, centro_l_preto-metade_tamanho);
				Point pt2 (centro_c_preto+metade_tamanho, centro_l_preto+metade_tamanho);
				rectangle(output,pt1,pt2,Scalar( 0, 255, 255 ));
				cv::drawMarker (output, Point(centro_c_preto, centro_l_preto), Scalar( 0, 0, 255 ), MARKER_CROSS, 40, 1, 8);
			}
		}
		else if(estado == 1) // espera carrinho parar
	  {
			stopped++;
			client.sendUint(11);
			if(stopped == 3)
			{
				estado = 2;
				stopped = 0;
			}
		}
		else if(estado == 2) // tenta reconhecer digito
		{
			// Faz o template match
			#pragma omp parallel for
			for(int i = 0; i < 9; i++)
			{
				templ_match_result[i] = matchTemplateSame(frame,templ_placa[i],CV_TM_CCORR);
			}

			// Procura resultado com maior correlação
			#pragma omp parallel for
			for (int i = 0; i < 9; i++)
			{
				max_corr_aux[i] = 0.0;
				for(int l=0; l<templ_match_result[i].rows; l++)
				{
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
			}
			for(int i = 0; i < 9; i++)
			{
				if(max_corr_aux[i] > max_corr)
				{
					size = i;
					max_corr = max_corr_aux[i];
					centro_c_preto = _c[i];
					centro_l_preto = _l[i];
					}
				}

			// Verifica valor da máxima correlação para descobrir se há placa
			if (max_corr < 0.21) // Não há placa
			{
				client.sendUint(11);
				estado = 0;
			}
			else // Há placa
			{
				metade_tamanho = round(55*pow(0.81, size))/2;

				// Se o carrinho estiver próximo da placa, tenta ler placa
				if(size == 0 )
				{
					// Corta interior da placa
					Mat_<COR> placa(size_digit, size_digit);
					for(int i = 0; i < placa.rows; i++)
					{
						for(int j = 0; j < placa.cols; j++)
						{
							placa(i,j) = output(centro_l_preto-metade_tamanho+round(deltam_n*metade_tamanho)+i,centro_c_preto-metade_tamanho+round(deltam_n*metade_tamanho)+j);
						}
					}

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

					int prediction;

					if(bbox_error != 1)
					{
					  // Preve valor
						ind.knnSearch(resized.reshape(0,1), indices, dists, 1);
						prediction = mnist.ay(indices[0]);
						char str[100];
						printf("prediction = %d\n", prediction);
						sprintf(str, "Prediction = %d", prediction);
						putTxt (output, 10, 10, str, COR (0,255,0), 1);
					}
					else
					{
						prediction = 90;
						bbox_error = 0;
					}

					// envia comando para o servidor de acordo com o digito lido
					if(prediction == 0 || prediction == 1)
					{
						client.sendUint(11);
						estado = 5;
						mov = 11;
						frm = 0;
					}
					else if(prediction == 2)
					{
						estado = 3;
						client.sendUint(12);
						mov = 12;
						frm = 0;
					}
					else if(prediction == 3)
					{
						estado = 3;
						client.sendUint(13);
						mov = 13;
						frm = 0;
					}
					else if(prediction == 4 || prediction == 5)
					{
						estado = 3;
						client.sendUint(16);
						mov = 16;
						frm = 0;
					}
					else if(prediction == 6 || prediction == 7)
					{
						estado = 3;
						client.sendUint(14);
						mov = 14;
						frm = 0;
					}
					else if(prediction == 8 || prediction == 9)
					{
						estado = 3;
						client.sendUint(15);
						mov = 15;
						frm = 0;
					}
			}
 			// Desenha o retangulo amarelo na imagem original
			Point pt1 (centro_c_preto-metade_tamanho, centro_l_preto-metade_tamanho);
			Point pt2 (centro_c_preto+metade_tamanho, centro_l_preto+metade_tamanho);
			rectangle(output,pt1,pt2,Scalar( 0, 255, 255 ));
			}
		}
		else if(estado == 3) //espera servidor executar a acao, temporizacao feita
		//pela quantidade de frames recebidos
		{
			client.sendUint(30);
			frm ++;
			if((mov == 12 || mov == 13) && frm == 30)
			{
				frm = 0;
				estado = 4;
			}
			else if((mov == 14 || mov == 15) && frm == 16)
			{
				frm = 0;
				estado = 4;
			}
			else if((mov == 11) && frm == 2)
			{
				frm = 0;
				estado = 4;
			}
			else if((mov == 16) && frm == 60)
			{
				frm = 0;
				estado = 4;
			}
		}
		else if(estado == 4)
		{
			client.sendUint(11);
			estado = 0;
		}

		char str[100];
		sprintf(str, "Estado = %d", 0);
		putTxt (output, 30, 10, str, COR (255,0,0), 1);

		imshow("raspcar_cam", output);
    vo << output;
    ch = waitKey(30);
	} while(estado != 5);

	//parar carrinho
	client.sendUint(11);
	imshow("raspcar_cam", frame_cor);
	vo << output;
	printf("End!!!\n");

  return 0;
}
