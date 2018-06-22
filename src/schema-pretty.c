
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "schema.h"
#include "schema-private.h"

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
