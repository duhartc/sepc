/*****************************************************
 * Copyright Grégory Mounié 2008-2013                *
 * This code is distributed under the GLPv3 licence. *
 * Ce code est distribué sous la licence GPLv3+.     *
 *****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "mem.h"
#include <stdbool.h>

/** squelette du TP allocateur memoire */

void *zone_memoire = 0;

/* ecrire votre code ici */
typedef struct zone_libre zl;
struct zone_libre {
    zl *next;
    void *zone;
};
zl *tzl[BUDDY_MAX_INDEX + 1]; //tableau de pointeurs vers des zones libres

int 
mem_init()
{
  if (! zone_memoire)
    zone_memoire = (void *) malloc(ALLOC_MEM_SIZE);
  if (zone_memoire == 0)
    {
      perror("mem_init:");
      return -1;
    }

  /* ecrire votre code ici */
  for (int i = 0; i< BUDDY_MAX_INDEX; i++) {
      // la case i contient la liste des blocs de taille 2^i
      tzl[i] = NULL;
  }
  //on a un bloc unique de taille ALLOC_MEM_SIZE (=2^BUDDY_MAX_INDEX)
  tzl[BUDDY_MAX_INDEX] = zone_memoire;
  tzl[BUDDY_MAX_INDEX]->zone = zone_memoire;
  tzl[BUDDY_MAX_INDEX]->next = NULL;
  return 0;
}

bool available_size(unsigned int index, unsigned int *first_index ) {
    if (index > BUDDY_MAX_INDEX) {
        return false;
    }
    for (unsigned int i = index; i < BUDDY_MAX_INDEX + 1; i++) {
    
        if (tzl[i]!= NULL) {
            // si on trouve une zone libre de taille supérieure disponible
            *first_index = i;
            return true;
        }
    }
    return false;
}

void * divide_zone(unsigned int index, unsigned int first_index) {
    // index est la puissance correspondant à la taille que l'on souhaite allouer
    // first_index le premier index disponible pouvant la contenir   
    if (index==first_index) {            
        void * zone = tzl[first_index]->zone;
        // on retire l'ex-zone libre de la tzl
        tzl[first_index] = tzl[first_index]->next;
        return zone;
    }
    unsigned int index_new_zl = first_index - 1;
    // cut_zone est l'adresse de la nouvelle zone libre découpée
    void * cut_zone = (void *)((unsigned long)tzl[first_index]->zone 
                               +((unsigned long) 1 << (index_new_zl)));

    // on insère en tête dans la liste
    tzl[index_new_zl] = cut_zone;
    tzl[index_new_zl]->zone = cut_zone;
    // si on crée la zone, c'est qu'il n'y en avait pas avant 
    tzl[index_new_zl]->next = NULL;  

    return divide_zone(index, index_new_zl);
}
unsigned int tzl_index(unsigned long size) {
    unsigned int size_index = 0;
    if (size==ALLOC_MEM_SIZE) {
        size_index = BUDDY_MAX_INDEX;
    }
    else {
	    unsigned long size_temp = size;
        while (size != 0) {
            // on cherche l'indice correspondant dans la tzl donc
            // on cherche le k le plus grand possible tel que size = 2^k + reste)
            size = size >> 1;
            size_index++;
        }
        size_index--;
        // si la taille n'est pas une puissance de 2 (reste != 0)
        // on la stocke dans un bloc de puissance de 2 supérieure 
        if (size_temp > ((unsigned long) 1 << size_index)) {
            size_index++;
        }
    }
    return size_index;
}

void *
mem_alloc(unsigned long size)
{
  /*  ecrire votre code ici */
    // si on n'a pas alloué la mémoire
    if (zone_memoire == 0 || size == 0){
        return (void *)0;
    }
    unsigned int size_index = tzl_index(size);
    
    // si la taille demandée est trop grande
    unsigned int first_index = 0; //premier index possible disponible
    if (!available_size(size_index, &first_index)){
        return (void *)0;
    }
    // si on a une zone de la bonne taille disponible
    if (tzl[size_index] != NULL) {
        void * zone = tzl[size_index]->zone;
        // on retire l'ex-zone libre de la tzl
        tzl[size_index] = tzl[size_index]->next;
        return zone;
    }
    // si on doit diviser une zone libre
    else {
        return divide_zone(size_index, first_index);
    }
  return 0;  
}

