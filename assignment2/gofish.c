#include <stdio.h>
#include "deck.h"
#include "player.h"
#include "card.h"

int main(int args, char* argv[]) {
	fprintf(stdout, "Put your code here.");
  	char end = 'N';
	do {
		printf("Shuffling deck...\n");
		// shuffle cards, deal to user and computer
		shuffle();
		deal_player_cards(&user);
		deal_player_cards(&computer);
		// counter for turns
		int turn = 1;
		do {
			// print books and player's hand
			printf("\nPlayer 1's Hand - ");
			struct hand* curr = user.card_list;
			while(curr != NULL){
				char rank1 = curr->top.rank[0];
				char rank2 = curr->top.rank[1];
				char suit = curr->top.suit;
				printf("%c%c%c ", rank1, rank2, suit);
				curr = curr->next;
			}
			printf("\nPlayer 1's Book - ");
			for (int i = 0; i < 7; i++){
				printf("%c ",(&user)->book[i]);
			}
			printf("\nPlayer 2's Book - ");
			for (int i = 0; i < 7; i++){
				printf("%c ",(&computer)->book[i]);
			}
			printf("\nPlayer %d's turn, enter a Rank: ", turn%2);
			// player's turn if turn is odd, computer's turn if even
			if (turn % 2 == 1) {
				char rnk = user_play(&user);
				// if the computer has the rank, transfer
				if (search(&computer, rnk) == 1){
					transfer_cards(&computer, &user, rnk);
					check_add_book(&user);
				}
			} else {
				char rnk = computer_play(&computer);
				// if the user has the rank, transfer and check opponent
				if (search(&user, rnk) == 1){
					transfer_cards(&user, &computer, rnk);
					check_add_book(&computer);
				}
			}
			// update turn 
			turn++;
		} while (game_over(&user) != 1 && game_over(&computer) != 1);
		// when the game is over, reset players and prompt for another game
		reset_player(&user);
		reset_player(&computer);
		printf("\nDo you want to play again? [Y/N]: ");
		scanf("%c",&end);	
	} while (end != 'Y');
	// print exit message
	printf("Exiting.");
}

