//**********************************************************************
//                                                                     *
//               University Of North Carolina Charlotte                *
//                                                                     *
//Program: Cach performance tester                                     *
//Description: This program is used row-major and column-major to      *
//             evaluate the loop structure impact for cach performance *
//                                                                     *
//                                                                     *
//File Name: loop_yu.c                                                 *
//File Version: 1.0                                                    *
//Baseline: Homework_2                                                 *
//                                                                     *
//Course: ECGR5181                                                     *
//                                                                     *
//loop_yu.c                                                            *
//Programmed by: Yu Liu                                                * 
//Under Suppervision of: Dr. Hamed Tabkhi                              *
//                                                                     *
//Change the paramer in the constant defination area                   *
//    ROW: set the number of row                                       *
//    COL: set the number of column                                    *
//    TYPE: 'c' or 'r': c for column,  r for row                       *
//**********************************************************************  
#include <stdio.h>

//constant defination
#define ROW 10000
#define COL 10000
#define TYPE 'b' //c for column major,  r for row major,  b for both

//declare functions
void runColumnMajor(int array[ROW][COL]);
void runRowMajor(int array[ROW][COL]);

//**********************************************************************
// Function Name: main()                                               *
// Description: - initialize the array                                 *
//              - call row major or column major function              *
// Input file: none                                                    *
// Output file: none                                                   *
// Return: none                                                        *
//**********************************************************************
int main()
{
	int i, j;
	static int array[ROW][COL];
	//initial array valun
	for (i=0;i<ROW; i++)
	{
		for (j=0;j<COL; j++) array[i][j]= i+j;		
	}

	//run the test
	switch(TYPE)
	{
		case 'c' : runColumnMajor(array); break;
		case 'r' : runRowMajor(array);break;
		case 'b':{
			runColumnMajor(array);
			runRowMajor(array);
			break;
		}
	}// end of switch
	return 0;
}


//**********************************************************************
// Function Name: runRowMajor                                          *
// Description: run in way of row major                                *
// Input: array                                                        *
// Return: none                                                        *
//**********************************************************************
void runRowMajor(int array[ROW][COL])
{
	int i, j;
	for (i=0;i<ROW; i++)
	{
		for (j=0;j<COL; j++)
		{
			array[i][j]= 2*array[i][j];	
			//printf("array[%d][%d]=%d    ",i,j,array[i][j]);		
		}
		//printf("\n");
	}
	//printf("\n");
}


//**********************************************************************
// Function Name: runColumnMajor                                       *
// Description: run in way of column major                             *
// Input: array                                                        *
// Return: none                                                        *
//**********************************************************************
void runColumnMajor(int array[ROW][COL])
{
	int i, j;
	for (j=0;j<ROW; j++)
	{
		for (i=0;i<COL; i++)
		{
			array[i][j]= 2*array[i][j];	
			//printf("array[%d][%d]=%d    ",i,j,array[i][j]);		
		}
		//printf("\n");
	}
	//printf("\n");
}
