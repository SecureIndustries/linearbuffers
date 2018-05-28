
int schema_begin (void);
int schema_end (void);

int schema_set_namespace (const char *name);

int schema_enum_begin (const char *name, const char *type);
int schema_enum_end (void);

int schema_enum_entry_add (const char *name, const char *value);

int schema_table_begin (const char *name);
int schema_table_end (void);

int schema_table_field_add (const char *name, const char *type);
