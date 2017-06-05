#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>
#include <sys/queue.h>

#include <rte_common.h>
#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_atomic.h>
#include <rte_branch_prediction.h>
#include <rte_ring.h>
#include <rte_log.h>
#include <rte_mempool.h>
#include <cmdline_rdline.h>
#include <cmdline_parse.h>
#include <cmdline_socket.h>
#include <cmdline.h>
#include <rte_mbuf.h>

#include <time.h> 

#include "allHeaders.h"

#define RUNMAINDPDK
#ifdef RUNMAINDPDK

#define RTE_LOGTYPE_APP RTE_LOGTYPE_USER1
// #define MEMPOOL_F_SP_PUT         0x0

#define MBUF_CACHE_SIZE 128
#define NUM_MBUFS 4095
static const char *MBUF_POOL = "MBUF_POOL";

static const char *GenerateData1 = "GenerateData1";
static const char *GenerateData2 = "GenerateData2";
static const char *GenerateData3 = "GenerateData3";
static const char *GenerateData4 = "GenerateData4";

static const char *RetriveData1 = "RetriveData1";
static const char *RetriveData2 = "RetriveData2";
static const char *RetriveData3 = "RetriveData3";
static const char *RetriveData4 = "RetriveData4";

const unsigned APEP_LEN_DPDK = 512;

// static int i=0; 
struct rte_ring *Ring_GenerateData1;
struct rte_ring *Ring_GenerateData2;
struct rte_ring *Ring_GenerateData3;
struct rte_ring *Ring_GenerateData4;

struct rte_ring *Ring_RetriveData1;
struct rte_ring *Ring_RetriveData2;
struct rte_ring *Ring_RetriveData3;
struct rte_ring *Ring_RetriveData4;

struct rte_mempool *mbuf_pool;
	
volatile int quit = 0;

long int ReadData_count = 0;
long int Retrive_DPDK_count = 0;
long int GenerateData_Loop1_count = 0;
long int GenerateData_Loop2_count = 0;
long int GenerateData_Loop3_count = 0;
long int GenerateData_Loop4_count = 0;
long int Data_Retrive_Loop_count = 0;
long int ReadData_Loop_count = 0;

int N_CBPS, N_SYM, ScrLength, valid_bits;

struct timespec time1,time2,time_diff;	/** < Test the running time. >*/
int time_test_flag = 0;
struct timespec diff(struct timespec start, struct timespec end);

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in);
static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int modulate_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In);
static int CSD_encode_dpdk (__attribute__((unused)) struct rte_mbuf *Data_In);

static int ReadData_Loop();
static int GenerateData_Loop1();
static int GenerateData_Loop2();
static int GenerateData_Loop3();
static int Data_Retrive_Loop();

struct timespec diff(struct timespec start, struct timespec end)
{
    struct  timespec temp;

     if ((end.tv_nsec-start.tv_nsec)<0) {
         temp.tv_sec = end.tv_sec-start.tv_sec-1;
         temp.tv_nsec = 1000000000+end.tv_nsec-start.tv_nsec;
     } else {
         temp.tv_sec = end.tv_sec-start.tv_sec;
         temp.tv_nsec = end.tv_nsec-start.tv_nsec;
     }
     return temp;
}

static int InitData(unsigned char** p_databits)
{
	FILE *fp=fopen("send_din_dec.txt","rt");
	unsigned char *databits=(unsigned char*)malloc(APEP_LEN_DPDK*sizeof(unsigned char));
	*p_databits = databits;
	if(databits == NULL){
		printf("error");
		return 0;
	}
	unsigned int datatmp=0;
	for(int i=0;i<APEP_LEN_DPDK;i++){
	    fscanf(fp,"%ud",&datatmp);
	    databits[i]=datatmp&0x000000FF;
	}
	//memcpy(rte_pktmbuf_mtod(Data,unsigned char *), databits, APEP_LEN_DPDK);
	fclose(fp);
	return 0;
}

static int ReadData(__attribute__((unused)) struct rte_mbuf *Data_out, unsigned char* Data_in) 
{
	//printf("Data->buflen = %d\n",Data_out->buf_len);
	//printf("ReadData_count = %d\n", ReadData_count++);
	if(time_test_flag==0){
		time_test_flag = 1;
		clock_gettime(CLOCK_REALTIME, &time1); //CLOCK_REALTIME:系统实时时间   
		//CLOCK_MONOTONIC:从系统启动这一刻起开始计时,不受系统时间被用户改变的影响
		//CLOCK_PROCESS_CPUTIME_ID:本进程到当前代码系统CPU花费的时间
		//CLOCK_THREAD_CPUTIME_ID:本线程到当前代码系统CPU花费的时间
	}
	rte_memcpy(rte_pktmbuf_mtod(Data_out,unsigned char *), Data_in, APEP_LEN_DPDK);
	return 0;
}


