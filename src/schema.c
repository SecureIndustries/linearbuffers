
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "parser.lex.h"

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
	struct schema_enums enums;
	struct schema_tables tables;
	struct schema_attributes attributes;
};

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
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->value != NULL) {
		free(field->value);
		field->value = NULL;
	}
	if (value != NULL) {
		field->value = strdup(value);
		if (field->value == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_field_set_name (struct schema_enum_field *field, const char *name)
{
	if (field == NULL) {
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->name != NULL) {
		free(field->name);
		field->name = NULL;
	}
	if (name != NULL) {
		field->name = strdup(name);
		if (field->name == NULL) {
			fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "anum is invalid\n");
		goto bail;
	}
	if (anum->type != NULL) {
		free(anum->type);
		anum->type = NULL;
	}
	if (type != NULL) {
		anum->type = strdup(type);
		if (anum->type == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_set_name (struct schema_enum *anum, const char *name)
{
	if (anum == NULL) {
		fprintf(stderr, "anum is invalid\n");
		goto bail;
	}
	if (anum->name != NULL) {
		free(anum->name);
		anum->name = NULL;
	}
	if (name != NULL) {
		anum->name = strdup(name);
		if (anum->name == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_enum_add_field (struct schema_enum *anum, struct schema_enum_field *field)
{
	if (anum == NULL) {
		fprintf(stderr, "anum is invalid\n");
		goto bail;
	}
	if (field == NULL) {
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->name == NULL) {
		fprintf(stderr, "field name is invalid\n");
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
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	field->vector = !!vector;
	return 0;
bail:	return -1;
}

int schema_table_field_set_type (struct schema_table_field *field, const char *type)
{
	if (field == NULL) {
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->type != NULL) {
		free(field->type);
		field->type = NULL;
	}
	if (type != NULL) {
		field->type = strdup(type);
		if (field->type == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_table_field_set_name (struct schema_table_field *field, const char *name)
{
	if (field == NULL) {
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->name != NULL) {
		free(field->name);
		field->name = NULL;
	}
	if (name != NULL) {
		field->name = strdup(name);
		if (field->name == NULL) {
			fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "table is invalid\n");
		goto bail;
	}
	if (table->name != NULL) {
		free(table->name);
		table->name = NULL;
	}
	if (name != NULL) {
		table->name = strdup(name);
		if (table->name == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_table_add_field (struct schema_table *table, struct schema_table_field *field)
{
	if (table == NULL) {
		fprintf(stderr, "table is invalid\n");
		goto bail;
	}
	if (field == NULL) {
		fprintf(stderr, "field is invalid\n");
		goto bail;
	}
	if (field->name == NULL) {
		fprintf(stderr, "field name is invalid\n");
		goto bail;
	}
	if (field->type == NULL) {
		fprintf(stderr, "field type is invalid\n");
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
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (schema->namespace != NULL) {
		free(schema->namespace);
		schema->namespace = NULL;
	}
	if (name != NULL) {
		schema->namespace = strdup(name);
		if (schema->namespace == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
	}
	return 0;
bail:	return -1;
}

int schema_add_table (struct schema *schema, struct schema_table *table)
{
	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (table == NULL) {
		fprintf(stderr, "table is invalid\n");
		goto bail;
	}
	if (table->name == NULL) {
		fprintf(stderr, "table name is invalid\n");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&schema->tables, table, list);
	return 0;
bail:	return -1;
}

int schema_add_enum (struct schema *schema, struct schema_enum *anum)
{
	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (anum == NULL) {
		fprintf(stderr, "anum is invalid\n");
		goto bail;
	}
	if (anum->name == NULL) {
		fprintf(stderr, "anum name is invalid\n");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&schema->enums, anum, list);
	return 0;
bail:	return -1;
}

int schema_set_root (struct schema *schema, const char *name)
{
	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (schema->root != NULL) {
		free(schema->root);
		schema->root = NULL;
	}
	if (name != NULL) {
		schema->root = strdup(name);
		if (schema->root == NULL) {
			fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		for (nanum = anum->list.tqe_next; nanum; nanum = nanum->list.tqe_next) {
			if (strcmp(anum->name, nanum->name) == 0) {
				fprintf(stderr, "schema enum name: %s is invalid\n", anum->name);
				goto bail;
			}
		}
	}

	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (anum->type != NULL) {
			if (!type_is_scalar(anum->type)) {
				fprintf(stderr, "schema enum type: %s is invalid\n", anum->type);
				goto bail;
			}
		}
		TAILQ_FOREACH(anum_field, &anum->fields, list) {
			if (anum_field->value != NULL) {
				if (!value_is_scalar(anum_field->value)) {
					fprintf(stderr, "schema enum field value: %s is invalid\n", anum_field->value);
					goto bail;
				}
			}
		}
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		for (ntable = table->list.tqe_next; ntable; ntable = ntable->list.tqe_next) {
			if (strcmp(table->name, ntable->name) == 0) {
				fprintf(stderr, "schema table name: %s is invalid\n", table->name);
				goto bail;
			}
		}
	}

	TAILQ_FOREACH(table, &schema->tables, list) {
		TAILQ_FOREACH(table_field, &table->fields, list) {
			if (!type_is_valid(schema, table_field->type)) {
				fprintf(stderr, "schema table field type: %s is invalid\n", table_field->type);
				goto bail;
			}
		}
		TAILQ_FOREACH(table_field, &table->fields, list) {
			for (ntable_field = table_field->list.tqe_next; ntable_field; ntable_field = ntable_field->list.tqe_next) {
				if (strcmp(table_field->name, ntable_field->name) == 0) {
					fprintf(stderr, "schema table field name: %s is invalid\n", table_field->name);
					goto bail;
				}
			}
		}
	}

	if (schema->root == NULL) {
		fprintf(stderr, "schema root is invalid\n");
		goto bail;
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		if (strcmp(schema->root, table->name) == 0) {
			break;
		}
	}
	if (table == NULL) {
		fprintf(stderr, "schema root is invalid\n");
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}

	rc = schema_check(schema);
	if (rc != 0) {
		fprintf(stderr, "schema scheck failed\n");
		goto bail;
	}

	if (schema->namespace == NULL) {
		schema->namespace = strdup("linearbuffers");
		if (schema->namespace == NULL) {
			fprintf(stderr, "can not allocate memory");
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
		fprintf(stderr, "can not allocate memory");
		goto bail;
	}
	sprintf(schema->namespace_, "%s_", schema->namespace);

	schema->NAMESPACE = strdup(schema->namespace);
	if (schema->NAMESPACE == NULL) {
		fprintf(stderr, "can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->NAMESPACE); i++) {
		schema->NAMESPACE[i] = toupper(schema->NAMESPACE[i]);
	}

	schema->NAMESPACE_ = strdup(schema->namespace_);
	if (schema->NAMESPACE_ == NULL) {
		fprintf(stderr, "can not allocate memory");
		goto bail;
	}
	for (i = 0; i < strlen(schema->NAMESPACE_); i++) {
		schema->NAMESPACE_[i] = toupper(schema->NAMESPACE_[i]);
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
			fprintf(stderr, "schema anum type is invalid\n");
			goto bail;
		}
		anum->type_t = malloc(strlen(anum->type) + 1 + 1 + 4);
		if (anum->type_t == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			goto bail;
		}
		sprintf(anum->type_t, "%s_t", anum->type);
		anum->TYPE = strdup(anum->type);
		if (anum->TYPE == NULL) {
			fprintf(stderr, "schema anum type is invalid\n");
			goto bail;
		}
		for (i = 0; i < strlen(anum->TYPE); i++) {
			anum->TYPE[i] = toupper(anum->TYPE[i]);
		}
		anum->TYPE_C = malloc(strlen(anum->TYPE) + 1 + 1 + 4);
		if (anum->TYPE_C == NULL) {
			fprintf(stderr, "can not allocate memory\n");
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
						fprintf(stderr, "can not set schema enum field valud\n");
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
						fprintf(stderr, "can not set schema enum field valud\n");
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
		fprintf(stderr, "can not open file: %s\n", filename);
		goto bail;
	}

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	*size_out = size;
	if (max_size > 0 && size > max_size) {
		fprintf(stderr, "size is too big\n");
		goto bail;
	}
	rewind(fp);

	buf = malloc(size ? size : 1);
	if (buf == NULL) {
		fprintf(stderr, "can not allocate memory\n");
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
		fprintf(stderr, "filename is invalid\n");
		goto bail;
	}

	buffer = read_file(filename, -1, &buffer_length);
	if (buffer == NULL) {
		fprintf(stderr, "can not read file: %s\n", filename);
		goto bail;
	}

	bs = yy_scan_bytes(buffer, buffer_length);
	yy_switch_to_buffer(bs);
	rc = yyparse(&schema_parser);
	yy_delete_buffer(bs);

	if (rc != 0) {
		fprintf(stderr, "can not parse file: %s\n", filename);
		goto bail;
	}

	rc = schema_check(schema_parser.schema);
	if (rc != 0) {
		fprintf(stderr, "schema check for file: %s failed\n", filename);
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (filename == NULL) {
		fprintf(stderr, "filename is invalid\n");
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
		fprintf(stderr, "can not dump to file: %s\n", filename);
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
	return -1;
}

static int schema_generate_encoder_enum (struct schema *schema, struct schema_enum *anum, FILE *fp)
{
	struct schema_enum_field *anum_field;

	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (anum == NULL) {
		fprintf(stderr, "enum is invalid\n");
		goto bail;
	}
	if (fp == NULL) {
		fprintf(stderr, "fp is invalid\n");
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (anum == NULL) {
		fprintf(stderr, "enum is invalid\n");
		goto bail;
	}
	if (fp == NULL) {
		fprintf(stderr, "fp is invalid\n");
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

TAILQ_HEAD(namespace_elements, namespace_element);
struct namespace_element {
	TAILQ_ENTRY(namespace_element) list;
	char *string;
	size_t length;
};

struct namespace {
	struct namespace_elements elements;
	int dirty;
	char *linearized;
	size_t slinearized;
};

static void namespace_element_destroy (struct namespace_element *element)
{
	if (element == NULL) {
		return;
	}
	if (element->string != NULL) {
		free(element->string);
	}
	free(element);
}

static struct namespace_element * namespace_element_create (const char *string)
{
	struct namespace_element *element;
	element = malloc(sizeof(struct namespace_element));
	if (element == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(element, 0, sizeof(struct namespace_element));
	element->string = strdup(string);
	if (element->string == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	element->length = strlen(string);
	return element;
bail:	if (element != NULL) {
		namespace_element_destroy(element);
	}
	return NULL;
}

static const char * namespace_linearized (struct namespace *namespace)
{
	size_t slinearized;
	struct namespace_element *element;
	if (namespace == NULL) {
		return NULL;
	}
	if (namespace->dirty == 0) {
		return namespace->linearized;
	}
	slinearized = 0;
	TAILQ_FOREACH(element, &namespace->elements, list) {
		slinearized += element->length;
	}
	if (slinearized > namespace->slinearized) {
		if (namespace->linearized != NULL) {
			free(namespace->linearized);
			namespace->linearized = NULL;
		}
		namespace->linearized = malloc(slinearized + 1 + 4);
		if (namespace->linearized == NULL) {
			fprintf(stderr, "can not allocate memory\n");
			namespace->slinearized = 0;
			goto bail;
		}
		namespace->slinearized = slinearized;
	}
	slinearized = 0;
	TAILQ_FOREACH(element, &namespace->elements, list) {
		slinearized += sprintf(namespace->linearized + slinearized, "%s", element->string);
	}
	namespace->dirty = 0;
	return namespace->linearized;
bail:	return NULL;
}

static int namespace_push (struct namespace *namespace, const char *push)
{
	struct namespace_element *element;
	element = NULL;
	if (namespace == NULL) {
		fprintf(stderr, "namespace is invalid\n");
		goto bail;
	}
	if (push == NULL) {
		fprintf(stderr, "push is invalid\n");
		goto bail;
	}
	element = namespace_element_create(push);
	if (element == NULL) {
		fprintf(stderr, "can not create namespace element\n");
		goto bail;
	}
	TAILQ_INSERT_TAIL(&namespace->elements, element, list);
	namespace->dirty = 1;
	return 0;
bail:	if (element != NULL) {
		namespace_element_destroy(element);
	}
	return -1;
}

static int namespace_pop (struct namespace *namespace)
{
	struct namespace_element *element;
	element = NULL;
	if (namespace == NULL) {
		fprintf(stderr, "namespace is invalid\n");
		goto bail;
	}
	if (TAILQ_EMPTY(&namespace->elements)) {
		fprintf(stderr, "namespace is empty\n");
		goto bail;
	}
	element = TAILQ_LAST(&namespace->elements, namespace_elements);
	TAILQ_REMOVE(&namespace->elements, element, list);
	namespace_element_destroy(element);
	namespace->dirty = 1;
	return 0;
bail:	if (element != NULL) {
		namespace_element_destroy(element);
	}
	return -1;
}

static void namespace_destroy (struct namespace *namespace)
{
	struct namespace_element *element;
	struct namespace_element *nelement;
	if (namespace == NULL) {
		return;
	}
	TAILQ_FOREACH_SAFE(element, &namespace->elements, list, nelement) {
		TAILQ_REMOVE(&namespace->elements, element, list);
		namespace_element_destroy(element);
	}
	free(namespace);
}

static struct namespace * namespace_create (void)
{
	struct namespace *namespace;
	namespace = malloc(sizeof(struct namespace));
	if (namespace == NULL) {
		fprintf(stderr, "can not allocate memory\n");
		goto bail;
	}
	memset(namespace, 0, sizeof(struct namespace));
	TAILQ_INIT(&namespace->elements);
	return namespace;
bail:	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return NULL;
}

static int schema_generate_encoder_table (struct schema *schema, struct schema_table *table, struct namespace *namespace, uint64_t element, FILE *fp)
{
	uint64_t table_field_i;
	struct schema_table_field *table_field;

	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (table == NULL) {
		fprintf(stderr, "table is invalid\n");
		goto bail;
	}
	if (fp == NULL) {
		fprintf(stderr, "fp is invalid\n");
		goto bail;
	}

	fprintf(fp, "static inline int %sstart (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace));
	fprintf(fp, "{\n");
	fprintf(fp, "    int rc;\n");
	fprintf(fp, "    rc = linearbuffers_encoder_table_start(encoder, UINT64_C(%" PRIu64 "), UINT64_C(%" PRIu64 "));\n", element, table->nfields);
	fprintf(fp, "    return rc;\n");
	fprintf(fp, "}\n");
	table_field_i = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %sset_%s (struct linearbuffers_encoder *encoder, %s_t *value_%s, uint64_t value_%s_count)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_vector_%s(encoder, UINT64_C(%" PRIu64 "), value_%s, value_%s_count);\n", table_field->type, table_field_i, table_field->name, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_start(encoder, UINT64_C(%" PRIu64 "));\n", table_field_i);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_push (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_push_%s(encoder, value_%s);\n", table_field->type, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_set (struct linearbuffers_encoder *encoder, uint64_t at, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_set_%s(encoder, at, value_%s);\n", table_field->type, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_vector_end(encoder);\n");
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %s%s_set_%s (struct linearbuffers_encoder *encoder, %s%s_enum_t *value_%s, uint64_t count)\n", namespace_linearized(namespace), table->name, table_field->name, namespace_linearized(namespace), table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    (void) encoder;\n");
				fprintf(fp, "    (void) value_%s;\n", table_field->name);
				fprintf(fp, "    (void) count;\n");
				fprintf(fp, "    return 0;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_%s_start (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    (void) encoder;\n");
				fprintf(fp, "    return 0;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_%s_push (struct linearbuffers_encoder *encoder, %s%s_enum_t value_%s)\n", namespace_linearized(namespace), table->name, table_field->name, namespace_linearized(namespace), table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    (void) encoder;\n");
				fprintf(fp, "    (void) value_%s;\n", table_field->name);
				fprintf(fp, "    return 0;\n");
				fprintf(fp, "}\n");
				fprintf(fp, "static inline int %s%s_%s_end (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace), table->name, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    (void) encoder;\n");
				fprintf(fp, "    return 0;\n");
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
				schema_generate_encoder_table(schema, type_get_table(schema, table_field->type), namespace, UINT64_MAX, fp);
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
				fprintf(stderr, "type is invalid: %s\n", table_field->type);
				goto bail;
			}
		} else {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline int %sset_%s (struct linearbuffers_encoder *encoder, %s_t value_%s)\n", namespace_linearized(namespace), table_field->name, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), value_%s);\n", table_field->type, table_field_i, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline int %sset_%s (struct linearbuffers_encoder *encoder, %s%s_enum_t value_%s)\n", namespace_linearized(namespace), table_field->name, schema->namespace_, table_field->type, table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    int rc;\n");
				fprintf(fp, "    rc = linearbuffers_encoder_table_set_%s(encoder, UINT64_C(%" PRIu64 "), value_%s);\n", type_get_enum(schema, table_field->type)->type, table_field_i, table_field->name);
				fprintf(fp, "    return rc;\n");
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				schema_generate_encoder_table(schema, type_get_table(schema, table_field->type), namespace, table_field_i, fp);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				fprintf(stderr, "type is invalid: %s\n", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
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
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (filename == NULL) {
		fprintf(stderr, "filename is invalid\n");
		goto bail;
	}

	rc = schema_build(schema);
	if (rc != 0) {
		fprintf(stderr, "can not build schema\n");
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
		fprintf(stderr, "can not dump to file: %s\n", filename);
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <linearbuffers/encoder.h>\n");

	if (!TAILQ_EMPTY(&schema->enums)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_ENUM_API)\n", schema->NAMESPACE);
		fprintf(fp, "#define %s_ENUM_API\n", schema->NAMESPACE);
		fprintf(fp, "\n");

		TAILQ_FOREACH(anum, &schema->enums, list) {
			schema_generate_encoder_enum(schema, anum, fp);
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_ENCODER_API)\n", schema->NAMESPACE);
		fprintf(fp, "#define %s_ENCODER_API\n", schema->NAMESPACE);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			fprintf(stderr, "schema root is invalid\n");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			fprintf(stderr, "can not create namespace\n");
			goto bail;
		}
		namespace_push(namespace, schema->namespace_);
		namespace_push(namespace, table->name);
		namespace_push(namespace, "_");
		schema_generate_encoder_table(schema, table, namespace, UINT64_MAX, fp);
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
	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return -1;
}

static int schema_generate_decoder_table (struct schema *schema, struct schema_table *table, struct namespace *namespace, uint64_t element, FILE *fp)
{
	uint64_t table_field_i;
	struct schema_table_field *table_field;

	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (table == NULL) {
		fprintf(stderr, "table is invalid\n");
		goto bail;
	}
	if (fp == NULL) {
		fprintf(stderr, "fp is invalid\n");
		goto bail;
	}

	(void) element;

	if (element == UINT64_MAX) {
		fprintf(fp, "static inline int %sdecode (struct linearbuffers_encoder *encoder)\n", namespace_linearized(namespace));
		fprintf(fp, "{\n");
		fprintf(fp, "    (void) encoder;\n");
		fprintf(fp, "    return 0;\n");
		fprintf(fp, "}\n");
	}
	table_field_i = 0;
	TAILQ_FOREACH(table_field, &table->fields, list) {
		if (table_field->vector) {
			if (type_is_scalar(table_field->type)) {
			} else if (type_is_enum(schema, table_field->type)) {
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				namespace_push(namespace, table_field->type);
				namespace_push(namespace, "_");
				schema_generate_decoder_table(schema, type_get_table(schema, table_field->type), namespace, UINT64_MAX, fp);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				fprintf(stderr, "type is invalid: %s\n", table_field->type);
				goto bail;
			}
		} else {
			if (type_is_scalar(table_field->type)) {
				fprintf(fp, "static inline %s_t %sget_%s (struct linearbuffers_decoder *decoder)\n", table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    %s_t value;\n", table_field->type);
				fprintf(fp, "    memcpy(&offset, decoder->buffer + (sizeof(uint64_t) * UINT64_C(%" PRIu64 ")), sizeof(offset));\n", table_field_i);
				fprintf(fp, "    memcpy(&value, decoder->buffer + offset, sizeof(value));\n");
				fprintf(fp, "    return value;\n");
				fprintf(fp, "}\n");
			} else if (type_is_enum(schema, table_field->type)) {
				fprintf(fp, "static inline %s%s_enum_t %sget_%s (struct linearbuffers_decoder *decoder)\n", schema->namespace_, table_field->type, namespace_linearized(namespace), table_field->name);
				fprintf(fp, "{\n");
				fprintf(fp, "    uint64_t offset;\n");
				fprintf(fp, "    %s%s_enum_t value;\n", schema->namespace_, table_field->type);
				fprintf(fp, "    memcpy(&offset, decoder->buffer + (sizeof(uint64_t) * UINT64_C(%" PRIu64 ")), sizeof(offset));\n", table_field_i);
				fprintf(fp, "    memcpy(&value, decoder->buffer + offset, sizeof(value));\n");
				fprintf(fp, "    return value;\n");
				fprintf(fp, "}\n");
			} else if (type_is_table(schema, table_field->type)) {
				namespace_push(namespace, table_field->name);
				namespace_push(namespace, "_");
				schema_generate_decoder_table(schema, type_get_table(schema, table_field->type), namespace, table_field_i, fp);
				namespace_pop(namespace);
				namespace_pop(namespace);
			} else {
				fprintf(stderr, "type is invalid: %s\n", table_field->type);
				goto bail;
			}
		}
		table_field_i += 1;
	}

	return 0;
bail:	return -1;
}

int schema_generate_decoder (struct schema *schema, const char *filename)
{
	int rc;
	FILE *fp;

	struct namespace *namespace;
	struct schema_enum *anum;
	struct schema_table *table;

	fp = NULL;
	namespace = NULL;

	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (filename == NULL) {
		fprintf(stderr, "filename is invalid\n");
		goto bail;
	}

	rc = schema_build(schema);
	if (rc != 0) {
		fprintf(stderr, "can not build schema\n");
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
		fprintf(stderr, "can not dump to file: %s\n", filename);
		goto bail;
	}

	fprintf(fp, "\n");
	fprintf(fp, "#include <stddef.h>\n");
	fprintf(fp, "#include <stdint.h>\n");
	fprintf(fp, "#include <string.h>\n");
	fprintf(fp, "#include <linearbuffers/decoder.h>\n");

	if (!TAILQ_EMPTY(&schema->enums)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_ENUM_API)\n", schema->NAMESPACE);
		fprintf(fp, "#define %s_ENUM_API\n", schema->NAMESPACE);
		fprintf(fp, "\n");

		TAILQ_FOREACH(anum, &schema->enums, list) {
			schema_generate_decoder_enum(schema, anum, fp);
		}

		fprintf(fp, "\n");
		fprintf(fp, "#endif\n");
	}

	if (!TAILQ_EMPTY(&schema->tables)) {
		fprintf(fp, "\n");
		fprintf(fp, "#if !defined(%s_DECODER_API)\n", schema->NAMESPACE);
		fprintf(fp, "#define %s_DECODER_API\n", schema->NAMESPACE);
		fprintf(fp, "\n");

		TAILQ_FOREACH(table, &schema->tables, list) {
			if (strcmp(schema->root, table->name) == 0) {
				break;
			}
		}
		if (table == NULL) {
			fprintf(stderr, "schema root is invalid\n");
			goto bail;
		}

		namespace = namespace_create();
		if (namespace == NULL) {
			fprintf(stderr, "can not create namespace\n");
			goto bail;
		}
		namespace_push(namespace, schema->namespace_);
		namespace_push(namespace, table->name);
		namespace_push(namespace, "_");
		schema_generate_decoder_table(schema, table, namespace, UINT64_MAX, fp);
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
	if (namespace != NULL) {
		namespace_destroy(namespace);
	}
	return -1;
}
