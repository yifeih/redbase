/* File:  generate.c
 * -----------------
 * This file generates all the outfiles that will be loaded into my
 * database.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Numbers of relations I want for my database */
#define NUM_COMPANY 25                /* this one shouldn't be changed */
#define NUM_DEALER (NUM_COMPANY * 8)
#define NUM_STORE 400
#define NUM_PLAYER 4000
#define NUM_RACQUET (NUM_PLAYER * 4)
#define NUM_COMPANYSPONSORED (NUM_PLAYER / 20)
#define NUM_DEALERSTORE (NUM_STORE * 10)

#define MAX_STREET_NUM 10

/* structures used to save information so that information will
 * relate from one relation to another.
 */
typedef struct {
  char *company;
  int AARAmembership;
} CompanySponsored;

typedef struct {
  int storeID;
  int dealerID;
} DealerStore;

/* Streets that will be put in as random Streets */
#define NUM_FIRST 61
char *first_names [NUM_FIRST] = {
  "Bob", "Joe", "Roger", "Carly", "Harry",
  "Fred", "Susan", "Hayden", "Mary", "Sue",
  "Steve", "George", "Bill", "Mike", "John",
  "Cathy", "Michelle", "Tina", "Amanda", "Diane",
  "Peter", "Frank", "Ken", "Mark", "Craig",
  "Dorthy", "Laura", "Elise", "Patricia", "Antonia",
  "Clay", "Aubry", "Leaf", "Devin", "Jaron",
  "Naazneen", "Elkova", "Pratima", "Nami", "Sarah",
  "James", "Seth", "Dave", "Jeff", "JT",
  "Kristin", "Liz", "Jennifer", "Kate", "Karen",
  "Flip", "Phillip", "Jason", "Sinbad", "Vineet",
  "Roy", "Rob", "Colleen", "Alex", "Hannah",
  "Pauline"
};

#define NUM_LAST 80
char *last_names [NUM_LAST] = {
  "Icenogle", "Barma", "Jindrich", "Kunz", "Smith",
  "Johnson", "Morgan", "Kurth", "Thompson", "Angwin",
  "Ho", "Lin", "Wang", "Ping", "Hau",
  "Nakajima", "Newton", "Widom", "Howard", "Sing",
  "Gonzales", "Booker", "Syer", "Shah", "Shaw",
  "Cavazzoni", "Fairchild", "Salisbury", "Dicks", "Douglas",
  "Lee", "Welch", "Lewis", "Clark", "Uckert",
  "Morris", "Wiltshire", "Larkin", "Melwani", "Dewan",
  "Tyebjee", "Jones", "Willis", "Brown", "Cooper", 
  "Topper", "Roberts", "Richards", "Goldstein", "Burnstein",
  "Katz", "Wilhelm", "Howe", "Thatcher", "Reagan",
  "Bush", "Pero", "Hart", "Major", "Belare", 
  "Motwani", "Mathews", "Marly", "Harper", "Gains",
  "Simon", "Garica", "Townson", "Young", "Bowie",
  "Kim", "Graalfs", "Powell", "Baker", "Barns",
  "McHugh", "Zelesko", "Gossain", "Goldman", "Zim"
};

#define NUM_STORE_NAMES 20
char *store_names [NUM_STORE_NAMES] = {
  "Big 5 Sports", "Copelands", "Play It Again Sports", "Courtesy Sports",
  "Oshmans", "Ripe Plum Sports", "Coast West Sports", "Magic Sports",
  "Racquetball Plus", "Sports Fever", "McAdams Sporting Goods", "Becker",
  "LA Sports", "Super Sport", "Head To Toe Sport", "Roll Out Goods",
  "Target", "Lechmere", "7-11", "Frys"
};

