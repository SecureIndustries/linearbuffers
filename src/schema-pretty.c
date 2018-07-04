
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>

#include "debug.h"
#include "schema.h"
#include "schema-private.h"

int schema_generate_pretty (struct schema *schema, FILE *fp)
{
	struct schema_enum *anum;
	struct schema_enum_field *anum_field;

	struct schema_table *table;
	struct schema_table_field *table_field;

	struct schema_attribute *attribute;

	if (schema == NULL) {
		linearbuffers_errorf("schema is invalid");
		goto bail;
	}
	if (fp == NULL) {
		linearbuffers_errorf("fp is invalid");
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
			if (table_field->container == schema_container_type_none)  {
				fprintf(fp, "\t%s: %s", table_field->name, table_field->type);
			} else {
				fprintf(fp, "\t%s: [ %s ]", table_field->name, table_field->type);
			}
			if (table_field->value != NULL) {
				fprintf(fp, " = %s", table_field->value);
			}
			if (!TAILQ_EMPTY(&table_field->attributes)) {
				fprintf(fp, " (");
				TAILQ_FOREACH(attribute, &table_field->attributes, list) {
					fprintf(fp, "%s%s", (TAILQ_FIRST(&table_field->attributes) == attribute) ? " " : ", ", attribute->name);
					if (attribute->value != NULL) {
						fprintf(fp, " = %s", attribute->value);
					}

				}
				fprintf(fp, " )");
			}
			fprintf(fp, ";\n");
		}
		fprintf(fp, "}\n");
	}

	return 0;
bail:	return -1;
}
