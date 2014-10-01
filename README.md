A mini relational databased for the CS346 Stanford class. See the website for more details: http://web.stanford.edu/class/cs346/project.html

To compile, type "make".

To create a database:
./dbcreate "dbname"

To use the database created:
./redbase "dbname"

To destroy the database:
./dedestroy "dbname"

My implementation documentation for each layer can be found in the "doc" folder.

The additional component I implemented was a dynamic programming query optimizer. The documentation is in the pdf labeled query_optimizer_DOC.
