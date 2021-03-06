//**********************************************************************
//                                                                     *
//               University Of North Carolina Charlotte                *
//                                                                     *
//Program: Cache Simulator                                             *
//Description: This program is used to read .out file generated by pin *
//             tool, which is in 64 bits                               *
//                                                                     *
//                                                                     *
//File Name: cachesim_pin.c                                            *
//File Version: 1.0                                                    *
//Baseline: Homework_1                                                 *
//                                                                     *
//Course: ECGR5181                                                     *
//                                                                     *
//cachesim_pin.c                                                       *
//Programmed by: Yu Liu                                                * 
//Under Suppervision of: Dr. Hamed Tabkhi                              *
//                                                                     *
//Input file: pinatrace_linpack.out or pinatrace_dhrystone.out         *
//data in 64 bits, formate:                                            *
//address + operation type + address + \n , operation type are :       *
//    R: data read                                                     *
//    W: data write                                                    *
//    I: instrunction fetch                                            *
//**********************************************************************   
#include <stdio.h>
#include <math.h>
#include "stdbool.h"

//constant declarations  
#define CACHESIZE_INST 0 //must be 2^n , eg. 0 for 0K, 16384 for 16K
#define CACHESIZE_DATA 65536 //must be 2^n, 65536 for 64K, 16384 for 16K
#define BLOCKSIZE 32		//must be 2^n, 32 for 32B, 128 for 128B
#define WAYLEN 4		//direct way is 1, 4 way is 4
#define RECORDTYPE 'u' //u: unified, s: shared
#define FILENAME_INPUT  "pinatrace_dhrystone.out" // if run dhrystone data, change to "pinatrace_dhrysonte.out"
#define FILENAME_OUTPUT "result_drystone.txt" // if run dhrystone data, change to "result_dhrystone.txt"


#define MAXINDEX 8192 //no need change, up max cache size to 64K and min 
#define MAXWAY 8 //no need change, block size up to 8

//functions declarations:
void mainsimulator ();
long long getIndex(long long address);
long long getBlockTag (long long address);
void updateLRU(long long index, int way, bool hit,long long getBlockTag);

