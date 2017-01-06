/*main.c*/

//
// Chicago Crime Lookup via hashing and AVL trees.
//
// Michael Mei
// Linux Mint 18(Sarah) using gcc
// U. of Illinois, Chicago
// CS251, Fall 2016
// HW #9
//

#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "avl.h"
#include "mymem.h"

#define TRUE  1
#define FALSE 0


//
// Defines information about one crime:
//
typedef struct Crime
{
  char  caseNum[10];
  char  dateTime[24];
  char  IUCR[5];
  int   arrested;
  int   domestic;
  int   area;
  int   year;
  struct Crime *next;
} Crime;


//
// Global variables:
//
int g_collisions = 0;  // instrument # of collisions for reporting:


//
// Hash function:
//
int hash(char *CaseNum, int N)
{
  //
  // case numbers are 7 or 8 chars long, with the last 6 as digits.  some examples:
  // HZ256372, HY266148, HY299741, HH254987, G219399.  Since there could be millions 
  // of cases, 6 digits are not enough.  so we need to take advantage of the letters 
  // too...
  //
  int len = strlen(CaseNum);

  if (len < 8 || len > 9)  // invalid, e.g. perhaps user enters an invalid case #:
    return -1;

  int advance = len - 6;  // come from the end in case there are more than 2 digits to start:
  char *p = CaseNum + advance;  // ptr to first digit:

  int i = atoi(p);

  assert(i > 0 && i < 1000000);  // 6 meaningful digits:

  if (len > 7) // use the 2nd letter:
  {
    char c = CaseNum[1];

    int diff = abs('Z' - c);
    i = (diff * 1000000) + i;
  }

  //
  // return hash value:
  //
  return i % N;  // whatever we have, make sure it falls within 0..N-1:
}


//
// parseCrime:
//
// parses given line and stores the fields in the given Crime struct:
//
void parseCrime(char *line, Crime *c)
{
  // format:
  //
  // Case Number, Date, IUCR, Arrest, Domestic, Beat, District, Ward, Community Area, Year
  // HZ256372, 01/01/2015 12:00:00 AM, 0281, false, false, 0334, 003, 7, 43, 2015
  // HZ257172, 11/24/2015 05:30:00 PM, 0820, false, false, 1124, 011, 28, 27, 2015
  // HY266148, 05/19/2015 01:12:00 AM, 0560, true, false, 1933, 019, 44, 6, 2015
  //
  char *token;

  // case number:
  token = strtok(line, ",");
  assert(token != NULL);
  strcpy(c->caseNum, token);

  // datetime:
  token = strtok(NULL, ",");
  assert(token != NULL);
  strcpy(c->dateTime, token);

  // IUCR:
  token = strtok(NULL, ",");
  assert(token != NULL);
  strcpy(c->IUCR, token);

  // arrested:
  token = strtok(NULL, ",");
  assert(token != NULL);
  if (strcmp(token, "true") == 0)
    c->arrested = TRUE;
  else
    c->arrested = FALSE;

  // domestic:
  token = strtok(NULL, ",");
  assert(token != NULL);
  if (strcmp(token, "true") == 0)
    c->domestic = TRUE;
  else
    c->domestic = FALSE;

  // beat: --- skip
  token = strtok(NULL, ",");
  assert(token != NULL);
  // district: --- skip
  token = strtok(NULL, ",");
  assert(token != NULL);
  // ward: --- skip
  token = strtok(NULL, ",");
  assert(token != NULL);

  // area:
  token = strtok(NULL, ",");
  assert(token != NULL);
  c->area = atoi(token);

  assert(c->area >= 0 && c->area <= 77);

  // year:
  token = strtok(NULL, ",");
  assert(token != NULL);
  c->year = atoi(token);

  token = strtok(NULL, ",");
  assert(token == NULL);  // no more tokens on this line:
}


