/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison interface for Yacc-like parsers in C
   
      Copyright (C) 1984, 1989-1990, 2000-2011 Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     RW_CREATE = 258,
     RW_DROP = 259,
     RW_TABLE = 260,
     RW_INDEX = 261,
     RW_LOAD = 262,
     RW_SET = 263,
     RW_HELP = 264,
     RW_PRINT = 265,
     RW_EXIT = 266,
     RW_SELECT = 267,
     RW_FROM = 268,
     RW_WHERE = 269,
     RW_INSERT = 270,
     RW_DELETE = 271,
     RW_UPDATE = 272,
     RW_AND = 273,
     RW_INTO = 274,
     RW_VALUES = 275,
     T_EQ = 276,
     T_LT = 277,
     T_LE = 278,
     T_GT = 279,
     T_GE = 280,
     T_NE = 281,
     T_EOF = 282,
     NOTOKEN = 283,
     RW_RESET = 284,
     RW_IO = 285,
     RW_BUFFER = 286,
     RW_RESIZE = 287,
     RW_QUERY_PLAN = 288,
     RW_ON = 289,
     RW_OFF = 290,
     T_INT = 291,
     T_REAL = 292,
     T_STRING = 293,
     T_QSTRING = 294,
     T_SHELL_CMD = 295
   };
#endif
/* Tokens.  */
#define RW_CREATE 258
#define RW_DROP 259
#define RW_TABLE 260
#define RW_INDEX 261
#define RW_LOAD 262
#define RW_SET 263
#define RW_HELP 264
#define RW_PRINT 265
#define RW_EXIT 266
#define RW_SELECT 267
#define RW_FROM 268
#define RW_WHERE 269
#define RW_INSERT 270
#define RW_DELETE 271
#define RW_UPDATE 272
#define RW_AND 273
#define RW_INTO 274
#define RW_VALUES 275
#define T_EQ 276
#define T_LT 277
#define T_LE 278
#define T_GT 279
#define T_GE 280
#define T_NE 281
#define T_EOF 282
#define NOTOKEN 283
#define RW_RESET 284
#define RW_IO 285
#define RW_BUFFER 286
#define RW_RESIZE 287
#define RW_QUERY_PLAN 288
#define RW_ON 289
#define RW_OFF 290
#define T_INT 291
#define T_REAL 292
#define T_STRING 293
#define T_QSTRING 294
#define T_SHELL_CMD 295




#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 2068 of yacc.c  */
#line 71 "parse.y"

    int ival;
    CompOp cval;
    float rval;
    char *sval;
    NODE *n;



/* Line 2068 of yacc.c  */
#line 140 "y.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


