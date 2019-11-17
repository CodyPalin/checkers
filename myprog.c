#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/times.h>
#include <time.h>
#include "myprog.h"
 
#ifndef CLK_TCK
#define CLK_TCK CLOCKS_PER_SEC
#endif

float SecPerMove;
char board[8][8];
char bestmove[12];
int me,cutoff,endgame;
long NumNodes;
int MaxDepth;

/*** For timing ***/
clock_t start;
struct tms bff;

/*** For the jump list ***/
int jumpptr = 0;
int jumplist[48][12];

/*** For the move list ***/
int numLegalMoves = 0;
int movelist[48][12];

/* Print the amount of time passed since my turn began */
void PrintTime(void)
{
    clock_t current;
    float total;

    current = times(&bff);
    total = (float) ((float)current-(float)start)/CLK_TCK;
    fprintf(stderr, "Time = %f\n", total);
}

/* Determine if I'm low on time */
int LowOnTime(void) 
{
    clock_t current;
    float total;

    current = times(&bff);
    total = (float) ((float)current-(float)start);
    //fprintf(stderr, "time elapsed:%f",total);
    if(total >= (SecPerMove-0.1)) return 1; else return 0;
}

/* Copy a square state */
void CopyState(char *dest, char src)
{
    char state;
    
    *dest &= Clear;
    state = src & 0xE0;
    *dest |= state;
}

/* Reset board to initial configuration */
void ResetBoard(void)
{
        int x,y;
    char pos;

        pos = 0;
        for(y=0; y<8; y++)
        for(x=0; x<8; x++)
        {
                if(x%2 != y%2) {
                        board[y][x] = pos;
                        if(y<3 || y>4) board[y][x] |= Piece; else board[y][x] |= Empty;
                        if(y<3) board[y][x] |= Red; 
                        if(y>4) board[y][x] |= White;
                        pos++;
                } else board[y][x] = 0;
        }
    endgame = 0;
}

/* Add a move to the legal move list */
void AddMove(char move[12])
{
    int i;

    for(i=0; i<12; i++) movelist[numLegalMoves][i] = move[i];
    numLegalMoves++;
}

/* Finds legal non-jump moves for the King at position x,y */
void FindKingMoves(char board[8][8], int x, int y) 
{
    int i,j,x1,y1;
    char move[12];

    memset(move,0,12*sizeof(char));

    /* Check the four adjacent squares */
    for(j=-1; j<2; j+=2)
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        /* Make sure we're not off the edge of the board */
        if(y1<0 || y1>7 || x1<0 || x1>7) continue; 
        if(empty(board[y1][x1])) {  /* The square is empty, so we can move there */
            move[0] = number(board[y][x])+1;
            move[1] = number(board[y1][x1])+1;    
            AddMove(move);
        }
    }
}

/* Finds legal non-jump moves for the Piece at position x,y */
void FindMoves(int player, char board[8][8], int x, int y) 
{
    int i,j,x1,y1;
    char move[12];

    memset(move,0,12*sizeof(char));

    /* Check the two adjacent squares in the forward direction */
    if(player == 1) j = 1; else j = -1;
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        /* Make sure we're not off the edge of the board */
        if(y1<0 || y1>7 || x1<0 || x1>7) continue; 
        if(empty(board[y1][x1])) {  /* The square is empty, so we can move there */
            move[0] = number(board[y][x])+1;
            move[1] = number(board[y1][x1])+1;    
            AddMove(move);
        }
    }
}

/* Adds a jump sequence the the legal jump list */
void AddJump(char move[12])
{
    int i;
    
    for(i=0; i<12; i++) jumplist[jumpptr][i] = move[i];
    jumpptr++;
}