//
// inputCrimes
//
// Inputs the crimes from the given file, building and storing these crimes 
// in a hash table.  Chaining is used to deal with collisions.  A pointer
// to the hash table is returned; the size of the hash table is also returned
// via the 2nd function argument.
//
Crime **inputCrimes(char *filename, int *ht_size, int *crimeCount)
{
  FILE *input;
  char  line[256];
  int   linesize = sizeof(line) / sizeof(line[0]);
  int   startYear, endYear;

  //
  // open file, make sure it worked:
  //
  input = fopen(filename, "r");
  if (input == NULL)
  {
    printf("**Error: unable to open '%s'.\n\n", filename);
    exit(-1);
  }

  //
  // read the range of years from 1st line:
  //
  printf(">> reading:   '%s'\n", filename);
  fscanf(input, "%d %d", &startYear, &endYear);
  fgets(line, linesize, input);  // discard rest of line

  printf(">> years:      %d..%d\n", startYear, endYear);

  //
  // allocate space for hash table: assume 300,000 crimes/year, so compute size
  // we need with a 20% load factor (i.e. 80% unused):
  //
  int years = endYear - startYear + 1;
  int N = years * 300000 * 5 /*load factor, 5 => 20%*/;

  Crime **hashtable = (Crime **)mymalloc(N * sizeof(Crime *));
  if (hashtable == NULL)  // malloc failed :-(
  {
    printf("**Error: unable allocate memory for hash table (%d).\n\n", N);
    exit(-1);
  }

  //
  // initialize hash table entries to NULL, i.e. empty chains:
  //
  for (int i = 0; i < N; ++i)
    hashtable[i] = NULL;

  // 
  // now start reading the crime data:
  //
  fgets(line, linesize, input);  // discard 2nd line --- column headers

  while (fgets(line, linesize, input))  // start reading data:
  {
    line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:

    // allocate memory for this crime:
    Crime *c = (Crime *)mymalloc(sizeof(Crime));
    if (c == NULL)  // alloc failed :-(
    {
      printf("**Error: unable allocate memory for Crime struct.\n\n");
      exit(-1);
    }

    // fill crime struct with data from file:
    parseCrime(line, c);

    // link into hashtable:
    int index = hash(c->caseNum, N);

    if (hashtable[index] != NULL)
      g_collisions++;

    c->next = hashtable[index];  // existing chain follows us:
    hashtable[index] = c;        // insert @ head:

    (*crimeCount)++;
  }

  fclose(input);

  //
  // stats:
  //
  printf(">> # crimes:   %d\n", *crimeCount);
  printf(">> ht size:    %d\n", N);
  printf(">> collisions: %d\n", g_collisions);

  //
  // return hash table pointer and size:
  *ht_size = N;

  return hashtable;
}


//
// inputAreas
//
// Reads the file of chicago community areas, of which there should be 78.
// Returns pointer to array of community names.
//
char **inputAreas(char *filename)
{
  FILE *input;
  char  line[256];
  int   linesize = sizeof(line) / sizeof(line[0]);

  //
  // open file, check to make sure it worked:
  //
  input = fopen(filename, "r");
  if (input == NULL)
  {
    printf("**Error: unable to open '%s'.\n\n", filename);
    exit(-1);
  }

  printf(">> reading:   '%s'\n", filename);

  //
  // allocate array for community names:
  //
  int N = 78;
  char **areas = (char **)mymalloc(N * sizeof(char *));

  //
  // now read the names and fill the array:
  //
  fgets(line, linesize, input);  // discard 1st line --- column headers

  int count = 0;

  while (fgets(line, linesize, input))  // start reading data:
  {
    char *token;

    line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:

    //
    // format:  area #, area name
    //
    token = strtok(line, ",");
    assert(token != NULL);

    int index = atoi(line);

    token = strtok(NULL, ",");
    assert(token != NULL);

    int len = strlen(token) + 1;
    char *p = (char *)mymalloc(len * sizeof(char));
    strcpy(p, token);

    areas[index] = p;

    token = strtok(NULL, ",");
    assert(token == NULL);  // no more tokens on this line:

    count++;
  }

  assert(count == N);

  fclose(input);

  //
  // return areas array pointer and size:
  //
  return areas;
}

