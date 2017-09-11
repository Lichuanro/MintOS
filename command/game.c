#include "stdio.h"

#define N	15

void initGame(void);
void printChessboard(void);
void playChess(void);
int judge(int, int);

void ClearArr(char *arr, int length);
int mat_init(int *mat);
int mat_left(int *mat);
int mat_right(int *mat);
int mat_up(int *mat);
int mat_down(int *mat);
int mat_reach(int *mat);
int mat_insert(int *mat);
void mat_print(int *mat);
void GuessNumber();
void MaigcSquare();
int array(int n);

unsigned int _seed2 = 0xDEADBEEF;
int mtrx[100];
int chessboard[N + 1][N + 1] = { 0 };
int whoseTurn = 0;

int main()
{  
    printf("Please choose a game:\n");
    printf("1. MineSweeper\n");
    printf("2. 2048\n");
    printf("3. GuessNumber\n");
    printf("4. Maigc Square\n");
    printf("5. GoBang\n");

    char x[2];
    ClearArr(x, 2);
    int r = read(0, x, 2);
    int i = x[0]-49+1;

    switch(i)
    {
        case 1:
            MineSweeper();
            break;
        case 2:
            Game2048();
            break;
        case 3:
            GuessNumber();
            break;
        case 4:
            MagicSquare();
            break;
        case 5:
            GoBang();
            break;
        default:
            printf("Invaild number!\n");
    }
} 


void ClearArr(char *arr, int length)
{
    int i;
    for (i = 0; i < length; i++)
        arr[i] = 0;
}

void srand(unsigned int seed)
{
    _seed2 = seed;
}

int rand() {
    unsigned int next = _seed2;
    unsigned int result;

    next *= 1103515245;
    next += 12345;
    result = ( unsigned int  ) ( next / 65536 ) % 2048;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    next *= 1103515245;
    next += 12345;
    result <<= 10;
    result ^= ( unsigned int ) ( next / 65536 ) % 1024;

    _seed2 = next;

    return result;
}


int Game2048()
{
    int mat[16] = {0};
    int state = 0;
    char keys[128];
    printf("**************************************\n");
    printf("*               2048                 *\n");
    printf("**************************************\n");
    printf("*  Enter a/w/s/d to go             *\n");
    printf("*       left/up/down/right;            *\n");
    printf("*  Enter q to quit                    *\n");
    printf("**************************************\n");
    while(1)
    {
        mat_init(mat);

        while(1)
        {
            printf("type in the direction(w a s d):");
            ClearArr(keys, 128);
            int r = read(0, keys, 128);

            if (strcmp(keys, "a") == 0)
            {
                state = mat_left(mat);
            }
            else if (strcmp(keys, "s") == 0)
            {
                state=mat_down(mat);
            }
            else if (strcmp(keys, "w") == 0)
            {
                state=mat_up(mat);
            }
            else if (strcmp(keys, "d") == 0)
            {
                state=mat_right(mat);
            }
            else if (strcmp(keys, "q") == 0)
            {
                return 0;
            }
            else
            {
                printf("Input Invalid, Please retry\n");
                continue;
            }

            if(state==0){
                printf("can't add,try again!\n");
                continue;
            }
            if(mat_reach(mat)){
                printf("You Win\n");
                break;
            }
            if(!mat_insert(mat)){
                printf("You Lose\n");
                break;
            }
            mat_print(mat);
        }

        printf("another one?(y or n):");

        ClearArr(keys, 128);
        int r = read(0, keys, 128);
        if (strcmp(keys, "n"))
        {
            break;
        }
    }
    return 0;
}

int mat_init(int *mat)
{
    int i, j;
    mat_insert(mat);
    mat_insert(mat);
    mat_print(mat);
    return 0;
}

int mat_left(int *mat){
    printf("Left\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_right(int *mat){
    printf("Right\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=3,k=3,last=3;j>=0;j--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=j-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(j=3;j>=0;j--){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                j--;
            }
        }
    }
    return flag;
}

int mat_up(int *mat){
    printf("Up\n");

    int i,j;
    int flag=0;

    int k=0,temp[4]={0},last=0;
    for(i=0;i<4;i++){
        memset(temp,0,sizeof(int)*4);
        for(j=0,k=0,last=0;j<4;j++){
            if(mat[j*4+i]!=0){
                temp[k]=mat[j*4+i];
                mat[j*4+i]=0;
                last=j+1;
                k++;
            }
        }
        if(k<last) flag=1;
        for(j=0;j<3;j++){
            if(temp[j]>0&&temp[j]==temp[j+1]){
                temp[j]+=temp[j];
                temp[j+1]=0;
                flag=1;
            }
        }
        for(j=0,k=0;k<4;k++){
            if(temp[k]!=0){
                mat[j*4+i]=temp[k];
                j++;
            }
        }
    }
    return flag;
}

