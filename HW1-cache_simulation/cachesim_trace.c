//**********************************************************************
//                                                                     *
//               University Of North Carolina Charlotte                *
//                                                                     *
//Program: Cache Simulator                                             *
//Description: This program is used to read trace.din file including   *
//             memory access operations (data read and write,nstr read)*
//                                                                     *
//                                                                     *
//File Name: cachesim_trace.c                                          *
//File Version: 1.0                                                    *
//Baseline: Homework_1                                                 *
//                                                                     *
//Course: ECGR5181                                                     *
//                                                                     *
//cachesim_trace.c                                                     *
//Programmed by: Yu Liu                                                *     
//Under Suppervision of: Dr. Hamed Tabkhi                              *
//                                                                     *
//Format of input file: trace.din                                      *
//Data formate  :                                                      *
//operation type + address + \n , operation type are :                 *
//    0: data read                                                     *
//    1: data write                                                    *
//    2: instrunction fetch                                            *
//**********************************************************************                                                                     *
#include <stdio.h>
#include <math.h>
#include "stdbool.h"

//constant declarations 
#define CACHESIZE 32768 //must be 2^n, 65536 for 64K, 16384 for 16K
#define BLOCKSIZE 8 //must be 2^n, 32 for 32B, 128 for 128B
#define WAYLEN 1 //direct way is 1, 4 way is 4
#define RECORDTYPE 's' // u for unified, s for splitted.
#define FILENAME_INPUT  "trace.din"
#define FILENAME_OUTPUT "result_trace.txt"


#define MAXINDEX 4096 //do not change, up max cache size to 64K and min:
#define MAXWAY 8 //do not change, block size up to 8

//functions declarations:
void mainsimulator (char *op);
int getIndex(int  address);
int getBlockTag (int  address);
void updateLRU(int index, int way, bool hit,int getBlockTag);

//cache structure declaration:
typedef struct
{
int tag;
int LRUbit; //according to wayLen, eg. 0,1,2,3
} cache;
cache cacheLine[MAXINDEX][MAXWAY]; //declare a multi-dimensional array of cache line 

//global variables declarations:
int splitCacheSize;
// declare and initialize the couters
int readMiss=0; 
int writeMiss=0;
int instMiss=0;
int readHit=0; 
int writeHit=0;
int instHit=0;


//**********************************************************************
// Function Name: main()                                               *
// Description: - Call functions to:  mainsimulator                    *
//              - Run mainsimulator function to calculat under         *
//                unified(share) and splitted conditions               *
// Input file: trace.din                                               *
// Output file: result_trace.txt                                       *
// Return: int                                                         *
//**********************************************************************

int main()
{

// initialize the counters
switch(RECORDTYPE)	
	{
		char *op; // char array. read:0, write:1, instruction:op2 
		case 'u': // unified(shared) cache
		splitCacheSize=CACHESIZE;
		op="012"; // data and instruction unified mode
		mainsimulator(op);
		break;
		case 's': //split cash of 50% of instruction's and 50% of data's
		splitCacheSize=CACHESIZE/2; //eg. 16K data cache and 16K instruction cache 
		op="222";// for instruction type (operant is 2)
		mainsimulator(op);
		op="001";// for data type (operant is  0 or 1)
		mainsimulator(op);
		break;
	}	
	FILE *outputFile;//Open file to outpur result
	char *filename=FILENAME_OUTPUT; 
	outputFile=fopen(filename,"w");
	fprintf(outputFile,"Cache Size: %d\nBlock Size: %d\nWay: %d\nData Type: %c\nData File: %s\n\n",CACHESIZE,BLOCKSIZE,WAYLEN,RECORDTYPE,FILENAME_INPUT);
	fprintf(outputFile,"               Total         Instruction         Data(Read+Write)         Read         Write \n");
	fprintf(outputFile,"Misses      %8d            %8d                %8d      %8d      %8d\n",(readMiss+writeMiss+instMiss),instMiss,(readMiss+writeMiss),readMiss,writeMiss);
	fprintf(outputFile,"Hits        %8d            %8d                %8d      %8d      %8d\n",(readHit+writeHit+instHit),instHit,(readHit+writeHit),readHit,writeHit);
	fprintf(outputFile,"Total       %8d            %8d                %8d      %8d      %8d\n",(readHit+writeHit+instHit+readMiss+writeMiss+instMiss),(instMiss+instHit),(readMiss+readHit+writeMiss+writeHit),(readMiss+readHit),(writeMiss+writeHit));
	//output miss rate:
 	float missRateTotal=((float)readMiss+(float)writeMiss+(float)instMiss)/((float)readHit+(float)writeHit+(float)instHit+(float)readMiss+(float)writeMiss+(float)instMiss);
	float missRateInst = (float)instMiss/((float)instMiss+(float)instHit);
	float missRateData =((float)readMiss+(float)writeMiss)/((float)readMiss+readHit+(float)writeMiss+(float)writeHit);
	float missRateRead = (float)readMiss/((float)readMiss+(float)readHit);
	float missRateWrite = (float)writeMiss/((float)writeMiss+(float)writeHit);
	fprintf(outputFile,"Miss Rate     %.4f              %.4f                  %.4f        %.4f        %.4f\n",missRateTotal,missRateInst,missRateData,missRateRead,missRateWrite);

	fflush(outputFile);
	fclose(outputFile);
return 0;
} //end of main

