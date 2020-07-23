#include <immintrin.h>
#include <thread>
#include <unistd.h>
#include <iostream>

using namespace std;

#define n_threads 1
#define OPSIZE 1000000000
typedef struct Account{
	long balance;
	long number;
} __attribute__((aligned(64))) account_t;

typedef struct Bank{
	account_t* accounts;
	long size;
} bank_t;

bool done = 0;
long *tx, *_abort, *capacity, *debug, *failed, *conflict, *zero;
int fallback_lock;

bool 
rtm_begin(int id)
{   
    while(true) { 
        unsigned stat;
        stat = _xbegin ();
        if(stat == _XBEGIN_STARTED) {
            return true;
        } else {
            _abort[id]++;
            if (stat == 0){
                zero[id]++;
            }
            //call some fallback function
            if (stat& _XABORT_CONFLICT){
                conflict[id]++;
            }

            //will not succeed on a retry
            if ((stat &  _XABORT_RETRY) == 0) {
                failed[id]++;
                //grab a fallback lock
                while (!__sync_bool_compare_and_swap(&fallback_lock,0,1)) {
                }
                return false;
            }
        }
    }
}

void* f1(bank_t* bank, int id){
	for(int i=0; i<OPSIZE; i++){ 
		int src = rand()%bank->size;
		int dst = rand()%bank->size;
		while(src == dst){
			dst = rand()%bank->size;
		} 
		
		int in_rtm, y;
		while(true){
			in_rtm = rtm_begin(id);
			y = fallback_lock;
			bank->accounts[src].balance--;
			bank->accounts[dst].balance++;
			if (in_rtm){
			    _xend();
				tx[id]++;
				break;
			}else{
			    while(!__sync_bool_compare_and_swap(&fallback_lock, 1, 0)){
			    }
/*
			unsigned stat =  _xbegin();
			if(stat == _XBEGIN_STARTED){
				bank->accounts[src].balance++;	
				bank->accounts[dst].balance--;
				_xend();	
				tx[id]++;
				break;
			}else{
*/
				unsigned stat = in_rtm;
				_abort[id]++;

				if (stat == 0){
					zero[id]++;
				}
				if (stat & _XABORT_CONFLICT){
					conflict[id]++;
				}
				if (stat & _XABORT_CAPACITY){
					capacity[id]++;
				}
				if (stat & _XABORT_DEBUG){
					debug[id]++;
				}
				if ((stat & _XABORT_RETRY) == 0){
					failed[id]++;
					break;
				}
				if (stat & _XABORT_NESTED){
					printf("[ PANIC ] _XABORT_NESTED\n");
					exit(-1);
				}
				if (stat & _XABORT_EXPLICIT){
					printf("[ panic ] _XBEGIN_EXPLICIT\n");
					exit(-1);
				}
			}
		}
	}
	return NULL;
}
void* f2(bank_t* bank){
	printf("_heartbeat function\n");
	long last_txs=0, last_aborts=0, last_capacities=0, last_debugs=0, last_faileds=0, last_conflicts=0, last_zeros = 0;
	long txs=0, aborts=0, capacities=0, debugs=0, faileds=0, conflicts=0, zeros = 0;
	while(1){
		last_txs = txs;
		last_aborts = aborts;
		last_capacities = capacities;
		last_debugs = debugs;
		last_conflicts = conflicts;
		last_faileds = faileds;
		last_zeros = zeros;

		txs=aborts=capacities=debugs=faileds=conflicts=zeros = 0;
		for(int i=0; i<n_threads; i++){
			txs += tx[i];
			aborts += _abort[i];
			faileds += failed[i];
			capacities += capacity[i];
			debugs += debug[i];
			conflicts += conflict[i];
			zeros += zero[i];
		}

		printf("txs\t%ld\taborts\t\t%ld\tfaileds\t%ld\tcapacities\t%ld\tdebugs\t%ld\tconflit\t%ld\tzero\t%ld\n", 
			txs - last_txs, aborts - last_aborts , faileds - last_faileds, 
			capacities- last_capacities, debugs - last_debugs, conflicts - last_conflicts,
			zeros- last_zeros);
		
		sleep(1);
	}
}

int main(int argc, char** argv){
	int accounts = 10240;

	bank_t* bank = new bank_t;
	bank->accounts = new account_t[accounts];
	bank->size = accounts;

	for(int i=0; i<accounts; i++){
		bank->accounts[i].number = i;
		bank->accounts[i].balance = 0;
	}

	thread* pid[n_threads];
	tx = new long[n_threads];
	_abort = new long[n_threads];
	capacity = new long[n_threads];
	debug = new long[n_threads];
	failed = new long[n_threads];
	conflict = new long[n_threads];
	zero = new long[n_threads];

	thread* _heartbeat = new thread(f2, bank);
	for(int i=0; i<n_threads; i++){
		tx[i] = _abort[i] = capacity[i] = debug[i] = failed[i] = conflict[i] = zero[i] =  0;
		pid[i] = new thread(f1, bank, i);
	}

//	sleep(5);
	for(int i=0; i<n_threads;i++){
		pid[i]->join();
	}
	return 0;
}