//cache structure declaration:
typedef struct
{
long long tag;
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
// Input file: pinatrace_linpack.out or pinatrace_dhrystone.out        *
// Output file: result_linpack.txt or result_dhrystone.txt             *
// Return: int                                                         *
//**********************************************************************
int main()
{
switch(RECORDTYPE)	
	{
		char *op; // // char array. read:R, write: W, instruction:I 
		case 'u': // unified(shared) cache
		splitCacheSize=CACHESIZE_DATA+CACHESIZE_INST;
		op="RWI";  
		mainsimulator(op);
		break;
		case 's': //split cache and data cache
		splitCacheSize=CACHESIZE_DATA; 
		//op="RRW";// for data type (operant is R or W)
		op="RRW";
		mainsimulator(op);
		splitCacheSize=CACHESIZE_INST; 
		//op="III";// for instruction type (operant is I)
		op="III";
		mainsimulator(op);
		break;
	}	
	FILE *outputFile;//Open file to outpur result
	char *filename=FILENAME_OUTPUT; 
	outputFile=fopen(filename,"w");
	fprintf(outputFile,"Instruction Cache Size: %d\nData Cache Size: %d\nBlock Size: %d\nWay: %d\nData Type: %c\nData File: %s\n\n",CACHESIZE_INST,CACHESIZE_DATA, BLOCKSIZE,WAYLEN,RECORDTYPE,FILENAME_INPUT);
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
	fprintf(outputFile,"Miss Rate     %.4f             %.4f                  %.4f        %.4f        %.4f\n",missRateTotal,missRateInst,missRateData,missRateRead,missRateWrite);
	fflush(outputFile);
	fclose(outputFile);

return 0;
}

//**********************************************************************
// Function Name: mainsimulator                                        *
// Description: Filter data type of 0: read, 1:write, 2: instruction.  * 
//              Simulate and calculate the miss and hit rat in  global *
//              variables of readMiss, writeMiss, instMiss, readHit,   *
//              writeHit, instHit.                                     *
// Input: char array                                                   *
// Return: none                                                        *
//**********************************************************************

void mainsimulator(char *op) //R,W,I: read, write, instruction
{
// define main parameter
int indexNum = splitCacheSize/BLOCKSIZE; 
int indexBits = log2(indexNum);

//declare local variables
long long index;
int way;
bool hit;
long long requiredTag;
int opInt; //fits the operant type in .din file, int format
char opChar; // fits the operant type in xxx.out generated by pin 
int i,j;
long long address; //48 bits for trace file created by pin
char addrPin[10]; //Hexadecimal

//initialize the cache line 
for (i=0;i<= indexNum-1;i++)
{
 	for (j=0;j<=WAYLEN-1;j++)
 	{
 		//set the insert sequence for blank block is from way 0 to way_length eg, for 4 way: 0,1,2,3
 		cacheLine[i][j].LRUbit=j; 
 		cacheLine[i][j].tag= 0xffffffffffffffff; //64 bits format
 	}
}

FILE *inputFile;//reading .out file to get record
char *filename =FILENAME_INPUT;
inputFile=fopen(filename,"r");

// main fuction: calculate hits and misses, lloop to end of file 
while(fscanf(inputFile,"%s %c %*[^x]x%llx",addrPin,&opChar,&address)!= EOF) // "%*[^x]x%llx" means pick up the hexadecimal digits after Ox 
{
	if(op[0]==opChar || op[1]==opChar || op[2]==opChar)
 	{
		index= getIndex(address); //get index from one selected item in .din file	
		requiredTag=getBlockTag(address); //get block tag 
		hit= false;
	    for(way=0;way<=WAYLEN-1;way++)
		{
			if (cacheLine[index][way].tag==requiredTag) //if tag matches, increase hit counter.
			{
				switch (opChar)
				{
					case 'R': readHit++; break;
					case 'W': writeHit++; break;
					case 'I': instHit++; break;
				}
				hit = true;
	    		updateLRU(index, way, hit, requiredTag);//update the LRU for this block
	    		way=WAYLEN-1;//match the tag, then exit the loop	
	    	}
	    }// end of for loop	    		
		if (hit==false)
	    	{
				//if miss, increase miss counter;write or replace with the new tag
				switch (opChar)
				{
					case 'R': readMiss++; break;
					case 'W': writeMiss++; break;
					case 'I': instMiss++; break;
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
// Input: long long integer                                            *
// Return: long long integer                                           *
//**********************************************************************
long long getIndex(long long address)
{
	long long index;
	int offsetBits = log2(BLOCKSIZE);
	int indexBits = log2(splitCacheSize/BLOCKSIZE);// eg.: cache size 32K, blick size is 8, then index bits is 12.
	unsigned long long indexShiftBits = ~(0xffffffffffffffff<<indexBits);// 64 bits,for pin generated data format
	index= (address >> offsetBits)&indexShiftBits; //get index value
	return index;	
}

//**********************************************************************
// Function Name: getBlockTag                                          *
// Description: get block tag from the given data of address           *
// Input: long long integer                                            *
// Return: long long integer                                           *
//**********************************************************************
long long getBlockTag(long long address)
{
	long long tag;
	int offsetBits = log2(BLOCKSIZE);
	int indexBits = log2(splitCacheSize/BLOCKSIZE);
	unsigned long long tagShiftBits= ~(0xffffffffffffffff<<(64-offsetBits-indexBits)); //same mathod as waht for getting index bits, data is totally 64 bits.
	tag= (address >> (offsetBits+indexBits))& tagShiftBits;// get tag value

	return tag;	
}

//**********************************************************************
// Function Name: updateLRU                                            *
// Description: get index, associativity(way), hit status, tag, replace*
//              hit tag or replace tag and update the LRU status.      *
// Input: long long integer, bool                                      *
// Return: none                                                        *
//**********************************************************************
void updateLRU(long long index, int way, bool hit,long long requiredTag)
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
	}// end of if (hit==true)
	else
	{
		// miss: replace the clock tag, which the LRU is 0
		for (i=0;i<=WAYLEN-1;i++)
		{
			if (cacheLine[index][i].LRUbit==0) // find the least used way or blank way
			{
				cacheLine[index][i].tag=requiredTag; //replace or input with the new block tag
				//cacheLine[index][i].validity=1; // mark the block as a used one
				cacheLine[index][i].LRUbit=WAYLEN-1; //set the LRU as the most recent used
			}else{
				cacheLine[index][i].LRUbit--; 
				}
		}//end of for
	}//end of else
}// end of function "updateLRU"
