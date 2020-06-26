//**********************************************************************
//                                                                     *
//               University Of North Carolina Charlotte                *
//                                                                     *
//Program: prefether                                                   *
//Description: This program is to simulate the hit/miss rate of several*
//             type of predictors such as one level, local, global,    *
//             Gshare and hybrid.                                      *
//                                                                     *
//                                                                     *
//File Name: branchprd_yu.c                                               *
//File Version: 1.0                                                    *
//Baseline: Homework_4                                                 *
//                                                                     *
//Course: ECGR5181                                                     *
//                                                                     *
//Programmed by: Yu Liu                                                * 
//Under Suppervision of: Dr. Hamed Tabkhi                              *
//                                                                     *
//Input file: branch-trace-gcc.trace or branches_0_dhrystone.out       *
//      or branches_0_linpack.out                                      *
//                                                                     *
//Output file:branch_prediction.txt                                    *
//**********************************************************************   
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h> 
#include "stdbool.h"

#define FILENAME_INPUT  "branch-trace-gcc.trace" // if run dhrystone data, change to "pinatrace_dhrysonte.out"
#define FILENAME_OUTPUT "branch_prediction.txt" // if run dhrystone data, cHange to "result_dhrystone.txt"

//constant declarations  
#define PREDICTOR_TYPE  'H' //upper case.O:one level, G:Global, S: Gshare,  L: Two level local, H: Hybrid
#define PHT_SIZE 1024 
#define LHR_SIZE 128 //size of Local history register entry.Optionals: 256 (8 bits),128 (7 bits),64 (6 bits), 16(4 bits), 4(2 bits)
#define GHR_BITS 10 //width of bit counter: optionals are 10,8,6,4,2
#define PC_SHIFT_BITS 5// the last x bits of program counter to be ignored. options are 0,2,3,5


int PHT[PHT_SIZE]; //pattern history table
int choiceTable[PHT_SIZE]; //for tournament predictor
int LHR[LHR_SIZE]; //local history register
int hitTaken=0;
int missTaken=0;
int hitNotaken=0;
int missNotaken=0;
int branchNum=0;


void mainPredictor();
int getPcValue(unsigned long long pcAddress, unsigned long long pcMask);
unsigned long long decToHex(unsigned long long decNum);
void checkPhtHit(int phtIndex, int actucalTakenStatus);
int choosePredictor(int pcValue, int phtIndexLocal, int phtIndexGshare);
void updateChoiceTable(int pcValue,int phtIndexGshare,int phtIndexLocal,int actualTakenStatus);
void updateGHR(int *GHR_value,int ghrMask,int takenStatus);
void updateLHR(int pcValueLocl,int pcMask, int actucalTakenStatus);
char * getPostfix(char *fileName);

//**********************************************************************
// Function Name: main()                                               *
// Description: - Call functions to:  mainsimulator to run similation  *
//              - Output the simulating result;                        *
// Input file: pinatrace_linpack.out or pinatrace_dhrystone.out        *
// Output file: result_linpack.txt or result_dhrystone.txt             *
// Return: int                                                         *
//**********************************************************************
int main()
{
	mainPredictor ();
	FILE *outputFile;//Open file to outpur result
	char *filename=FILENAME_OUTPUT; 
	outputFile=fopen(filename,"w");
	
	fprintf(outputFile,"Input Data File: %s\nOutput Data File: %s\n\n",FILENAME_INPUT,FILENAME_OUTPUT);
	fprintf(outputFile,"Preditor type(G:Global, L: local, S: Gshare, O:one level): %c\nPattern History Table size: %d\nLocal History Register Size: %d\nGlobal History Register Bits: %d\nPC Shift Bits: %d\n\n",PREDICTOR_TYPE,PHT_SIZE,LHR_SIZE,GHR_BITS,PC_SHIFT_BITS);

	fprintf(outputFile,"Branches             Total         Taken            Not Take\n");
	fprintf(outputFile,"Misses            %8d      %8d            %8d\n",(missTaken+missNotaken),missTaken, missNotaken);
	fprintf(outputFile,"Hits              %8d      %8d            %8d\n",(hitTaken+hitNotaken),hitTaken,hitNotaken);
	fprintf(outputFile,"Total             %8d      %8d            %8d\n",branchNum,(missTaken+hitTaken),(missNotaken+hitNotaken));
	fprintf(outputFile,"Hit Rate            %.4f        %.4f              %.4f \n",((float)hitTaken+(float)hitNotaken)/(float)branchNum,(float)hitTaken/((float)hitTaken+(float)missTaken),(float)hitNotaken/((float)hitNotaken+(float)missNotaken));
	fprintf(outputFile,"Miss Rate           %.4f        %.4f              %.4f \n",1-((float)hitTaken+(float)hitNotaken)/(float)branchNum,1-(float)hitTaken/((float)hitTaken+(float)missTaken),1-(float)hitNotaken/((float)hitNotaken+(float)missNotaken));
	
	fflush(outputFile);
	fclose(outputFile);
return 0;
}

