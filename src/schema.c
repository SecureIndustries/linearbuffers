
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "schema.h"

TAILQ_HEAD(schema_attributes, schema_attribute);
struct schema_attribute {
	TAILQ_ENTRY(schema_attribute) list;
	char *name;
	char *value;
};

TAILQ_HEAD(schema_enum_fields, schema_enum_field);
struct schema_enum_field {
	TAILQ_ENTRY(schema_enum_field) list;
	char *name;
	char *value;
	struct schema_attributes attributes;
};

TAILQ_HEAD(schema_enums, schema_enum);
struct schema_enum {
	TAILQ_ENTRY(schema_enum) list;
	char *name;
	char *type;
	struct schema_enum_fields fields;
	struct schema_attributes attributes;
};

TAILQ_HEAD(schema_table_fields, schema_table_field);
struct schema_table_field {
	TAILQ_ENTRY(schema_table_field) list;
	char *name;
	char *type;
	int array;
	struct schema_attributes attributes;
};

TAILQ_HEAD(schema_tables, schema_table);
struct schema_table {
	TAILQ_ENTRY(schema_table) list;
	char *name;
	struct schema_table_fields fields;
	struct schema_attributes attributes;
};

struct schema {
	struct schema_enums enums;
	struct schema_tables tables;
	struct schema_attributes attributes;
};

void schema_attribute_destroy (struct schema_attribute *attribute)
{
	if (attribute == NULL) {
		return;
	}
	if (attribute->name != NULL) {
		free(attribute->name);
	}
	if (attribute->value != NULL) {
		free(attribute->value);
	}
	free(attribute);
}

void schema_enum_field_destroy (struct schema_enum_field *field)
{
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (field == NULL) {
		return;
	}
	if (field->name != NULL) {
		free(field->name);
	}
	if (field->value != NULL) {
		free(field->value);
	}
	TAILQ_FOREACH_SAFE(attribute, &field->attributes, list, nattribute) {
		TAILQ_REMOVE(&field->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(field);
}

void schema_enum_destroy (struct schema_enum *table)
{
	struct schema_enum_field *field;
	struct schema_enum_field *nfield;
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (table == NULL) {
		return;
	}
	if (table->name != NULL) {
		free(table->name);
	}
	TAILQ_FOREACH_SAFE(field, &table->fields, list, nfield) {
		TAILQ_REMOVE(&table->fields, field, list);
		schema_enum_field_destroy(field);
	}
	TAILQ_FOREACH_SAFE(attribute, &table->attributes, list, nattribute) {
		TAILQ_REMOVE(&table->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(table);
}

void schema_table_field_destroy (struct schema_table_field *field)
{
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (field == NULL) {
		return;
	}
	if (field->name != NULL) {
		free(field->name);
	}
	if (field->type != NULL) {
		free(field->type);
	}
	TAILQ_FOREACH_SAFE(attribute, &field->attributes, list, nattribute) {
		TAILQ_REMOVE(&field->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(field);
}

void schema_table_destroy (struct schema_table *table)
{
	struct schema_table_field *field;
	struct schema_table_field *nfield;
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (table == NULL) {
		return;
	}
	if (table->name != NULL) {
		free(table->name);
	}
	TAILQ_FOREACH_SAFE(field, &table->fields, list, nfield) {
		TAILQ_REMOVE(&table->fields, field, list);
		schema_table_field_destroy(field);
	}
	TAILQ_FOREACH_SAFE(attribute, &table->attributes, list, nattribute) {
		TAILQ_REMOVE(&table->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(table);
}

void schema_destroy (struct schema *schema)
{
	struct schema_enum *anum;
	struct schema_enum *nanum;
	struct schema_table *table;
	struct schema_table *ntable;
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (schema == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(anum, &schema->enums, list, nanum) {
		TAILQ_REMOVE(&schema->enums, anum, list);
		schema_enum_destroy(anum);
	}
	TAILQ_FOREACH_SAFE(table, &schema->tables, list, ntable) {
		TAILQ_REMOVE(&schema->tables, table, list);
		schema_table_destroy(table);
	}
	TAILQ_FOREACH_SAFE(attribute, &schema->attributes, list, nattribute) {
		TAILQ_REMOVE(&schema->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(schema);
}

struct schema * schema_create (void)
{
	struct schema *schema;
	schema = malloc(sizeof(struct schema));
	if (schema == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(schema, 0, sizeof(struct schema));
	TAILQ_INIT(&schema->attributes);
	TAILQ_INIT(&schema->tables);
	return schema;
bail:	if (schema != NULL) {
		schema_destroy(schema);
	}
	return NULL;
}
