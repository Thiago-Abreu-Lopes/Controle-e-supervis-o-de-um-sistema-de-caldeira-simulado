//Definição de Bibliotecas
#include <pthread.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include "tela.h"
#include "sensores.h"
#include "atuadores.h"
#include "socket.h"
#include "referenciaT.h"
#include "bufduplo.h"

#define NUM_THREADS 6
#define TEMP_MINIMA  0.0
#define TEMP_MAX 80.0
#define NIVEL_MINIMO 0.1
#define NIVEL_MAX 3.0

#define	NSEC_PER_SEC    (1000000000) 	// Numero de nanosegundos em um segundo
#define N_AMOSTRAS      10000

struct termios info;//estrutura auxiliar na função de pausar o terminal para a troca dos valores de referencia

double ask_tempRef(){
    double ref_t;
    aloca_tela();
    do{
        printf("Digite a referencia para a temperatura --> ");
        scanf("%lf",&ref_t);
        if(ref_t>TEMP_MAX){printf("ESCOLHA UMA TEMPERATURA ACIMA DE 0 E ABAIXO DE %.2lf °\n",TEMP_MAX);}
    }while(ref_t>TEMP_MAX);
    libera_tela();
    return ref_t;
    
}

double ask_nivelRef(){
    double ref_h;
    aloca_tela();
    do{
        printf("Digite a referencia para o nivel de agua --> ");
        scanf("%lf",&ref_h);
        if(ref_h>NIVEL_MAX){printf("ESCOLHA UM VALOR ACIMA DE 0.1 E ABAIXO DE %.2lf m\n",NIVEL_MAX);}
    }while(ref_h>NIVEL_MAX);
    libera_tela();
    return ref_h;
    
}

/*4º requesito[INICIO] - mostrar na tela os valores atualizados dos sensores do sistema*/
void thread_mostra_status (void){
	double t, h, i ,o, a, refT,refH;
    int ch;
	while(1){
		
         do{
            t = sensor_getT();
		    h = sensor_getH();
		    a = sensor_getTAMB();
		    i = sensor_getTAIN();
		    o = sensor_getFAOUT();
            refT = ref_getT();
            refH = ref_getH();
		    aloca_tela();
		    system("tput reset");//limpa a tela(não oculta,limpa msm!!)
		    printf("---------------------------------------\n");
            printf("Referencia de Temperatura (T)--> %.2lf\n", refT);
            printf("Referencia de Nível (H)--> %.2lf\n", refH);
            printf("\033[0;32m");
		    printf("Temperatura (T)--> %.2lf\n", t);
		    printf("Nivel (H)--> %.2lf\n", h);
            printf("\033[0;34m");
            printf("Temperatura do Ambiente (Ta)--> %.2lf\n", a);
		    printf("Temperatura da Agua que Entra (Ti)--> %.2lf\n", i);
            printf("Fluxo da Agua que Sai (No)--> %.2lf\n", o);
            printf("\33[0;33m");
            //printf("PRESSIONE \"r\" PARA MUDAR OS VALORES DE REFERENCIA\n");
            printf("\033[0m");
		    printf("---------------------------------------\n");
		    libera_tela();
		    sleep(1);
            /*tcgetattr(0, &info);          /* get current terminal attirbutes; 0 is the file descriptor for stdin 
            info.c_lflag &= ~ICANON;      /* disable canonical mode 
            info.c_cc[VMIN] = 0;          /* wait until at least one keystroke available 
            info.c_cc[VTIME] = 0;         /* no timeout 
            tcsetattr(0, TCSANOW, &info); /* set immediately 
            ch=getchar();
            if (ch < 0) {
                if (ferror(stdin)) { printf("DEU RUIM!"); }
                clearerr(stdin);
            }  
            tcgetattr(0, &info);
            info.c_lflag |= ICANON;
            tcsetattr(0, TCSANOW, &info);*/	          
        }while(ch!=114);
            aloca_tela();
            ref_putT(ask_tempRef());
            ref_putH(ask_nivelRef());
		    libera_tela();					
	}
		
}