int mat_down(int *mat){
    printf("Down\n");

    int i,j;
    int flag=0;
    int k=0,temp[4]={0},last=0;
    for(j=0;j<4;j++){
        memset(temp,0,sizeof(int)*4);
        for(i=3,k=3,last=3;i>=0;i--){
            if(mat[i*4+j]!=0){
                temp[k]=mat[i*4+j];
                mat[i*4+j]=0;
                last=i-1;
                k--;
            }
        }
        if(k>last) flag=1;
        for(i=3;i>0;i--){
            if(temp[i]>0&&temp[i]==temp[i-1]){
                temp[i]+=temp[i];
                temp[i-1]=0;
                flag=1;
            }
        }
        for(i=3,k=3;k>=0;k--){
            if(temp[k]!=0){
                mat[i*4+j]=temp[k];
                i--;
            }
        }
    }
    return flag;
}

int mat_reach(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 2048)
                return 1;
        }
    }
    return 0;
}

int mat_insert(int *mat){
    char temp[16] = {0};
    int i, j, k = 0;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            if(mat[i*4+j] == 0){
                temp[k] = 4 * i + j;
                k++;
            }
        }
    }
    if(k == 0) return 0;
    k = temp[rand() % k];
    mat[((k-k%4)/4)*4+k%4]=2<<(rand()%2);
    return 1;
}

void mat_print(int *mat){
    int i, j;
    for(i = 0; i < 4; i++){
        for(j = 0; j < 4; j++){
            printf("%4d", mat[i*4+j]);
        }
        printf("\n");
    }
}

void GuessNumber()
{
    int a = rand() % 10;
    int times = 0;

    printf("This number is between 0 and 9 , please input your answer:\n");

    char x[2];
    ClearArr(x, 2);
    int r = read(0, x, 2);
    int i = x[0]-49+1;
    times ++;

    while(i != a)
    {
        if(i < a)
            printf("too small!\n");
        else
            printf("too big!\n");

        ClearArr(x, 2);
        int r = read(0, x, 2);
        i = x[0] - 49 + 1;

        times ++;
    }

    if(times <= 2)
        printf("You are really awesome!!!");
    else
        printf("You Win, aha , just so so");
    
    return;
}

void MagicSquare()
{
    int n;
    printf("please input n:\n");

    char x[2];
    ClearArr(x, 2);
    int r = read(0, x, 2);
    int i = x[0]-49+1;

    array(i);  /*调用array函数*/
    return ;
}

int array(int n)
{
    int i, j, no, num, max;
    
    if(n%2 == 0)  /*n是偶数，则加1使其变为奇数*/
    {
        n=n+1;
    }
    max=n*n;
    
    mtrx[n/2]=1;  /* 将1存入数组*/
    i=0;  /*自然数1所在行*/
    j=n/2;  /*自然数1所在列*/
    /*从2开始确定每个数的存放位置*/
    for(num=2; num<=max; num++)
    {
        i=i-1;
        j=j+1;
        if((num-1)%n == 0)  /*当前数是n的倍数*/
        {
            i=i+2;
            j=j-1;
        }
        if(i<0)  /*当前数在第0行*/
        {
            i=n-1;
        }
        if(j>n-1)  /*当前数在最后一列，即n-1列*/
        {
            j=0;
        }
        no=i*n+j;  /*找到当前数在数组中的存放位置*/
        mtrx[no]=num;
    }
    /*打印生成的魔方阵*/
    printf("%d-Magic Square is:",n);
    no=0;
    for(i=0; i<n; i++)
    {
        printf("\n");
        for(j=0; j<n; j++)
        {
            printf("%3d", mtrx[no]);
            no++;
        }
    }
    printf("\n");
    return 0;
}

int GoBang(void)
{
	initGame();

	while (1)
	{
		whoseTurn++;
		playChess();
	}

	return 0;
}

void initGame(void)
{
	char c;
	printChessboard();
}

void printChessboard(void)
{
	int i, j;

	for (i = 0; i <= N; i++)
	{
		for (j = 0; j <= N; j++)
		{
			if (0 == i)
				printf("%3d", j);
			else if (j == 0)
				printf("%3d", i);
			else if (1 == chessboard[i][j])
				printf("  @");
			else if (2 == chessboard[i][j])
				printf("  #");
			else
				printf("   ");
		}
		printf("\n");
	}
}

void playChess(void)
{
	int i, j, winner;

	if (1 == whoseTurn % 2)
	{
		printf("Player 1's turn:\n");

		char x[3];
		ClearArr(x, 3);
		int r = read(0, x, 4);
		int i = x[0]-49+1;
        int j = x[2] - 49 + 1;
        
        if(x[0] == 'q')
            return;

		chessboard[i][j] = 1;
	}
	else
	{
		printf("Player 2's turn: \n");

		char x[3];
		ClearArr(x, 3);
		int r = read(0, x, 4);
		int i = x[0]-49+1;
        int j = x[2] - 49 + 1;
        
        if(x[0] == 'q')
            return;

		chessboard[i][j] = 2;
	}

	printChessboard();
	
	if (judge(i, j))
	{
		if (1 == whoseTurn % 2)
			printf("Player wins！\n");
		else
			printf("Player wins！\n");
	}
}

