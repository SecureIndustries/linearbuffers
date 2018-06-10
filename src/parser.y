%{

#include <stdio.h>

#include "schema.h"
#include "parser.h"

%}

%parse-param { struct schema_parser *schema_parser }

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

%start Program

%%

Program:
	Schema												{
															schema_parser->schema_enum = NULL;
															schema_parser->schema_enum_field = NULL;
															schema_parser->schema_table = NULL;
															schema_parser->schema_table_field = NULL;
														}

Schema:
	/* empty */
	|	Namespace Enums Tables
	;

Namespace:
	/* empty */
	|	NAMESPACE STRING								{
															if (schema_parser->schema == NULL) {
																schema_parser->schema = schema_create();
															}
															schema_set_namespace(schema_parser->schema, $2);
														}
	;

Enums:
	/* empty */
	|	Enums Enum
	;

Enum:
		ENUM STRING BLOCK								{
															if (schema_parser->schema == NULL) {
																schema_parser->schema = schema_create();
															}
															schema_parser->schema_enum = schema_enum_create();
															schema_enum_set_name(schema_parser->schema_enum, $2);
														}
			EnumEntries
		ENDBLOCK										{
															schema_add_enum(schema_parser->schema, schema_parser->schema_enum);
														}
	
	|	ENUM STRING COLON STRING BLOCK					{
															if (schema_parser->schema == NULL) {
																schema_parser->schema = schema_create();
															}
															schema_parser->schema_enum = schema_enum_create();
															schema_enum_set_name(schema_parser->schema_enum, $2);
															schema_enum_set_type(schema_parser->schema_enum, $4);
														}
			EnumEntries
		ENDBLOCK										{
															schema_add_enum(schema_parser->schema, schema_parser->schema_enum);
														}
	
	;

EnumEntries:
		EnumEntry
	|	EnumEntries COMMA EnumEntry
	;

EnumEntry:
		STRING											{
															schema_parser->schema_enum_field = schema_enum_field_create();
															schema_enum_field_set_name(schema_parser->schema_enum_field, $1);
															schema_enum_add_field(schema_parser->schema_enum, schema_parser->schema_enum_field);
														}
	
	|	STRING EQUAL STRING								{
															schema_parser->schema_enum_field = schema_enum_field_create();
															schema_enum_field_set_name(schema_parser->schema_enum_field, $1);
															schema_enum_field_set_value(schema_parser->schema_enum_field, $3);
															schema_enum_add_field(schema_parser->schema_enum, schema_parser->schema_enum_field);
														}
	;

Tables:
	/* empty */
	|	Tables Table
	;

Table:
		TABLE STRING BLOCK								{
															if (schema_parser->schema == NULL) {
																schema_parser->schema = schema_create();
															}
															schema_parser->schema_table = schema_table_create();
															schema_table_set_name(schema_parser->schema_table, $2);
														}
			TableFields
		ENDBLOCK										{
															schema_add_table(schema_parser->schema, schema_parser->schema_table);
														}
	;

TableFields:
	/* empty */
	|	TableFields TableField
	;

TableField:
		STRING COLON STRING SEMICOLON					{
															schema_parser->schema_table_field = schema_table_field_create();
															schema_table_field_set_name(schema_parser->schema_table_field, $1);
															schema_table_field_set_type(schema_parser->schema_table_field, $3);
															schema_table_add_field(schema_parser->schema_table, schema_parser->schema_table_field);
														}

	|	STRING COLON VECTOR STRING ENDVECTOR SEMICOLON	{
															schema_parser->schema_table_field = schema_table_field_create();
															schema_table_field_set_name(schema_parser->schema_table_field, $1);
															schema_table_field_set_type(schema_parser->schema_table_field, $4);
															schema_table_field_set_vector(schema_parser->schema_table_field, 1);
															schema_table_add_field(schema_parser->schema_table, schema_parser->schema_table_field);
														}
	;
%%

int yyerror (char *s)
{
	printf("yyerror : %s\n",s);
	return 0;  
}
