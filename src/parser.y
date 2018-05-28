%{
#include <stdio.h>

#include "schema.h"

static struct schema *schema;
static struct schema_enum *schema_enum;
static struct schema_enum_field *schema_enum_field;
static struct schema_table *schema_table;
static struct schema_table_field *schema_table_field;

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
	| Namespace Enums Tables                         {
	                                                     schema = schema_create();
	                                                 }
	;

Namespace:
	/* empty */
	| NAMESPACE STRING                               {
	                                                     schema_set_namespace(schema, $2);
	                                                 }
	;

Enums:
	/* empty */
	| Enums Enum
	;

Enum:
	ENUM STRING BLOCK                                {
	                                                     schema_enum = schema_enum_create();
	                                                     schema_enum_set_name(schema_enum, $2);
	                                                 }
		EnumEntries
	ENDBLOCK                                         {
	                                                     schema_add_enum(schema, schema_enum);
	                                                 }
	
	|
	
	ENUM STRING COLON STRING BLOCK                   {
	                                                     schema_enum = schema_enum_create();
	                                                     schema_enum_set_name(schema_enum, $2);
	                                                     schema_enum_set_type(schema_enum, $4);
	                                                 }
		EnumEntries
	ENDBLOCK                                         {
	                                                     schema_add_enum(schema, schema_enum);
	                                                 }
	
	;

EnumEntries:
	/* empty */
	| EnumEntries EnumEntry COMMA
	| EnumEntries EnumEntry
	;

EnumEntry:
	  STRING                                         {
	                                                     schema_enum_field = schema_enum_field_create();
	                                                     schema_enum_field_set_name(schema_enum_field, $1);
	                                                     schema_enum_add_field(schema_enum, schema_enum_field);
	                                                 }
	
	|
	
	STRING EQUAL STRING                              {
	                                                     schema_enum_field = schema_enum_field_create();
	                                                     schema_enum_field_set_name(schema_enum_field, $1);
	                                                     schema_enum_field_set_value(schema_enum_field, $3);
	                                                     schema_enum_add_field(schema_enum, schema_enum_field);
	                                                 }
	;

Tables:
	/* empty */
	| Tables Table
	;

Table:
	TABLE STRING BLOCK                               {
	                                                     schema_table = schema_table_create();
	                                                     schema_table_set_name(schema_table, $2);
	                                                 }
		TableFields
	ENDBLOCK                                         {
	                                                     schema_add_table(schema, schema_table);
	                                                 }
	;

TableFields:
	/* empty */
	| TableFields TableField
	;

TableField:
	  STRING COLON STRING SEMICOLON                  {
	                                                     schema_table_field = schema_table_field_create();
	                                                     schema_table_field_set_name(schema_table_field, $1);
	                                                     schema_table_field_set_type(schema_table_field, $3);
	                                                     schema_table_add_field(schema_table, schema_table_field);
	                                                 }
	|
	
	STRING COLON VECTOR STRING ENDVECTOR SEMICOLON   {
	                                                     schema_table_field = schema_table_field_create();
	                                                     schema_table_field_set_name(schema_table_field, $1);
	                                                     schema_table_field_set_type(schema_table_field, $4);
	                                                     schema_table_add_field(schema_table, schema_table_field);
	                                                 }
	;
%%

int yyerror (char *s)
{
	printf("yyerror : %s\n",s);
	return 0;  
}