#define NUM_STREETS 35
char *streets [NUM_STREETS] = {
  "1st Street", "2nd Street", "3rd Street",
  "Lincoln Street", "Main Street", "Down Town Street",
  "Palm Drive", "Town Hall Way", "Miracle Drive", 
  "Friends Circle", "Washington Street", "Burning Bridges Road",
  "Cypress Way", "Sycamore Road", "Zelkova Street", 
  "Daisy Lane", "Iris Circle", "Rose Road",
  "Fay Way", "Jane Lane", "Jewel Road", 
  "Central Road", "Alma Street", "University Drive",
  "Santa Teresa Road", "Gates Road", "Mission Expressway",
  "Madison Blvd.", "Trail Head Way", "Grimzly Drive",
  "Ransom Road", "Willowspree Lane", "Newell Road", "Palm Drive",
  "Elm Street"
};

/* random Cities */
#define NUM_CITIES 30
char *cities [NUM_CITIES] = {
  "New York", "Auburn", "Lincoln", "Harmon", "Placer", 
  "Helen", "Washington", "Reed", "Spring Hill", "River Bottom",
  "Desperado", "Meadow Sweep", "Sleepy Town", "Townsville", "Beck",
  "Hilltop", "Lonely Swallow", "Colfax", "Palo Alto", "Tall Tree",
  "Woodside", "Water Front", "Hollow Tree", "Sick Dog", "Lone Cow",
  "Seekonk", "Barrington", "East Palo Alto", "Brighton", "Newton"
};

/* random States */
#define NUM_STATES 20
char *states [NUM_STATES] = {
  "CA", "OH", "MA", "FL", "NY", "MN", "OR", "NJ", "ND", "SD",
  "UT", "CO", "AZ", "WA", "WY", "NC", "SC", "VT", "MN", "ME"
};

/* random Companies */
char *company_names [NUM_COMPANY] = {
  "Ektelon", "Head", "Spalding", "Wilson", "Johnson", 
  "Gemini", "E-Force", "Pro Kennex", "Gosen", "Marty Hogan",
  "CourtPlay", "Swain", "Ellis", "Genesis", "Santa Anne Company",
  "First USA", "Nike", "Reebok", "Nelson", "Beefcake",
  "Bubble Face", "Skip Shot", "Roll Out", "Pinch", "Z-Ball"
};

/* RandChar -- Function that returns a random Character */
char RandChar(void) 
{
  return ((rand() % 26) + 'a');
}

/* RandDig -- returns a random digit */
char RandDig(void) 
{
  return((rand() % 10) + '0');
}

/* RandInt -- returns a random integer between 0 and one below the 
 * ---------- the inputted character */
int RandInt(int oneAboveMax) 
{
  return(rand() % oneAboveMax);
}

/* RandomName -- place a random name in the given array that are 
 * ------------- taken from the random first and last names constant
 * arrays above */
void RandomName(char *name) 
{
  int num, len, i;
  char *first;
  char *last;

  num = RandInt(NUM_FIRST);
  first = first_names[num];
  num = RandInt(NUM_LAST);
  last = last_names[num];

  strcpy(name, first);
  len = strlen(first);
  name[len] = ' ';
  for(i = len+1; *last != '\0' ; i++) {
    name[i] = *last;
    last++;
  }
  name[i] = '\0';
}

/* RandomDecimal -- place a random integer (from digits) into the 
 * ---------------- numChars - 1 number of characters of the dec
 * array. */
void RandomDecimal(char *dec, int numChars)
{
  int i;
  for(i = 0; i < numChars - 1; i++) {
    dec[i] = RandDig();
  }
  dec[i] = '\0';
}

/* OpenFile -- open a file and make sure that it is ok -- then
 * ----------- return the file pointer */
FILE *OpenFile(char *name)
{
  FILE *file;
  
  file = fopen(name, "w");
  if(file == 0) {
    fprintf(stderr, "Can't open file %s\n", name);
    exit(-1);
  }
  return(file);
}

/* PrintAddress -- Print a single address to me file specified */
void PrintAddress(FILE *outfile)
{
  int num;
  char word[50];

  num = RandInt(MAX_STREET_NUM);
  if(num <= 2) num = 3;
  RandomDecimal(word, num);
  fprintf(outfile, "%s,", word);
  num = RandInt(NUM_STREETS);
  fprintf(outfile, "%s,", streets[num]);
  num = RandInt(NUM_CITIES);
  fprintf(outfile, "%s,", cities[num]);
  num = RandInt(NUM_STATES);
  fprintf(outfile, "%s,", states[num]);
}

