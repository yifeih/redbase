/* A Bison parser, made by GNU Bison 2.5.  */

/* Bison implementation for Yacc-like parsers in C
   
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

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output.  */
#define YYBISON 1

/* Bison version.  */
#define YYBISON_VERSION "2.5"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 0

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1

/* Using locations.  */
#define YYLSP_NEEDED 0



/* Copy the first part of user declarations.  */

/* Line 268 of yacc.c  */
#line 1 "parse.y"

/*
 * parser.y: yacc specification for RQL
 *
 * Authors: Dallan Quass
 *          Jan Jannink
 *          Jason McHugh
 *
 * originally by: Mark McAuliffe, University of Wisconsin - Madison, 1991
 *
 * 1997: Added "print buffer", "print io", "reset io" and the "*" in
 * SFW Query.
 * 1998: Added "reset buffer", "resize buffer [int]", "queryplans on",
 * and "queryplans off".
 * 2000: Added "const" to yyerror-header
 *
 */

#include <cstdio>
#include <iostream>
#include <sys/types.h>
#include <cstdlib>
#include <unistd.h>
#include "redbase.h"
#include "parser_internal.h"
#include "pf.h"     // for PF_PrintError
#include "rm.h"     // for RM_PrintError
#include "ix.h"     // for IX_PrintError
#include "sm.h"
#include "ql.h"

using namespace std;

  // Added by Wendy Tobagus
#ifndef yyrestart
void yyrestart(FILE*);
#endif

// The PF_STATS indicates that we will be tracking statistics for the PF
// Layer.  The Manager is defined within pf_buffermgr.cc.  
// We include it within the parser so that a system command can display
// statistics about the DB.
#ifdef PF_STATS
#include "statistics.h"

// This is defined within the pf_buffermgr.cc
extern StatisticsMgr *pStatisticsMgr;

#endif    // PF_STATS

/*
 * string representation of tokens; provided by scanner
 */
extern char *yytext;

/*
 * points to root of parse tree
 */
static NODE *parse_tree;

int bExit;                 // when to return from RBparse

int bQueryPlans;           // When to print the query plans

PF_Manager *pPfm;          // PF component manager
SM_Manager *pSmm;          // SM component manager
QL_Manager *pQlm;          // QL component manager



/* Line 268 of yacc.c  */
#line 142 "y.tab.c"

/* Enabling traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif

/* Enabling verbose error messages.  */
#ifdef YYERROR_VERBOSE
# undef YYERROR_VERBOSE
# define YYERROR_VERBOSE 1
#else
# define YYERROR_VERBOSE 0
#endif

/* Enabling the token table.  */
#ifndef YYTOKEN_TABLE
# define YYTOKEN_TABLE 0
#endif


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

/* Line 293 of yacc.c  */
#line 71 "parse.y"

    int ival;
    CompOp cval;
    float rval;
    char *sval;
    NODE *n;



/* Line 293 of yacc.c  */
#line 268 "y.tab.c"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif


/* Copy the second part of user declarations.  */


/* Line 343 of yacc.c  */
#line 280 "y.tab.c"

#ifdef short
# undef short
#endif

#ifdef YYTYPE_UINT8
typedef YYTYPE_UINT8 yytype_uint8;
#else
typedef unsigned char yytype_uint8;
#endif

#ifdef YYTYPE_INT8
typedef YYTYPE_INT8 yytype_int8;
#elif (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
typedef signed char yytype_int8;
#else
typedef short int yytype_int8;
#endif

#ifdef YYTYPE_UINT16
typedef YYTYPE_UINT16 yytype_uint16;
#else
typedef unsigned short int yytype_uint16;
#endif

#ifdef YYTYPE_INT16
typedef YYTYPE_INT16 yytype_int16;
#else
typedef short int yytype_int16;
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif ! defined YYSIZE_T && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned int
# endif
#endif

#define YYSIZE_MAXIMUM ((YYSIZE_T) -1)

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YYUSE(e) ((void) (e))
#else
# define YYUSE(e) /* empty */
#endif

/* Identity function, used to suppress warnings about constant conditions.  */
#ifndef lint
# define YYID(n) (n)
#else
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static int
YYID (int yyi)
#else
static int
YYID (yyi)
    int yyi;
#endif
{
  return yyi;
}
#endif