/* Finds legal jump sequences for the King at position x,y */
int FindKingJump(int player, char board[8][8], char move[12], int len, int x, int y) 
{
    int i,j,x1,y1,x2,y2,FoundJump = 0;
    char one,two,mymove[12],myboard[8][8];

    memcpy(mymove,move,12*sizeof(char));

    /* Check the four adjacent squares */
    for(j=-1; j<2; j+=2)
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        y2 = y+2*j; x2 = x+2*i;
        /* Make sure we're not off the edge of the board */
        if(y2<0 || y2>7 || x2<0 || x2>7) continue; 
        one = board[y1][x1];
        two = board[y2][x2];
        /* If there's an enemy piece adjacent, and an empty square after hum, we can jump */
        if(!empty(one) && color(one) != player && empty(two)) {
            /* Update the state of the board, and recurse */
            memcpy(myboard,board,64*sizeof(char));
            myboard[y][x] &= Clear;
            myboard[y1][x1] &= Clear;
            mymove[len] = number(board[y2][x2])+1;
            FoundJump = FindKingJump(player,myboard,mymove,len+1,x+2*i,y+2*j);
            if(!FoundJump) {
                FoundJump = 1;
                AddJump(mymove);
            }
        }
    }
    return FoundJump;
}

/* Finds legal jump sequences for the Piece at position x,y */
int FindJump(int player, char board[8][8], char move[12], int len, int x, int y) 
{
    int i,j,x1,y1,x2,y2,FoundJump = 0;
    char one,two,mymove[12],myboard[8][8];

    memcpy(mymove,move,12*sizeof(char));

    /* Check the two adjacent squares in the forward direction */
    if(player == 1) j = 1; else j = -1;
    for(i=-1; i<2; i+=2)
    {
        y1 = y+j; x1 = x+i;
        y2 = y+2*j; x2 = x+2*i;
        /* Make sure we're not off the edge of the board */
        if(y2<0 || y2>7 || x2<0 || x2>7) continue; 
        one = board[y1][x1];
        two = board[y2][x2];
        /* If there's an enemy piece adjacent, and an empty square after hum, we can jump */
        if(!empty(one) && color(one) != player && empty(two)) {
            /* Update the state of the board, and recurse */
            memcpy(myboard,board,64*sizeof(char));
            myboard[y][x] &= Clear;
            myboard[y1][x1] &= Clear;
            mymove[len] = number(board[y2][x2])+1;
            FoundJump = FindJump(player,myboard,mymove,len+1,x+2*i,y+2*j);
            if(!FoundJump) {
                FoundJump = 1;
                AddJump(mymove);
            }
        }
    }
    return FoundJump;
}