/* FillAndPrintCompanies -- Get the info (some random and some set
 * ------------------------ in the const array above called company_
 * names) for the companies and place it in the allotted file for
 * this information in the form of the relations for Company */
void FillAndPrintCompanies(char *companies[])
{
  FILE *outfile;
  int num, i;
  char word[31];

  outfile = OpenFile("company.load.txt");
  for(i = 0; i < NUM_COMPANY; i++) {
    fprintf(outfile, "%s,", companies[i]);
    PrintAddress(outfile);
    RandomDecimal(word, 12);
    word[8] = '.';
    fprintf(outfile, "%s\n", word);
  }
  fclose(outfile);
}

/* FillAndPrintDealers -- Print out the dealer information */
void FillAndPrintDealers(char *companies[], char *dealers[])
{
  FILE *outfile;
  int num, i;
  char word[31];

  outfile = OpenFile("dealer.load.txt");
  for(i = 0; i < NUM_DEALER; i++) {
    /* assign this dealer to a random company -- need this info later */
    dealers[i] = companies[RandInt(NUM_COMPANY)];
    RandomName(word);
    fprintf(outfile, "%d,%s,", i+1, word);
    PrintAddress(outfile);
    RandomDecimal(word, 10);
    word[6] = '.';
    fprintf(outfile, "%s,%s\n", word, dealers[i]);
  }
  fclose(outfile);
}

/* FillAndPrintStores -- Fill out and print out the Store information */
void FillAndPrintStores(void)
{
  FILE *outfile;
  int num, i;
  char word[31];

  outfile = OpenFile("store.load.txt");
  for(i = 0; i < NUM_STORE; i++) {
    num = RandInt(NUM_STORE_NAMES);
    fprintf(outfile, "%d,%s,", i+1, store_names[num]);
    PrintAddress(outfile);
    RandomDecimal(word, 6);
    word[2] = '.';
    fprintf(outfile, "%s\n", word);
  }
  fclose(outfile);
}

/* FillAndPrintDealerStores -- assign all the stores to dealers
 * --------------------------- randomly */
void FillAndPrintDealerStore(DealerStore deal_store[])
{
  FILE *outfile;
  int num, i, j, seen;
  char word[31];

  outfile = OpenFile("dealerstore.load.txt");
  for(i = 0; i < NUM_DEALERSTORE; i++) {
    /* choose a random store and dealer */
    deal_store[i].storeID = RandInt(NUM_STORE) + 1;
    deal_store[i].dealerID = RandInt(NUM_DEALER) + 1;

    /* make sure that this pair does not already exist */
    seen = 0;
    for(j = 0; j < i; j++) {
      if(deal_store[j].storeID == deal_store[i].storeID &&
	 deal_store[j].dealerID == deal_store[i].dealerID) {
	seen = 1;
	break;
      }
    }

    /* if we have seen this before then go back and do this
     * iteration again -- if not then print it out */
    if(!seen) {
      fprintf(outfile, "%d,%d\n", deal_store[i].storeID,
	      deal_store[i].dealerID);
    } else {
      i--;
    }
  }
  fclose(outfile);
}

/* FillAndPrintPlayer -- assign all players randomly */
void FillAndPrintPlayers(int players[])
{
  FILE *outfile;
  int num, i;
  char word[31];

  outfile = OpenFile("player.load.txt");

  /* first fill the player to all have no sponsorships */
  for(i = 0; i < NUM_PLAYER; i++) players[i] = 0;

  /* now fill up the 1 in 20 that get sponsorships and give
   * them random types of sponsorships */
  for(i = 0; i < NUM_COMPANYSPONSORED; i++) {
    num = RandInt(NUM_PLAYER);
    if(players[num] == 0) {
      players[num] = RandInt(3) + 1;
    } else {
      i--;
    }
  }

  /* now fill in the rest of the information to do with the player */
  for(i = 0; i < NUM_PLAYER; i++) {
    RandomName(word);
    fprintf(outfile, "%d,%s,", i+1, word);
    PrintAddress(outfile);
    fprintf(outfile, "%d\n", players[i]);
  }
  fclose(outfile);
}

