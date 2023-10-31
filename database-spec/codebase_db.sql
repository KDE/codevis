BEGIN TRANSACTION;
DROP TABLE IF EXISTS "db_option";
CREATE TABLE IF NOT EXISTS "db_option" (
	"key"	integer NOT NULL,
	"value"	integer NOT NULL,
	PRIMARY KEY("key")
);
DROP TABLE IF EXISTS "source_file";
CREATE TABLE IF NOT EXISTS "source_file" (
	"id"	integer,
	"version"	integer NOT NULL,
	"package_id"	bigint,
	"component_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"is_header"	boolean NOT NULL,
	"hash"	text NOT NULL,
	CONSTRAINT "fk_source_file_package" FOREIGN KEY("package_id") REFERENCES "source_package"("id") deferrable initially deferred,
	CONSTRAINT "fk_source_file_component" FOREIGN KEY("component_id") REFERENCES "source_component"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "error_messages";
CREATE TABLE IF NOT EXISTS "error_messages" (
	"id"	integer,
	"version"	integer NOT NULL,
	"error_kind"	integer NOT NULL,
	"fully_qualified_name"	text NOT NULL,
	"error_message"	text NOT NULL,
	"file_name"	text NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "source_package";
CREATE TABLE IF NOT EXISTS "source_package" (
	"id"	integer,
	"version"	integer NOT NULL,
	"parent_id"	bigint,
	"source_repository_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"disk_path"	text NOT NULL,
	CONSTRAINT "fk_source_package_source_repository" FOREIGN KEY("source_repository_id") REFERENCES "source_repository"("id") deferrable initially deferred,
	CONSTRAINT "fk_source_package_parent" FOREIGN KEY("parent_id") REFERENCES "source_package"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "class_hierarchy";
CREATE TABLE IF NOT EXISTS "class_hierarchy" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	CONSTRAINT "fk_class_hierarchy_source" FOREIGN KEY("source_id") REFERENCES "class_declaration"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_class_hierarchy_target" FOREIGN KEY("target_id") REFERENCES "class_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "includes";
CREATE TABLE IF NOT EXISTS "includes" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	CONSTRAINT "fk_includes_source" FOREIGN KEY("source_id") REFERENCES "source_file"("id") deferrable initially deferred,
	CONSTRAINT "fk_includes_target" FOREIGN KEY("target_id") REFERENCES "source_file"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "source_component";
CREATE TABLE IF NOT EXISTS "source_component" (
	"id"	integer,
	"version"	integer NOT NULL,
	"qualified_name"	text NOT NULL,
	"name"	text NOT NULL,
	"package_id"	bigint,
	CONSTRAINT "fk_source_component_package" FOREIGN KEY("package_id") REFERENCES "source_package"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "class_declaration";
CREATE TABLE IF NOT EXISTS "class_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"parent_namespace_id"	bigint,
	"class_namespace_id"	bigint,
	"parent_package_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"kind"	integer NOT NULL,
	"access"	integer NOT NULL,
	CONSTRAINT "fk_class_declaration_parent_package" FOREIGN KEY("parent_package_id") REFERENCES "source_package"("id") deferrable initially deferred,
	CONSTRAINT "fk_class_declaration_parent_namespace" FOREIGN KEY("parent_namespace_id") REFERENCES "namespace_declaration"("id") deferrable initially deferred,
	CONSTRAINT "fk_class_declaration_class_namespace" FOREIGN KEY("class_namespace_id") REFERENCES "class_declaration"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "field_declaration";
CREATE TABLE IF NOT EXISTS "field_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"class_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"signature"	text NOT NULL,
	"access"	integer NOT NULL,
	"is_static"	boolean NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_field_declaration_class" FOREIGN KEY("class_id") REFERENCES "class_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "source_repository";
CREATE TABLE IF NOT EXISTS "source_repository" (
	"id"	integer,
	"version"	integer NOT NULL,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"disk_path"	text NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "component_relation";
CREATE TABLE IF NOT EXISTS "component_relation" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_component_relation_source" FOREIGN KEY("source_id") REFERENCES "source_component"("id") deferrable initially deferred,
	CONSTRAINT "fk_component_relation_target" FOREIGN KEY("target_id") REFERENCES "source_component"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "method_declaration";
CREATE TABLE IF NOT EXISTS "method_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"class_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"signature"	text NOT NULL,
	"return_type"	text NOT NULL,
	"template_parameters"	text NOT NULL,
	"access"	integer NOT NULL,
	"is_virtual"	boolean NOT NULL,
	"is_pure"	boolean NOT NULL,
	"is_static"	boolean NOT NULL,
	"is_const"	boolean NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_method_declaration_class" FOREIGN KEY("class_id") REFERENCES "class_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "function_declaration";
CREATE TABLE IF NOT EXISTS "function_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"namespace_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"signature"	text NOT NULL,
	"return_type"	text NOT NULL,
	"template_parameters"	text NOT NULL,
	CONSTRAINT "fk_function_declaration_namespace" FOREIGN KEY("namespace_id") REFERENCES "namespace_declaration"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "variable_declaration";
CREATE TABLE IF NOT EXISTS "variable_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"namespace_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	"signature"	text NOT NULL,
	"is_global"	boolean NOT NULL,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_variable_declaration_namespace" FOREIGN KEY("namespace_id") REFERENCES "namespace_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "namespace_declaration";
CREATE TABLE IF NOT EXISTS "namespace_declaration" (
	"id"	integer,
	"version"	integer NOT NULL,
	"parent_id"	bigint,
	"name"	text NOT NULL,
	"qualified_name"	text NOT NULL,
	CONSTRAINT "fk_namespace_declaration_parent" FOREIGN KEY("parent_id") REFERENCES "namespace_declaration"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT)
);
DROP TABLE IF EXISTS "uses_in_the_interface";
CREATE TABLE IF NOT EXISTS "uses_in_the_interface" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_uses_in_the_interface_target" FOREIGN KEY("target_id") REFERENCES "class_declaration"("id") deferrable initially deferred,
	CONSTRAINT "fk_uses_in_the_interface_source" FOREIGN KEY("source_id") REFERENCES "class_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "dependencies";
CREATE TABLE IF NOT EXISTS "dependencies" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	CONSTRAINT "fk_dependencies_target" FOREIGN KEY("target_id") REFERENCES "source_package"("id") deferrable initially deferred,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_dependencies_source" FOREIGN KEY("source_id") REFERENCES "source_package"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "uses_in_the_implementation";
CREATE TABLE IF NOT EXISTS "uses_in_the_implementation" (
	"id"	integer,
	"target_id"	bigint,
	"source_id"	bigint,
	PRIMARY KEY("id" AUTOINCREMENT),
	CONSTRAINT "fk_uses_in_the_implementation_target" FOREIGN KEY("target_id") REFERENCES "class_declaration"("id") deferrable initially deferred,
	CONSTRAINT "fk_uses_in_the_implementation_source" FOREIGN KEY("source_id") REFERENCES "class_declaration"("id") deferrable initially deferred
);
DROP TABLE IF EXISTS "namespace_source_file";
CREATE TABLE IF NOT EXISTS "namespace_source_file" (
	"source_file_id"	bigint NOT NULL,
	"namespace_id"	bigint NOT NULL,
	PRIMARY KEY("source_file_id","namespace_id"),
	CONSTRAINT "fk_namespace_source_file_key1" FOREIGN KEY("source_file_id") REFERENCES "source_file"("id") on delete cascade deferrable initially deferred,
	CONSTRAINT "fk_namespace_source_file_key2" FOREIGN KEY("namespace_id") REFERENCES "namespace_declaration"("id") on delete cascade deferrable initially deferred
);
DROP TABLE IF EXISTS "class_source_file";
CREATE TABLE IF NOT EXISTS "class_source_file" (
	"source_file_id"	bigint NOT NULL,
	"class_id"	bigint NOT NULL,
	CONSTRAINT "fk_class_source_file_key1" FOREIGN KEY("source_file_id") REFERENCES "source_file"("id") on delete cascade deferrable initially deferred,
	CONSTRAINT "fk_class_source_file_key2" FOREIGN KEY("class_id") REFERENCES "class_declaration"("id") on delete cascade deferrable initially deferred,
	PRIMARY KEY("source_file_id","class_id")
);
DROP TABLE IF EXISTS "udt_component";
CREATE TABLE IF NOT EXISTS "udt_component" (
	"component_id"	bigint NOT NULL,
	"udt_id"	bigint NOT NULL,
	CONSTRAINT "fk_udt_component_key1" FOREIGN KEY("component_id") REFERENCES "source_component"("id") on delete cascade deferrable initially deferred,
	PRIMARY KEY("component_id","udt_id"),
	CONSTRAINT "fk_udt_component_key2" FOREIGN KEY("udt_id") REFERENCES "class_declaration"("id") on delete cascade deferrable initially deferred
);
DROP TABLE IF EXISTS "method_argument_class";
CREATE TABLE IF NOT EXISTS "method_argument_class" (
	"type_class_id"	bigint NOT NULL,
	"method_id"	bigint NOT NULL,
	PRIMARY KEY("type_class_id","method_id"),
	CONSTRAINT "fk_method_argument_class_key2" FOREIGN KEY("method_id") REFERENCES "method_declaration"("id") on delete cascade deferrable initially deferred,
	CONSTRAINT "fk_method_argument_class_key1" FOREIGN KEY("type_class_id") REFERENCES "class_declaration"("id") on delete cascade deferrable initially deferred
);
DROP TABLE IF EXISTS "field_type";
CREATE TABLE IF NOT EXISTS "field_type" (
	"type_class_id"	bigint NOT NULL,
	"field_id"	bigint NOT NULL,
	PRIMARY KEY("type_class_id","field_id"),
	CONSTRAINT "fk_field_type_key2" FOREIGN KEY("field_id") REFERENCES "field_declaration"("id") on delete cascade deferrable initially deferred,
	CONSTRAINT "fk_field_type_key1" FOREIGN KEY("type_class_id") REFERENCES "class_declaration"("id") on delete cascade deferrable initially deferred
);
DROP TABLE IF EXISTS "global_function_source_file";
CREATE TABLE IF NOT EXISTS "global_function_source_file" (
    "id"	integer,
    "source_file_id"	bigint,
    "function_id"	bigint,
    PRIMARY KEY("id" AUTOINCREMENT),
    CONSTRAINT "fk_global_function_source_file_source_id" FOREIGN KEY("source_file_id") REFERENCES "source_file"("id") deferrable initially deferred,
    CONSTRAINT "fk_global_function_source_file_function_id" FOREIGN KEY("function_id") REFERENCES "function_declaration"("id") deferrable initially deferred
);

DROP INDEX IF EXISTS "namespace_source_file_source_file_source_file";
CREATE INDEX IF NOT EXISTS "namespace_source_file_source_file_source_file" ON "namespace_source_file" (
	"source_file_id"
);
DROP INDEX IF EXISTS "namespace_source_file_namespace_declaration_namespace";
CREATE INDEX IF NOT EXISTS "namespace_source_file_namespace_declaration_namespace" ON "namespace_source_file" (
	"namespace_id"
);
DROP INDEX IF EXISTS "class_source_file_source_file_source_file";
CREATE INDEX IF NOT EXISTS "class_source_file_source_file_source_file" ON "class_source_file" (
	"source_file_id"
);
DROP INDEX IF EXISTS "class_source_file_class_declaration_class";
CREATE INDEX IF NOT EXISTS "class_source_file_class_declaration_class" ON "class_source_file" (
	"class_id"
);
DROP INDEX IF EXISTS "udt_component_source_component_component";
CREATE INDEX IF NOT EXISTS "udt_component_source_component_component" ON "udt_component" (
	"component_id"
);
DROP INDEX IF EXISTS "udt_component_class_declaration_udt";
CREATE INDEX IF NOT EXISTS "udt_component_class_declaration_udt" ON "udt_component" (
	"udt_id"
);
DROP INDEX IF EXISTS "method_argument_class_class_declaration_type_class";
CREATE INDEX IF NOT EXISTS "method_argument_class_class_declaration_type_class" ON "method_argument_class" (
	"type_class_id"
);
DROP INDEX IF EXISTS "method_argument_class_method_declaration_method";
CREATE INDEX IF NOT EXISTS "method_argument_class_method_declaration_method" ON "method_argument_class" (
	"method_id"
);
DROP INDEX IF EXISTS "field_type_class_declaration_type_class";
CREATE INDEX IF NOT EXISTS "field_type_class_declaration_type_class" ON "field_type" (
	"type_class_id"
);
DROP INDEX IF EXISTS "field_type_field_declaration_field";
CREATE INDEX IF NOT EXISTS "field_type_field_declaration_field" ON "field_type" (
	"field_id"
);
DROP INDEX IF EXISTS "source_repository_qualified_name";
CREATE INDEX IF NOT EXISTS "source_repository_qualified_name" ON "source_repository" (
	"qualified_name"
);
DROP INDEX IF EXISTS "class_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "class_declaration_qualified_name" ON "class_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "class_hierarchy_target_id";
CREATE INDEX IF NOT EXISTS "class_hierarchy_target_id" ON "class_hierarchy" (
	"target_id"
);
DROP INDEX IF EXISTS "class_hierarchy_source_id";
CREATE INDEX IF NOT EXISTS "class_hierarchy_source_id" ON "class_hierarchy" (
	"source_id"
);
DROP INDEX IF EXISTS "component_relation_target_id";
CREATE INDEX IF NOT EXISTS "component_relation_target_id" ON "component_relation" (
	"target_id"
);
DROP INDEX IF EXISTS "component_relation_source_id";
CREATE INDEX IF NOT EXISTS "component_relation_source_id" ON "component_relation" (
	"source_id"
);
DROP INDEX IF EXISTS "dependencies_target_id";
CREATE INDEX IF NOT EXISTS "dependencies_target_id" ON "dependencies" (
	"target_id"
);
DROP INDEX IF EXISTS "dependencies_source_id";
CREATE INDEX IF NOT EXISTS "dependencies_source_id" ON "dependencies" (
	"source_id"
);
DROP INDEX IF EXISTS "field_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "field_declaration_qualified_name" ON "field_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "function_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "function_declaration_qualified_name" ON "function_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "function_declaration_signature";
CREATE INDEX IF NOT EXISTS "function_declaration_signature" ON "function_declaration" (
	"signature"
);
DROP INDEX IF EXISTS "includes_target_id";
CREATE INDEX IF NOT EXISTS "includes_target_id" ON "includes" (
	"target_id"
);
DROP INDEX IF EXISTS "includes_source_id";
CREATE INDEX IF NOT EXISTS "includes_source_id" ON "includes" (
	"source_id"
);
DROP INDEX IF EXISTS "method_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "method_declaration_qualified_name" ON "method_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "method_declaration_signature";
CREATE INDEX IF NOT EXISTS "method_declaration_signature" ON "method_declaration" (
	"signature"
);
DROP INDEX IF EXISTS "namespace_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "namespace_declaration_qualified_name" ON "namespace_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "source_component_qualified_name";
CREATE INDEX IF NOT EXISTS "source_component_qualified_name" ON "source_component" (
	"qualified_name"
);
DROP INDEX IF EXISTS "source_file_qualified_name";
CREATE INDEX IF NOT EXISTS "source_file_qualified_name" ON "source_file" (
	"qualified_name"
);
DROP INDEX IF EXISTS "source_package_qualified_name";
CREATE INDEX IF NOT EXISTS "source_package_qualified_name" ON "source_package" (
	"qualified_name"
);
DROP INDEX IF EXISTS "uses_in_the_implementation_target_id";
CREATE INDEX IF NOT EXISTS "uses_in_the_implementation_target_id" ON "uses_in_the_implementation" (
	"target_id"
);
DROP INDEX IF EXISTS "uses_in_the_implementation_source_id";
CREATE INDEX IF NOT EXISTS "uses_in_the_implementation_source_id" ON "uses_in_the_implementation" (
	"source_id"
);
DROP INDEX IF EXISTS "uses_in_the_interface_target_id";
CREATE INDEX IF NOT EXISTS "uses_in_the_interface_target_id" ON "uses_in_the_interface" (
	"target_id"
);
DROP INDEX IF EXISTS "uses_in_the_interface_source_id";
CREATE INDEX IF NOT EXISTS "uses_in_the_interface_source_id" ON "uses_in_the_interface" (
	"source_id"
);
DROP INDEX IF EXISTS "variable_declaration_qualified_name";
CREATE INDEX IF NOT EXISTS "variable_declaration_qualified_name" ON "variable_declaration" (
	"qualified_name"
);
DROP INDEX IF EXISTS "global_function_source_file_id";
CREATE INDEX IF NOT EXISTS "global_function_source_file_id" ON "global_function_source_file" (
    "id"
);

COMMIT;
