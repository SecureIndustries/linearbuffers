
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "parser.lex.h"

#define LINEARBUFFERS_DEBUG_NAME "schema"

#include "debug.h"
#include "queue.h"
#include "parser.h"
#include "schema.h"
#include "schema-private.h"

#if !defined(MIN)
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

static const struct {
	const char *name;
	uint64_t value;
	uint64_t size;
} schema_count_types[] = {
	[schema_count_type_default] = { "uint64", schema_count_type_uint64, sizeof(uint64_t) },
	[schema_count_type_uint8]   = { "uint8" , schema_count_type_uint8 , sizeof(uint8_t)  },
	[schema_count_type_uint16]  = { "uint16", schema_count_type_uint16, sizeof(uint16_t) },
	[schema_count_type_uint32]  = { "uint32", schema_count_type_uint32, sizeof(uint32_t) },
	[schema_count_type_uint64]  = { "uint64", schema_count_type_uint64, sizeof(uint64_t) }
};

const char * schema_count_type_name (uint32_t type)
{
	uint64_t i;
	for (i = 0; i < sizeof(schema_count_types) / sizeof(schema_count_types[0]); i++) {
		if (type == schema_count_types[i].value) {
			return schema_count_types[i].name;
		}
	}
	return "default";
}

uint32_t schema_count_type_value (const char *type)
{
	uint64_t i;
	for (i = 0; i < sizeof(schema_count_types) / sizeof(schema_count_types[0]); i++) {
		if (strcmp(type, schema_count_types[i].name) == 0) {
			return schema_count_types[i].value;
		}
	}
	return schema_count_type_default;
}

uint64_t schema_count_type_size (uint32_t type)
{
	if (type >= (sizeof(schema_count_types) / sizeof(schema_count_types[0]))) {
		return -1;
	}
	return schema_count_types[type].size;
}

static const struct {
	const char *name;
	uint64_t value;
	uint64_t size;
} schema_offset_types[] = {
	[schema_offset_type_default] = { "uint64", schema_offset_type_uint64, sizeof(uint64_t) },
	[schema_offset_type_uint8]   = { "uint8" , schema_offset_type_uint8 , sizeof(uint8_t)  },
	[schema_offset_type_uint16]  = { "uint16", schema_offset_type_uint16, sizeof(uint16_t) },
	[schema_offset_type_uint32]  = { "uint32", schema_offset_type_uint32, sizeof(uint32_t) },
	[schema_offset_type_uint64]  = { "uint64", schema_offset_type_uint64, sizeof(uint64_t) }
};

const char * schema_offset_type_name (uint32_t type)
{
	uint64_t i;
	for (i = 0; i < sizeof(schema_offset_types) / sizeof(schema_offset_types[0]); i++) {
		if (type == schema_offset_types[i].value) {
			return schema_offset_types[i].name;
		}
	}
	return "default";
}

uint32_t schema_offset_type_value (const char *type)
{
	uint64_t i;
	for (i = 0; i < sizeof(schema_offset_types) / sizeof(schema_offset_types[0]); i++) {
		if (strcmp(type, schema_offset_types[i].name) == 0) {
			return schema_offset_types[i].value;
		}
	}
	return schema_offset_type_default;
}

uint64_t schema_offset_type_size (uint32_t type)
{
	if (type >= (sizeof(schema_offset_types) / sizeof(schema_offset_types[0]))) {
		return -1;
	}
	return schema_offset_types[type].size;
}

uint64_t schema_inttype_size (const char *type)
{
	if (type == NULL) {
		return 0;
	}
	if (strcmp(type, "int8") == 0) {
		return sizeof(int8_t);
	}
	if (strcmp(type, "uint8") == 0) {
		return sizeof(int8_t);
	}
	if (strcmp(type, "int16") == 0) {
		return sizeof(int16_t);
	}
	if (strcmp(type, "uint16") == 0) {
		return sizeof(uint16_t);
	}
	if (strcmp(type, "int32") == 0) {
		return sizeof(int32_t);
	}
	if (strcmp(type, "uint32") == 0) {
		return sizeof(uint32_t);
	}
	if (strcmp(type, "int64") == 0) {
		return sizeof(int64_t);
	}
	if (strcmp(type, "uint64") == 0) {
		return sizeof(uint64_t);
	}
	if (strcmp(type, "float") == 0) {
		return sizeof(float);
	}
	if (strcmp(type, "double") == 0) {
		return sizeof(double);
	}
	return 0;
}

