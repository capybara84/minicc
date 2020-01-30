int printf(char *, ...);

int print_board(int board[][10]) {
  int i, j;
  for (i = 0; i < 10; i++) {
    for (j = 0; j < 10; j++)
      if (board[i][j])
        printf("Q ");
      else
        printf(". ");
    printf("\n");
  }
  printf("\n\n");
}

int conflict(int board[][10], int row, int col) {
  int i,j;
  for (i = 0; i < row; i++) {
    if (board[i][col])
      return 1;
    j = row - i;
    if (0 < col - j + 1 && board[i][col - j])
      return 1;
    if (col + j < 10 && board[i][col + j])
      return 1;
  }
  return 0;
}

int solve(int board[][10], int row) {
  int i;
  if (row > 9) {
    print_board(board);
    return 0;
  }
  for (i = 0; i < 10; i++) {
    if (conflict(board, row, i)) {
    } else {
      board[row][i] = 1;
      solve(board, row + 1);
      board[row][i] = 0;
    }
  }
}

int main() {
  int board[100];
  int i;
  for (i = 0; i < 100; i++)
    board[i] = 0;
  solve(board, 0);
  return 0;
}
