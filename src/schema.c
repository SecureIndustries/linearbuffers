
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.lex.h"

#define LINEARBUFFERS_DEBUG_NAME "schema"

#include "debug.h"
#include "queue.h"
#include "schema.h"
#include "parser.h"

#if !defined(MIN)
#define MIN(a, b)	(((a) < (b)) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a, b)	(((a) > (b)) ? (a) : (b))
#endif

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
	char *type_t;
	char *TYPE;
	char *TYPE_C;
	struct schema_enum_fields fields;
	struct schema_attributes attributes;
};

TAILQ_HEAD(schema_table_fields, schema_table_field);
struct schema_table_field {
	TAILQ_ENTRY(schema_table_field) list;
	char *name;
	char *type;
	int vector;
	struct schema_attributes attributes;
};

TAILQ_HEAD(schema_tables, schema_table);
struct schema_table {
	TAILQ_ENTRY(schema_table) list;
	char *name;
	uint64_t nfields;
	struct schema_table_fields fields;
	struct schema_attributes attributes;
};

struct schema {
	char *namespace;
	char *namespace_;
	char *NAMESPACE;
	char *NAMESPACE_;
	char *root;
	char *ROOT;
	struct schema_enums enums;
	struct schema_tables tables;
	struct schema_attributes attributes;
};

static int inttype_size (const char *type)
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
	return 0;
}

static int type_is_scalar (const char *type)
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

static int type_is_string (const char *type)
{
	if (type == NULL) {
		return 0;
	}
	if (strcmp(type, "string") == 0) {
		return 1;
	}
	return 0;
}

static int type_is_enum (struct schema *schema, const char *type)
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

static struct schema_enum * type_get_enum (struct schema *schema, const char *type)
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

static struct schema_table * type_get_table (struct schema *schema, const char *type)
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

static int type_is_table (struct schema *schema, const char *type)
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

static int type_is_valid (struct schema *schema, const char *type)
{
	int rc;
	if (schema == NULL) {
		return 0;
	}
	if (type == NULL) {
		return 0;
	}
	rc = type_is_scalar(type);
	if (rc == 1) {
		return 1;
	}
	rc = type_is_string(type);
	if (rc == 1) {
		return 1;
	}
	rc = type_is_enum(schema, type);
	if (rc == 1) {
		return 1;
	}
	rc = type_is_table(schema, type);
	if (rc == 1) {
		return 1;
	}
	return 0;
}

