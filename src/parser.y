%{

#include <stdio.h>
#include <stdint.h>

#include "schema.h"
#include "parser.h"

%}

%parse-param { struct schema_parser *schema_parser }
%error-verbose

%union
{
    char *sval;
};

%token <sval> STRING
%token OPTION
%token ENUM
%token TABLE
%token STRUCT
%token BLOCK
%token ENDBLOCK
%token COLON
%token SEMICOLON
%token COMMA
%token EQUAL
%token BRACKET
%token ENDBRACKET
%token PARENTHESIS;
%token ENDPARENTHESIS;

%start Program

%%

Program:
                                                        {
                                                            schema_parser->schema = schema_create();
                                                            if (schema_parser->schema == NULL) {
                                                                fprintf(stderr, "can not create schema\n");
                                                                YYERROR;
                                                            }
                                                            schema_parser->schema_enum = NULL;
                                                            schema_parser->schema_enum_field = NULL;
                                                            schema_parser->schema_table = NULL;
                                                            schema_parser->schema_table_field = NULL;
                                                        }
    Schema                                              {
                                                            schema_parser->schema_enum = NULL;
                                                            schema_parser->schema_enum_field = NULL;
                                                            schema_parser->schema_table = NULL;
                                                            schema_parser->schema_table_field = NULL;
                                                        }

Schema:
    /* empty */
    |    Schema Option
    |    Schema Enum
    |    Schema Struct
    |    Schema Table
    ;

Option:
         OPTION STRING EQUAL STRING SEMICOLON           {
                                                            int rc;
                                                            if (strcmp($2, "namespace") == 0) {
                                                                rc = schema_set_namespace(schema_parser->schema, $4);
                                                                if (rc != 0) {
                                                                    fprintf(stderr, "can not set schema namespace\n");
                                                                    YYERROR;
                                                                }
                                                            } else if (strcmp($2, "count_type") == 0) {
                                                                rc = schema_set_count_type(schema_parser->schema, $4);
                                                                if (rc != 0) {
                                                                    fprintf(stderr, "can not set schema count_type\n");
                                                                    YYERROR;
                                                                }
                                                            } else if (strcmp($2, "offset_type") == 0) {
                                                                rc = schema_set_offset_type(schema_parser->schema, $4);
                                                                if (rc != 0) {
                                                                    fprintf(stderr, "can not set schema offset_type\n");
                                                                    YYERROR;
                                                                }
                                                            } else {
                                                                fprintf(stderr, "unknown option: '%s' = '%s';\n", $2, $4);
                                                                YYERROR;
                                                            }
                                                            free($2);
                                                        }
    ;

Enum:
        ENUM STRING BLOCK                               {
                                                            int rc;
                                                            schema_parser->schema_enum = schema_enum_create();
                                                            if (schema_parser->schema_enum == NULL) {
                                                                fprintf(stderr, "can not create schema enum\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_set_name(schema_parser->schema_enum, $2);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum name\n");
                                                                YYERROR;
                                                            }
                                                            free($2);
                                                        }
            EnumEntries
        ENDBLOCK                                        {
                                                            int rc;
                                                            rc = schema_add_enum(schema_parser->schema, schema_parser->schema_enum);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema enum\n");
                                                                YYERROR;
                                                            }
                                                        }
    
    |    ENUM STRING COLON STRING BLOCK                 {
                                                            int rc;
                                                            schema_parser->schema_enum = schema_enum_create();
                                                            if (schema_parser->schema_enum == NULL) {
                                                                fprintf(stderr, "can not create schema enum\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_set_name(schema_parser->schema_enum, $2);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_set_type(schema_parser->schema_enum, $4);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum name\n");
                                                                YYERROR;
                                                            }
                                                            free($2);
                                                        }
            EnumEntries
        ENDBLOCK                                        {
                                                            int rc;
                                                            rc = schema_add_enum(schema_parser->schema, schema_parser->schema_enum);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema enum\n");
                                                                YYERROR;
                                                            }
                                                        }
    
    ;

EnumEntries:
         EnumEntry
    |    EnumEntries COMMA EnumEntry
    ;

EnumEntry:
        STRING                                          {
                                                            int rc;
                                                            schema_parser->schema_enum_field = schema_enum_field_create();
                                                            if (schema_parser->schema_enum_field == NULL) {
                                                                fprintf(stderr, "can not create schema enum field");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_field_set_name(schema_parser->schema_enum_field, $1);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum field name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_add_field(schema_parser->schema_enum, schema_parser->schema_enum_field);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema enum field\n");
                                                                YYERROR;
                                                            }
                                                            free($1);
                                                        }
    
    |    STRING EQUAL STRING                            {
                                                            int rc;
                                                            schema_parser->schema_enum_field = schema_enum_field_create();
                                                            if (schema_parser->schema_enum_field == NULL) {
                                                                fprintf(stderr, "can not create schema enum field");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_field_set_name(schema_parser->schema_enum_field, $1);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum field name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_field_set_value(schema_parser->schema_enum_field, $3);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema enum field type\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_enum_add_field(schema_parser->schema_enum, schema_parser->schema_enum_field);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema enum field\n");
                                                                YYERROR;
                                                            }
                                                            free($1);
                                                            free($3);
                                                        }
    ;