#if ! defined yyoverflow || YYERROR_VERBOSE

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's `empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (YYID (0))
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
	     && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS && (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* ! defined yyoverflow || YYERROR_VERBOSE */


#if (! defined yyoverflow \
     && (! defined __cplusplus \
	 || (defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yytype_int16 yyss_alloc;
  YYSTYPE yyvs_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (sizeof (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (sizeof (yytype_int16) + sizeof (YYSTYPE)) \
      + YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)				\
    do									\
      {									\
	YYSIZE_T yynewbytes;						\
	YYCOPY (&yyptr->Stack_alloc, Stack, yysize);			\
	Stack = &yyptr->Stack_alloc;					\
	yynewbytes = yystacksize * sizeof (*Stack) + YYSTACK_GAP_MAXIMUM; \
	yyptr += yynewbytes / sizeof (*yyptr);				\
      }									\
    while (YYID (0))

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from FROM to TO.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(To, From, Count) \
      __builtin_memcpy (To, From, (Count) * sizeof (*(From)))
#  else
#   define YYCOPY(To, From, Count)		\
      do					\
	{					\
	  YYSIZE_T yyi;				\
	  for (yyi = 0; yyi < (Count); yyi++)	\
	    (To)[yyi] = (From)[yyi];		\
	}					\
      while (YYID (0))
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  65
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   110

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  47
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  38
/* YYNRULES -- Number of rules.  */
#define YYNRULES  78
/* YYNRULES -- Number of states.  */
#define YYNSTATES  137

/* YYTRANSLATE(YYLEX) -- Bison symbol number corresponding to YYLEX.  */
#define YYUNDEFTOK  2
#define YYMAXUTOK   295

#define YYTRANSLATE(YYX)						\
  ((unsigned int) (YYX) <= YYMAXUTOK ? yytranslate[YYX] : YYUNDEFTOK)

/* YYTRANSLATE[YYLEX] -- Bison symbol number corresponding to YYLEX.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
      42,    43,    45,     2,    44,     2,    46,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,    41,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40
};

#if YYDEBUG
/* YYPRHS[YYN] -- Index of the first RHS symbol of rule number YYN in
   YYRHS.  */
static const yytype_uint8 yyprhs[] =
{
       0,     0,     3,     6,     8,    10,    12,    14,    16,    18,
      20,    22,    24,    26,    28,    30,    32,    34,    36,    38,
      40,    42,    44,    46,    48,    50,    52,    55,    58,    61,
      64,    68,    71,    74,    81,    88,    92,    99,   105,   110,
     113,   116,   118,   124,   132,   137,   145,   149,   151,   154,
     156,   158,   162,   164,   168,   170,   174,   176,   178,   181,
     183,   187,   189,   193,   195,   197,   201,   203,   205,   207,
     209,   211,   213,   215,   217,   219,   221,   223,   225
};

/* YYRHS -- A `-1'-separated list of the rules' RHS.  */
static const yytype_int8 yyrhs[] =
{
      48,     0,    -1,    49,    41,    -1,    40,    -1,     1,    -1,
      27,    -1,    50,    -1,    51,    -1,    52,    -1,    84,    -1,
      56,    -1,    57,    -1,    58,    -1,    59,    -1,    65,    -1,
      66,    -1,    67,    -1,    68,    -1,    60,    -1,    64,    -1,
      61,    -1,    62,    -1,    63,    -1,    54,    -1,    55,    -1,
      53,    -1,    33,    34,    -1,    33,    35,    -1,    29,    31,
      -1,    10,    31,    -1,    32,    31,    36,    -1,    10,    30,
      -1,    29,    30,    -1,     3,     5,    38,    42,    69,    43,
      -1,     3,     6,    38,    42,    38,    43,    -1,     4,     5,
      38,    -1,     4,     6,    38,    42,    38,    43,    -1,     7,
      38,    42,    39,    43,    -1,     8,    38,    21,    39,    -1,
       9,    82,    -1,    10,    38,    -1,    11,    -1,    12,    71,
      13,    74,    76,    -1,    15,    19,    38,    20,    42,    80,
      43,    -1,    16,    13,    38,    76,    -1,    17,    38,     8,
      73,    21,    79,    76,    -1,    70,    44,    69,    -1,    70,
      -1,    38,    38,    -1,    72,    -1,    45,    -1,    73,    44,
      72,    -1,    73,    -1,    38,    46,    38,    -1,    38,    -1,
      75,    44,    74,    -1,    75,    -1,    38,    -1,    14,    77,
      -1,    84,    -1,    78,    18,    77,    -1,    78,    -1,    73,
      83,    79,    -1,    73,    -1,    81,    -1,    81,    44,    80,
      -1,    81,    -1,    39,    -1,    36,    -1,    37,    -1,    38,
      -1,    84,    -1,    22,    -1,    23,    -1,    24,    -1,    25,
      -1,    21,    -1,    26,    -1,    -1
};

/* YYRLINE[YYN] -- source line where rule number YYN was defined.  */
static const yytype_uint16 yyrline[] =
{
       0,   162,   162,   167,   177,   183,   192,   193,   194,   195,
     202,   203,   204,   205,   209,   210,   211,   212,   216,   217,
     218,   219,   220,   221,   222,   223,   227,   233,   244,   252,
     257,   265,   276,   289,   296,   303,   310,   317,   325,   332,
     339,   346,   354,   361,   368,   375,   382,   386,   393,   400,
     401,   408,   412,   419,   423,   430,   434,   441,   448,   452,
     459,   463,   470,   477,   481,   488,   492,   499,   503,   507,
     514,   518,   525,   529,   533,   537,   541,   545,   551
};
#endif

#if YYDEBUG || YYERROR_VERBOSE || YYTOKEN_TABLE
/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "$end", "error", "$undefined", "RW_CREATE", "RW_DROP", "RW_TABLE",
  "RW_INDEX", "RW_LOAD", "RW_SET", "RW_HELP", "RW_PRINT", "RW_EXIT",
  "RW_SELECT", "RW_FROM", "RW_WHERE", "RW_INSERT", "RW_DELETE",
  "RW_UPDATE", "RW_AND", "RW_INTO", "RW_VALUES", "T_EQ", "T_LT", "T_LE",
  "T_GT", "T_GE", "T_NE", "T_EOF", "NOTOKEN", "RW_RESET", "RW_IO",
  "RW_BUFFER", "RW_RESIZE", "RW_QUERY_PLAN", "RW_ON", "RW_OFF", "T_INT",
  "T_REAL", "T_STRING", "T_QSTRING", "T_SHELL_CMD", "';'", "'('", "')'",
  "','", "'*'", "'.'", "$accept", "start", "command", "ddl", "dml",
  "utility", "queryplans", "buffer", "statistics", "createtable",
  "createindex", "droptable", "dropindex", "load", "set", "help", "print",
  "exit", "query", "insert", "delete", "update", "non_mt_attrtype_list",
  "attrtype", "non_mt_select_clause", "non_mt_relattr_list", "relattr",
  "non_mt_relation_list", "relation", "opt_where_clause",
  "non_mt_cond_list", "condition", "relattr_or_value", "non_mt_value_list",
  "value", "opt_relname", "op", "nothing", 0
};
#endif

# ifdef YYPRINT
/* YYTOKNUM[YYLEX-NUM] -- Internal token number corresponding to
   token YYLEX-NUM.  */
static const yytype_uint16 yytoknum[] =
{
       0,   256,   257,   258,   259,   260,   261,   262,   263,   264,
     265,   266,   267,   268,   269,   270,   271,   272,   273,   274,
     275,   276,   277,   278,   279,   280,   281,   282,   283,   284,
     285,   286,   287,   288,   289,   290,   291,   292,   293,   294,
     295,    59,    40,    41,    44,    42,    46
};
# endif

/* YYR1[YYN] -- Symbol number of symbol that rule YYN derives.  */
static const yytype_uint8 yyr1[] =
{
       0,    47,    48,    48,    48,    48,    49,    49,    49,    49,
      50,    50,    50,    50,    51,    51,    51,    51,    52,    52,
      52,    52,    52,    52,    52,    52,    53,    53,    54,    54,
      54,    55,    55,    56,    57,    58,    59,    60,    61,    62,
      63,    64,    65,    66,    67,    68,    69,    69,    70,    71,
      71,    72,    72,    73,    73,    74,    74,    75,    76,    76,
      77,    77,    78,    79,    79,    80,    80,    81,    81,    81,
      82,    82,    83,    83,    83,    83,    83,    83,    84
};

/* YYR2[YYN] -- Number of symbols composing right hand side of rule YYN.  */
static const yytype_uint8 yyr2[] =
{
       0,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     2,     2,     2,     2,
       3,     2,     2,     6,     6,     3,     6,     5,     4,     2,
       2,     1,     5,     7,     4,     7,     3,     1,     2,     1,
       1,     3,     1,     3,     1,     3,     1,     1,     2,     1,
       3,     1,     3,     1,     1,     3,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     0
};

/* YYDEFACT[STATE-NAME] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE doesn't specify something else to do.  Zero
   means the default is an error.  */
static const yytype_uint8 yydefact[] =
{
       0,     4,     0,     0,     0,     0,    78,     0,    41,     0,
       0,     0,     0,     5,     0,     0,     0,     3,     0,     0,
       6,     7,     8,    25,    23,    24,    10,    11,    12,    13,
      18,    20,    21,    22,    19,    14,    15,    16,    17,     9,
       0,     0,     0,     0,     0,     0,    70,    39,    71,    31,
      29,    40,    54,    50,     0,    49,    52,     0,     0,     0,
      32,    28,     0,    26,    27,     1,     2,     0,     0,    35,
       0,     0,     0,     0,     0,     0,     0,    78,     0,    30,
       0,     0,     0,     0,    38,    53,    57,    78,    56,    51,
       0,     0,    44,    59,     0,     0,     0,    47,     0,     0,
      37,    42,     0,     0,     0,    58,    61,     0,    48,    33,
       0,    34,    36,    55,    68,    69,    67,     0,    66,    76,
      72,    73,    74,    75,    77,     0,     0,    63,    78,    64,
      46,    43,     0,    62,    60,    45,    65
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
      -1,    18,    19,    20,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,    34,    35,    36,
      37,    38,    96,    97,    54,    55,    56,    87,    88,    92,
     105,   106,   128,   117,   118,    47,   125,    93
};

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
#define YYPACT_NINF -102
static const yytype_int8 yypact[] =
{
      11,  -102,     2,    40,   -33,   -29,    -8,   -28,  -102,   -34,
      22,    46,    23,  -102,   -14,    29,    15,  -102,    62,    24,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
      26,    28,    30,    31,    21,    49,  -102,  -102,  -102,  -102,
    -102,  -102,    27,  -102,    54,  -102,    33,    34,    36,    63,
    -102,  -102,    39,  -102,  -102,  -102,  -102,    37,    38,  -102,
      41,    42,    43,    47,    48,    50,    56,    64,    50,  -102,
      51,    52,    53,    44,  -102,  -102,  -102,    64,    55,  -102,
      58,    50,  -102,  -102,    71,    57,    59,    60,    65,    66,
    -102,  -102,    48,     0,    32,  -102,    75,    -4,  -102,  -102,
      51,  -102,  -102,  -102,  -102,  -102,  -102,    67,    61,  -102,
    -102,  -102,  -102,  -102,  -102,    -4,    50,  -102,    64,  -102,
    -102,  -102,     0,  -102,  -102,  -102,  -102
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int8 yypgoto[] =
{
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
    -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,  -102,
    -102,  -102,   -26,  -102,  -102,    19,   -78,    -6,  -102,   -86,
     -25,  -102,   -27,   -35,  -101,  -102,  -102,    25
};

/* YYTABLE[YYPACT[STATE-NUM]].  What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule which
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
#define YYTABLE_NINF -79
static const yytype_int16 yytable[] =
{
      94,   101,    49,    50,    52,    44,   129,    40,    41,    45,
      51,    53,     1,   104,     2,     3,    60,    61,     4,     5,
       6,     7,     8,     9,   129,    39,    10,    11,    12,   127,
      46,    48,   114,   115,    52,   116,   114,   115,    13,   116,
      14,    57,   135,    15,    16,    42,    43,   127,   104,    63,
      64,    17,   -78,   119,   120,   121,   122,   123,   124,    58,
      62,    59,    65,    71,    67,    66,    68,    74,    69,    70,
      72,    78,    76,    73,    77,    79,    90,    75,    91,    80,
      81,    83,    84,    82,   130,    85,    86,   100,    52,    95,
      98,    99,   107,   126,    89,   108,   113,   136,   133,   102,
     103,   134,   109,     0,   110,   132,     0,     0,   111,   112,
     131
};

#define yypact_value_is_default(yystate) \
  ((yystate) == (-102))

#define yytable_value_is_error(yytable_value) \
  YYID (0)

static const yytype_int16 yycheck[] =
{
      78,    87,    30,    31,    38,    38,   107,     5,     6,    38,
      38,    45,     1,    91,     3,     4,    30,    31,     7,     8,
       9,    10,    11,    12,   125,     0,    15,    16,    17,   107,
      38,     6,    36,    37,    38,    39,    36,    37,    27,    39,
      29,    19,   128,    32,    33,     5,     6,   125,   126,    34,
      35,    40,    41,    21,    22,    23,    24,    25,    26,    13,
      31,    38,     0,    42,    38,    41,    38,    13,    38,    38,
      21,     8,    38,    46,    38,    36,    20,    44,    14,    42,
      42,    39,    39,    42,   110,    38,    38,    43,    38,    38,
      38,    38,    21,    18,    75,    38,   102,   132,   125,    44,
      42,   126,    43,    -1,    44,    44,    -1,    -1,    43,    43,
      43
};

/* YYSTOS[STATE-NUM] -- The (internal number of the) accessing
   symbol of state STATE-NUM.  */
static const yytype_uint8 yystos[] =
{
       0,     1,     3,     4,     7,     8,     9,    10,    11,    12,
      15,    16,    17,    27,    29,    32,    33,    40,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    58,    59,
      60,    61,    62,    63,    64,    65,    66,    67,    68,    84,
       5,     6,     5,     6,    38,    38,    38,    82,    84,    30,
      31,    38,    38,    45,    71,    72,    73,    19,    13,    38,
      30,    31,    31,    34,    35,     0,    41,    38,    38,    38,
      38,    42,    21,    46,    13,    44,    38,    38,     8,    36,
      42,    42,    42,    39,    39,    38,    38,    74,    75,    72,
      20,    14,    76,    84,    73,    38,    69,    70,    38,    38,
      43,    76,    44,    42,    73,    77,    78,    21,    38,    43,
      44,    43,    43,    74,    36,    37,    39,    80,    81,    21,
      22,    23,    24,    25,    26,    83,    18,    73,    79,    81,
      69,    43,    44,    79,    77,    76,    80
};

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		(-2)
#define YYEOF		0

#define YYACCEPT	goto yyacceptlab
#define YYABORT		goto yyabortlab
#define YYERROR		goto yyerrorlab


/* Like YYERROR except do call yyerror.  This remains here temporarily
   to ease the transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  However,
   YYFAIL appears to be in use.  Nevertheless, it is formally deprecated
   in Bison 2.4.2's NEWS entry, where a plan to phase it out is
   discussed.  */

#define YYFAIL		goto yyerrlab
#if defined YYFAIL
  /* This is here to suppress warnings from the GCC cpp's
     -Wunused-macros.  Normally we don't worry about that warning, but
     some users do, and we want to make it easy for users to remove
     YYFAIL uses, which will produce warnings from Bison 2.5.  */
#endif

#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)					\
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    {								\
      yychar = (Token);						\
      yylval = (Value);						\
      YYPOPSTACK (1);						\
      goto yybackup;						\
    }								\
  else								\
    {								\
      yyerror (YY_("syntax error: cannot back up")); \
      YYERROR;							\
    }								\
while (YYID (0))


#define YYTERROR	1
#define YYERRCODE	256


/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#define YYRHSLOC(Rhs, K) ((Rhs)[K])
#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)				\
    do									\
      if (YYID (N))                                                    \
	{								\
	  (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;	\
	  (Current).first_column = YYRHSLOC (Rhs, 1).first_column;	\
	  (Current).last_line    = YYRHSLOC (Rhs, N).last_line;		\
	  (Current).last_column  = YYRHSLOC (Rhs, N).last_column;	\
	}								\
      else								\
	{								\
	  (Current).first_line   = (Current).last_line   =		\
	    YYRHSLOC (Rhs, 0).last_line;				\
	  (Current).first_column = (Current).last_column =		\
	    YYRHSLOC (Rhs, 0).last_column;				\
	}								\
    while (YYID (0))
#endif


/* This macro is provided for backward compatibility. */

#ifndef YY_LOCATION_PRINT
# define YY_LOCATION_PRINT(File, Loc) ((void) 0)
#endif


/* YYLEX -- calling `yylex' with the right arguments.  */

#ifdef YYLEX_PARAM
# define YYLEX yylex (YYLEX_PARAM)
#else
# define YYLEX yylex ()
#endif

/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)			\
do {						\
  if (yydebug)					\
    YYFPRINTF Args;				\
} while (YYID (0))

# define YY_SYMBOL_PRINT(Title, Type, Value, Location)			  \
do {									  \
  if (yydebug)								  \
    {									  \
      YYFPRINTF (stderr, "%s ", Title);					  \
      yy_symbol_print (stderr,						  \
		  Type, Value); \
      YYFPRINTF (stderr, "\n");						  \
    }									  \
} while (YYID (0))


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_value_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_value_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (!yyvaluep)
    return;
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# else
  YYUSE (yyoutput);
# endif
  switch (yytype)
    {
      default:
	break;
    }
}


/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_symbol_print (FILE *yyoutput, int yytype, YYSTYPE const * const yyvaluep)
#else
static void
yy_symbol_print (yyoutput, yytype, yyvaluep)
    FILE *yyoutput;
    int yytype;
    YYSTYPE const * const yyvaluep;
#endif
{
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

  yy_symbol_value_print (yyoutput, yytype, yyvaluep);
  YYFPRINTF (yyoutput, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_stack_print (yytype_int16 *yybottom, yytype_int16 *yytop)
#else
static void
yy_stack_print (yybottom, yytop)
    yytype_int16 *yybottom;
    yytype_int16 *yytop;
#endif
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)				\
do {								\
  if (yydebug)							\
    yy_stack_print ((Bottom), (Top));				\
} while (YYID (0))


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yy_reduce_print (YYSTYPE *yyvsp, int yyrule)
#else
static void
yy_reduce_print (yyvsp, yyrule)
    YYSTYPE *yyvsp;
    int yyrule;
#endif
{
  int yynrhs = yyr2[yyrule];
  int yyi;
  unsigned long int yylno = yyrline[yyrule];
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %lu):\n",
	     yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr, yyrhs[yyprhs[yyrule] + yyi],
		       &(yyvsp[(yyi + 1) - (yynrhs)])
		       		       );
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)		\
do {					\
  if (yydebug)				\
    yy_reduce_print (yyvsp, Rule); \
} while (YYID (0))

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args)
# define YY_SYMBOL_PRINT(Title, Type, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef	YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


#if YYERROR_VERBOSE

# ifndef yystrlen
#  if defined __GLIBC__ && defined _STRING_H
#   define yystrlen strlen
#  else
/* Return the length of YYSTR.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static YYSIZE_T
yystrlen (const char *yystr)
#else
static YYSIZE_T
yystrlen (yystr)
    const char *yystr;
#endif
{
  YYSIZE_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
#  endif
# endif

# ifndef yystpcpy
#  if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#   define yystpcpy stpcpy
#  else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static char *
yystpcpy (char *yydest, const char *yysrc)
#else
static char *
yystpcpy (yydest, yysrc)
    char *yydest;
    const char *yysrc;
#endif
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
#  endif
# endif

# ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYSIZE_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYSIZE_T yyn = 0;
      char const *yyp = yystr;

      for (;;)
	switch (*++yyp)
	  {
	  case '\'':
	  case ',':
	    goto do_not_strip_quotes;

	  case '\\':
	    if (*++yyp != '\\')
	      goto do_not_strip_quotes;
	    /* Fall through.  */
	  default:
	    if (yyres)
	      yyres[yyn] = *yyp;
	    yyn++;
	    break;

	  case '"':
	    if (yyres)
	      yyres[yyn] = '\0';
	    return yyn;
	  }
    do_not_strip_quotes: ;
    }

  if (! yyres)
    return yystrlen (yystr);

  return yystpcpy (yyres, yystr) - yyres;
}
# endif

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return 1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return 2 if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYSIZE_T *yymsg_alloc, char **yymsg,
                yytype_int16 *yyssp, int yytoken)
{
  YYSIZE_T yysize0 = yytnamerr (0, yytname[yytoken]);
  YYSIZE_T yysize = yysize0;
  YYSIZE_T yysize1;
  enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
  /* Internationalized format string. */
  const char *yyformat = 0;
  /* Arguments of yyformat. */
  char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];
  /* Number of reported tokens (one for the "unexpected", one per
     "expected"). */
  int yycount = 0;

  /* There are many possibilities here to consider:
     - Assume YYFAIL is not used.  It's too flawed to consider.  See
       <http://lists.gnu.org/archive/html/bison-patches/2009-12/msg00024.html>
       for details.  YYERROR is fine as it does not invoke this
       function.
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yytoken != YYEMPTY)
    {
      int yyn = yypact[*yyssp];
      yyarg[yycount++] = yytname[yytoken];
      if (!yypact_value_is_default (yyn))
        {
          /* Start YYX at -YYN if negative to avoid negative indexes in
             YYCHECK.  In other words, skip the first -YYN actions for
             this state because they are default actions.  */
          int yyxbegin = yyn < 0 ? -yyn : 0;
          /* Stay within bounds of both yycheck and yytname.  */
          int yychecklim = YYLAST - yyn + 1;
          int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
          int yyx;

          for (yyx = yyxbegin; yyx < yyxend; ++yyx)
            if (yycheck[yyx + yyn] == yyx && yyx != YYTERROR
                && !yytable_value_is_error (yytable[yyx + yyn]))
              {
                if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                  {
                    yycount = 1;
                    yysize = yysize0;
                    break;
                  }
                yyarg[yycount++] = yytname[yyx];
                yysize1 = yysize + yytnamerr (0, yytname[yyx]);
                if (! (yysize <= yysize1
                       && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
                  return 2;
                yysize = yysize1;
              }
        }
    }

  switch (yycount)
    {
# define YYCASE_(N, S)                      \
      case N:                               \
        yyformat = S;                       \
      break
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
# undef YYCASE_
    }

  yysize1 = yysize + yystrlen (yyformat);
  if (! (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM))
    return 2;
  yysize = yysize1;

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return 1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yyarg[yyi++]);
          yyformat += 2;
        }
      else
        {
          yyp++;
          yyformat++;
        }
  }
  return 0;
}
#endif /* YYERROR_VERBOSE */