static int GenDataAndScramble_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In) 
{
	//printf("GenDataAndScramble_DPDK_count = %ld\n", GenDataAndScramble_DPDK_count++);
	
	unsigned char *databits = rte_pktmbuf_mtod(Data_In, unsigned char *);
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	GenDataAndScramble(data_scramble, ScrLength, databits, valid_bits);	

	return 0;
}

static int BCC_encoder_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("BCC_encoder_DPDK_count = %ld\n", BCC_encoder_DPDK_count++);

	int CodeLength = N_SYM*N_CBPS/N_STS;
	unsigned char *data_scramble = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	BCC_encoder_OPT(data_scramble, ScrLength, N_SYM, &BCCencodeout, CodeLength);

	return 0;
}

static int modulate_DPDK(__attribute__((unused)) struct rte_mbuf *Data_In)
{
	//printf("modulate_DPDK_count = %ld\n", modulate_DPDK_count++);

	unsigned char* BCCencodeout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, 0);
	unsigned char *stream_interweave_dataout = rte_pktmbuf_mtod_offset(Data_In, unsigned char *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	//complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);

	modulate_mapping(BCCencodeout, &stream_interweave_dataout, &subcar_map_data);

	return 0;
}	

static int CSD_encode_DPDK (__attribute__((unused)) struct rte_mbuf *Data_In)
{
	int i;
	//CSD_encode_DPDK_count++;
	//printf("CSD_encode_DPDK_count = %ld\n", CSD_encode_DPDK_count++);
	//complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);
	//complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	complex32 *csd_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, RTE_MBUF_DEFAULT_BUF_SIZE*8);
	complex32 *subcar_map_data = rte_pktmbuf_mtod_offset(Data_In, complex32 *, 0);
	//Data_CSD(&subcar_map_data, N_SYM, &csd_data);
	
	for(i=0;i<N_STS;i++){
		__Data_CSD_aux(&subcar_map_data, N_SYM, &csd_data,i);
	}
	return 0;
}

static int Data_Retrive_Loop() 
{
	void *Data_In_Retrive=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_RetriveData1, &Data_In_Retrive) >= 0 || 
			rte_ring_dequeue(Ring_RetriveData2, &Data_In_Retrive) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData3, &Data_In_Retrive) >= 0 ||
			rte_ring_dequeue(Ring_RetriveData4, &Data_In_Retrive) >= 0)
		{
			Retrive_DPDK_count++;
			rte_mempool_put(((struct rte_mbuf *)Data_In_Retrive)->pool, Data_In_Retrive);
		}
		else 
		{
			Data_Retrive_Loop_count++;
			usleep(10000);
			continue;
		}
		if(Retrive_DPDK_count >= 1000000)
		{
			quit = 1;
			clock_gettime(CLOCK_REALTIME, &time2);
			time_diff = diff(time1,time2);
			printf("Retrive_DPDK_count = %ld\n", Retrive_DPDK_count);
			printf("Start time # %.24s %ld Nanoseconds \n",ctime(&time1.tv_sec), time1.tv_nsec);
			printf("Stop time # %.24s %ld Nanoseconds \n",ctime(&time2.tv_sec), time2.tv_nsec);
			printf("Running time # %ld.%ld Seconds \n",time_diff.tv_sec, time_diff.tv_nsec);
			printf("ReadData_Loop_count = %ld\n", ReadData_Loop_count);
			printf("GenerateData_Loop1_count = %ld\n", GenerateData_Loop1_count);
			printf("GenerateData_Loop2_count = %ld\n", GenerateData_Loop2_count);
			printf("GenerateData_Loop3_count = %ld\n", GenerateData_Loop3_count);
			printf("GenerateData_Loop4_count = %ld\n", GenerateData_Loop4_count);
			printf("Data_Retrive_Loop_count = %ld\n", Data_Retrive_Loop_count);
		}


	}
	return 0;
}

static int GenerateData_Loop1() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData1, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData1, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop1_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop2() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData2, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData2, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop2_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int GenerateData_Loop3() 
{
	void *Data_In_GenerateData=NULL;
	while (!quit)
	{
		if (rte_ring_dequeue(Ring_GenerateData3, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData3, Data_In_GenerateData);
		}
		else 
		{	
			GenerateData_Loop3_count++;
			//usleep(10000);
			continue;
		}
	
	}
	return 0;
}

