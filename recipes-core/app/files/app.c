#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#define q	11		    /* for 2^11 points */
#define N	(1<<q)		/* N-point FFT, iFFT */

#ifndef PI
# define PI	3.14159265358979323846264338327950288
#endif

typedef float real;
typedef struct{real Re; real Im;} complex;

void fft( complex *v, int n, complex *tmp );
void* calcThread(void *par);
void* readThread(void* par);
void manager(int sig);
int values[N], sigRead, sigCalc;
sem_t dataLock;
sem_t readyFlag;

int main(int argc, char **argv){
    char *app_name = argv[0];
    char *dev_name = "/dev/ppgmod_dev";
    int fd  = -1;
    sigRead = 1;
    sigCalc = 1;
    pthread_t tRead, tCalc;
    setbuf(stdout, 0);

    
    if ((fd = open(dev_name, O_RDWR)) < 0) {
        fprintf(stderr, "%s: unable to open %s: %s\n", app_name, dev_name, strerror(errno));
        return( 1 );
    }
    
    printf("To terminate the app type ctrl-C\n");
    
    //Uses function manager to handle SIGINT signal
    signal(SIGINT, manager);
    
    /* Uses two semaphores:
     * dataLock to avoid changes on data while computing fft
     * readyFlag to indicates that 2048 samples are ready */
    sem_init(&dataLock, 0, 1);
    sem_init(&readyFlag, 0, 0);
    
    pthread_create(&tRead, NULL, readThread, (void*)&fd);
    pthread_create(&tCalc, NULL, calcThread, NULL);
    
    pause();
    
    pthread_join(tRead, NULL); 
    pthread_join(tCalc, NULL); 
    
    sem_destroy(&dataLock);
    sem_destroy(&readyFlag);
    close( fd );
}

void manager(int sig){
    sigRead = 0;
    sigCalc = 0;
}

void* readThread(void* par){
    struct timespec ts;
    ts.tv_sec   = 0;
    ts.tv_nsec  = 20000000;     //Set sleeptime to 20ms between reads
    int fd      = *((int*)par);
    int counter = 0;
    int val     = 0;

    while (sigRead){
        
        sem_wait(&dataLock);
        val = read(fd, (char*)&values[counter], sizeof(int));
        sem_post(&dataLock);
        counter++;
        
        if (counter == N-1){
            counter = 0;
            sem_post(&readyFlag);
        }
        
        nanosleep(&ts, &ts);
    }
    
    sem_post(&readyFlag);

    return NULL;
}

void fft( complex *v, int n, complex *tmp ){
  if (n > 1){			/* otherwise, do nothing and return */
      
    int k,m;    
    complex z, w, *vo, *ve;
    ve = tmp; 
    vo = tmp+n/2;
    
    for(k = 0; k < n/2; k++) {
      ve[k] = v[2*k];
      vo[k] = v[2*k+1];
    }
    
    fft(ve, n/2, v);		/* FFT on even-indexed elements of v[] */
    fft(vo, n/2, v);		/* FFT on odd-indexed elements of v[] */
    for(m = 0; m < n/2; m++) {
      w.Re = cos(2*PI*m/(double)n);
      w.Im = -sin(2*PI*m/(double)n);
      z.Re = w.Re*vo[m].Re - w.Im*vo[m].Im;	/* Re(w*vo[m]) */
      z.Im = w.Re*vo[m].Im + w.Im*vo[m].Re;	/* Im(w*vo[m]) */
      v[  m  ].Re = ve[m].Re + z.Re;
      v[  m  ].Im = ve[m].Im + z.Im;
      v[m+n/2].Re = ve[m].Re - z.Re;
      v[m+n/2].Im = ve[m].Im - z.Im;
    }
  }
  
  return;
}

void* calcThread(void *par){
    complex v[N], scratch[N];
    float abs[N];
    int k, m, i;
    int minIdx, maxIdx;

    sem_wait(&readyFlag);       //Waits until 2048 samples are ready
    
    while(sigCalc) {
        sem_wait(&dataLock);    //To avoid changes on data while copying, acquire the lock 
        
        // Initialize the complex array for FFT computation
        for(k=0; k<N; k++) {
            v[k].Re = values[k];
            v[k].Im = 0;
        }
        
        sem_post(&dataLock); 

        // FFT computation
        fft( v, N, scratch );

        // PSD computation
        for(k=0; k<N; k++) {
            abs[k] = (50.0/2048)*((v[k].Re*v[k].Re)+(v[k].Im*v[k].Im)); 
        }

        minIdx = (0.5*2048)/50;   // position in the PSD of the spectral line corresponding to 30 bpm
        maxIdx = 3*2048/50;       // position in the PSD of the spectral line corresponding to 180 bpm

        // Find the peak in the PSD from 30 bpm to 180 bpm
        m = minIdx;
        for(k = minIdx; k < maxIdx; k++) {
            if (abs[k] > abs[m])
                m = k;
        }
            
        // Print the heart beat in bpm
        printf( "\n\n\n%d bpm\n\n\n", (m)*60*50/2048 );
        
        sem_wait(&readyFlag);
        
    };
    
    return NULL;
}
