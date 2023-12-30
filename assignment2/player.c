#include "player.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * Instance Variables: user, computer   
 * ----------------------------------
 *  
 *  We only support 2 users: a human and a computer
 */
struct player user;
struct player computer;

int add_card(struct player* target, struct card* new_card){
	// new card to add
	struct hand* new;
	new = (struct hand*)malloc(sizeof(struct hand));
	// throw error if no card
	if (new == NULL){
		return -1;
	}
	// set attributes of new card
	new->top = *new_card;
	new->next = NULL;
	// if player has no cards, make this the first card in hand
	if (target->card_list == NULL){
		target->card_list = new;
	} 
	// otherwise, add to end of current hand
	else {
		struct hand* curr = target->card_list;
		while (curr->next != NULL) {
			curr = curr->next;
		}
		curr->next = new;
	}
	// update hand size
	target->hand_size++;
	return 0;
}

int remove_card(struct player* target, struct card* old_card){
	// get an iterator and track previous card
	struct hand* curr = target->card_list;
	struct hand* prev = NULL;
	// if no cards, return 0
	if (curr == NULL) {
		return 0;
	} 
	// if the iterator is not the card to remove, move on
	while(curr->top.suit != old_card->suit && curr->top.rank[0] != old_card->rank[0]) {
		prev = curr;
		curr = curr->next;
		if (curr == NULL) {
			return 0;
		}
	}
	// once card is found, skip the link in linked list of cards
	if (prev != NULL){
		prev->next = curr->next;
	} 
	// if found card is the first in the list, then make the next card the first
	else {
		target->card_list = curr->next;
	}
	// free memory from old pointer
	free(curr);
	// update hand size
	target->hand_size--;
	return 0;
}

char check_add_book(struct player* target){
	// get an iterator
	struct hand* curr = target->card_list;
	// loop to end of linked list
	while (curr != NULL){
		// count of the instances of this rank
		size_t count = 0;
		char curr_rank = curr->top.rank[0];
		struct hand* temp = curr;
		// iterate through list again, count occurrences of rank
		while (temp != NULL){
			if (temp->top.rank[0] == curr_rank) {
				count++;
			}
			temp = temp->next;
		}
		// if it occurs 4 times, then remove card and add to book
		if (count == 4){
			for (int i = 0; i < 4; i++){
				remove_card(target, &((struct card){curr_rank, ""}));
			}
			for (int i = 0; i < 7; i++){
				if (target->book[i] == 0) {
					target->book[i] = curr_rank;
					break;
				}
			}
			return curr_rank;
		}
		// check all cards in hand
		curr = curr->next;
	}
	return 0;
}

int search(struct player* target, char rank){
	// iterator to loop through linked list
	struct hand* curr = target->card_list;
	// if the rank exists in the player's hand, return 1
	while (curr != NULL){
		if(curr->top.rank[0] == rank){
			return 1;
		}
		curr = curr->next;
	}
	return 0;
}

int transfer_cards(struct player* src, struct player* dest, char rank){
	// counter for the number of cards transferred
	int count = 0;
	// loop until there are no more cards of the rank in target player hand
	while (search(src, rank) != 0){
		// iterator
		struct hand* curr = src->card_list;
		if (curr == NULL){
			return -1;
		}
		// go until you find one of the same rank
		// then add to dest and remove from src, update count
		while (curr->top.rank[0] != rank) {
			curr = curr->next;
			if (curr == NULL){
				return -1;
			}
		}
		count++;
		add_card(dest, &(curr->top));
		remove_card(src, &(curr->top));
	}
	return count;
}

int game_over(struct player* target){
	// get the size of the book to win 
	size_t bookSize = 7;
	// counter for how full player's book is
	size_t playerBook = 0;
	// if the book has nonzero values, it is full so add to count
	for (int i = 0; i < bookSize; i++){
		if (target->book[i] != 0){
			playerBook++;
		}
	}
	// if full, return 1, otherwise 0
	if (playerBook == bookSize){
		return 1;
	} else {
		return 0;
	}
}

int reset_player(struct player* target){
	// get iterator
	struct hand* curr = target->card_list;
	// loop til end of hand, remove cards
	while (curr != NULL){
		remove_card(target, &(curr->top));
		curr = curr->next;
	}
	size_t bookSize = 7;
	// reset book
	for (int i = 0; i < bookSize; i++){
		target->book[i] = 0;
	}
	// reset hand size
	target->hand_size = 0;
	return 0;
}

char computer_play(struct player* target){
	// array of possible ranks to play
	char ranks[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K'};
	// array to store all ranks the computer has in hand
	char availRanks[13];
	// add ranks if the computer has it in their hand 
	for (int i = 0; i < sizeof(ranks)/sizeof(ranks[0]); i++){
		if (search(target, ranks[i]) == 1){
			availRanks[i] = ranks[i];
		}
	}
	// get a random nonzero rank from available ranks
	size_t randI = rand() % 13;
	while (availRanks[randI] != '\0') {
		randI = rand() % 13;
	}
	// return random rank
	return availRanks[randI];
}

char user_play(struct player* target){
	// rank to return
	char rank;
	// prompt user and read input
	printf(" enter a Rank to play:");
	scanf("%c",&rank);
	// if not in their hand, prompt again and repeat
	while (search(target, rank) != 1) {
		printf("\nError - must have at least one card from rank to play");
		printf(" enter a Rank to play:\n");
		scanf("%c",&rank);
	}
	// return the rank	
	return rank;
}
