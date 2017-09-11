/*
 * Question from my facebook production engineer interview.
 *
 * Write a fuction "FindBattleship(int N)", where "N" is the size of a matrix.
 * This function should find the position of a battlehsip on a N x N matrix.
 * It will use another function, "HitBattleship(int pos1, int pos2)". We do not need to write this fucntion (but I did, for the completeness of the code).
 * It returns a boolean value if the position (pos1, pos2) holds a battleship.
 * The battleship must be of size 3, vertical or horizontal, never on diagonal.
 *
 * EX: if we have a battleship at ((4,2),(4,3),(4,4)) on an matrix of size 10

 *   . . . . . . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
     . . x x x . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
     . . . . . . . . . .
 *
 */



#include <stdio.h>
#include <math.h>

int HitBattleship(int row, int col)
{
  int A[6] = {10,6,11,6,12,6};
  int it, flag = 0;

  for(it = 0; it < 6; it = it + 2) {
    if(row == A[it] && col == A[it+1]) {
      flag = 1;
      break;
    }
  }
  return flag;
}

void FindBattleshipRecursive(int inf_row, int sup_row, int inf_col, int sup_col, int factor, int * hits)
{
    if(inf_row == sup_row && inf_col == sup_col) {
      if(HitBattleship(inf_row, inf_col)) {
        printf("(%d,%d) ", inf_row, inf_col);
        (*hits)++;
      }
      return;
    }

    if (*hits != 3) {
      FindBattleshipRecursive(inf_row, sup_row - factor/2, inf_col, sup_col - factor/2, factor/2, hits);                // Left superior submatrix
      FindBattleshipRecursive(inf_row, sup_row - factor/2, (sup_col - factor/2)+1, sup_col, factor/2, hits);            // Right superior submatrix
      FindBattleshipRecursive((sup_row - factor/2)+1, sup_row, inf_col, sup_col - factor/2, factor/2, hits);            // Left inferior submatrix
      FindBattleshipRecursive((sup_row - factor/2)+1, sup_row, (sup_col - factor/2)+1, sup_col, factor/2, hits);        // Right inferior submatrix
    }
}

void FindBattleship(int size)
{
  int hits = 0;
  int adjust = pow(ceil(sqrt(size)),2);

  FindBattleshipRecursive(0, adjust-1, 0, adjust-1, adjust, &hits);
}

int main()
{
  int size_matrix = 14;
  FindBattleship(size_matrix);
  printf("\n");

  return 0;
}