//**********************************************************************
// Function Name: mainPredictor                                        *
// Description: Predict with different prediction scheme.              * 
//              O:one level, G:Global, L: local, S: Gshare,  H: Hybrid *
//              Count the hit and miss rate.                           *
//                                                                     *
// Input: data file: .trace or .out format                             *
// Return: none                                                        *
//**********************************************************************
void mainPredictor ()
{
	int i;
	int pcBits;
	int pcValue; //program counter value
	int pcValueLocal; //for local predictor, 7 bits
	int phtIndexGshare=0;
	int phtIndexLocal=0;
	int phtIndex=0; // when PHT bits is 10, PHT size is 1024
	char actualTaken;
	int actualTakenBenchmark; // for benchmark generated data 
	unsigned int actualTakenStatus;
	char * fileType;
	
	int GHR_value=0;
	int chosenPhtIndex; // 2 bits counter 0,1: Gshare, 2,3: local
	
	unsigned long long pcMask,ghrMask, lhrMask;  
	unsigned long long pcAddress = 0xffffffffffffffff;
	unsigned long long pcAddressHex = 0xffffffffffffffff;
	unsigned long long pcAddressBenchmark = 0xffffffffffffffff; // for benchmark generated data 
	

	//pcBits= log2(PHT_SIZE);
	pcMask = PHT_SIZE-1;
	ghrMask = pow(2,GHR_BITS)-1; //initialize global history register mask	
	lhrMask = LHR_SIZE-1; // local history register maske,e.g. 1111111 when LHR is 128 entries  
	pcBits = log2(PHT_SIZE);
	
	int fscanfReturnValue = 1; //for verifying if the trace file is EOF 
 	
	// initialize pattern histoery table and choiceTable
	for (i=0;i<PHT_SIZE;i++)
	{
		PHT[i]=0; // initialize as strongly not taken
		choiceTable[i]=01; //Choose Gshare: 0,1; choose Local: 2,3
	}
	
	//initialize local history register table
	for (i=0;i<LHR_SIZE;i++) 
	{
		LHR[i]=0;
	}
		
		
	FILE *inputFile;//reading .out file to get record
	char *filename =FILENAME_INPUT;
	inputFile=fopen(filename,"r");

	
 	
	// main prediction fuction: calculate hits and misses
	while(fscanfReturnValue != EOF) 
	{
	 	if (getPostfix(FILENAME_INPUT)[0] == 'o') //return the postfix value is "out", out formate file
			{
			fscanfReturnValue  = fscanf(inputFile,"%llx %d %*[^\n]",&pcAddressBenchmark,&actualTakenBenchmark);
			actualTakenStatus = actualTakenBenchmark;
			pcAddressHex = pcAddressBenchmark;
			}
		else if (getPostfix(FILENAME_INPUT)[0] == 't') //return the postfix value is "trace", trace file
			{
			fscanfReturnValue = fscanf(inputFile,"%lld %c ",&pcAddress,&actualTaken);
			pcAddressHex=decToHex(pcAddress);	
			if (actualTaken == 'T' | actualTaken == 't') actualTakenStatus =1;
			else actualTakenStatus =0;
			}
		else break;

		switch (PREDICTOR_TYPE)
		{
			case 'O': //one level
					pcValue = getPcValue(pcAddressHex, pcMask);
					phtIndex = pcValue;
					checkPhtHit(phtIndex, actualTakenStatus);
					break;
			case 'G': //two level of global
					phtIndex =  GHR_value;
					checkPhtHit(phtIndex, actualTakenStatus);
					updateGHR(&GHR_value,ghrMask,actualTakenStatus); //update GHR
					break;
			case 'S': //Gshare
					pcValue = getPcValue(pcAddressHex, pcMask);										
					phtIndex = (pcValue>>(pcBits-GHR_BITS)) ^ GHR_value;

					checkPhtHit(phtIndex,actualTakenStatus);
					updateGHR(&GHR_value,ghrMask,actualTakenStatus); //update GHR
					break;
			case 'L': // two level of local, 7 bits, 128 entries of LHR; 10 bits for PHT
					
					pcValueLocal = getPcValue(pcAddressHex, lhrMask); // take 7 bits address from program counter
		 			phtIndex = LHR[pcValueLocal];
					checkPhtHit(phtIndex, actualTakenStatus); // check if taken then count the miss and hit number
					// update the local History Register
					updateLHR( pcValueLocal,pcMask,actualTakenStatus);

					break;
			case 'H':// tournament prediction
					//Local predictor
					pcValueLocal = getPcValue(pcAddressHex, lhrMask);// take 7 bits address from program counter
		 			phtIndexLocal = LHR[pcValueLocal];	
					
					//Gshare predictor		
					pcValue = getPcValue(pcAddressHex, pcMask);	
					phtIndexGshare = (pcValue>>(pcBits-GHR_BITS)) ^ GHR_value; 
					
					chosenPhtIndex = choosePredictor(pcValue, phtIndexLocal, phtIndexGshare); //Chose the predictors
					checkPhtHit(chosenPhtIndex,actualTakenStatus); //check and update the hit rate
					
					updateGHR(&GHR_value,ghrMask,actualTakenStatus);
					updateLHR( pcValueLocal,pcMask,actualTakenStatus);
					updateChoiceTable(pcValue, phtIndexGshare, phtIndexLocal, actualTakenStatus);
		}

	}//end of while
}// end of mainPredictor