/* Determines all of the legal moves possible for a given state */
int FindLegalMoves(struct State *state)
{
    int x,y;
    char move[12], board[8][8];

    memset(move,0,12*sizeof(char));
    jumpptr = numLegalMoves = 0;
    memcpy(board,state->board,64*sizeof(char));

    /* Loop through the board array, determining legal moves/jumps for each piece */
    for(y=0; y<8; y++)
    for(x=0; x<8; x++)
    {
        if(x%2 != y%2 && color(board[y][x]) == state->player && !empty(board[y][x])) {
            if(king(board[y][x])) { /* King */
                move[0] = number(board[y][x])+1;
                FindKingJump(state->player,board,move,1,x,y);
                if(!jumpptr) FindKingMoves(board,x,y);
            } 
            else if(piece(board[y][x])) { /* Piece */
                move[0] = number(board[y][x])+1;
                FindJump(state->player,board,move,1,x,y);
                if(!jumpptr) FindMoves(state->player,board,x,y);    
            }
        }    
    }
    if(jumpptr) {
        for(x=0; x<jumpptr; x++) 
        for(y=0; y<12; y++) 
        state->movelist[x][y] = jumplist[x][y];
        state->numLegalMoves = jumpptr;
    } 
    else {
        for(x=0; x<numLegalMoves; x++) 
        for(y=0; y<12; y++) 
        state->movelist[x][y] = movelist[x][y];
        state->numLegalMoves = numLegalMoves;
    }
    return (jumpptr+numLegalMoves);
}
int MinVal(struct State *state, double alpha, double beta, int depth)
{
    if(LowOnTime())
    {
        fprintf(stderr,"interrupting...\n");
        return -__INT_MAX__;
    }
        
    int x;
    depth --;
    if(depth <=0)
    {
        double rval = evalBoard(state->board);
        //fprintf(stderr, "In MinVal, depth = 0, evaluation = %f\n", rval);
        return rval;
    }
    //fprintf(stderr, "In MinVal, depth = %i, alpha = %f, beta = %f\n", depth, alpha, beta);
    FindLegalMoves(state);
    //fprintf(stderr,"number of legal moves: %d\n",state->numLegalMoves);
    for(x=0;x<state->numLegalMoves;x++)
    {
        struct State foobar;
        double rval;
        memcpy(&foobar, state, sizeof(foobar));
        if(state->player==1) foobar.player = 2;
        else foobar.player = 1;
        PerformMove(foobar.board, foobar.movelist[x], MoveLength(foobar.movelist[x]));
        rval = MaxVal(&foobar, alpha, beta, depth);
        if(rval == -__INT_MAX__)
            return -__INT_MAX__;
        if(rval < beta)
        {
            beta = rval;
        }
        if(beta <= alpha) return alpha;
    }
    return beta;
}
int MaxVal(struct State *state, double alpha, double beta, int depth)
{
    if(LowOnTime())
        {
            fprintf(stderr,"interrupting...\n");
            return -__INT_MAX__;
        }
    int x;
    depth --;
    if(depth <=0)
    {
        double rval = evalBoard(state->board);
        //fprintf(stderr, "In MinVal, depth = %i, alpha = %f, beta = %f\n", depth, alpha, beta);
        return rval;
    }
    //fprintf(stderr, "In MaxVal, depth = %i\n", depth);
    FindLegalMoves(state);
    //fprintf(stderr,"number of legal moves: %d\n",(*state).numLegalMoves);
    for(x=0;x<state->numLegalMoves;x++)
    {
        struct State foobar;
        double rval;
        memcpy(&foobar, state, sizeof(foobar));
        if(state->player==1) foobar.player = 2;
        else foobar.player = 1;
        PerformMove(foobar.board, foobar.movelist[x], MoveLength(foobar.movelist[x]));
        rval = MinVal(&foobar, alpha, beta, depth);
        if(rval == -__INT_MAX__)
            return -__INT_MAX__;
        if(rval > alpha)
        {
            alpha = rval;
        }
        if(alpha >= beta) return beta;
    }
    return alpha;
}
/* Employ your favorite search to find the best move here.  */
/* This example code shows you how to call the FindLegalMoves function */
/* and the PerformMove function */
void FindBestMove(int player)
{
    int chosenMove; 
    struct State state; 

    /* Set up the current state */
    state.player = player;
    memcpy(state.board,board,64*sizeof(char));
    memset(bestmove,0,12*sizeof(char));


    /* Find the legal moves for the current state */
    FindLegalMoves(&state);
    //fprintf(stderr,"number of legal moves: %d\n",state.numLegalMoves);


    chosenMove = rand()%state.numLegalMoves;
    int depth = 0;
    while(depth < MaxDepth)
    {
        int bestdepthmove = chosenMove;
        double alpha = -10000;
        double beta = -alpha;
        double bestScore = alpha;
        depth++;
        int ExploredMove;
        for(ExploredMove=0;ExploredMove<state.numLegalMoves; ExploredMove++)
        {
            //fprintf(stderr, "exploring move: %d\n", ExploredMove);
            struct State foobar;
            //double rval;
            memcpy(&foobar, &state, sizeof(foobar));
            if(state.player ==1) foobar.player = 2;
            else foobar.player = 1;
            //fprintf(stderr, "performing move\n");
            //PrintBoard(&foobar);
            PerformMove(foobar.board, foobar.movelist[ExploredMove], MoveLength(foobar.movelist[ExploredMove]));
            //fprintf(stderr, "move performed\n");
            //PrintBoard(&foobar);
            int val = MinVal(&foobar, alpha, beta, depth);
            if(val == -__INT_MAX__)
            {
                fprintf(stderr, "interrupting search at depth: %d\n",depth);
                goto MakeMove;
            }
            if(val > bestScore)
            {
                bestScore = val;
                bestdepthmove = ExploredMove;
            }
        }
        chosenMove = bestdepthmove;
        char text[16];
        MoveToText(state.movelist[chosenMove],text);
        fprintf(stderr, "found best move (%s) at depth %d with value of%f\n",text, depth, bestScore);
    }
    MakeMove:
    memcpy(bestmove,state.movelist[chosenMove],MoveLength(state.movelist[chosenMove]));
    //fprintf(stderr, "eval: %f , me: %d \n", evalBoard(state->board), me);
}
/*
[0][1]	[0][3]	[0][5]	[0][7]	
[1][0]	[1][2]	[1][4]	[1][6]	
[2][1]	[2][3]	[2][5]	[2][7]	
[3][0]	[3][2]	[3][4]	[3][6]	
[4][1]	[4][3]	[4][5]	[4][7]	
[5][0]	[5][2]	[5][4]	[5][6]	
[6][1]	[6][3]	[6][5]	[6][7]	
[7][0]	[7][2]	[7][4]	[7][6]
*/