//**********************************************************************
// Function Name: mainsimulator                                        *
// Description: Filter data type of 0: read, 1:write, 2: instruction.  * 
//              Simulate and calculate the miss and hit rat in  global *
//              variables of readMiss, writeMiss, instMiss, readHit,   *
//              writeHit, instHit.                                     *
// Input: char array                                                   *
// Return: none                                                        *
//**********************************************************************

void mainsimulator(char *op) 
{
// define main parameter
int indexNum = splitCacheSize/BLOCKSIZE; 
int indexBits = log2(indexNum);
int address; //Hexadecimal

//declare local variables
int opInt; //operant type in .din file, int format
int index;
int way;
bool hit;
int requiredTag;
int i,j;

//initialize the cache line 
for (i=0;i<= indexNum-1;i++)
{
 	for (j=0;j<=WAYLEN-1;j++)
 	{
 		//set the insert sequence for blank block is from way 0 to way_length eg, for 4 way: 0,1,2,3
 		cacheLine[i][j].LRUbit=j; 
 		cacheLine[i][j].tag= 0xffffffff;
 	}
}

FILE *inputFile;// input trace file for reading data
char *filename =FILENAME_INPUT;
inputFile=fopen(filename,"r");

// main fuction: calculate hits and misses
while(fscanf(inputFile,"%d%x",&opInt,&address)!= EOF)
{	
	char opChar=(char)(opInt+48); //change format of operant to char.
	if(op[0]==opChar || op[1]==opChar || op[2]==opChar)
 	{
		index= getIndex(address); //get index from one selected item in .din file	
		requiredTag=getBlockTag(address); //get block tag from one selected item in .din file
		hit= false;
	    for(way=0;way<=WAYLEN-1;way++)
		{
			if (cacheLine[index][way].tag==requiredTag) //if tag matches, increase hit counter.
			{
				switch (opInt)
				{
				case 0: readHit++; break;
				case 1: writeHit++; break;
				case 2: instHit++; break;
				}
				hit = true;
	    		updateLRU(index, way, hit, requiredTag);//update the LRU for this block
	    		way=WAYLEN-1;//match the tag, then exit the loop	
	    	}
	    }// end of for loop	    		
		
		if (hit==false)
	    	{
				//if miss, increase miss counter;write or replace with the new tag
				switch (opInt)
				{
				case 0: readMiss++; break;
				case 1: writeMiss++; break;
				case 2: instMiss++; break;
				}
				updateLRU(index, way, hit, requiredTag);
				}
	} //end of if
}//end of while loop

fflush(inputFile);
fclose(inputFile);
}//end of miansimulator

//**********************************************************************
// Function Name: getIndex                                             *
// Description: get index from the given data of address               *
// Input: integer                                                      *
// Return: integer                                                     *
//**********************************************************************
int getIndex(int address)
{
	int index;
	int offsetBits = log2(BLOCKSIZE);
	int indexBits = log2(splitCacheSize/BLOCKSIZE);
	// for AND operation. eg index bit IS 12, then index number "AND" with 0x0ffff
	int indexShiftBits = ~(0xffffffff<<indexBits);
	index= (address >> offsetBits)&indexShiftBits; //get index value
	return index;	
}

//**********************************************************************
// Function Name: getBlockTag                                          *
// Description: get block tag from the given data of address           *
// Input: integer                                                      *
// Return: integer                                                     *
//**********************************************************************
int getBlockTag(int address)
{
	int tag;
	int offsetBits = log2(BLOCKSIZE);
	int indexBits = log2(splitCacheSize/BLOCKSIZE);
	int tagShiftBits= ~(0xffffffff<<(32-offsetBits-indexBits)); 
	tag= (address >> (offsetBits+indexBits))& tagShiftBits;// get tag value
	return tag;	
}

//**********************************************************************
// Function Name: updateLRU                                            *
// Description: get index, associativity(way), hit status, tag, replace*
//              hit tag or replace tag and update the LRU status.      *
// Input: integer, bool                                                *
// Return: none                                                        *
//**********************************************************************
void updateLRU(int index, int way, bool hit,int requiredTag)
{
/* There are 3 scinarios:
1. hit and the hiited way is the most recently used already - no need update the LRU bit;
2. hit but the hitteed way is not the most recently use - update all the LRU bit;
3. not hit - replay the block tag and update the LRU bit. 
The least recently used LRU bit is 0. Eg.:  a cache of 4 ways associativity, they are:3,2,1,0
*/

int i;
	if (hit== true ) 
	{
		if (way== WAYLEN-1) return;//hit and is already the most recently used, no need update LRU
		else{
			//hit,  but is not the most recently used, to update the LRU
			for (i=0;i<=WAYLEN-1;i++)
			{
				// reduce those higher (more recnetly used than selected one) LRU bit, lower keep the same
				if (cacheLine[index][i].LRUbit>cacheLine[index][way].LRUbit) cacheLine[index][i].LRUbit--;
			}
			cacheLine[index][way].LRUbit=WAYLEN-1; // update it as the most recently used one
			return; 
		}//end of else
	}// end of hit==true
	else
	{
		// if missed: replace the clock tag, which the LRU is 0
		for (i=0;i<=WAYLEN-1;i++)
		{
			if (cacheLine[index][i].LRUbit==0) // find the least used way or blank way
			{
				cacheLine[index][i].tag=requiredTag; //replace or input with the new block tag
				cacheLine[index][i].LRUbit=WAYLEN-1; //set the LRU as the most recent used
			}else{
				cacheLine[index][i].LRUbit--; 
				}
		}//end of for
	}//end of else
}// end of function "updateLRU"