int schema_type_is_scalar (const char *type)
{
	if (type == NULL) {
		return 0;
	}
	if (strcmp(type, "int8") == 0) {
		return 1;
	}
	if (strcmp(type, "uint8") == 0) {
		return 1;
	}
	if (strcmp(type, "int16") == 0) {
		return 1;
	}
	if (strcmp(type, "uint16") == 0) {
		return 1;
	}
	if (strcmp(type, "int32") == 0) {
		return 1;
	}
	if (strcmp(type, "uint32") == 0) {
		return 1;
	}
	if (strcmp(type, "int64") == 0) {
		return 1;
	}
	if (strcmp(type, "uint64") == 0) {
		return 1;
	}
	return 0;
}

int schema_type_is_float (const char *type)
{
	if (type == NULL) {
		return 0;
	}
	if (strcmp(type, "float") == 0) {
		return 1;
	}
	if (strcmp(type, "double") == 0) {
		return 1;
	}
	return 0;
}

int schema_type_is_string (const char *type)
{
	if (type == NULL) {
		return 0;
	}
	if (strcmp(type, "string") == 0) {
		return 1;
	}
	return 0;
}

int schema_type_is_enum (struct schema *schema, const char *type)
{
	struct schema_enum *anum;
	if (schema == NULL) {
		return 0;
	}
	if (type == NULL) {
		return 0;
	}
	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (strcmp(anum->name, type) == 0) {
			return 1;
		}
	}
	return 0;
}

struct schema_enum * schema_type_get_enum (struct schema *schema, const char *type)
{
	struct schema_enum *anum;
	if (schema == NULL) {
		return NULL;
	}
	if (type == NULL) {
		return NULL;
	}
	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (strcmp(anum->name, type) == 0) {
			return anum;
		}
	}
	return NULL;
}

struct schema_table * schema_type_get_table (struct schema *schema, const char *type)
{
	struct schema_table *table;
	if (schema == NULL) {
		return NULL;
	}
	if (type == NULL) {
		return NULL;
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		if (strcmp(table->name, type) == 0) {
			return table;
		}
	}
	return NULL;
}

int schema_type_is_table (struct schema *schema, const char *type)
{
	struct schema_table *table;
	if (schema == NULL) {
		return 0;
	}
	if (type == NULL) {
		return 0;
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		if (strcmp(table->name, type) == 0) {
			return 1;
		}
	}
	return 0;
}

int schema_type_is_valid (struct schema *schema, const char *type)
{
	int rc;
	if (schema == NULL) {
		return 0;
	}
	if (type == NULL) {
		return 0;
	}
	rc = schema_type_is_scalar(type);
	if (rc == 1) {
		return 1;
	}
	rc = schema_type_is_float(type);
	if (rc == 1) {
		return 1;
	}
	rc = schema_type_is_string(type);
	if (rc == 1) {
		return 1;
	}
	rc = schema_type_is_enum(schema, type);
	if (rc == 1) {
		return 1;
	}
	rc = schema_type_is_table(schema, type);
	if (rc == 1) {
		return 1;
	}
	return 0;
}

int schema_value_is_scalar (const char *value)
{
	int rc;
	if (value == NULL) {
		return 0;
	}
	if (*value == '+') {
		value++;
	} else if (*value == '-') {
		value++;
	}
	rc = strcspn(value, "0123456789");
	if (rc != 0) {
		return 0;
	}
	return 1;
}

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

