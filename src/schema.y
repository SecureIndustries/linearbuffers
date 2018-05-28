%{
#include <stdio.h>
#include "schema.h"
%}

// Symbols.
%union
{
	char	*sval;
};
%token <sval> STRING
%token NAMESPACE
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
	| Namespace Enums Tables                         { schema_begin(); }
	;

Namespace:
	/* empty */
	| NAMESPACE STRING                               { schema_set_namespace($2); }
	;

Enums:
	/* empty */
	| Enums Enum
	;

Enum:
	ENUM STRING BLOCK                                { schema_enum_begin($2, NULL); }
		EnumEntries
	ENDBLOCK                                         { schema_enum_end(); }
	
	|
	
	ENUM STRING COLON STRING BLOCK                   { schema_enum_begin($2, $4); }
		EnumEntries
	ENDBLOCK                                         { schema_enum_end(); }
	
	;

EnumEntries:
	/* empty */
	| EnumEntries EnumEntry COMMA
	| EnumEntries EnumEntry
	;

EnumEntry:
	  STRING                                         { schema_enum_entry_add($1, NULL); }
	| STRING EQUAL STRING                            { schema_enum_entry_add($1, $3); }
	;

Tables:
	/* empty */
	| Tables Table
	;

Table:
	TABLE STRING BLOCK                               { schema_table_begin($2); }
		TableFields
	ENDBLOCK                                         { schema_table_end(); }
	;

TableFields:
	/* empty */
	| TableFields TableField
	;

TableField:
	  STRING COLON STRING SEMICOLON                  { schema_table_field_add($1, $3); }
	| STRING COLON VECTOR STRING ENDVECTOR SEMICOLON { schema_table_field_add($1, $4); }
	;
%%

int yyerror (char *s)
{
	printf("yyerror : %s\n",s);
	return 0;  
}
