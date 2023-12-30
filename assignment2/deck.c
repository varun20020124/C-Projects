#include "deck.h"
#include <stdio.h>
#include <time.h>
/*
 * Variable: deck_instance
 * -----------------------
 *  
 * Go Fish uses a single deck
 */
struct deck deck_instance;

int shuffle() {
	// if deck is empty, then fill it again with cards
	if (deck_size()==0){
		char suits[] = {'S', 'D', 'C', 'H'};
		char ranks[] = {'A', '2', '3', '4', '5', '6', '7', '8', '9', '10', 'J', 'Q', 'K'};

		// get number of suits and ranks available
		int nsuits = sizeof(suits) / sizeof(suits[0]);
		int ncards = sizeof(ranks) / sizeof(ranks[0]);
		
		// add suits and ranks to cards in order
		for (int s = 0; s < nsuits; s++){
			for (int r = 0; r < ncards; r++){
				char rank = ranks[r];
				char suit = suits[s];
				deck_instance.list[ncards*s + r].rank[0] = rank;
			       	deck_instance.list[ncards*s + r].suit = suit;
			}
		}
		deck_instance.top_card = 0;	
	}
	// shuffle existing deck, swap pointers to each card
	srand(time(NULL));
	for (int i = deck_instance.top_card; i < 52; i++){
		// get an index of a card between the current card and the last card
		int randInd = rand() % (52 - i) + i;
		// swap cards
		struct card temp = deck_instance.list[randInd];
		deck_instance.list[randInd] = deck_instance.list[i];
		deck_instance.list[i] = temp;
	}	
   	return 0;
}

int deal_player_cards(struct player* target) {
	// number of random cards to deal to player
	int nHand = 7;
	// if not enough cards to deal, error
	if (deck_size() < 7){
		return -1;
	}
	// add top card to target player's hand
	for (int i = 0; i < nHand; i++){
		struct card* next = next_card();
		if (next == NULL){
			return -1;
		}
		add_card(target, next);
	}
	return 0;
}
struct card* next_card(){ 
	// error if no more cards in deck
	if (deck_size() == 0){
		return NULL;
	}
	// get the address of the top card
	struct card* topCard = &deck_instance.list[deck_instance.top_card];
	// make the top card empty by making rank 0
	deck_instance.list[deck_instance.top_card].rank[0] = '0';
	// increment top card index, return old top card address
	deck_instance.top_card++;
	return topCard;	
}

size_t deck_size(){
	return 52 - deck_instance.top_card;	
}

