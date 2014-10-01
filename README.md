redbase
=======

A mini relational databased for the CS346 Stanford class. See the website for more details: http://web.stanford.edu/class/cs346/project.html

To compile, type "make".

To create a database:
./dbcreate "dbname"

To use the database created:
./redbase "dbname"

To destroy the database:
./dbdestroy "dbname"

My implementation documentation for each layer can be found in the "doc" folder.

Here is a VERY high level overview for the "parts" of the system:
RM - Record Manager
The Record manager organizes pages into databases. It keeps a header page with
informaiton about the record sizes, the number of pages in the table, etc. Each
page mantains information about how the records are stored on the page. There
is also an iterator for scanning through the records of the table

IX - Index Layer
This layer holds the index layer. It manages a set of pages that store the nodes
of the B+ tree, and implements the logic of insertion, deletion, and search. There
is also an iterator for retrieving indices, with or without conditions.

SM - System Management
Files involved with this have to do with creating, destroying and manipulating the
database. When the appropriate instructions are given by the user to redbase,
SM will organize the files for keeping track of databases, tables, indices. It
also deals with loading data into databases.

QL - Query Layer
This is the most interesting part of the system. It takes the output of the parser
(that was given to us) and implements the appropraite SQL query. Details of what
is implemented is in http://web.stanford.edu/class/cs346/ql.html.
The QL layer constructs the query tree plan, and executes them with iterators. It
constructs the tree with objects that I've called "nodes" in the files, and
keeps a pointer to the root. Then, when a record is requested from the root, it
recursively calls for the next record from the leaves.

The additional component I implemented was a dynamic programming query optimizer.
The documentation is in the pdf labeled query_optimizer_DOC.

It is a simplified version of the Selinger Query Optimizer that uses only
dynamic programming. It keeps an estimate of the number of records and unique
values of each attribute for each table, and uses estimates to determine
the order of the join. See query_optimizer_DOC for more details! :)