//
// inputCodes
//
// Input the IUCR codes and its descriptions into an AVL tree
// Returns pointer to root of AVL tree
//
AVLNode *inputCodes(AVLNode *root, char *filename) 
{
	FILE *input;
	char line[256];
	int linesize = sizeof(line)/sizeof(line[0]);
	int count;

	//
	// open file, check to make sure it worked:
	//
	input = fopen(filename, "r");
	if (input == NULL)
	{
		printf("**Error: unable to open '%s'.\n\n", filename);
		exit(-1);
	}

	fgets(line, linesize, input);  // discard 1st line --- column headers
	line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:


	while (!feof(input))  // start reading data:
  	{
	    AVLElementType T;
	    char *token;
	    count++;

	    //allocate memory for a code
	    
	    //
	    // format:  IUCR #, PRIMARY DESCRIPTION, SECONDARY DESCRIPTION
	    //
	    token = strtok(line, ",");		//IUCR
	    assert(token != NULL);

	    strcpy(T.IUCR, token);

	    token = strtok(NULL, ",");		//PRIMARY DESCRIPTION
	    assert(token != NULL);

	    strcpy(T.PrimaryDesc, token);

	    token = strtok(NULL, ",");		//SECONDARY DESCRIPTION
	    assert(token != NULL);

	    strcpy(T.SecondaryDesc, token);

	    //insert the code into an AVL tree
	    root = Insert(root, T);

	    fgets(line, linesize, input);
		line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:
  	}

  	//PrintInorder(root);

  	fclose(input);

  	return root;


}

void freeHashTableHelper(Crime *hashtable)
{
	if (hashtable == NULL)
		return;
	else 
	{
		Crime *temp = hashtable;
		hashtable = hashtable->next;
		freeHashTableHelper(hashtable);
		myfree(temp);
	}
}

void freeHashTable(Crime **hashtable, int N)
{

	int i;
	for (i = 0; i < N; i++)
	{
		freeHashTableHelper(hashtable[i]);
	}

	myfree(hashtable);
}

void freeAreas(char *areas[])
{
	int i;
	for (i = 0; i < 78; i++)
	{
		myfree(areas[i]);
	}

	myfree(areas);

}

void freeCodes(AVLNode *root)
{
	if (root == NULL)
		return;
	else
	{
		freeCodes(root->left);
		freeCodes(root->right);
		myfree(root);
	}
}

//
// computeCrimeInfo
// 
// (1) Compute the total # of crimes T in each area
// (2) The % of T against the total # of crimes across Chicago
// (3) The total # of particular crime in this area
// (4) The % of this crime agianst the total # of cirmes in this area
// (5) The overall % that this crime occurs in Chicago
//
void computeCrimeInfo(Crime *c, Crime **hashtable, char *areas[], AVLNode *root, int totalCrimes, int N)
{
	int totalCrimesInArea = 0;			// (1)
	double percentCrimesInArea = 0;		// (2)
	int totalCrimesTInArea = 0;			// (3)
	double percentCrimesTInArea = 0;	// (4)
	double percentCrimeT = 0;			// (5)

	int totalCrimesT = 0;

	//
	// Iterate through hash table, computing (1) and (3)
	//
	int i;
	for (i = 0; i < N; i++)
	{
		if (hashtable[i] != NULL)	//check if there is an element in hash table
		{
			if (c->area == hashtable[i]->area) 	//check if the element is the area we want
			{
				Crime *ptr = hashtable[i];

				while (ptr != NULL) {
					totalCrimesInArea++;	//increment total # crimes in area
					ptr = ptr->next;
				}

				if (strcmp(c->IUCR, hashtable[i]->IUCR) == 0)	//total crimes T in area
					totalCrimesTInArea++;
			}

			if (strcmp(c->IUCR, hashtable[i]->IUCR) == 0)	//total crimes T in chicago
				totalCrimesT++;
		}
	}

	percentCrimesInArea = totalCrimesInArea / (double) totalCrimes * 100;
	percentCrimesTInArea = totalCrimesTInArea / (double) totalCrimesInArea * 100;
	percentCrimeT = totalCrimesT / (double) totalCrimes * 100;

	//output crime info
	printf("AREA: %d => %s\n", c->area, areas[c->area]);
	printf("  # of crimes in area:	%d\n", totalCrimesInArea);
	printf("  %% of chicago crime:	%f%%\n", percentCrimesInArea);
	printf("\n");

	AVLElementType temp;
  	strcpy(temp.IUCR, c->IUCR);
  	AVLNode *code = Contains(root, temp);

  	printf("CRIME: %s => ", c->IUCR);

  	if (code == NULL)
  		printf("UNKNOWN\n");
  	else
  		printf("%s: %s\n", code->value.PrimaryDesc, code->value.SecondaryDesc);

	printf("  # of THIS crime in area:	%d\n", totalCrimesTInArea);
	printf("  %% of THIS crime in area:	%f%%\n", percentCrimesTInArea);
	printf("  %% of THIS crime in chicago:	%f%%\n", percentCrimeT);
}