//**********************************************************************
// Function Name: updateChoiceTable                                    *
// Description: update choice table for Hibrid predictor               * 
// Input: program coounter, PHT index number or two predictors,actual  *
//                          taken result                               *
// Return: none                                                        *
//**********************************************************************
void updateChoiceTable(int pcValue,int phtIndexGshare,int phtIndexLocal,int actualTakenStatus)
{	
	int prd_local, prd_Gshare;
	if (PHT[phtIndexLocal]>=2) 
		prd_local =1; //local predictor predicts "taken"
	else 
		prd_local=0; //local predictor predicts "not taken"
	if (PHT[phtIndexGshare]>=2)  
		prd_Gshare =1; //Gshare predicts "taken"
	else 
		prd_Gshare = 0; //Gshare predicts "not taken"

	if (prd_local != prd_Gshare) //when 2 predictors' prediction are different
		{
			if (prd_Gshare == actualTakenStatus)// Gshare is correctly predicted
				{
					if (choiceTable[pcValue]>0) choiceTable[pcValue]--;
				}
			else //local is correctly 
				{
					if (choiceTable[pcValue]<3) choiceTable[pcValue]++;
				}
		}
}

//**********************************************************************
// Function Name: getPcValue                                           *
// Description: get program counter address value in nominated bits    * 
// Input: program counter address, nominated bit number                *
// Return: program counter address value be transfered                 *
//**********************************************************************
int getPcValue(unsigned long long pcAddress, unsigned long long pcMask)
{
	int phtIndex;
	phtIndex= (pcAddress >> PC_SHIFT_BITS)& pcMask;//e.g >>2, & 1111111111
	return phtIndex;
}