/* FillAndPrintCompanySponsored -- match up players with their
 * ------------------------------- companies if they have
 * sponsorships */
void FillAndPrintCompanySponsored(char *companies[], 
				  int players[],
				  CompanySponsored con_spons[])
{
  FILE *outfile;
  int i, j;

  outfile = OpenFile("companysponsored.load.txt");
  j = 0;
  for(i = 0; i < NUM_PLAYER; i++) {
    if(players[i] != 0) {
      con_spons[j].AARAmembership = i+1;
      con_spons[j].company = companies[RandInt(NUM_COMPANY)];
      fprintf(outfile, "%s,%d\n", con_spons[j].company,
	      con_spons[j].AARAmembership);
      j++;
    }
  }
  if(j != NUM_COMPANYSPONSORED) 
    fprintf(stderr, "Wrong number of sponsored players\n");
  fclose(outfile);
}

/* FillAndPrintRacquets -- Fill in the racquets -- this is by far
 * ----------------------- the hardest thing because we have all
 * the relational information between players and such to be added
 * here and we have to hook up everyone to racquets as well as to
 * other things */
void FillAndPrintRacquets(char *dealers[], int players[], 
			  CompanySponsored com_spons[], 
			  DealerStore deal_store[])
{
  FILE *outfile;
  int i, j, num, dealerID, storeID, model, dealerStore;
  char *company;
  char price[31];

  outfile = OpenFile("racquet.load.txt");

  /* outter loop goes through all the racquets */
  for(i = 0; i < NUM_RACQUET; i++) {

    /* find a random player to belong to this racquet */
    num = RandInt(NUM_PLAYER);
    fprintf(outfile, "%d,", i+1);

    /* this case the player is not sponsored so the racquet it bought */
    if(players[num] == 0) {
      /* chose a random dealer store pair */
      dealerStore = RandInt(NUM_DEALERSTORE);
      /* chose a random model and price the model according to it's 
       * model (the higher model price the higher the price by a factor
       * of ten */
      model = RandInt(4)+1;
      RandomDecimal(price, model+4);
      price[model] = '.';
      dealerID = deal_store[dealerStore].dealerID;
      storeID = deal_store[dealerStore].storeID;
      /* get what company from the dealerID */
      company = dealers[dealerID - 1];

    /* this case is for when the player is sponsored */
    } else {
      /* chose a model */
      model = RandInt(4)+1;
      /* fill in the empty price */
      price[0] = '0';
      price[1] = '.';
      price[2] = '0';
      price[3] = '0';
      price[4] = '\0';
      /* there is no dealer or storeID for this racquet */
      dealerID = 0;
      storeID = 0;

      /* now go through the sponsored players and find the company
       * that this person is associated with so that we can 
       * get the name of the model from the model number and the
       * company. */
      for(j = 0; j < NUM_COMPANYSPONSORED; j++) {
	if(com_spons[j].AARAmembership == num+1) {
	  company = com_spons[j].company;
	  break;
	}
      }
      if(j == NUM_COMPANYSPONSORED) 
	fprintf(stderr, "Can't find a player in sponsorship list\n");
    }
    /* print out rest of the info */
    fprintf(outfile, "%s model %d,%s,%d,%d,%d\n", company, 
	    model, price, dealerID, storeID, num+1);

  }
  fclose(outfile);
}

main()
{
  char *dealers[NUM_DEALER]; /* contains name of company */
  int players[NUM_PLAYER]; /* contains sponsorship type */

  /* company/player pairs of sponsorship contracts */
  CompanySponsored com_spons[NUM_COMPANYSPONSORED]; 

  /* contains dealer/store pairs */
  DealerStore deal_store[NUM_DEALERSTORE]; 


  FillAndPrintCompanies(company_names);
  FillAndPrintDealers(company_names, dealers);
  FillAndPrintStores();
  FillAndPrintDealerStore(deal_store);
  FillAndPrintPlayers(players);
  FillAndPrintCompanySponsored(company_names, players, com_spons);
  FillAndPrintRacquets(dealers, players, com_spons, deal_store);
}
