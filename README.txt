~~~~~~~~~~~~~~~~~~~~~~~~~~~
Checkers AI program
Date: November 17, 2019
Author: Cody Palin
~~~~~~~~~~~~~~~~~~~~~~~~~~~
Individual Contribution:
As i was working solo on this project, the entirety of the program was
developed myself. I spent around 16 hours on this program.

Development:
    The majority of time was spent debugging various issues with the initial 
minimax algorithm. I solved the issues that were present in the in class 
implementation  which turned out to be a problem with attempting to perform 
moves that were not legal moves, this issue was present due to a simple 
error involving calling perform move on a pointer to a move rather than 
on the actual move. This issue among a few others had to be solved before
I could begin improving the algorithm itself.

    Once I had the algorithm working the first thing I did to improve it was
to utilize as much of the time as possible. I modified the minimax algorithm
to run an iterative deepening search, and had each minval or maxval function
check if the time was low, if not, then the search would continue, otherwise,
the program would quickly hop out of its recursive minval/maxval call and
set the move to the best move found in the previous iteration of the iterative
deepening search. I chose this method as it seemed like the quickest way to
escape from the current minimax search, that way I could give the algorithm
as much time as possible to search as many branches as possible.

    The next improvement I attempted to implement was to change the board 
evaluation slightly, I noticed while watching a few games that the computer
would not exchange pieces even when it was ahead in material greatly, I came
up with the idea of simply doing a ratio for pieces against opponent's pieces.
This only made sense to me, as a player should prefer to have 5 against the 
opponent's 4 over 6 against the opponent's 5. I would later realize that there 
was a flaw here, as I observed this iteration be consistently defeated by the
original pieces-enemy pieces evaluation. The reason for this turned out to be
the fact that (redpieces/whitepieces)-1 was not equivalent to the opposite equation
when evaluating whitepieces. After removing this change and adding more improvements,
I would later bring this idea back. The new equation looks like so:
(whitesum-redsum)+(whitesum/redsum-1);
and 
(redsum-whitesum)-(whitesum/redsum-1);
This change made the program perform much better, forcing piece exchanges when ahead,
and avoiding them when behind, even sacrificing pieces for a king piece.

    Another improvement I made was to improve the board eval's speed, this change was
less noticable but any improvement in speed will help. I made use of pointers to iterate 
through the pieces on the board instead of the original board eval and printboard
statements which would iterate through all 64 squares. Now only 32 iterations of this loop
are done.

    I noticed that the player would, even when ahead in its evaluation, make repeated
moves with king pieces, initially I attempted to solve this by storing the previous two
moves and lowering the evaluation of repeated moves, but the program simply worked around
this by making moves that were not a repetition of two moves but a repetition of 3 or 4 
moves. Eventually i solved this by saving all of the moves that were equivalently evaluated
and choosing a random one to actually take. This, combined with the improved board evaluation
made it so that the end game would slowly go more and more in favor of my player, as it tried 
new moves and evaluated these situations instead of repeating the same few moves while the
opponent did the same.



Testing:
    While I was solving the initial issues with the algorithm, I tested the 
program to see if it would run on the non-graphical version. I used plenty
of fprintf statements to print out various values so that I could see
what certain values were in certain states, ensuring the board evaluation
was working correctly, seeing how the player thought it was doing on each move,
as well as ensuring new featues I was adding were working as intended.

    One way I made sure that my improvements were effective was to save an
older iteration of my program, mainly a simple depth search with no improvements,
that still played a formidable game. I played the newer iterations against this
and observed how well they performed against it. When I was impatient with both
of the programs' performance in certain situations, I even played against it 
myself.