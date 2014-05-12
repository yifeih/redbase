#!/usr/bin/python

minpeople = 1
maxpeople = 15

minitem = 100
maxitem = 120

cities = []
cities.append( "San Francisco" );
cities.append( "Stanford" );
cities.append( "Mountain View" );
cities.append( "Oakland" );
cities.append( "Palo Alto" );

numrows = 100000

import random
import math

for i in xrange( numrows ):
   person = random.randint( minpeople, maxpeople );
   item = random.randint( minitem, maxitem );
   city = cities[ random.randint( 0, len( cities ) - 1 ) ]
   quantity = abs( random.gauss( 0, 1 ) ) + 1;
   pid = i
  
   print "%d,%d,%s,%d,%d" % ( person, item, city, quantity, pid )

