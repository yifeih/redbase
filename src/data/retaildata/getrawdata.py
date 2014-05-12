#!/usr/bin/python

import pgdb

db = pgdb.connect()

c = db.cursor()

import sys

tabletodump = sys.argv[ 1 ]
   
c.execute( "select * from %s" % tabletodump );

for row in c.fetchall():
   print ",".join( [ str( x ).strip() for x in row ] )


