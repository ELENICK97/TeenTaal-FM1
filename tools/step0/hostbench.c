/* Step 0: host instruction count per sample of the probe oscillators (perf_event_open),
 * the method of Felucca's tests/regress.c. Not part of the firmware. */
#define _GNU_SOURCE
#include <linux/perf_event.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <sys/ioctl.h>
int16_t sin_tab[1025];
void osc_q15(int32_t*,int,uint32_t*,uint32_t,int16_t);
void osc_f32(int32_t*,int,float*,float*,float,float);
void biquad_f32(float*,int,const float*,float*);
static int pfd(void){ struct perf_event_attr a; memset(&a,0,sizeof a); a.type=PERF_TYPE_HARDWARE; a.size=sizeof a;
  a.config=PERF_COUNT_HW_INSTRUCTIONS; a.disabled=1; a.exclude_kernel=1; a.exclude_hv=1;
  return (int)syscall(SYS_perf_event_open,&a,0,-1,-1,0); }
#define N 64
#define BLOCKS 20000
int main(void){
  for(int i=0;i<=1024;i++) sin_tab[i]=(int16_t)lrint(32767*sin(2*M_PI*i/1024));
  int fd=pfd(); if(fd<0){perror("perf_event_open"); return 1;}
  static int32_t out[N]; static float fx[N]; uint32_t ph=0; float s1=0,s2=0.01f,c[5]={.2f,.4f,.2f,-.5f,.3f},z[2]={0};
  long long v; const char *nm[3]={"osc_q15 (table+lerp)","osc_f32 (resonator)","biquad_f32"};
  for(int k=0;k<3;k++){ ioctl(fd,PERF_EVENT_IOC_RESET,0); ioctl(fd,PERF_EVENT_IOC_ENABLE,0);
    for(int b=0;b<BLOCKS;b++){ if(k==0) osc_q15(out,N,&ph,97000000u,20000); else if(k==1) osc_f32(out,N,&s1,&s2,1.99f,1000.f); else biquad_f32(fx,N,c,z);} 
    ioctl(fd,PERF_EVENT_IOC_DISABLE,0); read(fd,&v,sizeof v);
    printf("%-22s %.1f host instr/sample\n",nm[k],(double)v/(N*BLOCKS)); }
  return 0; }