Struct:
        STRUCT STRING BLOCK                             {
                                                            int rc;
                                                            schema_parser->schema_table = schema_table_create();
                                                            if (schema_parser->schema_table == NULL) {
                                                                fprintf(stderr, "can not create schema table\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_set_name(schema_parser->schema_table, $2);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_set_type(schema_parser->schema_table, schema_container_type_struct);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table type\n");
                                                                YYERROR;
                                                            }
                                                            free($2);
                                                        }
            TableFields
        ENDBLOCK                                        {
                                                            int rc;
                                                            rc = schema_add_table(schema_parser->schema, schema_parser->schema_table);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema table\n");
                                                                YYERROR;
                                                            }
                                                        }
    ;

Table:
        TABLE STRING BLOCK                              {
                                                            int rc;
                                                            schema_parser->schema_table = schema_table_create();
                                                            if (schema_parser->schema_table == NULL) {
                                                                fprintf(stderr, "can not create schema table\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_set_name(schema_parser->schema_table, $2);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table name\n");
                                                                YYERROR;
                                                            }
                                                            free($2);
                                                        }
            TableFields
        ENDBLOCK                                        {
                                                            int rc;
                                                            rc = schema_add_table(schema_parser->schema, schema_parser->schema_table);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema table\n");
                                                                YYERROR;
                                                            }
                                                        }
    ;

TableFields:
    /* empty */
    |    TableFields TableField
    ;

TableField:
        STRING COLON STRING                             {
                                                            int rc;
                                                            schema_parser->schema_table_field = schema_table_field_create();
                                                            if (schema_parser->schema_table_field == NULL) {
                                                                fprintf(stderr, "can not create schema table field\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_name(schema_parser->schema_table_field, $1);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_type(schema_parser->schema_table_field, schema_container_type_none, $3);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field type\n");
                                                                YYERROR;
                                                            }
                                                            free($1);
                                                            free($3);
                                                        }
            Attributes
        SEMICOLON                                       {
                                                            int rc;
                                                            rc = schema_table_add_field(schema_parser->schema_table, schema_parser->schema_table_field);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema table field\n");
                                                                YYERROR;
                                                            }
                                                            schema_parser->schema_table_field = NULL;
                                                        }

    |   STRING COLON STRING EQUAL STRING
                                                        {
                                                            int rc;
                                                            schema_parser->schema_table_field = schema_table_field_create();
                                                            if (schema_parser->schema_table_field == NULL) {
                                                                fprintf(stderr, "can not create schema table field\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_name(schema_parser->schema_table_field, $1);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_type(schema_parser->schema_table_field, schema_container_type_none, $3);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field type\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_value(schema_parser->schema_table_field, $5);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field type\n");
                                                                YYERROR;
                                                            }
                                                            free($1);
                                                            free($3);
                                                            free($5);
                                                        }
            Attributes
        SEMICOLON                                       {
                                                            int rc;
                                                            rc = schema_table_add_field(schema_parser->schema_table, schema_parser->schema_table_field);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema table field\n");
                                                                YYERROR;
                                                            }
                                                            schema_parser->schema_table_field = NULL;
                                                        }

    |    STRING COLON BRACKET STRING ENDBRACKET
                                                        {
                                                            int rc;
                                                            schema_parser->schema_table_field = schema_table_field_create();
                                                            if (schema_parser->schema_table_field == NULL) {
                                                                fprintf(stderr, "can not create schema table field\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_name(schema_parser->schema_table_field, $1);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field name\n");
                                                                YYERROR;
                                                            }
                                                            rc = schema_table_field_set_type(schema_parser->schema_table_field, schema_container_type_vector, $4);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not set schema table field type\n");
                                                                YYERROR;
                                                            }
                                                            free($1);
                                                            free($4);
                                                        }
            Attributes
        SEMICOLON                                       {
                                                            int rc;
                                                            rc = schema_table_add_field(schema_parser->schema_table, schema_parser->schema_table_field);
                                                            if (rc != 0) {
                                                                fprintf(stderr, "can not add schema table field\n");
                                                                YYERROR;
                                                            }
                                                            schema_parser->schema_table_field = NULL;
                                                        }

    ;

Attributes:
    /* empty */
    |    PARENTHESIS AttributeEntries ENDPARENTHESIS
    ;

AttributeEntries:
         AttributeEntry
    |    AttributeEntries COMMA AttributeEntry
    ;

AttributeEntry:
         STRING                                         {
                                                            int rc;
                                                            if (schema_parser->schema_table_field != NULL) {
                                                                rc = schema_table_field_add_attribute(schema_parser->schema_table_field, $1, "true");
                                                                if (rc != 0) {
                                                                    fprintf(stderr, "can not add schema table field attribute\n");
                                                                    YYERROR;
                                                                }
                                                            }
                                                            free($1);
                                                        }

    |    STRING EQUAL STRING                            {
                                                            int rc;
                                                            if (schema_parser->schema_table_field != NULL) {
                                                                rc = schema_table_field_add_attribute(schema_parser->schema_table_field, $1, $3);
                                                                if (rc != 0) {
                                                                    fprintf(stderr, "can not add schema table field attribute\n");
                                                                    YYERROR;
                                                                }
                                                            }
                                                            free($1);
                                                            free($3);
                                                        }
    ;

%%

int yyerror (struct schema_parser *schema_parser, char *s)
{
    (void) schema_parser;
    printf("yyerror : %s\n", s);
    return 0;  
}