int judge(int x, int y)
{
	int i, j;
	int t = 2 - whoseTurn % 2;

	for (i = x - 4, j = y; i <= x; i++)
	{
		if (i >= 1 && i <= N - 4 && t == chessboard[i][j] && t == chessboard[i + 1][j] && t == chessboard[i + 2][j] && t == chessboard[i + 3][j] && t == chessboard[i + 4][j])
			return 1;
	}
	for (i = x, j = y - 4; j <= y; j++)
	{
		if (j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i][j + 1] && t == chessboard[i][j + 1] && t == chessboard[i][j + 3] && t == chessboard[i][j + 4])
			return 1;
	}
	for (i = x - 4, j = y - 4; i <= x, j <= y; i++, j++)
	{
		if (i >= 1 && i <= N - 4 && j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i + 1][j + 1] && t == chessboard[i + 2][j + 2] && t == chessboard[i + 3][j + 3] && t == chessboard[i + 4][j + 4])
			return 1;
	}
	for (i = x + 4, j = y - 4; i >= 1, j <= y; i--, j++)
	{
		if (i >= 1 && i <= N - 4 && j >= 1 && j <= N - 4 && t == chessboard[i][j] && t == chessboard[i - 1][j + 1] && t == chessboard[i - 2][j + 2] && t == chessboard[i - 3][j + 3] && t == chessboard[i - 4][j + 4])
			return 1;
	}

	return 0;
}

int MineSweeper()
{
    char ui[8][8];

    for(int i = 0 ; i < 8 ; i++)
        for(int j = 0 ; j < 8 ; j++)
            ui[i][j] = '+';

    int map[8][8]=
        {  
            0,0,0,0,0,0,0,0,  
            0,0,1,0,0,1,0,0,  
            0,0,0,0,1,0,0,0,  
            0,0,0,0,0,1,0,0,  
            0,0,1,0,0,0,0,0,  
            0,0,1,0,0,0,0,0,  
            0,1,0,1,1,0,0,0,  
            1,0,0,0,0,0,0,0  
        };  

    int p[8][2]={{-1,-1} ,{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};  
    int i=0,j=0;   
    int h=0,l=0; 
    int h1=0,l1=0;  
    int n=0;
    int win=0;  
    char x[2]; char y[2];

    printf("**************************************\n");
    printf("*              MineSweeper           *\n");
    printf("**************************************\n");
    printf("*  Enter 1-8 twice to locate the mine*\n");
    printf("*  Enter q to quit                   *\n");
    printf("**************************************\n\n");
    while(1)  
    {  
        for(i=0;i<8;i++)  
        {  
            for(j=0;j<8;j++)  
            {  
                printf("%c ",ui[i][j]);  
            }  
            printf("\n");  
        }  

        printf("please enter the row number:");  
        ClearArr(x, 2);
        int r = read(0, x, 2);
        h = x[0]-49+1;
        if(x[0] == 'q') 
            break;
        while(!(h >=0 && h <= 7))
        {
            printf("Wrong input!\nplease enter the row number:");
            r = read(0, x, 2);h = x[0]-49+1;
        }

        printf("please enter the col number:");
        ClearArr(x, 2);
        int r1 = read(0, y, 2);
        l = y[0]-49+1;
        if(y[0] == 'q') 
            break;

        while(!(l >=0 && l <= 7))
        {
            printf("Invaild number! Please input again:\n");
            r1 = read(0, y, 2);l = y[0]-49+1;
        }
        
        if(map[h-1][l-1]==1)  
        {  
            printf("You Die!\n");  
            break;  
        }  
        h=h-1;  
        l=l-1;   

        while(i<8)  
        {  
            n=0;  
            h1=h;  
            l1=l;   
            h1= h1+p[i][0];  
            l1=l1+p[i][1];  
            if(h1>=0&&h1<8&&l1>=0&&l1<8)  
                {  
                    if(map[h1][l1]==1)  
                        {  
                            n++;  
                        }    
                }  

            i++;  
        }  

        switch(n)  
        {  
            case 0:  
                ui[h][l]='0';  
                    break;  
            case 1:  
                ui[h][l]='1';  
                    break;  
            case 2:  
                ui[h][l]='2';  
                    break;  
            case 3:  
                ui[h][l]='3';  
                    break;  
            case 4:  
                ui[h][l]='4';  
                    break;  
            case 5:  
                ui[h][l]='5';  
                    break;  
            case 6:  
                ui[h][l]='6';  
                    break;  
            case 7:  
                ui[h][l]='7';  
                    break;  
            case 8:  
                ui[h][l]='8';  
                    break;  
        }  
        win++;  

        if(win==54)  
        {  
            printf("You win!\n");  
                break;  
        }   
    }  
    return 0;  
}