struct schema_attribute * schema_attribute_create (void)
{
	struct schema_attribute *attribute;
	attribute = malloc(sizeof(struct schema_attribute));
	if (attribute == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(attribute, 0, sizeof(struct schema_attribute));
	return attribute;
bail:	if (attribute != NULL) {
		schema_attribute_destroy(attribute);
	}
	return NULL;
}

int schema_enum_field_set_value (struct schema_enum_field *field, const char *value)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->value != NULL) {
		free(field->value);
		field->value = NULL;
	}
	if (value != NULL) {
		field->value = strdup(value);
		if (field->value == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_field_set_name (struct schema_enum_field *field, const char *name)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->name != NULL) {
		free(field->name);
		field->name = NULL;
	}
	if (name != NULL) {
		field->name = strdup(name);
		if (field->name == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
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

struct schema_enum_field * schema_enum_field_create (void)
{
	struct schema_enum_field *field;
	field = malloc(sizeof(struct schema_enum_field));
	if (field == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(field, 0, sizeof(struct schema_enum_field));
	TAILQ_INIT(&field->attributes);
	return field;
bail:	if (field != NULL) {
		schema_enum_field_destroy(field);
	}
	return NULL;
}

int schema_enum_set_type (struct schema_enum *anum, const char *type)
{
	if (anum == NULL) {
		linearbuffers_errorf("anum is invalid");
		goto bail;
	}
	if (anum->type != NULL) {
		free(anum->type);
		anum->type = NULL;
	}
	if (type != NULL) {
		anum->type = strdup(type);
		if (anum->type == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_set_name (struct schema_enum *anum, const char *name)
{
	if (anum == NULL) {
		linearbuffers_errorf("anum is invalid");
		goto bail;
	}
	if (anum->name != NULL) {
		free(anum->name);
		anum->name = NULL;
	}
	if (name != NULL) {
		anum->name = strdup(name);
		if (anum->name == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_add_field (struct schema_enum *anum, struct schema_enum_field *field)
{
	if (anum == NULL) {
		linearbuffers_errorf("anum is invalid");
		goto bail;
	}
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->name == NULL) {
		linearbuffers_errorf("field name is invalid");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&anum->fields, field, list);
	return 0;
bail:	return -1;
}

void schema_enum_destroy (struct schema_enum *anum)
{
	struct schema_enum_field *field;
	struct schema_enum_field *nfield;
	struct schema_attribute *attribute;
	struct schema_attribute *nattribute;
	if (anum == NULL) {
		return;
	}
	if (anum->name != NULL) {
		free(anum->name);
	}
	if (anum->type != NULL) {
		free(anum->type);
	}
	if (anum->TYPE != NULL) {
		free(anum->TYPE);
	}
	TAILQ_FOREACH_SAFE(field, &anum->fields, list, nfield) {
		TAILQ_REMOVE(&anum->fields, field, list);
		schema_enum_field_destroy(field);
	}
	TAILQ_FOREACH_SAFE(attribute, &anum->attributes, list, nattribute) {
		TAILQ_REMOVE(&anum->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(anum);
}

struct schema_enum * schema_enum_create (void)
{
	struct schema_enum *anum;
	anum = malloc(sizeof(struct schema_enum));
	if (anum == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(anum, 0, sizeof(struct schema_enum));
	TAILQ_INIT(&anum->attributes);
	TAILQ_INIT(&anum->fields);
	return anum;
bail:	if (anum != NULL) {
		schema_enum_destroy(anum);
	}
	return NULL;
}

int schema_table_field_set_vector (struct schema_table_field *field, int vector)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	field->vector = (vector) ? 1 : 0;
	return 0;
bail:	return -1;
}

int schema_table_field_set_list (struct schema_table_field *field, int list)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	field->vector = (list) ? 2 : 0;
	return 0;
bail:	return -1;
}

int schema_table_field_set_type (struct schema_table_field *field, const char *type)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->type != NULL) {
		free(field->type);
		field->type = NULL;
	}
	if (type != NULL) {
		field->type = strdup(type);
		if (field->type == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_table_field_set_name (struct schema_table_field *field, const char *name)
{
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->name != NULL) {
		free(field->name);
		field->name = NULL;
	}
	if (name != NULL) {
		field->name = strdup(name);
		if (field->name == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
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

struct schema_table_field * schema_table_field_create (void)
{
	struct schema_table_field *field;
	field = malloc(sizeof(struct schema_table_field));
	if (field == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(field, 0, sizeof(struct schema_table_field));
	TAILQ_INIT(&field->attributes);
	return field;
bail:	if (field != NULL) {
		schema_table_field_destroy(field);
	}
	return NULL;
}

int schema_table_set_name (struct schema_table *table, const char *name)
{
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (table->name != NULL) {
		free(table->name);
		table->name = NULL;
	}
	if (name != NULL) {
		table->name = strdup(name);
		if (table->name == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_table_add_field (struct schema_table *table, struct schema_table_field *field)
{
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (field == NULL) {
		linearbuffers_errorf("field is invalid");
		goto bail;
	}
	if (field->name == NULL) {
		linearbuffers_errorf("field name is invalid");
		goto bail;
	}
	if (field->type == NULL) {
		linearbuffers_errorf("field type is invalid");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&table->fields, field, list);
	table->nfields += 1;
	return 0;
bail:	return -1;
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
		table->nfields -= 1;
		schema_table_field_destroy(field);
	}
	TAILQ_FOREACH_SAFE(attribute, &table->attributes, list, nattribute) {
		TAILQ_REMOVE(&table->attributes, attribute, list);
		schema_attribute_destroy(attribute);
	}
	free(table);
}

struct schema_table * schema_table_create (void)
{
	struct schema_table *table;
	table = malloc(sizeof(struct schema_table));
	if (table == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(table, 0, sizeof(struct schema_table));
	TAILQ_INIT(&table->attributes);
	TAILQ_INIT(&table->fields);
	return table;
bail:	if (table != NULL) {
		schema_table_destroy(table);
	}
	return NULL;
}

int schema_set_namespace (struct schema *schema, const char *name)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (schema->namespace != NULL) {
		free(schema->namespace);
		schema->namespace = NULL;
	}
	if (name != NULL) {
		schema->namespace = strdup(name);
		if (schema->namespace == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_set_count_type (struct schema *schema, const char *type)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (type != NULL &&
	    strcmp(type, "uint8") != 0 &&
	    strcmp(type, "uint16") != 0 &&
	    strcmp(type, "uint32") != 0 &&
	    strcmp(type, "uint64") != 0) {
		linearbuffers_errorf("type is invalid");
		goto bail;
	}
	schema->count_type = schema_count_type_default;
	if (type != NULL) {
		schema->count_type = schema_count_type_value(type);
		if (schema->count_type == schema_count_type_default) {
			linearbuffers_errorf("type is invalid");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_set_offset_type (struct schema *schema, const char *type)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (type != NULL &&
	    strcmp(type, "uint8") != 0 &&
	    strcmp(type, "uint16") != 0 &&
	    strcmp(type, "uint32") != 0 &&
	    strcmp(type, "uint64") != 0) {
		linearbuffers_errorf("type is invalid");
		goto bail;
	}
	schema->offset_type = schema_offset_type_default;
	if (type != NULL) {
		schema->offset_type = schema_offset_type_value(type);
		if (schema->offset_type == schema_offset_type_default) {
			linearbuffers_errorf("type is invalid");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_add_table (struct schema *schema, struct schema_table *table)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (table->name == NULL) {
		linearbuffers_errorf("table name is invalid");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&schema->tables, table, list);
	return 0;
bail:	return -1;
}

int schema_add_enum (struct schema *schema, struct schema_enum *anum)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (anum == NULL) {
		linearbuffers_errorf("anum is invalid");
		goto bail;
	}
	if (anum->name == NULL) {
		linearbuffers_errorf("anum name is invalid");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&schema->enums, anum, list);
	return 0;
bail:	return -1;
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
	if (schema->namespace != NULL) {
		free(schema->namespace);
	}
	if (schema->NAMESPACE != NULL) {
		free(schema->NAMESPACE);
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
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(schema, 0, sizeof(struct schema));
	TAILQ_INIT(&schema->attributes);
	TAILQ_INIT(&schema->tables);
	TAILQ_INIT(&schema->enums);
	return schema;
bail:	if (schema != NULL) {
		schema_destroy(schema);
	}
	return NULL;
}

static int schema_check (struct schema *schema)
{
	struct schema_enum *anum;
	struct schema_enum *nanum;
	struct schema_enum_field *anum_field;

	struct schema_table *table;
	struct schema_table *ntable;
	struct schema_table_field *table_field;
	struct schema_table_field *ntable_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		for (nanum = anum->list.tqe_next; nanum; nanum = nanum->list.tqe_next) {
			if (strcmp(anum->name, nanum->name) == 0) {
				linearbuffers_errorf("schema enum name: %s is invalid", anum->name);
				goto bail;
			}
		}
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (anum->type != NULL) {
			if (!schema_type_is_scalar(anum->type)) {
				linearbuffers_errorf("schema enum type: %s is invalid", anum->type);
				goto bail;
			}
		}
		TAILQ_FOREACH(anum_field, &anum->fields, list) {
			if (anum_field->value != NULL) {
				if (!schema_value_is_scalar(anum_field->value)) {
					linearbuffers_errorf("schema enum field value: %s is invalid", anum_field->value);
					goto bail;
				}
			}
		}
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		for (ntable = table->list.tqe_next; ntable; ntable = ntable->list.tqe_next) {
			if (strcmp(table->name, ntable->name) == 0) {
				linearbuffers_errorf("schema table name: %s is invalid", table->name);
				goto bail;
			}
		}
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		TAILQ_FOREACH(table_field, &table->fields, list) {
			if (!schema_type_is_valid(schema, table_field->type)) {
				linearbuffers_errorf("schema table field type: %s is invalid", table_field->type);
				goto bail;
			}
		}
		TAILQ_FOREACH(table_field, &table->fields, list) {
			for (ntable_field = table_field->list.tqe_next; ntable_field; ntable_field = ntable_field->list.tqe_next) {
				if (strcmp(table_field->name, ntable_field->name) == 0) {
					linearbuffers_errorf("schema table field name: %s is invalid", table_field->name);
					goto bail;
				}
			}
		}
	}

	return 0;
bail:	return -1;
}

static int schema_build (struct schema *schema)
{
	int rc;
	size_t i;

	struct schema_enum *anum;
	struct schema_enum_field *anum_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}

	rc = schema_check(schema);
	if (rc != 0) {
		linearbuffers_errorf("schema scheck failed");
		goto bail;
	}

	if (schema->count_type == schema_count_type_default) {
		schema->count_type = schema_count_type_uint32;
	}
	if (schema->offset_type == schema_offset_type_default) {
		schema->offset_type = schema_count_type_uint32;
	}

	if (schema->namespace == NULL) {
		schema->namespace = strdup("linearbuffers");
		if (schema->namespace == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
	for (i = 0; i < strlen(schema->namespace); i++) {
		schema->namespace[i] = tolower(schema->namespace[i]);
		if (schema->namespace[i] == '.') {
			schema->namespace[i] = '_';
		}
	}

	schema->NAMESPACE = strdup(schema->namespace);
	if (schema->NAMESPACE == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->NAMESPACE); i++) {
		schema->NAMESPACE[i] = toupper(schema->NAMESPACE[i]);
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (anum->type == NULL) {
			TAILQ_FOREACH(anum_field, &anum->fields, list) {
				if (anum_field->value != NULL) {
					if (*anum_field->value == '-') {
						break;
					}
				}
			}
			if (anum_field != NULL) {
				int64_t value;
				int64_t maximum;
				int64_t minimum;
				minimum = INT64_MAX;
				maximum = INT64_MIN;
				TAILQ_FOREACH(anum_field, &anum->fields, list) {
					if (anum_field->value != NULL) {
						value = strtoll(anum_field->value, NULL, 0);
						minimum = MIN(minimum, value);
						maximum = MAX(maximum, value);
					}
				}
				if (minimum < INT32_MIN || maximum > INT32_MAX) {
					anum->type = strdup("int64");
				} else if (minimum < INT16_MIN || maximum > INT16_MAX) {
					anum->type = strdup("int32");
				} else if (minimum < INT8_MIN || maximum > INT8_MAX) {
					anum->type = strdup("int16");
				} else {
					anum->type = strdup("int8");
				}
			} else {
				uint64_t value;
				uint64_t maximum;
				uint64_t minimum;
				minimum = UINT64_MAX;
				maximum = 0;
				TAILQ_FOREACH(anum_field, &anum->fields, list) {
					if (anum_field->value != NULL) {
						value = strtoull(anum_field->value, NULL, 0);
						minimum = MIN(minimum, value);
						maximum = MAX(maximum, value);
					}
				}
				if (maximum > UINT32_MAX) {
					anum->type = strdup("uint64");
				} else if (maximum > UINT16_MAX) {
					anum->type = strdup("uint32");
				} else if (maximum > UINT8_MAX) {
					anum->type = strdup("uint16");
				} else {
					anum->type = strdup("uint8");
				}
			}
		}
		if (anum->type == NULL) {
			linearbuffers_errorf("schema anum type is invalid");
			goto bail;
		}
		anum->TYPE = strdup(anum->type);
		if (anum->TYPE == NULL) {
			linearbuffers_errorf("schema anum type is invalid");
			goto bail;
		}
		for (i = 0; i < strlen(anum->TYPE); i++) {
			anum->TYPE[i] = toupper(anum->TYPE[i]);
		}
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (strncmp(anum->type, "int", 3) == 0) {
			int64_t pvalue;
			pvalue = 0;
			TAILQ_FOREACH(anum_field, &anum->fields, list) {
				if (anum_field->value == NULL) {
					rc = asprintf(&anum_field->value, "INT64_C(%" PRIi64 ")", pvalue);
					if (rc < 0) {
						linearbuffers_errorf("can not set schema enum field value");
						goto bail;
					}
				} else {
					pvalue = strtoll(anum_field->value, NULL, 0);
				}
				pvalue += 1;
			}
		} else {
			uint64_t pvalue;
			pvalue = 0;
			TAILQ_FOREACH(anum_field, &anum->fields, list) {
				if (anum_field->value == NULL) {
					rc = asprintf(&anum_field->value, "UINT64_C(%" PRIu64 ")", pvalue);
					if (rc < 0) {
						linearbuffers_errorf("can not set schema enum field value");
						goto bail;
					}
				} else {
					pvalue = strtoull(anum_field->value, NULL, 0);
				}
				pvalue += 1;
			}
		}
	}

	return 0;
bail:	return -1;
}

static char * read_file (const char *filename, size_t max_size, size_t *size_out)
{
	FILE *fp;
	size_t size, pos, n, _out;
	char *buf;

	size = 0;
	buf = NULL;
	size_out = size_out ? size_out : &_out;

	fp = fopen(filename, "rb");
	if (fp == NULL) {
		linearbuffers_errorf("can not open file: %s", filename);
		goto bail;
	}

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	*size_out = size;
	if (max_size > 0 && size > max_size) {
		linearbuffers_errorf("size is too big");
		goto bail;
	}
	rewind(fp);

	buf = malloc(size ? size : 1);
	if (buf == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}

	pos = 0;
	while ((n = fread(buf + pos, 1, size - pos, fp))) {
		pos += n;
	}
	if (pos != size) {
		goto bail;
	}
	fclose(fp);
	*size_out = size;
	return buf;

bail:	if (fp) {
		fclose(fp);
	}
	if (buf) {
		free(buf);
	}
	*size_out = size;
	return 0;
}

int yyparse (struct schema_parser *schema_parser);

struct schema * schema_parse_file (const char *filename)
{
	int rc;
	char *buffer;
	size_t buffer_length;

	YY_BUFFER_STATE bs;
	struct schema_parser schema_parser;

	buffer = NULL;
	memset(&schema_parser, 0, sizeof(struct schema_parser));

	if (filename == NULL) {
		linearbuffers_errorf("filename is invalid");
		goto bail;
	}

	buffer = read_file(filename, -1, &buffer_length);
	if (buffer == NULL) {
		linearbuffers_errorf("can not read file: %s", filename);
		goto bail;
	}

	bs = yy_scan_bytes(buffer, buffer_length);
	yy_switch_to_buffer(bs);
	rc = yyparse(&schema_parser);
	yy_delete_buffer(bs);

	if (rc != 0) {
		linearbuffers_errorf("can not parse file: %s", filename);
		goto bail;
	}

	rc = schema_check(schema_parser.schema);
	if (rc != 0) {
		linearbuffers_errorf("schema check for file: %s failed", filename);
		goto bail;
	}
	rc = schema_build(schema_parser.schema);
	if (rc != 0) {
		linearbuffers_errorf("schema build for file: %s failed", filename);
		goto bail;
	}

	free(buffer);
	return schema_parser.schema;
bail:	if (buffer != NULL) {
		free(buffer);
	}
	if (schema_parser.schema != NULL) {
		schema_destroy(schema_parser.schema);
	}
	return NULL;
}