/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

/*ARGSUSED*/
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
static void
yydestruct (const char *yymsg, int yytype, YYSTYPE *yyvaluep)
#else
static void
yydestruct (yymsg, yytype, yyvaluep)
    const char *yymsg;
    int yytype;
    YYSTYPE *yyvaluep;
#endif
{
  YYUSE (yyvaluep);

  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {

      default:
	break;
    }
}


/* Prevent warnings from -Wmissing-prototypes.  */
#ifdef YYPARSE_PARAM
#if defined __STDC__ || defined __cplusplus
int yyparse (void *YYPARSE_PARAM);
#else
int yyparse ();
#endif
#else /* ! YYPARSE_PARAM */
#if defined __STDC__ || defined __cplusplus
int yyparse (void);
#else
int yyparse ();
#endif
#endif /* ! YYPARSE_PARAM */


/* The lookahead symbol.  */
int yychar;

/* The semantic value of the lookahead symbol.  */
YYSTYPE yylval;

/* Number of syntax errors so far.  */
int yynerrs;


/*----------.
| yyparse.  |
`----------*/

#ifdef YYPARSE_PARAM
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void *YYPARSE_PARAM)
#else
int
yyparse (YYPARSE_PARAM)
    void *YYPARSE_PARAM;
#endif
#else /* ! YYPARSE_PARAM */
#if (defined __STDC__ || defined __C99__FUNC__ \
     || defined __cplusplus || defined _MSC_VER)