static int value_is_scalar (const char *value)
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
	if (anum->type_t != NULL) {
		free(anum->type_t);
	}
	if (anum->TYPE != NULL) {
		free(anum->TYPE);
	}
	if (anum->TYPE_C != NULL) {
		free(anum->TYPE_C);
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
	field->vector = !!vector;
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

int schema_set_root (struct schema *schema, const char *name)
{
	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (schema->root != NULL) {
		free(schema->root);
		schema->root = NULL;
	}
	if (name != NULL) {
		schema->root = strdup(name);
		if (schema->root == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
	}
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
	if (schema->namespace_ != NULL) {
		free(schema->namespace_);
	}
	if (schema->NAMESPACE != NULL) {
		free(schema->NAMESPACE);
	}
	if (schema->NAMESPACE_ != NULL) {
		free(schema->NAMESPACE_);
	}
	if (schema->root != NULL) {
		free(schema->root);
	}
	if (schema->ROOT != NULL) {
		free(schema->ROOT);
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
			if (!type_is_scalar(anum->type)) {
				linearbuffers_errorf("schema enum type: %s is invalid", anum->type);
				goto bail;
			}
		}
		TAILQ_FOREACH(anum_field, &anum->fields, list) {
			if (anum_field->value != NULL) {
				if (!value_is_scalar(anum_field->value)) {
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
			if (!type_is_valid(schema, table_field->type)) {
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

	if (schema->root == NULL) {
		linearbuffers_errorf("schema root is invalid");
		goto bail;
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		if (strcmp(schema->root, table->name) == 0) {
			break;
		}
	}
	if (table == NULL) {
		linearbuffers_errorf("schema root is invalid");
		goto bail;
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
	schema->namespace_ = malloc(strlen(schema->namespace) + 1 + 1);
	if (schema->namespace_ == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	sprintf(schema->namespace_, "%s_", schema->namespace);

	schema->NAMESPACE = strdup(schema->namespace);
	if (schema->NAMESPACE == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->NAMESPACE); i++) {
		schema->NAMESPACE[i] = toupper(schema->NAMESPACE[i]);
	}

	schema->NAMESPACE_ = strdup(schema->namespace_);
	if (schema->NAMESPACE_ == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->NAMESPACE_); i++) {
		schema->NAMESPACE_[i] = toupper(schema->NAMESPACE_[i]);
	}

	schema->ROOT = strdup(schema->root);
	if (schema->ROOT == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->ROOT); i++) {
		schema->ROOT[i] = toupper(schema->ROOT[i]);
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
		anum->type_t = malloc(strlen(anum->type) + 1 + 1 + 4);
		if (anum->type_t == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
		sprintf(anum->type_t, "%s_t", anum->type);
		anum->TYPE = strdup(anum->type);
		if (anum->TYPE == NULL) {
			linearbuffers_errorf("schema anum type is invalid");
			goto bail;
		}
		for (i = 0; i < strlen(anum->TYPE); i++) {
			anum->TYPE[i] = toupper(anum->TYPE[i]);
		}
		anum->TYPE_C = malloc(strlen(anum->TYPE) + 1 + 1 + 4);
		if (anum->TYPE_C == NULL) {
			linearbuffers_errorf("can not allocate memory");
			goto bail;
		}
		sprintf(anum->TYPE_C, "%s_C", anum->TYPE);
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

int schema_generate_pretty (struct schema *schema, const char *filename)
{
	FILE *fp;

	struct schema_enum *anum;
	struct schema_enum_field *anum_field;

	struct schema_table *table;
	struct schema_table_field *table_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (filename == NULL) {
		linearbuffers_errorf("filename is invalid");
		goto bail;
	}

	if (strcmp(filename, "stdout") == 0) {
		fp = stdout;
	} else if (strcmp(filename, "stderr") == 0) {
		fp = stderr;
	} else {
		fp = fopen(filename, "w");
	}
	if (fp == NULL) {
		linearbuffers_errorf("can not dump to file: %s", filename);
		goto bail;
	}

	if (schema->namespace != NULL) {
		fprintf(fp, "\n");
		fprintf(fp, "namescpace %s\n", schema->namespace);
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		fprintf(fp, "\n");
		if (anum->type != NULL) {
			fprintf(fp, "enum %s : %s {\n", anum->name, anum->type);
		} else {
			fprintf(fp, "enum %s {\n", anum->name);
		}
		TAILQ_FOREACH(anum_field, &anum->fields, list) {
			if (anum_field->value != NULL) {
				fprintf(fp, "\t%s = %s", anum_field->name, anum_field->value);
			} else {
				fprintf(fp, "\t%s", anum_field->name);
			}
			if (anum_field->list.tqe_next != NULL) {
				fprintf(fp, ",\n");
			} else {
				fprintf(fp, "\n");
			}
		}
		fprintf(fp, "}\n");
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		fprintf(fp, "\n");
		fprintf(fp, "table %s {\n", table->name);
		TAILQ_FOREACH(table_field, &table->fields, list) {
			if (table_field->vector == 0)  {
				fprintf(fp, "\t%s: %s", table_field->name, table_field->type);
			} else {
				fprintf(fp, "\t%s: [ %s ]", table_field->name, table_field->type);
			}
			fprintf(fp, ";\n");
		}
		fprintf(fp, "}\n");
	}

	if (fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	return 0;
bail:	if (fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	if (fp != stdout &&
	    fp != stderr &&
	    filename != NULL) {
		unlink(filename);
	}
	return -1;
}

static int schema_generate_encoder_enum (struct schema *schema, struct schema_enum *anum, FILE *fp)
{
	struct schema_enum_field *anum_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (anum == NULL) {
		linearbuffers_errorf("enum is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
		goto bail;
	}

	fprintf(fp, "typedef %s %s%s_enum_t;\n", anum->type_t, schema->namespace_, anum->name);

	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		if (anum_field->value != NULL) {
			fprintf(fp, "#define %s%s_%s ((%s%s_enum_t) %s(%s))\n", schema->namespace_, anum->name, anum_field->name, schema->namespace_, anum->name, anum->TYPE_C, anum_field->value);
		} else {
			fprintf(fp, "#define %s%s_%s\n", schema->namespace_, anum->name, anum_field->name);
		}
	}

	fprintf(fp, "\n");
	fprintf(fp, "static inline const char * %s%s_string (%s%s_enum_t value)\n", schema->namespace_, anum->name, schema->namespace_, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s%s_%s: return \"%s\";\n", schema->namespace_, anum->name, anum_field->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return \"%s\";\n", "");
	fprintf(fp, "}\n");
	fprintf(fp, "static inline int %s%s_is_valid (%s%s_enum_t value)\n", schema->namespace_, anum->name, schema->namespace_, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s%s_%s: return 1;\n", schema->namespace_, anum->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return 0;\n");
	fprintf(fp, "}\n");

	return 0;
bail:	return -1;
}

static int schema_generate_decoder_enum (struct schema *schema, struct schema_enum *anum, FILE *fp)
{
	struct schema_enum_field *anum_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (anum == NULL) {
		linearbuffers_errorf("enum is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
		goto bail;
	}

	fprintf(fp, "typedef %s %s%s_enum_t;\n", anum->type_t, schema->namespace_, anum->name);

	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		if (anum_field->value != NULL) {
			fprintf(fp, "#define %s%s_%s ((%s%s_enum_t) %s(%s))\n", schema->namespace_, anum->name, anum_field->name, schema->namespace_, anum->name, anum->TYPE_C, anum_field->value);
		} else {
			fprintf(fp, "#define %s%s_%s\n", schema->namespace_, anum->name, anum_field->name);
		}
	}

	fprintf(fp, "\n");
	fprintf(fp, "static inline const char * %s%s_string (%s%s_enum_t value)\n", schema->namespace_, anum->name, schema->namespace_, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s%s_%s: return \"%s\";\n", schema->namespace_, anum->name, anum_field->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return \"%s\";\n", "");
	fprintf(fp, "}\n");
	fprintf(fp, "static inline int %s%s_is_valid (%s%s_enum_t value)\n", schema->namespace_, anum->name, schema->namespace_, anum->name);
	fprintf(fp, "{\n");
	fprintf(fp, "    switch (value) {\n");
	TAILQ_FOREACH(anum_field, &anum->fields, list) {
		fprintf(fp, "        case %s%s_%s: return 1;\n", schema->namespace_, anum->name, anum_field->name);
	}
	fprintf(fp, "    }\n");
	fprintf(fp, "    return 0;\n");
	fprintf(fp, "}\n");

	return 0;
bail:	return -1;
}

TAILQ_HEAD(element_entries, element_entry);
struct element_entry {
	TAILQ_ENTRY(element_entry) list;
	struct schema_table *table;
	uint64_t id;
	uint64_t offset;
};

struct element {
	struct element_entries entries;
	uint64_t nentries;
};

static void element_entry_destroy (struct element_entry *entry)
{
	if (entry == NULL) {
		return;
	}
	free(entry);
}

static struct element_entry * element_entry_create (struct schema_table *table, uint64_t id, uint64_t offset)
{
	struct element_entry *entry;
	entry = malloc(sizeof(struct element_entry));
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct element_entry));
	entry->table = table;
	entry->id = id;
	entry->offset = offset;
	return entry;
bail:	if (entry != NULL) {
		element_entry_destroy(entry);
	}
	return NULL;
}

static int element_push (struct element *element, struct schema_table *table, uint64_t id, uint64_t offset)
{
	struct element_entry *entry;
	entry = NULL;
	if (element == NULL) {
		linearbuffers_errorf("element is invalid");
		goto bail;
	}
	entry = element_entry_create(table, id, offset);
	if (entry == NULL) {
		linearbuffers_errorf("can not create element entry");
		goto bail;
	}
	element->nentries += 1;
	TAILQ_INSERT_TAIL(&element->entries, entry, list);
	return 0;
bail:	if (entry != NULL) {
		element_entry_destroy(entry);
	}
	return -1;
}

static int element_pop (struct element *element)
{
	struct element_entry *entry;
	entry = NULL;
	if (element == NULL) {
		linearbuffers_errorf("element is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&element->entries)) {
		linearbuffers_errorf("element is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&element->entries, element_entries);
	TAILQ_REMOVE(&element->entries, entry, list);
	element->nentries -= 1;
	element_entry_destroy(entry);
	return 0;
bail:	if (entry != NULL) {
		element_entry_destroy(entry);
	}
	return -1;
}

static void element_destroy (struct element *element)
{
	struct element_entry *entry;
	struct element_entry *nentry;
	if (element == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(entry, &element->entries, list, nentry) {
		TAILQ_REMOVE(&element->entries, entry, list);
		element_entry_destroy(entry);
	}
	free(element);
}

static struct element * element_create (void)
{
	struct element *element;
	element = malloc(sizeof(struct element));
	if (element == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(element, 0, sizeof(struct element));
	TAILQ_INIT(&element->entries);
	return element;
bail:	if (element != NULL) {
		element_destroy(element);
	}
	return NULL;
}

TAILQ_HEAD(namespace_entries, namespace_entry);
struct namespace_entry {
	TAILQ_ENTRY(namespace_entry) list;
	char *string;
	size_t length;
};

struct namespace {
	struct namespace_entries entries;
	int dirty;
	char *linearized;
	size_t slinearized;
};

static void namespace_entry_destroy (struct namespace_entry *entry)
{
	if (entry == NULL) {
		return;
	}
	if (entry->string != NULL) {
		free(entry->string);
	}
	free(entry);
}

static struct namespace_entry * namespace_entry_create (const char *string)
{
	struct namespace_entry *entry;
	entry = malloc(sizeof(struct namespace_entry));
	if (entry == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(entry, 0, sizeof(struct namespace_entry));
	entry->string = strdup(string);
	if (entry->string == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	entry->length = strlen(string);
	return entry;
bail:	if (entry != NULL) {
		namespace_entry_destroy(entry);
	}
	return NULL;
}

static const char * namespace_linearized (struct namespace *namespace)
{
	size_t slinearized;
	struct namespace_entry *entry;
	if (namespace == NULL) {
		return NULL;
	}
	if (namespace->dirty == 0) {
		return namespace->linearized;
	}
	slinearized = 0;
	TAILQ_FOREACH(entry, &namespace->entries, list) {
		slinearized += entry->length;
	}
	if (slinearized > namespace->slinearized) {
		if (namespace->linearized != NULL) {
			free(namespace->linearized);
			namespace->linearized = NULL;
		}
		namespace->linearized = malloc(slinearized + 1 + 4);
		if (namespace->linearized == NULL) {
			linearbuffers_errorf("can not allocate memory");
			namespace->slinearized = 0;
			goto bail;
		}
		namespace->slinearized = slinearized;
	}
	slinearized = 0;
	TAILQ_FOREACH(entry, &namespace->entries, list) {
		slinearized += sprintf(namespace->linearized + slinearized, "%s", entry->string);
	}
	namespace->dirty = 0;
	return namespace->linearized;
bail:	return NULL;
}

static int namespace_push (struct namespace *namespace, const char *push)
{
	struct namespace_entry *entry;
	entry = NULL;
	if (namespace == NULL) {
		linearbuffers_errorf("namespace is invalid");
		goto bail;
	}
	if (push == NULL) {
		linearbuffers_errorf("push is invalid");
		goto bail;
	}
	entry = namespace_entry_create(push);
	if (entry == NULL) {
		linearbuffers_errorf("can not create namespace entry");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&namespace->entries, entry, list);
	namespace->dirty = 1;
	return 0;
bail:	if (entry != NULL) {
		namespace_entry_destroy(entry);
	}
	return -1;
}

static int namespace_pop (struct namespace *namespace)
{
	struct namespace_entry *entry;
	entry = NULL;
	if (namespace == NULL) {
		linearbuffers_errorf("namespace is invalid");
		goto bail;
	}
	if (TAILQ_EMPTY(&namespace->entries)) {
		linearbuffers_errorf("namespace is empty");
		goto bail;
	}
	entry = TAILQ_LAST(&namespace->entries, namespace_entries);
	TAILQ_REMOVE(&namespace->entries, entry, list);
	namespace_entry_destroy(entry);
	namespace->dirty = 1;
	return 0;
bail:	if (entry != NULL) {
		namespace_entry_destroy(entry);
	}
	return -1;
}

static void namespace_destroy (struct namespace *namespace)
{
	struct namespace_entry *entry;
	struct namespace_entry *nentry;
	if (namespace == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(entry, &namespace->entries, list, nentry) {
		TAILQ_REMOVE(&namespace->entries, entry, list);
		namespace_entry_destroy(entry);
	}
	free(namespace);
}

static struct namespace * namespace_create (void)
{
	struct namespace *namespace;
	namespace = malloc(sizeof(struct namespace));
	if (namespace == NULL) {
		linearbuffers_errorf("can not allocate memory");
		goto bail;
	}
	memset(namespace, 0, sizeof(struct namespace));
	TAILQ_INIT(&namespace->entries);
	return namespace;
bail:	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return NULL;
}

static int schema_generate_encoder_table (struct schema *schema, struct schema_table *table, struct namespace *namespace, uint64_t element, uint64_t offset, FILE *fp)
{
	int rc;
	uint64_t table_field_i;
	uint64_t table_field_s;
	struct schema_table_field *table_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
		goto bail;
	}

	table_field_s = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_scalar(table_field->type)) {
			table_field_s += inttype_size(table_field->type);
		} else if (type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_enum(schema, table_field->type)) {
			table_field_s += inttype_size(type_get_enum(schema, table_field->type)->type);
		} else if (type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else {
			linearbuffers_errorf("type is invalid: %s", table_field->type);
			goto bail;
		}

	}
	fprintf(fp, "static inline int %sstart (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace));
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_start(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "));\n", element, offset, table->nfields, table_field_s);
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");

	table_field_i = 0;
	table_field_s = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, const %s_t *value_%s, uint64_t value_%s_count)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", table_field->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start(encoder, UINT64_C(%" PRIu64 "));\n", table_field_i);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_push (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_set_at (struct linearbuffers_encoder *encoder, uint64_t at, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_set_%s(encoder, at, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, const char **value_%s, uint64_t value_%s_count)\n", namespace_linearized(namespace), table_field->name, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", table_field->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start(encoder, UINT64_C(%" PRIu64 "));\n", table_field_i);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_push (struct linearbuffers_encoder *encoder, const char *value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_set_at (struct linearbuffers_encoder *encoder, uint64_t at, const char *value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_set_%s(encoder, at, value_%s);\n", table_field->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, %s%s_enum_t *value_%s, uint64_t value_%s_count)\n", namespace_linearized(namespace), table_field->name, schema->namespace_, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", type_get_enum(schema, table_field->type)->type, table_field_i, table_field_s, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start(encoder, UINT64_C(%" PRIu64 "));\n", table_field_i);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_%s_push (struct linearbuffers_encoder *encoder, %s%s_enum_t value_%s)\n", namespace_linearized(namespace), table->name, table_field->name, schema->namespace_, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", type_get_enum(schema, table_field->type)->type, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_set_at (struct linearbuffers_encoder *encoder, uint64_t at, %s%s_enum_t value_%s)\n", namespace_linearized(namespace), table_field->name, schema->namespace_, table_field->type, table_field->type);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_set_%s(encoder, at, value_%s);\n", type_get_enum(schema, table_field->type)->type, table_field->type);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				fprintf(fp, "static inline int %s%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start(encoder, UINT64_C(%" PRIu64 "));\n", table_field_i);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				namespace_push(namespace, table_field->type);
				namespace_push(namespace, "_");
				rc = schema_generate_encoder_table(schema, type_get_table(schema, table_field->type), namespace, UINT64_MAX, UINT64_MAX, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate encoder for table: %s", table_field->name);
					goto bail;
				}
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
				fprintf(fp, "static inline int %s%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		} else {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, const char *value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, %s%s_enum_t value_%s)\n", namespace_linearized(namespace), table_field->name, schema->namespace_, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "), value_%s);\n", type_get_enum(schema, table_field->type)->type, table_field_i, table_field_s, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				rc = schema_generate_encoder_table(schema, type_get_table(schema, table_field->type), namespace, table_field_i, table_field_s, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate encoder for table: %s", table_field->name);
					goto bail;
				}
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_scalar(table_field->type)) {
			table_field_s += inttype_size(table_field->type);
		} else if (type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_enum(schema, table_field->type)) {
			table_field_s += inttype_size(type_get_enum(schema, table_field->type)->type);
		} else if (type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		}
	}
	fprintf(fp, "static inline int %send (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace));
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_end(encoder);\n");
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");

	return 0;
bail:	return -1;
}

int schema_generate_encoder (struct schema *schema, const char *filename)
{
	int rc;
	FILE *fp;

	struct namespace *namespace;
	struct schema_enum *anum;
	struct schema_table *table;

	fp = NULL;
	namespace = NULL;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (filename == NULL) {
		linearbuffers_errorf("filename is invalid");
		goto bail;
	}

	rc = schema_build(schema);
	if (rc != 0) {
		linearbuffers_errorf("can not build schema");
		goto bail;
	}

	if (strcmp(filename, "stdout") == 0) {
		fp = stdout;
	} else if (strcmp(filename, "stderr") == 0) {
		fp = stderr;
	} else {
		fp = fopen(filename, "w");
	}
	if (fp == NULL) {
		linearbuffers_errorf("can not dump to file: %s", filename);
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <linearbuffers/encoder.h>\n");

	if (!TAILQ_EMPTY(&schema->enums)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(anum, &schema->enums, list) {
			schema_generate_encoder_enum(schema, anum, fp);
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENCODER_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_ENCODER_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			linearbuffers_errorf("schema root is invalid");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			linearbuffers_errorf("can not create namespace");
			goto bail;
		}
		namespace_push(namespace, schema->namespace_);
		namespace_push(namespace, table->name);
		namespace_push(namespace, "_");
		rc = schema_generate_encoder_table(schema, table, namespace, UINT64_MAX, UINT64_MAX, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate encoder for table: %s", table->name);
			goto bail;
		}
		namespace_destroy(namespace);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	return 0;
bail:	if (fp != NULL &&
	    fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	if (fp != stdout &&
	    fp != stderr &&
	    filename != NULL) {
		unlink(filename);
	}
	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return -1;
}

static int schema_generate_decoder_table (struct schema *schema, struct schema_table *table, struct namespace *namespace, struct element *element, int decoder_use_memcpy, FILE *fp)
{
	int rc;
	uint64_t table_field_i;
	uint64_t table_field_s;
	struct schema_table_field *table_field;

	struct element_entry *element_entry;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
		goto bail;
	}

	if (TAILQ_EMPTY(&element->entries)) {
		fprintf(fp, "static inline int %sdecode (struct linearbuffers_decoder *decoder, const void *buffer, uint64_t length)\n", namespace_linearized(namespace));
		fprintf(fp, "{\n");
		fprintf(fp, "    int rc;\n");
		fprintf(fp, "    rc = linearbuffers_decoder_init(decoder, buffer, length);\n");
		fprintf(fp, "    return rc;\n");
		fprintf(fp, "}\n");
	}

	table_field_i = 0;
	table_field_s = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		fprintf(fp, "static inline int %s%s_present (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
		fprintf(fp, "{\n");
		fprintf(fp, "    uint8_t present;\n");
		fprintf(fp, "    uint64_t offset;\n");
		fprintf(fp, "    offset = 0;\n");
		TAILQ_FOREACH(element_entry, &element->entries, list) {
			if (decoder_use_memcpy) {
				fprintf(fp, "    memcpy(&present, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(present));\n", sizeof(uint8_t) * (element_entry->id / 8));
			} else {
				fprintf(fp, "    present = *(uint8_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", sizeof(uint8_t) * (element_entry->id / 8));
			}
			fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (element_entry->id % 8)));
			fprintf(fp, "        return 0;\n");
			fprintf(fp, "    }\n");
			if (decoder_use_memcpy) {
				fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
			} else {
				fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
			}
		}
		if (decoder_use_memcpy) {
			fprintf(fp, "    memcpy(&present, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(present));\n", sizeof(uint8_t) * (table_field_i / 8));
		} else {
			fprintf(fp, "    present = *(uint8_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", sizeof(uint8_t) * (table_field_i / 8));
		}
		fprintf(fp, "    if (!(present & 0x%02x)) {\n", (1 << (table_field_i % 8)));
		fprintf(fp, "        return 0;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    return 1;\n");
		fprintf(fp, "}\n");
		if (table_field->vector) {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline const %s_t * %s%s_get (struct linearbuffers_decoder *decoder)\n", table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return decoder->buffer + offset + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
				fprintf(fp, "}\n");
				fprintf(fp, "static inline %s_t %s%s_get_at (struct linearbuffers_decoder *decoder, uint64_t at)\n", table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return ((%s_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 ")))[at];\n", table_field->type, sizeof(uint64_t));
				fprintf(fp, "}\n");
				fprintf(fp, "static inline uint64_t %s%s_get_count (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&count, decoder->buffer + offset, sizeof(count));\n");
				} else {
					fprintf(fp, "    count = *(uint64_t *) (decoder->buffer + offset);\n");
				}
				fprintf(fp, "    return count;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline uint64_t %s%s_get_length (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&count, decoder->buffer + offset, sizeof(count));\n");
				} else {
					fprintf(fp, "    count = *(uint64_t *) (decoder->buffer + offset);\n");
				}
				fprintf(fp, "    return count * sizeof(%s_t);\n", table_field->type);
				fprintf(fp, "}\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "static inline const char * %s%s_get_at (struct linearbuffers_decoder *decoder, uint64_t at)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + sizeof(uint64_t) + (sizeof(uint64_t) * at), sizeof(offset));\n");
				} else {
					fprintf(fp, "    offset = ((uint64_t *) (decoder->buffer + offset + sizeof(uint64_t)))[at];\n");
				}
				fprintf(fp, "    return (const char *) (decoder->buffer + offset);\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline uint64_t %s%s_get_count (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&count, decoder->buffer + offset, sizeof(count));\n");
				} else {
					fprintf(fp, "    count = *(uint64_t *) (decoder->buffer + offset);\n");
				}
				fprintf(fp, "    return count;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline const %s%s_enum_t * %s%s_get (struct linearbuffers_decoder *decoder)\n", schema->namespace_, table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return decoder->buffer + offset + UINT64_C(%" PRIu64 ");\n", sizeof(uint64_t));
				fprintf(fp, "}\n");
				fprintf(fp, "static inline %s%s_enum_t %s%s_get_at (struct linearbuffers_decoder *decoder, uint64_t at)\n", schema->namespace_, table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return ((%s%s_enum_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 ")))[at];\n", schema->namespace_, table_field->type, sizeof(uint64_t));
				fprintf(fp, "}\n");
				fprintf(fp, "static inline uint64_t %s%s_get_count (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&count, decoder->buffer + offset, sizeof(count));\n");
				} else {
					fprintf(fp, "    count = *(uint64_t *) (decoder->buffer + offset);\n");
				}
				fprintf(fp, "    return count;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline uint64_t %s%s_get_length (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    uint64_t count;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&count, decoder->buffer + offset, sizeof(count));\n");
				} else {
					fprintf(fp, "    count = *(uint64_t *) (decoder->buffer + offset);\n");
				}
				fprintf(fp, "    return count * sizeof(%s%s_enum_t);\n", schema->namespace_, table_field->type);
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				namespace_push(namespace, table_field->type);
				namespace_push(namespace, "_");
				element_push(element, table, table_field_i, table_field_s);
				rc = schema_generate_decoder_table(schema, type_get_table(schema, table_field->type), namespace, element, decoder_use_memcpy, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate decoder for table: %s", table->name);
					goto bail;
				}
				element_pop(element);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		} else {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline %s_t %s%s_get (struct linearbuffers_decoder *decoder)\n", table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    %s_t value;\n", table_field->type);
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&value, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(value));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    value = *(%s_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return value;\n");
				fprintf(fp, "}\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "static inline const char * %s%s_get (struct linearbuffers_decoder *decoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(value));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return (const char *) (decoder->buffer + offset);\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline %s%s_enum_t %s%s_get (struct linearbuffers_decoder *decoder)\n", schema->namespace_, table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    %s%s_enum_t value;\n", schema->namespace_, table_field->type);
				fprintf(fp, "    offset = 0;\n");
				TAILQ_FOREACH(element_entry, &element->entries, list) {
					if (decoder_use_memcpy) {
						fprintf(fp, "    memcpy(&offset, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(offset));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					} else {
						fprintf(fp, "    offset = *(uint64_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", ((element_entry->table->nfields + 7) / 8) + element_entry->offset);
					}
				}
				if (decoder_use_memcpy) {
					fprintf(fp, "    memcpy(&value, decoder->buffer + offset + UINT64_C(%" PRIu64 "), sizeof(value));\n", ((table->nfields + 7) / 8) + table_field_s);
				} else {
					fprintf(fp, "    value = *(%s%s_enum_t *) (decoder->buffer + offset + UINT64_C(%" PRIu64 "));\n", schema->namespace_, table_field->type, ((table->nfields + 7) / 8) + table_field_s);
				}
				fprintf(fp, "    return value;\n");
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				element_push(element, table, table_field_i, table_field_s);
				rc = schema_generate_decoder_table(schema, type_get_table(schema, table_field->type), namespace, element, decoder_use_memcpy, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate decoder for table: %s", table->name);
					goto bail;
				}
				element_pop(element);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
		if (table_field->vector) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_scalar(table_field->type)) {
			table_field_s += inttype_size(table_field->type);
		} else if (type_is_string(table_field->type)) {
			table_field_s += sizeof(uint64_t);
		} else if (type_is_enum(schema, table_field->type)) {
			table_field_s += inttype_size(type_get_enum(schema, table_field->type)->type);
		} else if (type_is_table(schema, table_field->type)) {
			table_field_s += sizeof(uint64_t);
		}
	}

	return 0;
bail:	return -1;
}

int schema_generate_decoder (struct schema *schema, const char *filename, int decoder_use_memcpy)
{
	int rc;
	FILE *fp;

	struct element *element;
	struct namespace *namespace;

	struct schema_enum *anum;
	struct schema_table *table;

	fp = NULL;
	element = NULL;
	namespace = NULL;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (filename == NULL) {
		linearbuffers_errorf("filename is invalid");
		goto bail;
	}

	rc = schema_build(schema);
	if (rc != 0) {
		linearbuffers_errorf("can not build schema");
		goto bail;
	}

	if (strcmp(filename, "stdout") == 0) {
		fp = stdout;
	} else if (strcmp(filename, "stderr") == 0) {
		fp = stderr;
	} else {
		fp = fopen(filename, "w");
	}
	if (fp == NULL) {
		linearbuffers_errorf("can not dump to file: %s", filename);
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <string.h>\n");
	fprintf(fp, "#include <linearbuffers/decoder.h>\n");

	if (!TAILQ_EMPTY(&schema->enums)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(anum, &schema->enums, list) {
			schema_generate_decoder_enum(schema, anum, fp);
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_DECODER_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_DECODER_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			linearbuffers_errorf("schema root is invalid");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			linearbuffers_errorf("can not create namespace");
			goto bail;
		}
		rc  = namespace_push(namespace, schema->namespace_);
		rc |= namespace_push(namespace, table->name);
		rc |= namespace_push(namespace, "_");
		if (rc != 0) {
			linearbuffers_errorf("can not build namespace");
			goto bail;
		}

		element = element_create();
		if (element == NULL) {
			linearbuffers_errorf("can not create element");
			goto bail;
		}

		rc = schema_generate_decoder_table(schema, table, namespace, element, decoder_use_memcpy, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate decoder for table: %s", table->name);
			goto bail;
		}

		namespace_destroy(namespace);
		element_destroy(element);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	return 0;
bail:	if (fp != NULL &&
	    fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	if (fp != stdout &&
	    fp != stderr &&
	    filename != NULL) {
		unlink(filename);
	}
	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	if (element != NULL) {
		element_destroy(element);
	}
	return -1;
}

static int schema_generate_jsonify_table (struct schema *schema, struct schema_table *table, struct namespace *namespace, struct element *element, FILE *fp)
{
	int rc;
	char *prefix;
	uint64_t table_field_i;
	struct schema_table_field *table_field;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (table == NULL) {
		linearbuffers_errorf("table is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
		goto bail;
	}

	if (TAILQ_EMPTY(&element->entries)) {
		fprintf(fp, "static inline int %sjsonify (const void *buffer, uint64_t length, int (*emitter) (const char *format, ...))\n", namespace_linearized(namespace));
		fprintf(fp, "{\n");
		fprintf(fp, "    int rc;\n");
		fprintf(fp, "    struct linearbuffers_decoder decoder;\n");
		fprintf(fp, "    rc = %sdecode(&decoder, buffer, length);\n", namespace_linearized(namespace));
		fprintf(fp, "    if (rc != 0) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    if (emitter == NULL) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    rc = emitter(\"{\\n\");\n");
		fprintf(fp, "    if (rc < 0) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
	}

	prefix = malloc(((element->nentries + 1) * 4) + 1);
	memset(prefix, ' ', (element->nentries + 1) * 4);
	prefix[(element->nentries + 1) * 4] = '\0';

	table_field_i = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        uint64_t at;\n");
				fprintf(fp, "        uint64_t count;\n");
				fprintf(fp, "        const %s_t *value;\n", table_field->type);
				fprintf(fp, "        value = %s%s_get(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        count = %s%s_get_count(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": [\\n\");\n", prefix, table_field->name);
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        for (at = 0; at < count; at++) {\n");
				if (strncmp(table_field->type, "int", 3) == 0) {
					fprintf(fp, "            rc = emitter(\"%s    %%\" PRIi64 \"%%s\\n\", (int64_t) value[at], ((at + 1) == count) ? \"\" : \",\");\n", prefix);
				} else {
					fprintf(fp, "            rc = emitter(\"%s    %%\" PRIu64 \"%%s\\n\", (uint64_t) value[at], ((at + 1) == count) ? \"\" : \",\");\n", prefix);
				}
				fprintf(fp, "            if (rc < 0) {\n");
				fprintf(fp, "                goto bail;\n");
				fprintf(fp, "            }\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        rc = emitter(\"%s]%s\\n\");\n", prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        uint64_t at;\n");
				fprintf(fp, "        uint64_t count;\n");
				fprintf(fp, "        const char *value;\n");
				fprintf(fp, "        count = %s%s_get_count(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": [\\n\");\n", prefix, table_field->name);
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        for (at = 0; at < count; at++) {\n");
				fprintf(fp, "            value = %s%s_get_at(&decoder, at);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "            rc = emitter(\"%s    \\\"%%s\\\"%%s\\n\", value, ((at + 1) == count) ? \"\" : \",\");\n", prefix);
				fprintf(fp, "            if (rc < 0) {\n");
				fprintf(fp, "                goto bail;\n");
				fprintf(fp, "            }\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        rc = emitter(\"%s]%s\\n\");\n", prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        uint64_t at;\n");
				fprintf(fp, "        uint64_t count;\n");
				fprintf(fp, "        const %s%s_enum_t *value;\n", schema->namespace_, table_field->type);
				fprintf(fp, "        value = %s%s_get(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        count = %s%s_get_count(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": [\\n\");\n", prefix, table_field->name);
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        for (at = 0; at < count; at++) {\n");
				if (strncmp(type_get_enum(schema, table_field->type)->type, "int", 3) == 0) {
					fprintf(fp, "            rc = emitter(\"%s    %%\" PRIi64 \"%%s\\n\", (int64_t) value[at], ((at + 1) == count) ? \"\" : \",\");\n", prefix);
				} else {
					fprintf(fp, "            rc = emitter(\"%s    %%\" PRIu64 \"%%s\\n\", (uint64_t) value[at], ((at + 1) == count) ? \"\" : \",\");\n", prefix);
				}
				fprintf(fp, "            if (rc < 0) {\n");
				fprintf(fp, "                goto bail;\n");
				fprintf(fp, "            }\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "        rc = emitter(\"%s]%s\\n\");\n", prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				namespace_push(namespace, table_field->type);
				namespace_push(namespace, "_");
				element_push(element, table, table_field_i, 0);
				rc = schema_generate_jsonify_table(schema, type_get_table(schema, table_field->type), namespace, element, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate jsonify for table: %s", table->name);
					goto bail;
				}
				element_pop(element);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		} else {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        %s_t value;\n", table_field->type);
				fprintf(fp, "        value = %s%s_get(&decoder);\n", namespace_linearized(namespace), table_field->name);
				if (strncmp(table_field->type, "int", 3) == 0) {
					fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": %%\" PRIi64 \"%s\\n\", (int64_t) value);\n", prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				} else {
					fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": %%\" PRIu64 \"%s\\n\", (uint64_t) value);\n", prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				}
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_string(table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        const char *value;\n");
				fprintf(fp, "        value = %s%s_get(&decoder);\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": \\\"%%s\\\"%s\\n\", value);\n", prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        %s%s_enum_t value;\n", schema->namespace_, table_field->type);
				fprintf(fp, "        value = %s%s_get(&decoder);\n", namespace_linearized(namespace), table_field->name);
				if (strncmp(table_field->type, "int", 3) == 0) {
					fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": %%\" PRIi64 \"%s\\n\", (int64_t) value);\n", prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				} else {
					fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": %%\" PRIu64 \"%s\\n\", (uint64_t) value);\n", prefix, table_field->name, ((table_field_i + 1) == table->nfields) ? "" : ",");
				}
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else if (type_is_table(schema, table_field->type)) {
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s\\\"%s\\\": {\\n\");\n", prefix, table_field->name);
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				element_push(element, table, table_field_i, 0);
				rc = schema_generate_jsonify_table(schema, type_get_table(schema, table_field->type), namespace, element, fp);
				if (rc != 0) {
					linearbuffers_errorf("can not generate jsonify for table: %s", table->name);
					goto bail;
				}
				element_pop(element);
				namespace_pop(namespace);
				namespace_pop(namespace);
				fprintf(fp, "    if (%s%s_present(&decoder)) {\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "        rc = emitter(\"%s}%s\\n\");\n", prefix, ((table_field_i + 1) == table->nfields) ? "" : ",");
				fprintf(fp, "        if (rc < 0) {\n");
				fprintf(fp, "            goto bail;\n");
				fprintf(fp, "        }\n");
				fprintf(fp, "    }\n");
			} else {
				linearbuffers_errorf("type is invalid: %s", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
	}

	if (TAILQ_EMPTY(&element->entries)) {
		fprintf(fp, "    rc = emitter(\"}\\n\");\n");
		fprintf(fp, "    if (rc != 0) {\n");
		fprintf(fp, "        goto bail;\n");
		fprintf(fp, "    }\n");
		fprintf(fp, "    return rc;\n");
		fprintf(fp, "bail:\n");
		fprintf(fp, "    return -1;\n");
		fprintf(fp, "}\n");
	}

	free(prefix);
	return 0;
bail:	return -1;
}

int schema_generate_jsonify (struct schema *schema, const char *filename, int decoder_use_memcpy)
{
	int rc;
	FILE *fp;

	struct element *element;
	struct namespace *namespace;

	struct schema_enum *anum;
	struct schema_table *table;

	fp = NULL;
	element = NULL;
	namespace = NULL;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (filename == NULL) {
		linearbuffers_errorf("filename is invalid");
		goto bail;
	}

	rc = schema_build(schema);
	if (rc != 0) {
		linearbuffers_errorf("can not build schema");
		goto bail;
	}

	if (strcmp(filename, "stdout") == 0) {
		fp = stdout;
	} else if (strcmp(filename, "stderr") == 0) {
		fp = stderr;
	} else {
		fp = fopen(filename, "w");
	}
	if (fp == NULL) {
		linearbuffers_errorf("can not dump to file: %s", filename);
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <string.h>\n");
	fprintf(fp, "#include <inttypes.h>\n");
	fprintf(fp, "#include <linearbuffers/decoder.h>\n");
	fprintf(fp, "#include <linearbuffers/jsonify.h>\n");

	if (!TAILQ_EMPTY(&schema->enums)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_ENUM_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_ENUM_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(anum, &schema->enums, list) {
			schema_generate_decoder_enum(schema, anum, fp);
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_DECODER_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_DECODER_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			linearbuffers_errorf("schema root is invalid");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			linearbuffers_errorf("can not create namespace");
			goto bail;
		}
		rc  = namespace_push(namespace, schema->namespace_);
		rc |= namespace_push(namespace, table->name);
		rc |= namespace_push(namespace, "_");
		if (rc != 0) {
			linearbuffers_errorf("can not build namespace");
			goto bail;
		}

		element = element_create();
		if (element == NULL) {
			linearbuffers_errorf("can not create element");
			goto bail;
		}

		rc = schema_generate_decoder_table(schema, table, namespace, element, decoder_use_memcpy, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate decoder for table: %s", table->name);
			goto bail;
		}

		namespace_destroy(namespace);
		element_destroy(element);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_%s_JSONIFY_API)\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "#define %s_%s_JSONIFY_API\n", schema->NAMESPACE, schema->ROOT);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			linearbuffers_errorf("schema root is invalid");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			linearbuffers_errorf("can not create namespace");
			goto bail;
		}
		rc  = namespace_push(namespace, schema->namespace_);
		rc |= namespace_push(namespace, table->name);
		rc |= namespace_push(namespace, "_");
		if (rc != 0) {
			linearbuffers_errorf("can not build namespace");
			goto bail;
		}

		element = element_create();
		if (element == NULL) {
			linearbuffers_errorf("can not create element");
			goto bail;
		}

		rc = schema_generate_jsonify_table(schema, table, namespace, element, fp);
		if (rc != 0) {
			linearbuffers_errorf("can not generate jsonify for table: %s", table->name);
			goto bail;
		}

		namespace_destroy(namespace);
		element_destroy(element);

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	return 0;
bail:	if (fp != NULL &&
	    fp != stdout &&
	    fp != stderr) {
		fclose(fp);
	}
	if (fp != stdout &&
	    fp != stderr &&
	    filename != NULL) {
		unlink(filename);
	}
	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	if (element != NULL) {
		element_destroy(element);
	}
	return -1;
}
