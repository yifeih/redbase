#include <fstream>
#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>

// 15
static char *firstName[] = {
  "Wendy", "William", "James", "Joyce", "Jack", 
  "Harry", "Mike", "Bill", "Hermione", "Frodo",
  "Sauron", "Gandalf", "Elrond", "Evelyn", "Michelle"
};

// 15
static char *lastName[] = {
  "Smith", "Clinton", "Kennedy", "Baggins", "Proudfeet",
  "Granger", "Potter", "Snipes", "Cruise", "Kidman",
  "Honggowibowo", "Bin laden", "Taktak", "Bush", "Atkins"
};

// 10
static char *deptName[] = {
  "Sales", "Marketing", "IT", "Research", "Finance",
  "Software Engineering", "Security", "Human Resources", "Statistics", "Accounting"
};

// 10
static char *relation[] = {
  "wife", "husband", "son", "daughter", "father",
  "mother", "brother", "sister", "uncle", "auntie"
};

// 20
static char *itemName[] = {
  "soft toy", "board game", "electronic game", "PS2", "iPod",
  "Nintendo 64 Gamecube", "Sega Dreamcast", "Microsoft XBOX", "GameBoy", "PS1",
  "Barbie dolls", "Cruise package", "Plane tickets", "House", "Apartment",
  "Car", "Luxury car", "Computer", "Laptop", "Speakers"
};
  
// 10
static char *projDesc[] = {
  "To improve network efficiency", "To minimize cost", 
  "Find more accounting loopholes to exploit", "Increase revenue",
  "Increase profit", "Reduce cost", "Increase employee efficiency",
  "Find cost cutting measures", "Find more customers", 
  "Extract more revenue from existing customers"
};

#define MAXSALARY   200000
#define PROJECTS    10
#define DEPTNAME    10
#define RELATION    10
#define FIRSTNAME   15
#define LASTNAME    15
#define ITEMNAME    20

#define NENTRIES    100000


int values[NENTRIES];

//
// Create an array of random nos. between 0 and n-1, without duplicates.
// put the nos. in values[]
//
void ran(int n, int randomize=true)
{
   int i, r, t, m;

   // Initialize values array 
   for (i = 0; i < NENTRIES; i++)
      values[i] = i;

   if (!randomize)
     return;

   printf("Randomizing... \n");

   // Randomize first n entries in values array
   for (i = 0, m = n; i < n-1; i++) {
      r = (int)(rand() % m--);
      t = values[m];
      values[m] = values[r];
      values[r] = t;
   }

}

//
// main
//
int main(int argc, char *argv[])
{
  
  int emp = 100;
  int dept = DEPTNAME;
  int sales = 1000;
  int items = 200;
  int emp_health = emp;
  int dep = emp * 3;
  int dep_health = dep;
  int managers = emp / 10;
  int projects = PROJECTS;
  int assigned = emp * 3;

  int random = 1;
  
  ofstream file;
  file.open("emp.data");
  if (random) ran(emp);
  for (int i = 0; i < emp; i++) {
    file << values[i] << "," 
	 << firstName[rand() % FIRSTNAME] << ","
	 << lastName[rand() % LASTNAME] << ","
	 << rand() % dept << ","
	 << (rand() * rand()) % MAXSALARY << ","
	 << rand() % managers << endl;
  }
  file.close();

  file.open("dept.data");
  if (random) ran(dept);
  for (int i = 0; i < dept; i++) {
    file << values[i] << "," 
	 << deptName[i] << ","
	 << rand() % managers << endl;    // dept head is one of the managers

  }
  file.close();

  file.open("sales.data");
  if (random) ran(sales);
  for (int i = 0; i < sales; i++) {
    file << values[i] << "," 
	 << rand() % emp << ","
	 << rand() % items << ","
	 << (rand() % 1000)/10.0 << endl;   // date as 0 to 99.9, 100 days

  }
  file.close();

  file.open("items.data");
  if (random) ran(items);
  for (int i = 0; i < items; i++) {
    file << values[i] << "," 
	 << itemName[rand() % ITEMNAME] << ","
	 << ((float)(rand() % 50000))/10.0 << endl; // price

  }
  file.close();

  int r;
  file.open("emph.data");
  if (random) ran(emp_health);
  for (int i = 0; i < emp_health; i++) {
    r = rand() % 2;
    file << values[i] << ",";
    if (r)
      file << "yes," << ((float)(rand() % 5000))/10.0 << endl;
    else 
      file << "no," << 0.0 << endl;
    
  }
  file.close();

  file.open("dep.data");
  if (random) ran(dep);
  for (int i = 0; i < dep; i++) {
    file << values[i] << ","
	 << rand() % emp << "," 
      	 << relation[rand() % RELATION] << endl;
  }
  file.close();

  file.open("deph.data");
  if (random) ran(dep_health);
  for (int i = 0; i < dep_health; i++) {
    r = rand() % 2;
    file << values[i] << ",";
    if (r)
      file << "yes," << ((float)(rand() % 5000))/10.0 << endl;
    else 
      file << "no," << 0.0 << endl;
  }
  file.close();
  
  file.open("managers.data");
  if (random) ran(managers);
  for (int i = 0; i < managers; i++) {
    file << values[i] << "," 
	 << rand() % dept << endl;
  }
  file.close();

  file.open("projects.data");
  if (random) ran(projects);
  for (int i = 0; i < projects; i++) {
    file << values[i] << "," 
	 << projDesc[i] << ","
	 << 1 + rand() % 20 << ","
	 << ((float)(rand() % 1000))/10.0 << endl; // up to 100.0%
  }
  file.close();

  file.open("assigned.data");
  if (random) ran(assigned);
  for (int i = 0; i < assigned; i++) {
    file << rand() % emp << "," 
	 << rand() % projects << endl;
  }
  file.close();

  return 0;

}