int
yyparse (void)
#else
int
yyparse ()

#endif
#endif
{
    int yystate;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus;

    /* The stacks and their tools:
       `yyss': related to states.
       `yyvs': related to semantic values.

       Refer to the stacks thru separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* The state stack.  */
    yytype_int16 yyssa[YYINITDEPTH];
    yytype_int16 *yyss;
    yytype_int16 *yyssp;

    /* The semantic value stack.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs;
    YYSTYPE *yyvsp;

    YYSIZE_T yystacksize;

  int yyn;
  int yyresult;
  /* Lookahead token as an internal (translated) token number.  */
  int yytoken;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;

#if YYERROR_VERBOSE
  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYSIZE_T yymsg_alloc = sizeof yymsgbuf;
#endif

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  yytoken = 0;
  yyss = yyssa;
  yyvs = yyvsa;
  yystacksize = YYINITDEPTH;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY; /* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */
  yyssp = yyss;
  yyvsp = yyvs;

  goto yysetstate;

/*------------------------------------------------------------.
| yynewstate -- Push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
 yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;

 yysetstate:
  *yyssp = yystate;

  if (yyss + yystacksize - 1 <= yyssp)
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYSIZE_T yysize = yyssp - yyss + 1;

#ifdef yyoverflow
      {
	/* Give user a chance to reallocate the stack.  Use copies of
	   these so that the &'s don't force the real ones into
	   memory.  */
	YYSTYPE *yyvs1 = yyvs;
	yytype_int16 *yyss1 = yyss;

	/* Each stack pointer address is followed by the size of the
	   data in use in that stack, in bytes.  This used to be a
	   conditional around just the two extra args, but that might
	   be undefined if yyoverflow is a macro.  */
	yyoverflow (YY_("memory exhausted"),
		    &yyss1, yysize * sizeof (*yyssp),
		    &yyvs1, yysize * sizeof (*yyvsp),
		    &yystacksize);

	yyss = yyss1;
	yyvs = yyvs1;
      }