double evalBoard(char board[8][8])
{   
    int y,x;
    double whitesum=0.0;
    double redsum=0.0;

    for(y=0; y<8; y++) {
        for(x=0; x<8; x++) if(x%2 != y%2)
        {   
            //fprintf(stderr, "[%d][%d]\t",y,x);
            if(king(board[y][x]))
            {   
                //if(color(currBoard->board[y][x]) == White) score += 2.0;
                if(board[y][x] & White) whitesum += 2.0;
                else redsum += 2.0;
            }
            else if(piece(board[y][x]))
            {   
                if(board[y][x] & White) whitesum += 1.0;
                else redsum += 1.0;
            }
        }
        //fprintf(stderr, "\n");
    }
    //PrintBoard(currBoard);
    double eval;
    if(me==2) 
        eval = whitesum-redsum;
    else 
        eval = redsum-whitesum;
    //fprintf(stderr, "eval: %f\n", eval);
    return eval;
}
void PrintBoard(struct State *currBoard)
{
    int y,x;
    for(y=0; y<8; y++)
    {
        for(x=0; x<8; x++)
        {
             if(x%2 != y%2)
             {
                 if(king(currBoard->board[y][x]))
                 {
                     if(currBoard->board[y][x] & White) fprintf(stderr,"B");
                     else fprintf(stderr,"A");
                 }
                 else if(piece(currBoard->board[y][x]))
                 {
                     if(currBoard->board[y][x] & White) fprintf(stderr,"b");
                     else fprintf(stderr,"a");
                 }
                 else fprintf(stderr," ");
             }
             else fprintf(stderr," ");
        }
        fprintf(stderr,"\n");
    }
}
/* Converts a square label to it's x,y position */
void NumberToXY(char num, int *x, int *y)
{
    int i=0,newy,newx;

    for(newy=0; newy<8; newy++)
    for(newx=0; newx<8; newx++)
    {
        if(newx%2 != newy%2) {
            i++;
            if(i==(int) num) {
                *x = newx;
                *y = newy;
                return;
            }
        }
    }
    *x = 0; 
    *y = 0;
}

/* Returns the length of a move */
int MoveLength(char move[12])
{
    int i;

    i = 0;
    while(i<12 && move[i]) i++;
    return i;
}    

/* Converts the text version of a move to its integer array version */
int TextToMove(char *mtext, char move[12])
{
    int i=0,len=0,last;
    char val,num[64];

    while(mtext[i] != '\0') {
        last = i;
        while(mtext[i] != '\0' && mtext[i] != '-') i++;
        strncpy(num,&mtext[last],i-last);
        num[i-last] = '\0';
        val = (char) atoi(num);
        if(val <= 0 || val > 32) return 0;
        move[len] = val;
        len++;
        if(mtext[i] != '\0') i++;
    }
    if(len<2 || len>12) return 0; else return len;
}