bool buddy(void *ptr1, void * ptr2, unsigned int index) {
    // retourne true si ptr1 et ptr2 sont des voisins
	unsigned long size;
	if (index==0)
		size = 1;
	else 
		size = (unsigned long) 1<<(index-1);

	return (ptr1 == ((ptr2-zone_memoire)^size)+zone_memoire);
}

void merge_zone(unsigned int index, void *ptr) {
    if (tzl[index] == NULL) {
        // si auncune zone libre de la taille souhaitée n'est disponible
        // on ajoute directement la zone libre
        tzl[index] = ptr;
        tzl[index]->zone = ptr;
        tzl[index]->next = NULL;
    }
    else {
        // sinon on essaie de créer une zone libre plus grande
        zl *cour = tzl[index];
	zl *prec = NULL;
        bool merge = false;
	void *debut_zone = ptr;
        while (cour != NULL) {
		if (buddy(cour, ptr, index)) {
                // si une des zones disponibles est un voisin
			if (prec == NULL) {
				tzl[index] = cour->next;
			} else {
				prec->next = cour->next;
			}

			if (cour < (zl*)ptr)
				debut_zone=cour;

			merge = true;
			break;
            }
	    prec = cour;
            cour = cour->next;
        }

        // si on a trouvé un voisin
        // on essaie de fusionner la nouvelle zone constituée des 2 voisins
        if (merge) merge_zone(index+1, debut_zone);
	else {
		zl * zone = tzl[index];
		tzl[index] = (zl*)ptr;
		printf("%i",index);
		fflush(stdout);
		tzl[index]->zone = ptr;
		printf("OK");
		fflush(stdout);
		tzl[index]->next = zone;
	       
	}
    }
}

int 
mem_free(void *ptr, unsigned long size)
{
	if (ptr < zone_memoire || size > ALLOC_MEM_SIZE || ptr > zone_memoire+ ALLOC_MEM_SIZE) {
		//on essaie de libèrer une zone qui n'est pas dans la zone mémoire
		return -1;
	}
	/* ecrire votre code ici */
	//size_index est l'index correspondant à la taille à libérer
	unsigned int size_index = tzl_index(size);


        /* //Recherche du compagnon et de son état */
	/* void * buddy = ((ptr-zone_memoire)^size)+zone_memoire; */
	
	/* bool free_buddy = false; */
	/* zl *cour = tzl[size_index]; */
	/* while (cour != NULL && !free_buddy) { */
	/* 	if (cour->zone == buddy) */
	/* 		free_buddy = true; */

	/* 	cour = cour->next; */
	/* } */

	/* //Si le compagnon est libre: on les regroupe dans un bloc à size_index+1 */
	/* if (tzl[size_index+1] == NULL) { */
	/* 	tzl[size_index+1]=ptr; */
	/* } */

	if (tzl[size_index] == NULL) {
		// si auncune zone libre de la taille souhaitée n'est disponible
		// on ajoute directement la zone libre
		tzl[size_index] = ptr;
		tzl[size_index]->zone = ptr;
		tzl[size_index]->next = NULL;
	}
	else if (size_index == BUDDY_MAX_INDEX) {
		tzl[size_index] = zone_memoire;
		tzl[size_index]->zone = zone_memoire;
		tzl[size_index]->next = NULL;
	}
	else {
		// sinon on fusionne le bloc libèré avec son compagnon
		merge_zone(size_index, ptr);
	}   
	return 0;
}


int
mem_destroy()
{
  /* ecrire votre code ici */
	
  free(zone_memoire);
  zone_memoire = 0;
  return 0;
}