static int ReadData_Loop() 
{
	struct rte_mbuf *Data =NULL;
	unsigned char* Data_in =NULL;
	void *Data_In_GenerateData=NULL;
	int dis_count = 0;
	InitData(&Data_in);
	while (!quit){
		Data = rte_pktmbuf_alloc(mbuf_pool);
		if (Data != NULL)
		{
			ReadData(Data, Data_in);
			//rte_ring_enqueue(Ring_Beforescramble, Data);
			dis_count++;
			if(dis_count == 17){
				dis_count = 0;
			}
			if(dis_count >= 0 && dis_count < 5){
				rte_ring_enqueue(Ring_GenerateData1, Data);
			}		
			else if(dis_count >= 5 && dis_count < 10){
				rte_ring_enqueue(Ring_GenerateData2, Data);
			}
			else if(dis_count >= 10 && dis_count < 15){
				rte_ring_enqueue(Ring_GenerateData3, Data);
			}
			else if(dis_count >= 15 && dis_count < 17){
				rte_ring_enqueue(Ring_GenerateData4, Data);
			}
		}
		else if (rte_ring_dequeue(Ring_GenerateData4, &Data_In_GenerateData) >= 0)
		{
			GenDataAndScramble_DPDK(Data_In_GenerateData);
			BCC_encoder_DPDK(Data_In_GenerateData);
			modulate_DPDK(Data_In_GenerateData);
			CSD_encode_DPDK(Data_In_GenerateData);
			rte_ring_enqueue(Ring_RetriveData4, Data_In_GenerateData);
		}
		else 
		{
			ReadData_Loop_count++;
			//usleep(100000);
			continue;
		}
	
	}
	return 0;
}



int
main(int argc, char **argv)
{
	const unsigned flags = 0;
	const unsigned ring_size = 4096;
	const unsigned pool_size = 256;
	const unsigned pool_cache = 128;
	const unsigned priv_data_sz = 0;
	int ret;
	// 运行一次得到preamble和HeLTF.
	generatePreambleAndHeLTF_csd();
	// 运行一次得到比特干扰码表。
	Creatnewchart();
	// 运行一次得到BCC编码表。
	init_BCCencode_table();
	// 运行一次得到生成导频的分流交织表
	initial_streamwave_table();
	// 运行一次得到CSD表。
	//initcsdTableForHeLTF();
	// 初始化函数，计算OFDM符号个数，字节长度
	//int N_CBPS, N_SYM, ScrLength, valid_bits;
   	GenInit(&N_CBPS, &N_SYM, &ScrLength, &valid_bits);
	///////////////////////////////////////////////////////////////////////////////////
	//unsigned lcore_id;
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Cannot init EAL\n");
	Ring_GenerateData1 = rte_ring_create(GenerateData1 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData2 = rte_ring_create(GenerateData2 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData3 = rte_ring_create(GenerateData3 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_GenerateData4 = rte_ring_create(GenerateData4 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	

	Ring_RetriveData1 = rte_ring_create(RetriveData1 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData2 = rte_ring_create(RetriveData2 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData3 = rte_ring_create(RetriveData3 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);
	Ring_RetriveData4 = rte_ring_create(RetriveData4 , ring_size, rte_socket_id(), RING_F_SP_ENQ|RING_F_SC_DEQ);

	if (Ring_GenerateData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_GenerateData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData1 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData2 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData3 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");
	if (Ring_RetriveData4 == NULL)
		rte_exit(EXIT_FAILURE, "Problem getting sending ring\n");

	/* Creates a new mempool in memory to hold the mbufs. */
	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", NUM_MBUFS,
		MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE*16, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool\n");

	RTE_LOG(INFO, APP, "Finished Process Init.\n");
	rte_eal_remote_launch(ReadData_Loop, NULL,1);
	rte_eal_remote_launch(GenerateData_Loop1, NULL,2);
	rte_eal_remote_launch(GenerateData_Loop2, NULL,3);
	rte_eal_remote_launch(GenerateData_Loop3, NULL,4);
	//rte_eal_remote_launch(GenerateData_Loop4, NULL,5);
	rte_eal_remote_launch(Data_Retrive_Loop, NULL,5);
	//rte_eal_remote_launch(retrieve_Loop, NULL,12);
	rte_eal_mp_wait_lcore();
	return 0;
}
#endif // RUN