//**********************************************************************
// Function Name: decToHex                                             *
// Description:transfer the number in decimal value to Hexdecimal value* 
// Input: Decimal number                                               *
// Return: Hexdecimal number                                           *
//**********************************************************************
unsigned long long decToHex(unsigned long long decNum)
{
	char hex[16]={'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
	int i=15;
	char a[16] = "0000000000000000";
	unsigned long long hexNum=0;
	int mod;
	while(decNum>0)
	{
		mod= decNum%16;
		a[i--]=hex[mod];
		decNum = decNum/16;	
	}
	for (i=0;i<=15;i++)
	{
		if (a[i]<='9') hexNum=16*hexNum+(a[i]-'0');
		else hexNum=16*hexNum+(10+a[i]-'a');
	}
	return hexNum;
}

//**********************************************************************
// Function Name: updateGHR                                            *
// Description: update Global History Register value                   * 
// Input: GHR address, GHR bit number , branch actual taken status     *
// Return: none                                                        *
//**********************************************************************
void updateGHR(int *GHR_value,int ghrMask,int actualTakenStatus)
{	
	*GHR_value=(*GHR_value << 1 | actualTakenStatus)& ghrMask;// keep the GHR bit length, eg. 10 

}

//**********************************************************************
// Function Name: updateLHR                                            *
// Description: update Local History Register value                    * 
// Input: LHR index, LHR bit number , branch actual taken status       *
// Return: none                                                        *
//**********************************************************************
void updateLHR(int pcValueLocal,int pcMask, int actualTakenStatus)
{	
	LHR[pcValueLocal]= (LHR[pcValueLocal]<< 1 | actualTakenStatus)& pcMask;

}

//**********************************************************************
// Function Name:  checkPhtHit                                         *
// Description: check what predictor predicts in PHT, compare with the * 
//              actual taken status, then count the hit and miss number*
// Input: PHT index, actual taken status                               *
// Return: none                                                        *
//**********************************************************************
void checkPhtHit(int phtIndex, int actucalTakenStatus)
{
		if (actucalTakenStatus == 1) // if actual is Taken
	{
		switch (PHT[phtIndex])
		{
			case 3:
				hitTaken++;
				break;
			case 2:
				hitTaken++;
				PHT[phtIndex]++;
				break;
			case 1:
				missTaken++;
				PHT[phtIndex]++;
				//PHT[phtIndex]=3;
				break;
			case 0:
				missTaken++;
				PHT[phtIndex]++;
				break;
			
		}
	}
	else // if actual is Not Taken
	{
		switch (PHT[phtIndex])
		{
			case 3:
				missNotaken++;
				PHT[phtIndex]--;
				break;
			case 2:
				missNotaken++;
				PHT[phtIndex]--;
				//PHT[phtIndex]=0;
				break;
			case 1:
				hitNotaken++;
				PHT[phtIndex]--;
				break;
			case 0:
				hitNotaken++;
				break;
		}
	}
	branchNum++;
}


//**********************************************************************
// Function Name: choosePredictor                                      *
// Description: For Hybrid predictor.                                  * 
//              choose to use Gshare predictor or Local predictor      *
// Input: program coounter, PHT index number or two predictors         *
// Return: the PHT index of the chosen predictor                       *
//**********************************************************************
int choosePredictor(int pcValue, int phtIndexLocal, int phtIndexGshare)
{
	int phtIndex;
	if (choiceTable[pcValue]==0 | choiceTable[pcValue]==1) 
		phtIndex= phtIndexGshare;
	else 
		phtIndex=phtIndexLocal;
	return phtIndex;
}

//**********************************************************************
// Function Name:  getPostfix                                          *
// Description: get the postfix of the filename, so identify the type  * 
//              of the data source                                     *
// Input: file name                                                    *
// Return: character address of the postfix                            *
//**********************************************************************
char * getPostfix(char *fileName) //this function is for identifying the data file type
	{
		int fileNameLen;
	 	fileNameLen = strlen(fileName);
	 	int j,k,l,isPostfix;
 	 	k=0;
	 	isPostfix=0;
	 	l=fileNameLen;
	 	char *postFix;
	 	for (j=0;j<fileNameLen;j++) //count the length of postfix, includes "."
		 	{
				if(fileName[j] == '.') isPostfix =1;
			  	if (isPostfix==1)  k++;
			}
		postFix=(char*)malloc(k-1);		
		for(j=k-1;j>=0;j--, l--) //record the postfix characters
		{
			postFix[j]=fileName[l];
		}
 	  if (isPostfix==1) return postFix; 
 	  else return "null";
 	  free(postFix);
	}
