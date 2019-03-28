//hashmap basic

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "uthash.h"
#include <string.h>



//SOURCE: https://troydhanson.github.io/uthash/userguide.html



typedef struct hashnode //dictionary node
{
    char *key;
    char *value;
    uint16_t value_len;
    uint16_t key_len;
    UT_hash_handle hh; /* makes this structure hashable */

}hashnode;

struct hashnode *head = NULL;

void add_movie(char *key, char *value, int key_len, int value_len) {
    struct hashnode *s;

    HASH_FIND_STR(head, key, s);// head->first node of Linkedlist; key->string to search for key; s-> output/* id already in the hash? */
    if (s==NULL) {
        
        s = (struct hashnode *)calloc(sizeof *s);
        
        s-> key_len = key_len;
        s-> value_len = value_len;

        s->key = malloc(key_len*sizeof  char);
        strncpy(s->key, key, keylen);
        s->value = malloc(value_len*sizeof  char);
        strncpy(s->key, key, keylen);
        
        HASH_ADD_KEYPTR( hh, head, s->key, strlen(s->key), s ); // insert "s" in hashtable "hh" instead of "key"
    }



struct hashnode *find_movie(char *key) {
    struct hashnode *s;

    HASH_FIND_STR( head, key, s );  /* s: output pointer */
    return s->value;
}

void delete_movie(struct hashnode *movie) {
    HASH_DEL(head, movie);
    // Möglicherweise problematisch?
    free(movie->key);
    free(movie->value);  /* movie: pointer to deletee */
    free(movie);
}

void delete_all() {
  struct hashnode *current_movie, *tmp;

  HASH_ITER(hh, head, current_movie, tmp) { //Jonas was  macht diese funktion?
    HASH_DEL(head, current_movie);  /* delete it (movies advances to next) */
    free(current_movie);             /* free it */
  }
}

void print_movies() {
    struct hashnode *s;

    for(s=head; s != NULL; s=(struct hashnode*)(s->hh.next)) {
        printf("movie id %s: name %s\n", s->key, s->value);
    }
}

int value_sort(struct hashnode *a, struct hashnode *b) { // sorted by value
    return strcmp(a->value,b->value); // return: >0= a<b; <= = a>b; 0=a==B;
}
int key_sort(struct hashnode *a, struct hashnode *b) { // sorted by key 
    return strcmp(a->key, b->key); // return: >0= a<b; <= = a>b; 0=a==B; 
}

void sort_by_name() {
    HASH_SORT(head, value_sort);
}


void sort_by_key() {
    HASH_SORT(head, key_sort);
}




//___________________________________________________Start_Main________________________________________________________________________________________________





int main(int argc, char const *argv[])
{
   char *key[] = { "joe", "bob", "betty", NULL };
   char *value[] = { "joe!", "bob!", "betty!", NULL};
    struct hashnode *s = NULL;

    for (int i = 0; key[i]; ++i) {
        add_movie(key[i], value[i]);
    }

  s = find_movie(key[2]); 
    if (s) printf("betty's id is %s\n", s->key);
    print_movies();
    sort_by_key();
    print_movies();



    /* free the hash table contents */
  delete_all();
    return 0;

}
//___________________________________________________ende_Main_________________________________________________________________________________________