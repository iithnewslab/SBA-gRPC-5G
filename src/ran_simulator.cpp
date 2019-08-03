#include "../src/ran_simulator.h"
#include <fstream>
time_t g_start_time;
time_t start = time(0) ;
int g_threads_count;
uint64_t g_req_dur;
uint64_t g_run_dur;
int g_tot_regs;
uint64_t g_tot_regstime;
pthread_mutex_t g_mux;
vector<thread> g_umon_thread;
vector<thread> g_dmon_thread;
vector<thread> g_threads;
thread g_rtt_thread;
TrafficMonitor g_traf_mon;
const int nstars = 100;   // maximum number of stars to distribute
const int nstars1 = 70;   // maximum number of stars to distribute in Load 1
const int nstars2 = 100;   // maximum number of stars to distribute in Load 2 
const int nstars3 = 90;   // maximum number of stars to distribute in Load 3 
const int nstars4 = 80;   // maximum number of stars to distribute in Load 4 
const int nstars5 = 90;   // maximum number of stars to distribute in Load 5 
 const int TimePeriod = 14000 ; 
int TimetoStart[nstars] = {0};
int arrivalsAtTime[TimePeriod*5] = {0};
int stopped_threads = 0;

void utraffic_monitor() {
	UdpClient sgw_s1_client;
	
	sgw_s1_client.set_client(g_trafmon_ip_addr);
	while (1) {
		g_traf_mon.handle_uplink_udata(sgw_s1_client);
	}
	
}

void dtraffic_monitor() {
	while (1) {
		g_traf_mon.handle_downlink_udata();		
	}
}

int schedule(){
    const int nrolls = 10000; // number of experiments
    int ueNum = 0 ; 
    int totalUEsArrivedinSystem = 0;
    for (int i = 0; i < 100; ++i)
    {
  	 TimetoStart[i] = 1000;
    }
   std::default_random_engine generator;
   std::poisson_distribution<int> distribution(100.1);
   std::cout << "poisson_distribution (mean=70.1):  Load 1 " << std::endl;
   int p[TimePeriod]={};
   int d1 ; // Load1 Dist
   int d2; // Load2 Dist
   int d3; //Load3 Dist
   int d4; // Load4 Dist 
   int d5;//Load5 Dist

   //Load Distribution 1 
       
   for ( d1=0; d1<nrolls; ++d1) {
     int number = distribution(generator);
     if (number<TimePeriod) ++p[number];
   }

   for ( d1=0; d1<TimePeriod; ++d1){
     	//std::cout << i << "   " <<p[i]*nstars1/nrolls <<endl;
  	    arrivalsAtTime[d1] = p[d1]*nstars1/nrolls;
  	    totalUEsArrivedinSystem += arrivalsAtTime[d1];
        //std::cout << i << ": " << std::string(p[d1]*nstars1/nrolls,'*') << std::endl;
        std::cout << d1 <<" "<<arrivalsAtTime[d1] <<endl;
   }


  for(int k=0;k < TimePeriod;k++)
  {
    	int numberUEatI = arrivalsAtTime[k] ;
    	for(int j = 0 ;j < numberUEatI ; j++)
    	{   
    		TimetoStart[ueNum] = k ; 
    		ueNum++;
    	}      
  }

    
  int startTime = 3;
  int numUEInc = 2;
  int lowerbound = 0;
  int upperbound = numUEInc; 
  for( int i = 0 ; i < 10 ; i++)
    {
      for(int j = lowerbound; j<= upperbound;j++)
          {
            TimetoStart[j] =  startTime+60*i;
          }
	lowerbound = lowerbound+numUEInc*(i+1);
    upperbound = upperbound+numUEInc*(i+2);
    }

   int k = 3 ;
   for(int i = 14 ; i < 19 ; i++)
   {
      for(int j = lowerbound; j<= upperbound;j++)
          {
            TimetoStart[j] =  startTime+60*i;
          }
        lowerbound = lowerbound+numUEInc*(k);
        upperbound = upperbound+numUEInc*(k+1);
        k--;

   } 	
}

int is_scheduled(int ran_num)
{ 
	time_t end = time(0);
	int elapsedtime = difftime(end, start);
	//TRACE(cout<<"elapsedtime is "<<elapsedtime;)
	if(TimetoStart[ran_num] == elapsedtime) {
         return 1 ; 
	}else 
	{
		return 0;
	}

}


void ping(){
	string cmd;
	
	cmd = "ping -I 172.16.1.3 172.16.0.2 -c 60 | grep \"^rtt\" >> ping.txt";
	cout << cmd << endl;
	system(cmd.c_str());
}