#else /* no yyoverflow */
# ifndef YYSTACK_RELOCATE
      goto yyexhaustedlab;
# else
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
	goto yyexhaustedlab;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
	yystacksize = YYMAXDEPTH;

      {
	yytype_int16 *yyss1 = yyss;
	union yyalloc *yyptr =
	  (union yyalloc *) YYSTACK_ALLOC (YYSTACK_BYTES (yystacksize));
	if (! yyptr)
	  goto yyexhaustedlab;
	YYSTACK_RELOCATE (yyss_alloc, yyss);
	YYSTACK_RELOCATE (yyvs_alloc, yyvs);
#  undef YYSTACK_RELOCATE
	if (yyss1 != yyssa)
	  YYSTACK_FREE (yyss1);
      }
# endif
#endif /* no yyoverflow */

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;

      YYDPRINTF ((stderr, "Stack size increased to %lu\n",
		  (unsigned long int) yystacksize));

      if (yyss + yystacksize - 1 <= yyssp)
	YYABORT;
    }

  YYDPRINTF ((stderr, "Entering state %d\n", yystate));

  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;

/*-----------.
| yybackup.  |
`-----------*/
yybackup:

  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either YYEMPTY or YYEOF or a valid lookahead symbol.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token: "));
      yychar = YYLEX;
    }

  if (yychar <= YYEOF)
    {
      yychar = yytoken = YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);

  /* Discard the shifted token.  */
  yychar = YYEMPTY;

  yystate = yyn;
  *++yyvsp = yylval;

  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- Do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     `$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];


  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
        case 2:

/* Line 1806 of yacc.c  */
#line 163 "parse.y"
    {
      parse_tree = (yyvsp[(1) - (2)].n);
      YYACCEPT;
   }
    break;

  case 3:

/* Line 1806 of yacc.c  */
#line 168 "parse.y"
    {
      if (!isatty(0)) {
        cout << ((yyvsp[(1) - (1)].sval)) << "\n";
        cout.flush();
      }
      system((yyvsp[(1) - (1)].sval));
      parse_tree = NULL;
      YYACCEPT;
   }
    break;

  case 4:

/* Line 1806 of yacc.c  */
#line 178 "parse.y"
    {
      reset_scanner();
      parse_tree = NULL;
      YYACCEPT;
   }
    break;

  case 5:

/* Line 1806 of yacc.c  */
#line 184 "parse.y"
    {
      parse_tree = NULL;
      bExit = 1;
      YYACCEPT;
   }
    break;

  case 9:

/* Line 1806 of yacc.c  */
#line 196 "parse.y"
    {
      (yyval.n) = NULL;
   }
    break;

  case 26:

/* Line 1806 of yacc.c  */
#line 228 "parse.y"
    {
      bQueryPlans = 1;
      cout << "Query plan display turned on.\n";
      (yyval.n) = NULL;
   }
    break;

  case 27:

/* Line 1806 of yacc.c  */
#line 234 "parse.y"
    { 
      bQueryPlans = 0;
      cout << "Query plan display turned off.\n";
      (yyval.n) = NULL;
   }
    break;

  case 28:

/* Line 1806 of yacc.c  */
#line 245 "parse.y"
    {
      if (pPfm->ClearBuffer())
         cout << "Trouble clearing buffer!  Things may be pinned.\n";
      else 
         cout << "Everything kicked out of Buffer!\n";
      (yyval.n) = NULL;
   }
    break;

  case 29:

/* Line 1806 of yacc.c  */
#line 253 "parse.y"
    {
      pPfm->PrintBuffer();
      (yyval.n) = NULL;
   }
    break;

  case 30:

/* Line 1806 of yacc.c  */
#line 258 "parse.y"
    {
      pPfm->ResizeBuffer((yyvsp[(3) - (3)].ival));
      (yyval.n) = NULL;
   }
    break;

  case 31:

/* Line 1806 of yacc.c  */
#line 266 "parse.y"
    {
      #ifdef PF_STATS
         cout << "Statistics\n";
         cout << "----------\n";
         pStatisticsMgr->Print();
      #else
         cout << "Statitisics not compiled.\n";
      #endif
      (yyval.n) = NULL;
   }
    break;

  case 32:

/* Line 1806 of yacc.c  */
#line 277 "parse.y"
    {
      #ifdef PF_STATS
         cout << "Statistics reset.\n";
         pStatisticsMgr->Reset();
      #else
         cout << "Statitisics not compiled.\n";
      #endif
      (yyval.n) = NULL;
   }
    break;

  case 33:

/* Line 1806 of yacc.c  */
#line 290 "parse.y"
    {
      (yyval.n) = create_table_node((yyvsp[(3) - (6)].sval), (yyvsp[(5) - (6)].n));
   }
    break;

  case 34:

/* Line 1806 of yacc.c  */
#line 297 "parse.y"
    {
      (yyval.n) = create_index_node((yyvsp[(3) - (6)].sval), (yyvsp[(5) - (6)].sval));
   }
    break;

  case 35:

/* Line 1806 of yacc.c  */
#line 304 "parse.y"
    {
      (yyval.n) = drop_table_node((yyvsp[(3) - (3)].sval));
   }
    break;

  case 36:

/* Line 1806 of yacc.c  */
#line 311 "parse.y"
    {
      (yyval.n) = drop_index_node((yyvsp[(3) - (6)].sval), (yyvsp[(5) - (6)].sval));
   }
    break;

  case 37:

/* Line 1806 of yacc.c  */
#line 318 "parse.y"
    {
      (yyval.n) = load_node((yyvsp[(2) - (5)].sval), (yyvsp[(4) - (5)].sval));
   }
    break;

  case 38:

/* Line 1806 of yacc.c  */
#line 326 "parse.y"
    {
      (yyval.n) = set_node((yyvsp[(2) - (4)].sval), (yyvsp[(4) - (4)].sval));
   }
    break;

  case 39:

/* Line 1806 of yacc.c  */
#line 333 "parse.y"
    {
      (yyval.n) = help_node((yyvsp[(2) - (2)].sval));
   }
    break;

  case 40:

/* Line 1806 of yacc.c  */
#line 340 "parse.y"
    {
      (yyval.n) = print_node((yyvsp[(2) - (2)].sval));
   }
    break;

  case 41:

/* Line 1806 of yacc.c  */
#line 347 "parse.y"
    {
      (yyval.n) = NULL;
      bExit = 1;
   }
    break;

  case 42:

/* Line 1806 of yacc.c  */
#line 355 "parse.y"
    {
      (yyval.n) = query_node((yyvsp[(2) - (5)].n), (yyvsp[(4) - (5)].n), (yyvsp[(5) - (5)].n));
   }
    break;

  case 43:

/* Line 1806 of yacc.c  */
#line 362 "parse.y"
    {
      (yyval.n) = insert_node((yyvsp[(3) - (7)].sval), (yyvsp[(6) - (7)].n));
   }
    break;

  case 44:

/* Line 1806 of yacc.c  */
#line 369 "parse.y"
    {
      (yyval.n) = delete_node((yyvsp[(3) - (4)].sval), (yyvsp[(4) - (4)].n));
   }
    break;

  case 45:

/* Line 1806 of yacc.c  */
#line 376 "parse.y"
    {
      (yyval.n) = update_node((yyvsp[(2) - (7)].sval), (yyvsp[(4) - (7)].n), (yyvsp[(6) - (7)].n), (yyvsp[(7) - (7)].n));
   }
    break;

  case 46:

/* Line 1806 of yacc.c  */
#line 383 "parse.y"
    {
      (yyval.n) = prepend((yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
   }
    break;

  case 47:

/* Line 1806 of yacc.c  */
#line 387 "parse.y"
    {
      (yyval.n) = list_node((yyvsp[(1) - (1)].n));
   }
    break;

  case 48:

/* Line 1806 of yacc.c  */
#line 394 "parse.y"
    {
      (yyval.n) = attrtype_node((yyvsp[(1) - (2)].sval), (yyvsp[(2) - (2)].sval));
   }
    break;

  case 50:

/* Line 1806 of yacc.c  */
#line 402 "parse.y"
    {
       (yyval.n) = list_node(relattr_node(NULL, (char*)"*"));
   }
    break;

  case 51:

/* Line 1806 of yacc.c  */
#line 409 "parse.y"
    {
      (yyval.n) = prepend((yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
   }
    break;

  case 52:

/* Line 1806 of yacc.c  */
#line 413 "parse.y"
    {
      (yyval.n) = list_node((yyvsp[(1) - (1)].n));
   }
    break;

  case 53:

/* Line 1806 of yacc.c  */
#line 420 "parse.y"
    {
      (yyval.n) = relattr_node((yyvsp[(1) - (3)].sval), (yyvsp[(3) - (3)].sval));
   }
    break;

  case 54:

/* Line 1806 of yacc.c  */
#line 424 "parse.y"
    {
      (yyval.n) = relattr_node(NULL, (yyvsp[(1) - (1)].sval));
   }
    break;

  case 55:

/* Line 1806 of yacc.c  */
#line 431 "parse.y"
    {
      (yyval.n) = prepend((yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
   }
    break;

  case 56:

/* Line 1806 of yacc.c  */
#line 435 "parse.y"
    {
      (yyval.n) = list_node((yyvsp[(1) - (1)].n));
   }
    break;

  case 57:

/* Line 1806 of yacc.c  */
#line 442 "parse.y"
    {
      (yyval.n) = relation_node((yyvsp[(1) - (1)].sval));
   }
    break;

  case 58:

/* Line 1806 of yacc.c  */
#line 449 "parse.y"
    {
      (yyval.n) = (yyvsp[(2) - (2)].n);
   }
    break;

  case 59:

/* Line 1806 of yacc.c  */
#line 453 "parse.y"
    {
      (yyval.n) = NULL;
   }
    break;

  case 60:

/* Line 1806 of yacc.c  */
#line 460 "parse.y"
    {
      (yyval.n) = prepend((yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
   }
    break;

  case 61:

/* Line 1806 of yacc.c  */
#line 464 "parse.y"
    {
      (yyval.n) = list_node((yyvsp[(1) - (1)].n));
   }
    break;

  case 62:

/* Line 1806 of yacc.c  */
#line 471 "parse.y"
    {
      (yyval.n) = condition_node((yyvsp[(1) - (3)].n), (yyvsp[(2) - (3)].cval), (yyvsp[(3) - (3)].n));
   }
    break;

  case 63:

/* Line 1806 of yacc.c  */
#line 478 "parse.y"
    {
      (yyval.n) = relattr_or_value_node((yyvsp[(1) - (1)].n), NULL);
   }
    break;

  case 64:

/* Line 1806 of yacc.c  */
#line 482 "parse.y"
    {
      (yyval.n) = relattr_or_value_node(NULL, (yyvsp[(1) - (1)].n));
   }
    break;

  case 65:

/* Line 1806 of yacc.c  */
#line 489 "parse.y"
    {
      (yyval.n) = prepend((yyvsp[(1) - (3)].n), (yyvsp[(3) - (3)].n));
   }
    break;

  case 66:

/* Line 1806 of yacc.c  */
#line 493 "parse.y"
    {
      (yyval.n) = list_node((yyvsp[(1) - (1)].n));
   }
    break;

  case 67:

/* Line 1806 of yacc.c  */
#line 500 "parse.y"
    {
      (yyval.n) = value_node(STRING, (void *) (yyvsp[(1) - (1)].sval));
   }
    break;

  case 68:

/* Line 1806 of yacc.c  */
#line 504 "parse.y"
    {
      (yyval.n) = value_node(INT, (void *)& (yyvsp[(1) - (1)].ival));
   }
    break;

  case 69:

/* Line 1806 of yacc.c  */
#line 508 "parse.y"
    {
      (yyval.n) = value_node(FLOAT, (void *)& (yyvsp[(1) - (1)].rval));
   }
    break;

  case 70:

/* Line 1806 of yacc.c  */
#line 515 "parse.y"
    {
      (yyval.sval) = (yyvsp[(1) - (1)].sval);
   }
    break;

  case 71:

/* Line 1806 of yacc.c  */
#line 519 "parse.y"
    {
      (yyval.sval) = NULL;
   }
    break;

  case 72:

/* Line 1806 of yacc.c  */
#line 526 "parse.y"
    {
      (yyval.cval) = LT_OP;
   }
    break;

  case 73:

/* Line 1806 of yacc.c  */
#line 530 "parse.y"
    {
      (yyval.cval) = LE_OP;
   }
    break;

  case 74:

/* Line 1806 of yacc.c  */
#line 534 "parse.y"
    {
      (yyval.cval) = GT_OP;
   }
    break;

  case 75:

/* Line 1806 of yacc.c  */
#line 538 "parse.y"
    {
      (yyval.cval) = GE_OP;
   }
    break;

  case 76:

/* Line 1806 of yacc.c  */
#line 542 "parse.y"
    {
      (yyval.cval) = EQ_OP;
   }
    break;

  case 77:

/* Line 1806 of yacc.c  */
#line 546 "parse.y"
    {
      (yyval.cval) = NE_OP;
   }
    break;



/* Line 1806 of yacc.c  */
#line 2166 "y.tab.c"
      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", yyr1[yyn], &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);

  *++yyvsp = yyval;

  /* Now `shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTOKENS] + *yyssp;
  if (0 <= yystate && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTOKENS];

  goto yynewstate;


