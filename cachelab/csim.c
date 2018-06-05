#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>

#define DEFAULT 0
#define VERBOSE 1
#define HELP 2

#define HIT 0
#define COLD_MISS 1
#define EVICIT_MISS 2

#define verbose_print(...) do{ \
		if(mode == VERBOSE)	\
			printf(__VA_ARGS__);	\
	}while(0)

typedef struct CacheLine{
	int valid;
	int count;
	long long tag;
}CacheLine;

CacheLine *cache = NULL;
unsigned int hit_count = 0, miss_count = 0, eviction_count = 0;
int mode = DEFAULT, s, E, b, set_num;
char file_name[500];


void help(){
	char *help_info = "Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t "
                    "<file>\nOptions:\n  -h Print this help message.\n  -v "
                    "Optional verbose flag.\n  -s <num> Number of set index "
                    "bits.\n  -E <num> Number of lines per set.\n  -b <num> "
                    "Number of block offset bits.\n  -t <file> Trace file.\n\n"
                    "Examples :\n linux> ./csim -s 4 -E 1 -b 4 -t "
                    "traces/yi.trace\n linux>  ./csim -v -s 8 -E 2 "
                    "-b 4 -t traces/yi.trace\n ";
	printf("%s", help_info);
	exit(0);
}

int updata_cache(long long address, size_t size){
	long long tag = address >> (s + b); 
	int set_index = (address>>b) & ((1<<s)-1);
	int res = -1, start_line = set_index * E, end_line = start_line + E;
	int empty_line = -1, max_count = -1, latest_unused_line = -1, replace_line;
	for(int line = start_line; line < end_line; line++){
		if(cache[line].valid == 1){
			if(cache[line].tag == tag){
				res = HIT;
				cache[line].count = 0;
			}
			else{
				cache[line].count ++;
				if(cache[line].count > max_count){
					max_count = cache[line].count;
					latest_unused_line = line;
				}
			}
		}
		else{
			empty_line = line;
		}
	}
	if(res != HIT){
		res = empty_line != -1? COLD_MISS:EVICIT_MISS;
		replace_line = empty_line != -1?empty_line:latest_unused_line;
		cache[replace_line].valid = 1;
		cache[replace_line].count = 0;
		cache[replace_line].tag = tag;
	}
	return res;
}

/* let count and print separate from cache updata.
  *
  */
void record(int type, char identifier, unsigned long long address, unsigned int size){
	verbose_print("%c %llx,%u", identifier, address, size);
	switch(type){
		case HIT:
			hit_count ++;
			verbose_print(" hit");
			break;
		case COLD_MISS:
			miss_count ++;
			verbose_print(" miss");
			break;
		case EVICIT_MISS:
			miss_count ++;
			eviction_count ++;
			verbose_print(" miss eviction");
			break;
	}
	// if 'M', the second store is always hit
	if(identifier == 'M'){
		hit_count ++;
		verbose_print(" hit");
	}
	verbose_print("\n");
}


int main(int argc, char *argv[])
{
	int opt;
	while(-1 != (opt = getopt(argc, argv, "hvs:E:b:t:"))){
		switch(opt) {
			case 'h':
				mode = HELP;
				break;
			case 'v':
				mode = VERBOSE;
				break;
			case 's':
				s = atoi(optarg);
				set_num = 1<<s;
				break;
			case 'E':
				E = atoi(optarg);
				break;
			case 'b':
				b = atoi(optarg);
				break;
			case 't':
				strcpy(file_name,optarg);
				break;
		}
	}

	if(mode == HELP)
		help();
	if((cache = (CacheLine *)malloc(set_num * E * sizeof(CacheLine))) == NULL)
		exit(0);
	for(int i = 0;i < set_num*E; i++){
		cache[i].valid = 0;
		cache[i].count = 0;
		cache[i].tag = 0;
	}
	FILE *pFile;
	pFile = fopen(file_name, "r");
	char identifier;
	long long unsigned address;
	int size;
	while(fscanf(pFile, " %c %llx,%d", &identifier, &address, &size)>0){
		/* according lab rules: ignore all instruction cache accesses*/
		if(identifier != 'I'){
			int res = updata_cache(address, size);
			record(res, identifier, address, size);
		}
	}
	fclose(pFile);
	free(cache);
    printSummary(hit_count, miss_count, eviction_count);
    return 0;
}