void simulate(int arg) {
	CLOCK::time_point mstart_time;
	CLOCK::time_point mstop_time;
	MICROSECONDS mtime_diff_us;		
	Ran ran;
	int status;
	int ran_num;
	bool ok;
	bool time_exceeded;
	ofstream myfile;

    myfile.open("responseTimesRAN.txt",std::ofstream::app);

	ran_num = arg;
	time_exceeded = false;
	ran.init(ran_num);
	ran.conn_mme();

	while (1) {
		if(is_scheduled(ran_num))
		{
		  // Run duration check
		//   cout<<"Starte ran_num "<<ran_num<<endl;
		  g_utils.time_check(g_start_time, g_req_dur, time_exceeded);
		  if (time_exceeded) {
			  	stopped_threads++;
				  cout<<stopped_threads<<endl;
		  		break;
		  }

		  // Start time
		  mstart_time = CLOCK::now();	

		  auto t_start = std::chrono::high_resolution_clock::now();
		  // Initial attach
		  ran.initial_attach();

		  auto t_end = std::chrono::high_resolution_clock::now();
          double elaspedTimeMs = std::chrono::duration<double, std::milli>(t_end-t_start).count();
          std::cout<<"response time (in msec) is "<<elaspedTimeMs;
          myfile << elaspedTimeMs<<std::endl;
          //myfile.close();
          
		  // Authentication
		  ok = ran.authenticate();
		  if (!ok) {
		  	TRACE(cout << "ransimulator_simulate:" << " autn failure" << endl;)
		  	return;
		  }

		  // Set security
		  ok = ran.set_security();
		  if (!ok) {
		  	TRACE(cout << "ransimulator_simulate:" << " security setup failure" << endl;)
		  	return;
		  }

		  // Set eps session
		  ok = ran.set_eps_session(g_traf_mon);
		  if (!ok) {
		  	TRACE(cout << "ransimulator_simulate:" << " eps session setup failure" << endl;)
		  	return;
		  }


	      

		  /*
		  // To find RTT
		  if (ran_num == 0) {
		  	g_rtt_thread = thread(ping);
		  	g_rtt_thread.detach();		
		  }
		  */ 

		  /* Data transfer */
		  ran.transfer_data(g_req_dur);
		  
		  // Detach
		  ok = ran.detach();
		  if (!ok) {
		  	TRACE(cout << "ransimulator_simulate:" << " detach failure" << endl;)
		  	return;
		  }

		  // Stop time
		  mstop_time = CLOCK::now();
		  
		  // Response time
		  mtime_diff_us = std::chrono::duration_cast<MICROSECONDS>(mstop_time - mstart_time);

		  /* Updating performance metrics */
		  g_sync.mlock(g_mux);
		  g_tot_regs++;
		  g_tot_regstime += mtime_diff_us.count();		
		  g_sync.munlock(g_mux);				
		 }
		
	}
}

void check_usage(int argc) {
	if (argc < 3) {
		TRACE(cout << "Usage: ./<ran_simulator_exec> THREADS_COUNT DURATION" << endl;)
		g_utils.handle_type1_error(-1, "Invalid usage error: ransimulator_checkusage");
	}
}

void init(char *argv[]) {
	g_start_time = time(0);
	g_threads_count = atoi(argv[1]);
	g_req_dur = atoi(argv[2]);
	g_tot_regs = 0;
	g_tot_regstime = 0;
	g_sync.mux_init(g_mux);	
	g_umon_thread.resize(NUM_MONITORS);
	g_dmon_thread.resize(NUM_MONITORS);
	g_threads.resize(g_threads_count);
	signal(SIGPIPE, SIG_IGN);
	ofstream myfile;
    myfile.open("responseTimesRAN.txt");
    myfile<<"";
    myfile.close();

}

void run() {
	int i;

	/* Tun */
	g_traf_mon.tun.set_itf("tun1", "172.16.0.1/16");
	g_traf_mon.tun.conn("tun1");

	/* Traffic monitor server */
	TRACE(cout << "Traffic monitor server started" << endl;)
	g_traf_mon.server.run(g_trafmon_ip_addr, g_trafmon_port);	

	// Uplink traffic monitor
	for (i = 0; i < NUM_MONITORS; i++) {
		g_umon_thread[i] = thread(utraffic_monitor);
		g_umon_thread[i].detach();		
	}

	// Downlink traffic monitor
	for (i = 0; i < NUM_MONITORS; i++) {
		g_dmon_thread[i] = thread(dtraffic_monitor);
		g_dmon_thread[i].detach();			
	}
	
	// Simulator threads
	for (i = 0; i < g_threads_count; i++) {
		g_threads[i] = thread(simulate, i);
	}	
	for (i = 0; i < g_threads_count; i++) {
		if (g_threads[i].joinable()) {
			g_threads[i].join();
		}
	}	
}

void print_results() {
	g_run_dur = difftime(time(0), g_start_time);
	
	cout << endl << endl;
	cout << "Requested duration has ended. Finishing the program." << endl;
	cout << "Total number of registrations is " << g_tot_regs << endl;
	cout << "Total time for registrations is " << g_tot_regstime * 1e-6 << " seconds" << endl;
	cout << "Total run duration is " << g_run_dur << " seconds" << endl;
	cout << "Latency is " << ((double)g_tot_regstime/g_tot_regs) * 1e-6 << " seconds" << endl;
	cout << "Throughput is " << ((double)g_tot_regs/g_run_dur) << endl;	
}

int main(int argc, char *argv[]) {
	check_usage(argc);
	init(argv);

	//start = time(NULL);
	//schedule();
	run();
	print_results();
	return 0;
}
