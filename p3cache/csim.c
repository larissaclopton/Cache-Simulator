#include "cachelab.h"
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#define BUFSIZE 40

typedef struct line {
	int tag;
	int valid;
	struct line  *next;
} line;

typedef struct _set {
	line *llist;
} set;

typedef struct _cache {
	set *sets;
} cache;

cache *sim_cache;
int hits = 0;
int misses = 0;
int evictions = 0;

line *create_llist(int E)
{
	int i;
	line *head;
	line *prev;

	line *first_line = (line *)malloc(sizeof(line));
	head = first_line;
	prev = head;

	for(i = 1; i < E; i++) {
		line *newline = (line *)malloc(sizeof(line));
		
		newline->tag = 0;
		newline->valid = 0;
		newline->next = NULL;
		
		prev->next = newline;
		prev = prev->next;
		
	}

	return head;
}	

line *evict_line(int tag, line *head)
{

	line *new_head;
	
	if(head->next != NULL) {
		new_head = head->next;
	
		line *new_tail = head;
		new_tail->tag = tag;
		new_tail->next = NULL;

		line *tmp;
		for(tmp = new_head; tmp->next != NULL; tmp = tmp->next) {
		}
	
		tmp->next = new_tail;

	} else {
		new_head = head;
		
		new_head->tag = tag;
	}

	return new_head; 

}

line *shift(int tag, line *head)
{

	line *tmp, *new_tail;
	line *prev = NULL;

	for(tmp = head; tmp->tag != tag; tmp = tmp->next) {
		prev = tmp;
	}

	if(tmp->next == NULL || !tmp->next->valid) {
		return head;
	}
	else if(prev == NULL) {
		line *new_head = head->next;
		head->next = NULL;
		
		for(tmp = new_head; tmp->next != NULL && tmp->next->valid; tmp = tmp->next) {
		}
		
		if(tmp->next == NULL) {
			tmp->next = head;
		}

		else {
			line *succ = tmp->next;
			tmp->next = head;
			head->next = succ;
		}
		return new_head;

	}
	else {

		prev->next = tmp->next;
		tmp->next = NULL;
		new_tail = tmp;

		for(tmp = head; tmp->next != NULL && tmp->next->valid; tmp = tmp->next) {
		}

		if(tmp->next == NULL) {
			tmp->next = new_tail;
		}
		else {
			line *succ = tmp->next;
			tmp->next = new_tail;
			new_tail->next = succ;
		}
		return head;
	}

}

void search_cache(int s, int t, int E, int verbose)
{

	line *lines = sim_cache->sets[s].llist;

	int placed = 0;

	line *tmp;

	for(tmp = lines; tmp != NULL; tmp = tmp->next) {
		if(tmp->tag == t && tmp->valid) {
			hits++;
			sim_cache->sets[s].llist = shift(t, lines);
			placed = 1;
			if(verbose)
				printf("hit\n");
			break;
		}
		else if(!(tmp->valid)) {
			misses++;
			placed = 1;
			if(verbose)
				printf("miss\n");
			tmp->tag = t;
			tmp->valid = 1;
			break;
		}
	}

	if(!placed) {
		misses++;
		evictions++;
		if(verbose)
			printf("miss eviction\n");
		
		sim_cache->sets[s].llist = evict_line(t, lines);
	}

}

char *bin_string(char hex) {

	char *result = (char *)malloc(sizeof(char)*5);

	char binary[16][5] = {"0000", "0001", "0010", "0011",
                              "0100", "0101", "0110", "0111",
                              "1000", "1001", "1010", "1011",
                              "1100", "1101", "1110", "1111"};

	int val;
	if(hex >= 'a')
		val = hex - 'a' + 10;
	else
		val = hex - '0';
	
	strcpy(result, binary[val]);
	return result;
}

char *hexstring_binarystring(char *address) {

	char *result = (char *)malloc(sizeof(char)*65);	

	char binary[65];
	binary[0] = '\0';
	int i;

	int leading = 16 - strlen(address);
	for(i = 0; i < leading; i++) {
		strcat(binary,bin_string('0'));
	}

	for(i = 0; address[i] != '\0'; i++) {
		strcat(binary,bin_string(address[i]));
	}

	strcpy(result, binary);
	return result;
}

void free_lines(line *head)
{
	line *tmp;
	
	while(head != NULL) {
		tmp = head;
		head = head->next;
		free(tmp);
	}

}


void free_cache(int S)
{
	int i;
	set *current_set;

	for(i = 0; i < S; i++) {
		current_set = &sim_cache->sets[i];
		free_lines(current_set->llist);
	}

	free(sim_cache->sets);
	free(sim_cache);	

}	

int main(int argc, char *argv[])
{

	int verbose = 0;
	int set_bits;
	int S;
	int E;
	int b;
	char *tracename = NULL;

	int i;
	for(i = 1; i < argc; i += 2) {
		char flag = argv[i][1];
		switch(flag) {
			case 'v':
				verbose = 1;
				break;
			case 's':
				set_bits = atoi(argv[i+1]);
				S = 1 << set_bits;
				break;
			case 'E':
				E = atoi(argv[i+1]);
				break;
			case 'b':
				b = atoi(argv[i+1]);
				break;
			case 't':
				tracename = argv[i+1];
				break;
			default:
				abort();
		}
	}

	sim_cache = (cache *)malloc(sizeof(cache));
	set *sets = (set *)malloc(S*sizeof(set));

	for(i = 0; i < S; i++) {
		sets[i].llist = create_llist(E);
	}

	sim_cache->sets = sets;
		
	FILE *fp;
	if((fp = fopen(tracename, "r")) == NULL) {
		fprintf(stderr, "Could not open trace %s\n", tracename);
		return -1;
	}

	char *type;
	char *address;
	char *addr;
	char buf[BUFSIZE] = " ";

	while(fgets(buf, sizeof(buf), fp) != NULL) {
			
		type = strtok(buf," ,");
		address = strtok(NULL," ,");
		addr = hexstring_binarystring(address);
		
		int tag_bits = 64 - set_bits - b;
		char tag[tag_bits+1];
		char set[set_bits+1];

		i = 0;
		while(i < tag_bits) {
			tag[i] = addr[i];
			i++;
		}
		tag[i] = '\0';
		int i = 0;
		while(i < set_bits){
			set[i] = addr[i+tag_bits];
			i++;
		}
		set[i] = '\0';		
		int cache_set = (int)strtol(set,NULL,2);
		int cache_tag = (int)strtol(tag,NULL,2);

		if((strcmp(type, "L") == 0) || (strcmp(type, "S") == 0)) {
			if(verbose) {
				printf("%s %s ", type, address);
			}
			search_cache(cache_set, cache_tag, E, verbose);
		} else if(strcmp(type, "M") == 0) {
			search_cache(cache_set, cache_tag, E, verbose);
			search_cache(cache_set, cache_tag, E, verbose);
		}
		
	}
	
	free_cache(S);

    	printSummary(hits, misses, evictions);
    	return 0;
}
