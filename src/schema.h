
struct schema_attribute;
struct schema_enum_field;
struct schema_enum;
struct schema_table_field;
struct schema_table;
struct schema;

enum {
	schema_count_type_default,
	schema_count_type_uint8,
	schema_count_type_uint16,
	schema_count_type_uint32,
	schema_count_type_uint64
};

enum {
	schema_offset_type_default,
	schema_offset_type_uint8,
	schema_offset_type_uint16,
	schema_offset_type_uint32,
	schema_offset_type_uint64
};

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
int schema_table_field_set_vector (struct schema_table_field *field, int vector);
int schema_table_field_set_value (struct schema_table_field *field, const char *value);
int schema_table_field_add_attribute (struct schema_table_field *field, const char *name, const char *value);
void schema_table_field_destroy (struct schema_table_field *field);
struct schema_table_field * schema_table_field_create (void);

int schema_table_set_name (struct schema_table *table, const char *name);
int schema_table_add_field (struct schema_table *table, struct schema_table_field *field);
void schema_table_destroy (struct schema_table *table);
struct schema_table * schema_table_create (void);

int schema_set_namespace (struct schema *schema, const char *name);
int schema_set_count_type (struct schema *schema, const char *type);
int schema_set_offset_type (struct schema *schema, const char *type);
int schema_add_enum (struct schema *schema, struct schema_enum *anum);
int schema_add_table (struct schema *schema, struct schema_table *table);
void schema_destroy (struct schema *schema);
struct schema * schema_create (void);

struct schema * schema_parse_file (const char *filename);

struct schema_enum * schema_type_get_enum (struct schema *schema, const char *type);
struct schema_table * schema_type_get_table (struct schema *schema, const char *type);

uint64_t schema_inttype_size (const char *type);
int schema_type_is_scalar (const char *type);
int schema_type_is_float (const char *type);
int schema_type_is_string (const char *type);
int schema_type_is_enum (struct schema *schema, const char *type);
int schema_type_is_table (struct schema *schema, const char *type);
int schema_type_is_valid (struct schema *schema, const char *type);
int schema_value_is_scalar (const char *value);

const char * schema_count_type_name (uint32_t type);
uint32_t schema_count_type_value (const char *type);
uint64_t schema_count_type_size (uint32_t type);

const char * schema_offset_type_name (uint32_t type);
uint32_t schema_offset_type_value (const char *type);
uint64_t schema_offset_type_size (uint32_t type);