//
// outputCrimeInfo:
//
void outputCrimeInfo(Crime *c, char *areas[], AVLNode *root)
{
  printf("%s:\n", c->caseNum);
  printf("  date/time: %s\n", c->dateTime);
  printf("  city area: %d => %s\n", c->area, areas[c->area]);
  printf("  IUCR code: %s => ", c->IUCR);

  AVLElementType temp;
  strcpy(temp.IUCR, c->IUCR);
  AVLNode *code = Contains(root, temp);

  if (code == NULL)
  	printf("UNKNOWN\n");
  else
  	printf("%s: %s\n", code->value.PrimaryDesc, code->value.SecondaryDesc);

  printf("  arrested:  %s\n", ((c->arrested) ? "true" : "false"));
  printf("  domestic violence:  %s\n", ((c->domestic) ? "true" : "false"));
}


//
// Main:
//
int main()
{
  printf("\n** Chicago Crime Lookup Program **\n\n");

  Crime **hashtable;  // array of Crime pointers:
  int     N;          // size of hash table
  char  **areas;      // array of community names (strings):
  AVLNode *codes = NULL;	  //AVL tree for descriptions of IUCR codes
  int totalCrimes = 0;	// number of crimes in chicago
  
  areas = inputAreas("Areas.csv");
  hashtable = inputCrimes("Crimes.csv", &N, &totalCrimes);
  codes = inputCodes(codes, "Codes.csv");

  printf("\n");

  //
  // crime lookup loop:
  //
  char line[256];
  int  linesize = sizeof(line) / sizeof(line[0]);

  printf("Enter a case number> ");
  fgets(line, linesize, stdin);
  line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:

  while (strlen(line) > 0)
  {
    int index = hash(line, N);

    if (index < 0)
    {
      printf("** invalid case #, try again...\n");
    }
    else
    {
      printf(">> hash index: %d <<\n", index);

      //
      // walk along the chain and see if we can find this case #:
      //
      Crime *c = hashtable[index];

      while (c != NULL)
      {
        if (strcmp(line, c->caseNum) == 0)  // found it!
          break;
        
        // otherwise keep looking:
        c = c->next;
      }

      // if get here, see if we found it:
      if (c == NULL)
        printf("** Case not found...\n");
      else {
        outputCrimeInfo(c, areas, codes);
        printf("\n");
        computeCrimeInfo(c, hashtable, areas, codes, totalCrimes, N);
      }

    }

    printf("\n");
    printf("Enter a case number> ");
    fgets(line, linesize, stdin);
    line[strcspn(line, "\r\n")] = '\0';  // strip EOL(s) char at the end:
  }

  //
  // done:
  //
  printf("\n** Done **\n\n");

  mymem_stats();	//print the memory stats

  printf("** Freeing memory...\n");

  freeHashTable(hashtable, N);		//free memory allocated by hash table

  freeAreas(areas);		//free memory allocated by areas

  freeCodes(codes);		//free memory allocated by codes

  mymem_stats();		//print the memory stats

  printf("\n\n");

  return 0;
}
