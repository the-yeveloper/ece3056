#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "cache.h"


extern uns64 cycle_count; // You can use this as timestamp for LRU

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE INIT FUNCTION -----------
////////////////////////////////////////////////////////////////////

Cache  *cache_new(uns64 size, uns64 assoc, uns64 linesize, uns64 repl_policy){

   Cache *c = (Cache *) calloc (1, sizeof (Cache));
   c->num_ways = assoc;
   c->repl_policy = repl_policy;

   if(c->num_ways > MAX_WAYS){
     printf("Change MAX_WAYS in cache.h to support %llu ways\n", c->num_ways);
     exit(-1);
   }

   // determine num sets, and init the cache
   c->num_sets = size/(linesize*assoc);
   c->sets  = (Cache_Set *) calloc (c->num_sets, sizeof(Cache_Set));

   return c;
}

////////////////////////////////////////////////////////////////////
// ------------- DO NOT MODIFY THE PRINT STATS FUNCTION -----------
////////////////////////////////////////////////////////////////////

void    cache_print_stats    (Cache *c, char *header){
  double read_mr =0;
  double write_mr =0;

  if(c->stat_read_access){
    read_mr=(double)(c->stat_read_miss)/(double)(c->stat_read_access);
  }

  if(c->stat_write_access){
    write_mr=(double)(c->stat_write_miss)/(double)(c->stat_write_access);
  }

  printf("\n%s_READ_ACCESS    \t\t : %10llu", header, c->stat_read_access);
  printf("\n%s_WRITE_ACCESS   \t\t : %10llu", header, c->stat_write_access);
  printf("\n%s_READ_MISS      \t\t : %10llu", header, c->stat_read_miss);
  printf("\n%s_WRITE_MISS     \t\t : %10llu", header, c->stat_write_miss);
  printf("\n%s_READ_MISSPERC  \t\t : %10.3f", header, 100*read_mr);
  printf("\n%s_WRITE_MISSPERC \t\t : %10.3f", header, 100*write_mr);
  printf("\n%s_DIRTY_EVICTS   \t\t : %10llu", header, c->stat_dirty_evicts);

  printf("\n");
}



////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Return HIT if access hits in the cache, MISS otherwise
// Also if mark_dirty is TRUE, then mark the resident line as dirty
// Update appropriate stats
////////////////////////////////////////////////////////////////////
int get_bits(Addr value, int start, int end){
  int result;
  assert(start >= end );
  result = value >> end;
  result = result % ( 1 << ( start - end + 1 ) );
  return result;
}



Flag    cache_access(Cache *c, Addr lineaddr, uns mark_dirty){
  Flag outcome=MISS;
  int i=0;
  int lengthOfCache=sizeof(Cache_Set)/sizeof(Cache_Line);
  int numOfSets=log2(c->num_sets);
 // printf("num of sets %d\n", numOfSets);
  int lengthOfTable=c->num_ways;
//  printf("length of Table %d\n", lengthOfTable);
 // printf("numOfSets %d\n", numOfSets);
  Addr currentAddress=lineaddr;
  int sixBits=get_bits(lineaddr,(numOfSets-1),0);

//  printf("address %x\n",currentAddress);
//  printf("cache address %x\n", c->sets[0].line[sixBits].tag);

  if (mark_dirty){
    c->stat_write_access++;
  }else{
    c->stat_read_access++;
  }
  while(i<lengthOfTable){

	Addr currTag=(c->sets[sixBits]).line[i].tag;
	if(currTag==currentAddress){
        outcome=HIT;
        c->sets[sixBits].line[i].last_access_time=cycle_count;
        if(mark_dirty){
            c->sets[sixBits].line[i].dirty=TRUE;
        }
        break;
	}
    i++;
  }
  // Your Code Goes Here
  if(outcome==MISS){
    if(mark_dirty){
        c->stat_write_miss++;
    }else{
        c->stat_read_miss++;
    }

  }
  return outcome;
}

////////////////////////////////////////////////////////////////////
// Note: the system provides the cache with the line address
// Install the line: determine victim using repl policy (LRU/RAND)
// copy victim into last_evicted_line for tracking writebacks
////////////////////////////////////////////////////////////////////

void    cache_install(Cache *c, Addr lineaddr, uns mark_dirty){

  // Your Code Goes Here
  // Note: You can use cycle_count as timestamp for LRU

  int i=0;
  int lengthOfCache=sizeof(Cache_Set)/sizeof(Cache_Line);
  int numOfSets=log2(c->num_sets);
  int lengthOfTable=c->num_ways;

 // printf("numOfSets %d\n", numOfSets);
  Addr currentAddress=lineaddr;
  int sixBits=get_bits(lineaddr,(numOfSets-1),0);
  int done=FALSE;

  while(i<lengthOfTable){
	Addr currTag=c->sets[sixBits].line[i].tag;
	if(c->sets[sixBits].line[i].tag==0){
        c->sets[sixBits].line[i].dirty=mark_dirty;
        c->sets[sixBits].line[i].last_access_time=cycle_count;
        c->sets[sixBits].line[i].tag=lineaddr;
        c->sets[sixBits].line[i].valid=TRUE;
        done=TRUE;
        break;
	}
    i++;
  }
  if(done==FALSE){
          if(c->repl_policy){
                  int random_num=rand()%c->num_ways;
                  c->last_evicted_line=c->sets[sixBits].line[random_num];
                  c->sets[sixBits].line[random_num].dirty=mark_dirty;
                  c->sets[sixBits].line[random_num].last_access_time=cycle_count;
                  c->sets[sixBits].line[random_num].tag=lineaddr;
                  c->sets[sixBits].line[random_num].valid=TRUE;
                   if(c->sets[sixBits].line[random_num].dirty){
                        c->stat_dirty_evicts++;
                    }

          }else{
                  i=0;
                  unsigned int minimumCycleCount;
                  int cacheBlock;
                  while(i<lengthOfTable){
                        Addr currTag=(c->sets[sixBits]).line[i].tag;
                        if(i==0){
                            minimumCycleCount=c->sets[sixBits].line[i].last_access_time;
                            cacheBlock=i;

                        }

                        if(minimumCycleCount>c->sets[sixBits].line[i].last_access_time){
                            minimumCycleCount=c->sets[sixBits].line[i].last_access_time;
                            cacheBlock=i;
                        }
                        i++;
                  }
                    if(c->sets[sixBits].line[cacheBlock].dirty){
                        c->stat_dirty_evicts++;
                    }
                    c->last_evicted_line=c->sets[sixBits].line[cacheBlock];
                    c->sets[sixBits].line[cacheBlock].dirty=mark_dirty;
                    c->sets[sixBits].line[cacheBlock].last_access_time=cycle_count;
                    c->sets[sixBits].line[cacheBlock].tag=lineaddr;
                    c->sets[sixBits].line[cacheBlock].valid=TRUE;



          }



  }
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