/*4º requesito [MEIO] - ler os sensores para imprimir pela função acima*/
void thread_le_sensor (void){ //Le Sensores periodicamente a cada 10ms
	
    int cont = 0;
    int intervalo = 100; //determina de quantos em quantos 'thread-le-sensor's' devo salvar o valor do sensor no .txt
	char msg_enviada[1000];
	struct timespec t, t_fim;
	long periodo = 10e6; //10e6ns ou 10ms
	
    FILE * dados_t;
	dados_t = fopen("sensor_t.txt", "w");
	if(dados_t ==NULL){
	    printf("Erro na abertura do arquivo do sensor de temperatura\n");
	    exit(1);
	}
    FILE * dados_h;
	dados_h = fopen("sensor_h.txt", "w");
	if(dados_h ==NULL){
	    printf("Erro na abertura do arquivo do sensor de nivel\n");
	    exit(1);
	}

	// Le a hora atual, coloca em t
	clock_gettime(CLOCK_MONOTONIC ,&t);
	while(1){
		// Espera ateh inicio do proximo periodo
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		
		//Envia mensagem via canal de comunicação para receber valores de sensores		
		sensor_putT(msg_socket("st-0"));
        sensor_putH(msg_socket("sh-0"));
        sensor_putTAMB(msg_socket("sta0"));
        sensor_putTAIN(msg_socket("sti0"));
        sensor_putFAOUT(msg_socket("sno0"));

		//8º requesito [INICIO] - gravação dos valores dos sensores.
        intervalo--;
        if(intervalo == 0 && cont < N_AMOSTRAS){
            
            fprintf(dados_t, "%2lf\n", sensor_getT());
            fprintf(dados_h, "%2lf\n", sensor_getH());
            intervalo = 100;
            cont++;        
        }
        //8º requesito [FIM]

		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}		
	}
    fclose(dados_t);
    fclose(dados_h);
        		
}
/*4º requesito [FIM]*/

/*5º requesito [INICIO] - estabelecer o alarme para 30 graus*/
void thread_alarme (void){
	
	while(1){
		sensor_alarmeT(30);
		aloca_tela();
        printf("\033[0;31m");
		printf("ALARME\n");
        printf("\033[0m");
		libera_tela();
		sleep(1);	
	}
		
}
/*5º requesito [FIM]*/

///Controle

/*1º requesito [INICIO] - Controle da temperatura*/
void thread_controle_temperatura (void){
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim;
	long periodo = 50e6; //50ms
	double temp, ref_tempMAX,ref_nivelMAX,ref_tempMIN,amb,ag_in,h;
	//le a hora atual
	clock_gettime(CLOCK_MONOTONIC, &t);
	while(1){
		//esperar até o próximo periodo			
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		temp = sensor_getT();
        amb = sensor_getTAMB();
        ag_in = sensor_getTAIN();
        h = sensor_getH();
		ref_tempMAX = ref_getT();
        ref_nivelMAX = ref_getH();
        ref_tempMIN = TEMP_MINIMA; 
        double ni,na,q;
		if(temp<ref_tempMAX){
            if(ag_in<10.0||amb<10.0){
                atuador_putQ(1000000.0);          
            }else{
                atuador_putQ(100000.0);
            }
            atuador_putNA(10.0);
            sprintf(msg_enviada, "aq-%lf", atuador_getQ());		
			msg_socket(msg_enviada);
            sprintf(msg_enviada, "ana%lf",  atuador_getNA());		
			msg_socket(msg_enviada);
			
			
		}

		if(temp==ref_tempMAX){
			atuador_putQ(10000.0);
			sprintf( msg_enviada, "aq-%lf",atuador_getQ());
	        msg_socket(msg_enviada);
			atuador_putNA(0.0);
	        sprintf( msg_enviada, "ana%lf", atuador_getNA());
			msg_socket(msg_enviada);
		}
        if(temp>ref_tempMAX){
            if(h>ref_nivelMAX){
                atuador_putNI(0.0);
                sprintf(msg_enviada, "ani%lf", atuador_getNI());		
			    msg_socket(msg_enviada);
            }
            atuador_putQ(10.0);			
			sprintf(msg_enviada, "aq-%lf", atuador_getQ());		
			msg_socket(msg_enviada);
            atuador_putNA(0.0);
	        sprintf( msg_enviada, "ana%lf", atuador_getNA());
			msg_socket(msg_enviada);
		}

		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC ,&t_fim);
		
		// Calcula o tempo de resposta observado em microsegundos
		atraso_fim = 1000000*(t_fim.tv_sec - t.tv_sec)   +   (t_fim.tv_nsec - t.tv_nsec)/1000;
		
		bufduplo_insereLeitura(atraso_fim);
		
		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}	
		
		
	}
}
/*1º requesito [FIM]*/