/*------------------------------------.
| yyerrlab -- here on detecting error |
`------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYEMPTY : YYTRANSLATE (yychar);

  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
#if ! YYERROR_VERBOSE
      yyerror (YY_("syntax error"));
#else
# define YYSYNTAX_ERROR yysyntax_error (&yymsg_alloc, &yymsg, \
                                        yyssp, yytoken)
      {
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = YYSYNTAX_ERROR;
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == 1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = (char *) YYSTACK_ALLOC (yymsg_alloc);
            if (!yymsg)
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = 2;
              }
            else
              {
                yysyntax_error_status = YYSYNTAX_ERROR;
                yymsgp = yymsg;
              }
          }
        yyerror (yymsgp);
        if (yysyntax_error_status == 2)
          goto yyexhaustedlab;
      }
# undef YYSYNTAX_ERROR
#endif
    }



  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
	 error, discard it.  */

      if (yychar <= YYEOF)
	{
	  /* Return failure if at end of input.  */
	  if (yychar == YYEOF)
	    YYABORT;
	}
      else
	{
	  yydestruct ("Error: discarding",
		      yytoken, &yylval);
	  yychar = YYEMPTY;
	}
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:

  /* Pacify compilers like GCC when the user code never invokes
     YYERROR and the label yyerrorlab therefore never appears in user
     code.  */
  if (/*CONSTCOND*/ 0)
     goto yyerrorlab;

  /* Do not reclaim the symbols of the rule which action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;	/* Each real token shifted decrements this.  */

  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
	{
	  yyn += YYTERROR;
	  if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYTERROR)
	    {
	      yyn = yytable[yyn];
	      if (0 < yyn)
		break;
	    }
	}

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
	YYABORT;


      yydestruct ("Error: popping",
		  yystos[yystate], yyvsp);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  *++yyvsp = yylval;


  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", yystos[yyn], yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturn;

