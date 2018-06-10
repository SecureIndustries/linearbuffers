
struct schema_attribute;
struct schema_enum_field;
struct schema_enum;
struct schema_table_field;
struct schema_table;
struct schema;

void schema_attribute_destroy (struct schema_attribute *attribute);
struct schema_attribute * schema_attribute_create (void);

int schema_enum_field_set_name (struct schema_enum_field *field, const char *name);
int schema_enum_field_set_value (struct schema_enum_field *field, const char *value);
void schema_enum_field_destroy (struct schema_enum_field *field);
struct schema_enum_field * schema_enum_field_create (void);

int schema_enum_set_name (struct schema_enum *anum, const char *name);
int schema_enum_set_type (struct schema_enum *anum, const char *type);
int schema_enum_add_field (struct schema_enum *anum, struct schema_enum_field *field);
void schema_enum_destroy (struct schema_enum *anum);
struct schema_enum * schema_enum_create (void);

int schema_table_field_set_name (struct schema_table_field *field, const char *name);
int schema_table_field_set_type (struct schema_table_field *field, const char *type);
void schema_table_field_destroy (struct schema_table_field *field);
struct schema_table_field * schema_table_field_create (void);

int schema_table_set_name (struct schema_table *table, const char *name);
int schema_table_add_field (struct schema_table *table, struct schema_table_field *field);
void schema_table_destroy (struct schema_table *table);
struct schema_table * schema_table_create (void);

int schema_set_namespace (struct schema *schema, const char *name);
int schema_add_enum (struct schema *schema, struct schema_enum *anum);
int schema_add_table (struct schema *schema, struct schema_table *table);
void schema_destroy (struct schema *schema);
struct schema * schema_create (void);

struct schema * schema_parse_file (const char *filename);