/*2º requesito [INICIO] - Controle do nível de água*/
void thread_controle_nivel (void){
	char msg_enviada[1000];
	long atraso_fim;
	struct timespec t, t_fim;
	long periodo = 70e6; //70ms
	double nivel, ref_nivelMAX,ref_nivelMIN;
    double base = 4.0, pesoEsp = 1000.0, fOut;
	//le a hora atual
	clock_gettime(CLOCK_MONOTONIC, &t);
	while(1){
		//esperar até o próximo periodo			
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &t, NULL);
		nivel = sensor_getH();
		ref_nivelMAX = ref_getH();
        ref_nivelMIN = NIVEL_MINIMO;
        fOut = sensor_getFAOUT();
        double ni,nf;
		if(nivel<ref_nivelMAX){ 
            if(2*fOut>100.0){
                ni=100.0;
            }else{
                ni=fOut*2;
            }
            if(ni==0){
                ni=100.0;
            }
          
			sprintf(msg_enviada, "ani%lf", ni); //Zera o fluxo de água que entra		
			msg_socket(msg_enviada);
            
			sprintf(msg_enviada, "anf%lf", 0.0); //Abre totalmente a vazão pelo esgoto	
			msg_socket(msg_enviada);
		}	
		if(nivel==ref_nivelMAX){ 
          
			sprintf(msg_enviada, "ani%lf", fOut); //Zera o fluxo de água que entra		
			msg_socket(msg_enviada);
            
			sprintf(msg_enviada, "anf%lf", atuador_getNA()); //Abre totalmente a vazão pelo esgoto	
			msg_socket(msg_enviada);
		}
        if(nivel>ref_nivelMAX){
            sprintf(msg_enviada, "ani%lf", fOut); //Zera o fluxo de água que entra		
			msg_socket(msg_enviada);
            
			sprintf(msg_enviada, "anf%lf", 100.0); //Abre totalmente a vazão pelo esgoto	
			msg_socket(msg_enviada);
        }

		
		// Le a hora atual, coloca em t_fim
		clock_gettime(CLOCK_MONOTONIC ,&t_fim);
		
		// Calcula o tempo de resposta observado em microsegundos
		/*atraso_fim = 1000000*(t_fim.tv_sec - t.tv_sec)   +   (t_fim.tv_nsec - t.tv_nsec)/1000;
		
		bufduplo_insereLeitura(atraso_fim);*/ //desnecessário
		
		// Calcula inicio do proximo periodo
		t.tv_nsec += periodo;
		while (t.tv_nsec >= NSEC_PER_SEC) {
			t.tv_nsec -= NSEC_PER_SEC;
			t.tv_sec++;
		}	
		
		
	}
}
/*2º requesito [FIM]*/