/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturn;

#if !defined(yyoverflow) || YYERROR_VERBOSE
/*-------------------------------------------------.
| yyexhaustedlab -- memory exhaustion comes here.  |
`-------------------------------------------------*/
yyexhaustedlab:
  yyerror (YY_("memory exhausted"));
  yyresult = 2;
  /* Fall through.  */
#endif

yyreturn:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval);
    }
  /* Do not reclaim the symbols of the rule which action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
		  yystos[*yyssp], yyvsp);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
#if YYERROR_VERBOSE
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
#endif
  /* Make sure YYID is used.  */
  return YYID (yyresult);
}



/* Line 2067 of yacc.c  */
#line 555 "parse.y"


//
// PrintError
//
// Desc: Print an error message by calling the proper component-specific
//       print-error function
//
void PrintError(RC rc)
{
   if (abs(rc) <= END_PF_WARN)
      PF_PrintError(rc);
   else if (abs(rc) <= END_RM_WARN)
      RM_PrintError(rc);
   else if (abs(rc) <= END_IX_WARN)
      IX_PrintError(rc);
   else if (abs(rc) <= END_SM_WARN)
      SM_PrintError(rc);
   else if (abs(rc) <= END_QL_WARN)
      QL_PrintError(rc);
   else
      cerr << "Error code out of range: " << rc << "\n";
}

