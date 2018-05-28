%{
#include <stdio.h>
%}

// Symbols.
%union
{
	char	*sval;
};
%token <sval> STRING
%token ENUM
%token TABLE
%token BLOCK
%token ENDBLOCK
%token COLON
%token SEMICOLON
%token COMMA
%token EQUAL
%token VECTOR
%token ENDVECTOR

%start Schema

%%

Schema:
	/* empty */
	| Enums Tables
	;

Enums:
	/* empty */
	| Enums Enum
	;

Enum:
	ENUM STRING BLOCK                                { printf("enum %s {\n", $2); }
		EnumEntries
	ENDBLOCK                                         { printf("}\n"); }
	
	|
	
	ENUM STRING COLON STRING BLOCK                   { printf("enum %s: %s {\n", $2, $4); }
		EnumEntries
	ENDBLOCK                                         { printf("}\n"); }
	
	;

EnumEntries:
	/* empty */
	| EnumEntries EnumEntry COMMA
	| EnumEntries EnumEntry
	;

EnumEntry:
	  STRING                                         { printf("\t%s,\n", $1); }
	| STRING EQUAL STRING                            { printf("\t%s = %s,\n", $1, $3); }
	;

Tables:
	/* empty */
	| Tables Table
	;

Table:
	TABLE STRING BLOCK                               { printf("table %s {\n", $2); }
		TableFields
	ENDBLOCK                                         { printf("}\n"); }
	;

TableFields:
	/* empty */
	| TableFields TableField
	;

TableField:
	  STRING COLON STRING SEMICOLON                  { printf("\t%s: %s;\n", $1, $3); }
	| STRING COLON VECTOR STRING ENDVECTOR SEMICOLON { printf("\t%s: [ %s ];\n", $1, $4); }
	;
%%

int yyerror (char *s)
{
	printf("yyerror : %s\n",s);
	return 0;  
}