/*7º requesito [INICIO] - armazenar tempos de resposta*/
void thread_grava_temp_resp(void){
	FILE * dados_f;
	dados_f = fopen("dados.txt", "w");
	if(dados_f ==NULL){
		printf("Erro na abertura do arquivo \n");
		exit(1);
	}
	
	int amostras = 1;
	while(amostras++<=N_AMOSTRAS/200){
		long * buf = bufduplo_esperaBufferCheio();
		int n2 = tamBuf();
		int tam = 0;
		while(tam<n2)
			fprintf(dados_f, "%4ld\n", buf[tam++]);
		aloca_tela();
		printf("Salvando Arquivo....\n");
		libera_tela();
	}
	fclose(dados_f);			
}
/*7º requesito [FIM]*/

/*6º requesito [INICIO]- atraves do teclado decidimos quais as referencias para o nível e a temperatura*/

void thread_troca_ref(){
    int ch;
    while(1){
        tcgetattr(0, &info);          /* captura os atributos atuais do terminal*/
        info.c_lflag &= ~ICANON;      /* desabilita o modo canonico*/
        info.c_cc[VMIN] = 0;          /* mantem o sistema rodando e lendo ao mesmo tempo */
        info.c_cc[VTIME] = 0;         /* tempo de espera */
        tcsetattr(0, TCSANOW, &info); /* atribui o valor a variavel stadin imediatamente */

        do{
            aloca_tela();
            ch=getchar();
            libera_tela();
            if (ch < 0) {
                if (ferror(stdin)) { aloca_tela(); printf("ERRO NA TROCA DE REFERENCIA!"); libera_tela();}
                clearerr(stdin);
            }            
            
        }while(ch!=114||ch!=82);
        ref_putT(ask_tempRef());
        ref_putH(ask_nivelRef());

        tcgetattr(0, &info); /* captura os atributos atuais do terminal*/
        info.c_lflag |= ICANON; /* habilita o modo canonico*/
        tcsetattr(0, TCSANOW, &info); /*atribui o valor a variavel stadin imediatamente*/    
    } 
}
/*6º requesito [FIM]*/

void main( int argc, char *argv[]) {
    ref_putT(ask_tempRef());
    ref_putH(ask_nivelRef());

	cria_socket(argv[1], atoi(argv[2]) );
    
	int ord_prio[NUM_THREADS]={1,59,1,99,1,95};
	pthread_t threads[NUM_THREADS];
	pthread_attr_t pthread_custom_attr[NUM_THREADS];
	struct sched_param priority_param[NUM_THREADS];

	//Configura escalonador do sistema
	for(int i=0;i<NUM_THREADS;i++){
		pthread_attr_init(&pthread_custom_attr[i]);
		pthread_attr_setscope(&pthread_custom_attr[i], PTHREAD_SCOPE_SYSTEM);
		pthread_attr_setinheritsched(&pthread_custom_attr[i], PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&pthread_custom_attr[i], SCHED_FIFO);
		priority_param[i].sched_priority = ord_prio[i];
		if (pthread_attr_setschedparam(&pthread_custom_attr[i], &priority_param[i])!=0)
	  		fprintf(stderr,"pthread_attr_setschedparam failed\n");
	}

	pthread_create(&threads[0], &pthread_custom_attr[0], (void *) thread_mostra_status, NULL);
	pthread_create(&threads[1], &pthread_custom_attr[1], (void *) thread_le_sensor, NULL);
	pthread_create(&threads[2], &pthread_custom_attr[2], (void *) thread_alarme, NULL);
	pthread_create(&threads[3], &pthread_custom_attr[3], (void *) thread_controle_temperatura, NULL);
	pthread_create(&threads[4], &pthread_custom_attr[4], (void *) thread_grava_temp_resp, NULL);
    pthread_create(&threads[5], &pthread_custom_attr[5], (void *) thread_controle_nivel, NULL);

	for(int i=0;i<NUM_THREADS;i++){
		pthread_join( threads[i], NULL);

	}
	    
}