/* Converts the integer array version of a move to its text version */
void MoveToText(char move[12], char *mtext)
{
    int i;
    char temp[8];

    mtext[0] = '\0';
    for(i=0; i<12; i++) {
        if(move[i]) {
            sprintf(temp,"%d",(int)move[i]);
            strcat(mtext,temp);
            strcat(mtext,"-");
        }
    }
    mtext[strlen(mtext)-1] = '\0';
}

/* Performs a move on the board, updating the state of the board */
void PerformMove(char board[8][8], char move[12], int mlen)
{
    //char text[16];
    //MoveToText(move,text);
    //fprintf(stderr, "performing move: %s\n",text);
    int i,j,x,y,x1,y1,x2,y2;

    NumberToXY(move[0],&x,&y);
    NumberToXY(move[mlen-1],&x1,&y1);
    CopyState(&board[y1][x1],board[y][x]);
    if(y1 == 0 || y1 == 7) board[y1][x1] |= King;
    board[y][x] &= Clear;
    NumberToXY(move[1],&x2,&y2);
    if(abs(x2-x) == 2) {
        for(i=0,j=1; j<mlen; i++,j++) {
            if(move[i] > move[j]) {
                y1 = -1; 
                if((move[i]-move[j]) == 9) x1 = -1; else x1 = 1;
            }
            else {
                y1 = 1;
                if((move[j]-move[i]) == 7) x1 = -1; else x1 = 1;
            }
            NumberToXY(move[i],&x,&y);
            board[y+y1][x+x1] &= Clear;
        }
    }
}
char testboard[8][8] = {};
int old_main(int argc, char *argv[])
{

    ResetBoard();
    struct State teststate;
    teststate.player = 1;
    memcpy(teststate.board,testboard,64*sizeof(char));
    PrintBoard(&teststate);
    fprintf(stderr,"evaluation of test board: %f\n",evalBoard(testboard));
    return 0;
}
int main(int argc, char *argv[])
{
    char buf[1028],move[12];
    int len,mlen,player1;

    /* Convert command line parameters */
    SecPerMove = (float) atof(argv[1]); /* Time allotted for each move */
    fprintf(stderr, "secpermove: %f\n", SecPerMove);
    //MaxDepth = (argc == 4) ? atoi(argv[3]) : -1;
    MaxDepth = 20;

fprintf(stderr, "%s SecPerMove == %lg\n", argv[0], SecPerMove);

    /* Determine if I am player 1 (red) or player 2 (white) */
    //fgets(buf, sizeof(buf), stdin);
    len=read(STDIN_FILENO,buf,1028);
    buf[len]='\0';
    if(!strncmp(buf,"Player1", strlen("Player1"))) 
    {
        fprintf(stderr, "I'm Player 1\n");
        player1 = 1; 
    }
    else 
    {
        fprintf(stderr, "I'm Player 2\n");
        player1 = 0;
    }
    if(player1) me = 1; else me = 2;

    /* Set up the board */ 
    ResetBoard();
    srand((unsigned int)time(0));

    if (player1) {
        start = times(&bff);
        goto determine_next_move;
    }

    for(;;) {
        /* Read the other player's move from the pipe */
        //fgets(buf, sizeof(buf), stdin);
        len=read(STDIN_FILENO,buf,1028);
        buf[len]='\0';
        start = times(&bff);
        memset(move,0,12*sizeof(char));

        /* Update the board to reflect opponents move */
        mlen = TextToMove(buf,move);
        PerformMove(board,move,mlen);
        
determine_next_move:
        /* Find my move, update board, and write move to pipe */
        if(player1) FindBestMove(1); else FindBestMove(2);
        if(bestmove[0] != 0) { /* There is a legal move */
            mlen = MoveLength(bestmove);
            PerformMove(board,bestmove,mlen);
            MoveToText(bestmove,buf);
        }
        else exit(1); /* No legal moves available, so I have lost */

        /* Write the move to the pipe */
        //printf("%s", buf);
        write(STDOUT_FILENO,buf,strlen(buf));
        fflush(stdout);
    }

    return 0;
}


