
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
	char *TYPE;
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
	struct schema_table_fields fields;
	struct schema_attributes attributes;
};

struct schema {
	char *namespace;
	char *namespace_;
	char *NAMESPACE;
	char *NAMESPACE_;
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

static int type_is_valid (struct schema *schema, const char *type)
{
	int rc;
	struct schema_enum *anum;
	struct schema_table *table;
	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		return 0;
	}
	if (type == NULL) {
		return 0;
	}
	rc = type_is_scalar(type);
	if (rc == 1) {
		return 1;
	}
	TAILQ_FOREACH(anum, &schema->enums, list) {
		if (strcmp(anum->name, type) == 0) {
			return 1;
		}
	}
	TAILQ_FOREACH(table, &schema->tables, list) {
		if (strcmp(table->name, type) == 0) {
			return 1;
		}
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
		schema->namespace = strdup("");
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
		anum->TYPE = strdup(anum->type);
		if (anum->TYPE == NULL) {
			fprintf(stderr, "schema anum type is invalid\n");
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
					rc = asprintf(&anum_field->value, "%" PRIi64 "", pvalue);
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
					rc = asprintf(&anum_field->value, "%" PRIu64 "", pvalue);
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

int schema_generate_encoder (struct schema *schema, const char *filename)
{
	int rc;
	FILE *fp;

	struct schema_enum *anum;
	struct schema_enum_field *anum_field;

	fp = NULL;

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
	fprintf(fp, "#include <stdint.h>\n");

	TAILQ_FOREACH(anum, &schema->enums, list) {
		fprintf(fp, "\n");
		if (anum->type != NULL) {
			fprintf(fp, "typedef %s_t %s%s_enum_t;\n", anum->type, schema->namespace_, anum->name);
		} else {
			fprintf(fp, "typedef %s_t %s%s_enum_t;\n", "uint32", schema->namespace_, anum->name);
		}
		TAILQ_FOREACH(anum_field, &anum->fields, list) {
			if (anum_field->value != NULL) {
				fprintf(fp, "#define %s%s_%s ((%s%s_enum_t) %s_C(%s))\n", schema->namespace_, anum->name, anum_field->name, schema->namespace_, anum->name, anum->TYPE, anum_field->value);
			} else {
				fprintf(fp, "#define %s%s_%s\n", schema->namespace_, anum->name, anum_field->name);
			}
		}
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
	return -1;
}

int schema_generate_decoder (struct schema *schema, const char *filename)
{
	if (schema == NULL) {
		fprintf(stderr, "schema is invalid\n");
		goto bail;
	}
	if (filename == NULL) {
		fprintf(stderr, "filename is invalid\n");
		goto bail;
	}
	return 0;
bail:	return -1;
}