//
// RBparse
//
// Desc: Parse redbase commands
//
void RBparse(PF_Manager &pfm, SM_Manager &smm, QL_Manager &qlm)
{
   RC rc;

   // Set up global variables to their defaults
   pPfm  = &pfm;
   pSmm  = &smm;
   pQlm  = &qlm;
   bExit = 0;
   bQueryPlans = 0;

   /* Do forever */
   while (!bExit) {

      /* Reset parser and scanner for a new query */
      new_query();

      /* Print a prompt */
      cout << PROMPT;

      /* Get the prompt to actually show up on the screen */
      cout.flush(); 

      /* If a query was successfully read, interpret it */
      if(yyparse() == 0 && parse_tree != NULL)
         if ((rc = interp(parse_tree))) {
            PrintError(rc);
            if (rc < 0)
               bExit = TRUE;
         }
   }
}

//
// Functions for printing the various structures to an output stream
//
ostream &operator<<(ostream &s, const AttrInfo &ai)
{
   return
      s << " attrName=" << ai.attrName
      << " attrType=" << 
      (ai.attrType == INT ? "INT" :
       ai.attrType == FLOAT ? "FLOAT" : "STRING")
      << " attrLength=" << ai.attrLength;
}

ostream &operator<<(ostream &s, const RelAttr &qa)
{
   return
      s << (qa.relName ? qa.relName : "NULL")
      << "." << qa.attrName;
}

ostream &operator<<(ostream &s, const Condition &c)
{
   s << "\n      lhsAttr:" << c.lhsAttr << "\n"
      << "      op=" << c.op << "\n";
   if (c.bRhsIsAttr)
      s << "      bRhsIsAttr=TRUE \n      rhsAttr:" << c.rhsAttr;
   else
      s << "      bRshIsAttr=FALSE\n      rhsValue:" << c.rhsValue;
   return s;
}

ostream &operator<<(ostream &s, const Value &v)
{
   s << "AttrType: " << v.type;
   switch (v.type) {
      case INT:
         s << " *(int *)data=" << *(int *)v.data;
         break;
      case FLOAT:
         s << " *(float *)data=" << *(float *)v.data;
         break;
      case STRING:
         s << " (char *)data=" << (char *)v.data;
         break;
   }
   return s;
}

ostream &operator<<(ostream &s, const CompOp &op)
{
   switch(op){
      case EQ_OP:
         s << " =";
         break;
      case NE_OP:
         s << " <>";
         break;
      case LT_OP:
         s << " <";
         break;
      case LE_OP:
         s << " <=";
         break;
      case GT_OP:
         s << " >";
         break;
      case GE_OP:
         s << " >=";
         break;
      case NO_OP:
         s << " NO_OP";
         break;
   }
   return s;
}

ostream &operator<<(ostream &s, const AttrType &at)
{
   switch(at){
      case INT:
         s << "INT";
         break;
      case FLOAT:
         s << "FLOAT";
         break;
      case STRING:
         s << "STRING";
         break;
   }
   return s;
}

/*
 * Required by yacc
 */
void yyerror(char const *s) // New in 2000
{
   puts(s);
}

#if 0
/*
 * Sometimes required
 */
int yywrap(void)
{
   return 1;
}
#endif

